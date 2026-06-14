#include <Arduino.h>
#include <esp_task_wdt.h>    // [SPRINT-1] Hardware Watchdog Timer
#include <time.h>            // [SPRINT-2] NTP timestamp
#include "types.h"
#include "SensorConfig.h"
#include "ACS712.h"
#include "DS18B20.h"
#include "Calibration.h"
#include "RelayControl.h"
#include "MqttClient.h"
#include "SerialCLI.h"
#include "EventLog.h"

// ── Credentials — dipisah agar tidak masuk ke git ─────────
#include "credentials.h"

// ── Interval & timing constants ───────────────────────────
constexpr uint32_t PUBLISH_INTERVAL_MS = 2000;   // kirim data ke broker (ms)
constexpr uint8_t  WDT_TIMEOUT_S       = 30;     // watchdog timeout (detik)

// ── Instance semua modul ──────────────────────────────────
ACS712 sensors[NUM_CURRENT_SENSORS] = {
    {0, CURRENT_PINS[0], CURRENT_TYPES[0]},
    {1, CURRENT_PINS[1], CURRENT_TYPES[1]},
    {2, CURRENT_PINS[2], CURRENT_TYPES[2]},
    {3, CURRENT_PINS[3], CURRENT_TYPES[3]},
    {4, CURRENT_PINS[4], CURRENT_TYPES[4]},
};

Calibration    cal;
DS18B20Manager tempSensors;
RelayControl   relay;
MqttClient     mqtt;
EventLog       eventLog;
#undef cli
SerialCLI      cli(cal, sensors, NUM_CURRENT_SENSORS, eventLog);

unsigned long lastPublish        = 0;
unsigned long lastPublishSuccess = 0;  // [SPRINT-2] untuk fail-safe heartbeat
bool          safeModeLatch      = false;  // latch: relay tidak bisa ON lagi setelah safe mode

// ── [TEST] Simulasi overcurrent dari dashboard ────────────────────────────
bool testOvercurrentActive = false;   // flag: inject arus palsu pada siklus berikutnya
int  testOvercurrentSensor = 0;       // index sensor yang di-test (0–4)

// ── [SPRINT-1] Proteksi otomatis — arus, suhu, sensor fault ──────────────
// Relay NORMALLY CLOSED: relay.on() = energized = kontak NC PUTUS = mesin MATI
//                        relay.off() = de-energized = kontak NC TERHUBUNG = mesin HIDUP
//
// [SPRINT-4] Edge detection: event hanya dicatat sekali per transisi clear→alarm
void checkAlarms(const MqttPayload& p) {
    static bool prevCurAlarm[NUM_CURRENT_SENSORS] = {};
    static bool prevTmpAlarm[NUM_TEMP_SENSORS]    = {};

    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        if (p.current[i].alarm) {
            if (!prevCurAlarm[i]) {
                time_t now; time(&now);
                eventLog.addAndSave({(uint32_t)now, SensorType::CURRENT, (uint8_t)i,
                                      p.current[i].ampere, AlarmType::OVERCURRENT});
                Serial.printf("[ALARM] Arus sensor[%d] %s %.2fA — relay ON (matikan mesin)\n",
                              i, CURRENT_NAMES[i], p.current[i].ampere);
            }
            prevCurAlarm[i] = true;
            relay.on();
            return;
        }
        prevCurAlarm[i] = false;
    }
    for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
        if (p.temp[i].sensorError) {
            if (!prevTmpAlarm[i]) {
                time_t now; time(&now);
                eventLog.addAndSave({(uint32_t)now, SensorType::TEMP, (uint8_t)i,
                                      p.temp[i].celsius, AlarmType::SENSOR_DISCONNECT});
                Serial.printf("[ALARM] Sensor suhu[%d] %s DISCONNECT — relay ON (matikan mesin)!\n",
                              i, TEMP_NAMES[i]);
            }
            prevTmpAlarm[i] = true;
            relay.on();
            return;
        }
        if (p.temp[i].alarm) {
            if (!prevTmpAlarm[i]) {
                time_t now; time(&now);
                eventLog.addAndSave({(uint32_t)now, SensorType::TEMP, (uint8_t)i,
                                      p.temp[i].celsius, AlarmType::OVERTEMP});
                Serial.printf("[ALARM] Suhu sensor[%d] %s %.1f°C — relay ON (matikan mesin)\n",
                              i, TEMP_NAMES[i], p.temp[i].celsius);
            }
            prevTmpAlarm[i] = true;
            relay.on();
            return;
        }
        prevTmpAlarm[i] = false;
    }
}

// ── [SPRINT-2] Fail-safe heartbeat ───────────────────────────────────────
// Jika tidak ada publish sukses dalam HEARTBEAT_TIMEOUT_MS →
// relay.on() untuk MEMUTUS mesin (NC: energize = kontak putus = mesin MATI)
// Kunci safeModeLatch sampai reboot agar mesin tidak bisa dinyalakan lagi
void checkHeartbeat() {
    if (safeModeLatch) {
        relay.on();   // NC: paksa energize setiap loop selama safe mode aktif
        return;
    }

    uint32_t elapsed = millis() - lastPublishSuccess;
    if (elapsed > HEARTBEAT_TIMEOUT_MS) {
        safeModeLatch = true;
        Serial.printf(
            "[SAFE] Tidak ada publish sukses selama %lus — relay ON (matikan mesin)! Reboot untuk reset.\n",
            elapsed / 1000);
        relay.on();   // NC: energize = putus kontak = mesin MATI
    }
}

// ── Perintah dari dashboard via MQTT ─────────────────────
// NC semantics: relay_on  = energize = kontak NC putus = mesin MATI
//               relay_off = de-energize = kontak NC terhubung = mesin HIDUP
void onCommand(const char* topic, const char* payload) {
    // Jika safe mode aktif, tolak perintah relay_off (mencegah mesin dinyalakan kembali)
    if (safeModeLatch && strstr(payload, "relay_off")) {
        Serial.println("[SAFE] Perintah relay_off ditolak — safe mode aktif, reboot untuk reset.");
        return;
    }

    if (strstr(payload, "relay_on"))  { relay.on();  return; }  // matikan mesin
    if (strstr(payload, "relay_off")) { relay.off(); return; }  // hidupkan mesin (resume)
    if (strstr(payload, "cal_save"))  { cal.save();  return; }
    if (strstr(payload, "cal_reset")) { cal.reset(); Serial.println("> Reset default"); return; }

    // cal_offset:<index>  — kalibrasi offset sensor ke-N (mesin harus mati)
    static constexpr char CMD_CAL_OFFSET[] = "cal_offset:";
    if (strncmp(payload, CMD_CAL_OFFSET, sizeof(CMD_CAL_OFFSET) - 1) == 0) {
        int idx = atoi(payload + sizeof(CMD_CAL_OFFSET) - 1);
        if (idx >= 0 && idx < (int)NUM_CURRENT_SENSORS) {
            cal.autoOffset(sensors[idx]);
            Serial.printf(">  cal_offset[%d] done vMid=%.3f\n",
                          idx, cal.getData(idx).vMid);
        }
        return;
    }

    // test_overcurrent  atau  test_overcurrent:<index>
    // Simulasi arus berlebih satu siklus → relay trip otomatis
    static constexpr char CMD_TEST_OC[] = "test_overcurrent";
    if (strncmp(payload, CMD_TEST_OC, sizeof(CMD_TEST_OC) - 1) == 0) {
        int idx = 0;
        if (payload[sizeof(CMD_TEST_OC) - 1] == ':') idx = atoi(payload + sizeof(CMD_TEST_OC));
        idx = constrain(idx, 0, (int)NUM_CURRENT_SENSORS - 1);
        testOvercurrentSensor = idx;
        testOvercurrentActive = true;
        Serial.printf("[TEST] Simulasi overcurrent sensor[%d] %s — akan trip pada siklus berikutnya\n",
                      idx, CURRENT_NAMES[idx]);
        return;
    }
}

void setup() {
    Serial.begin(115200);

    // [SPRINT-1] Aktifkan Hardware Watchdog Timer
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // panic=true → reboot otomatis
    esp_task_wdt_add(NULL);
    Serial.printf("[WDT] Hardware watchdog aktif — timeout %ds\n", WDT_TIMEOUT_S);

    cal.load();
    eventLog.load();
    tempSensors.begin();
    relay.begin();      // NC: relay OFF saat boot = kontak NC terhubung = mesin bisa jalan
    cli.begin();

    mqtt.setCommandCallback(onCommand);
    mqtt.begin(WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_PORT, MQTT_ID);
    // Setelah begin(): WiFi connect + NTP sync sudah dilakukan di dalam MqttClient

    // Init lastPublishSuccess di awal — beri grace period 2× HEARTBEAT sebelum cek
    lastPublishSuccess = millis();
}

void loop() {
    // [SPRINT-1] Reset watchdog
    esp_task_wdt_reset();

    mqtt.loop();   // non-blocking
    cli.loop();

    // [SPRINT-2] Cek fail-safe heartbeat setiap iterasi loop
    checkHeartbeat();

    if (millis() - lastPublish < PUBLISH_INTERVAL_MS) return;
    lastPublish = millis();

    // Baca semua sensor
    MqttPayload payload;
    // [SPRINT-2] timestamp diisi di dalam mqtt.publish() via NTP/fallback
    payload.timestamp = 0;  // placeholder — diisi ulang saat publish
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++)
        payload.current[i] = sensors[i].read(cal.getData(i));
    tempSensors.requestAll();  // satu request untuk semua sensor, non-blocking
    delay(DS18B20_CONVERSION_MS);
    for (int i = 0; i < NUM_TEMP_SENSORS; i++)
        payload.temp[i] = tempSensors.read(i);

    // [TEST] Inject nilai arus palsu jika test_overcurrent aktif
    if (testOvercurrentActive) {
        int si = testOvercurrentSensor;
        payload.current[si].ampere = CURRENT_ALARM[si] * 1.5f;  // 150% batas alarm
        payload.current[si].alarm  = true;
        testOvercurrentActive      = false;   // one-shot: reset setelah satu siklus
        Serial.printf("[TEST] Inject arus %.2fA ke sensor[%d] %s (alarm threshold: %.2fA)\n",
                      payload.current[si].ampere, si, CURRENT_NAMES[si], CURRENT_ALARM[si]);
    }

    checkAlarms(payload);     // cek alarm SEBELUM publish
    payload.relayOn = relay.isOn();

    // [SPRINT-2] Track publish sukses untuk heartbeat fail-safe
    bool ok = mqtt.publish(payload);
    if (ok) lastPublishSuccess = millis();

    cli.printLive(payload.current, NUM_CURRENT_SENSORS);
}
