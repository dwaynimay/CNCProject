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
#include "Logger.h"
#include "EmaFilter.h"
#include "SafetyLogic.h"
#include "SelfTest.h"
#include <stdarg.h>
#include <math.h>

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

MqttPayload   latestPayload;
bool          hasLatestPayload   = false;

// EMA display filter — alpha 0.3: smooth tapi cukup responsif untuk NEMA 23
// Alarm dicek dari raw, EMA hanya untuk nilai yang dikirim ke dashboard
constexpr float EMA_ALPHA = 0.3f;
float emaAmpere[NUM_CURRENT_SENSORS] = {};
bool  emaInit[NUM_CURRENT_SENSORS]   = {};

// ── [TEST] Simulasi overcurrent dari dashboard ────────────────────────────
bool testOvercurrentActive = false;   // flag: inject arus palsu pada siklus berikutnya
int  testOvercurrentSensor = 0;       // index sensor yang di-test (0–4)

// ── [TEST] Simulasi overtemp dari dashboard ───────────────────────────────
bool testOvertempActive = false;      // flag: inject suhu palsu pada siklus berikutnya
int  testOvertempSensor = 0;          // index sensor yang di-test (0–1)

// ── [SELF-TEST] Diagnostik terstruktur — non-destruktif, aman dijalankan kapan saja ──
// Pemeriksaan sanity/logika saja (tidak men-trip relay atau menulis EventLog asli).
// Untuk uji trip relay sungguhan, tetap pakai command test_overcurrent yang sudah ada.
SelfTestResult selfTestResults[SELFTEST_MAX_CHECKS];
uint8_t        selfTestCount = 0;
static char    selfTestNameBuf[SELFTEST_MAX_CHECKS][32];
void runSelfTest();  // forward decl — dipakai di onCommand(), didefinisikan di bawah

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
                LOG_E("[ALARM] Arus sensor[%d] %s %.2fA — relay ON (matikan mesin)\n",
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
                LOG_E("[ALARM] Sensor suhu[%d] %s DISCONNECT — relay ON (matikan mesin)!\n",
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
                LOG_E("[ALARM] Suhu sensor[%d] %s %.1f°C — relay ON (matikan mesin)\n",
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
        LOG_W("[SAFE] Tidak ada publish sukses selama %lus — relay ON (matikan mesin)! Reboot untuk reset.\n",
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
        LOG_W("[SAFE] Perintah relay_off ditolak — safe mode aktif, reboot untuk reset.\n");
        return;
    }

    if (strstr(payload, "relay_on"))  { relay.on();  return; }  // matikan mesin
    if (strstr(payload, "relay_off")) {
        // [HYSTERESIS] Tolak resume jika sensor masih di zona bahaya
        if (hasLatestPayload) {
            float tempC[NUM_TEMP_SENSORS], curA[NUM_CURRENT_SENSORS];
            for (int i = 0; i < NUM_TEMP_SENSORS; i++)    tempC[i] = latestPayload.temp[i].celsius;
            for (int i = 0; i < NUM_CURRENT_SENSORS; i++) curA[i]  = latestPayload.current[i].ampere;

            if (!isSafeToResume(tempC, TEMP_ALARM, TEMP_HYSTERESIS, NUM_TEMP_SENSORS,
                                 curA, CURRENT_ALARM, CURRENT_HYSTERESIS, NUM_CURRENT_SENSORS)) {
                LOG_W("[SAFE] Tolak resume! Sensor masih di zona bahaya (arus/suhu di atas ambang - hysteresis).\n");
                return;
            }
        }
        relay.off();
        LOG_I("[SAFE] Resume mesin berhasil.\n");
        return;
    }
    if (strstr(payload, "cal_save"))  { cal.save();  return; }
    if (strstr(payload, "cal_reset")) { cal.reset(); LOG_I("> Reset default\n"); return; }

    // cal_offset:<index>  — kalibrasi offset sensor ke-N (mesin harus mati)
    static constexpr char CMD_CAL_OFFSET[] = "cal_offset:";
    if (strncmp(payload, CMD_CAL_OFFSET, sizeof(CMD_CAL_OFFSET) - 1) == 0) {
        int idx = atoi(payload + sizeof(CMD_CAL_OFFSET) - 1);
        if (idx >= 0 && idx < (int)NUM_CURRENT_SENSORS) {
            cal.autoOffset(sensors[idx]);
            LOG_D(">  cal_offset[%d] done vMid=%.3f\n",
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
        LOG_D("[TEST] Simulasi overcurrent sensor[%d] %s — akan trip pada siklus berikutnya\n",
              idx, CURRENT_NAMES[idx]);
        return;
    }

    // test_overtemp  atau  test_overtemp:<index>
    // Simulasi suhu berlebih satu siklus → relay trip otomatis
    static constexpr char CMD_TEST_OT[] = "test_overtemp";
    if (strncmp(payload, CMD_TEST_OT, sizeof(CMD_TEST_OT) - 1) == 0) {
        int idx = 0;
        if (payload[sizeof(CMD_TEST_OT) - 1] == ':') idx = atoi(payload + sizeof(CMD_TEST_OT));
        idx = constrain(idx, 0, (int)NUM_TEMP_SENSORS - 1);
        testOvertempSensor = idx;
        testOvertempActive = true;
        LOG_D("[TEST] Simulasi overtemp sensor[%d] %s — akan trip pada siklus berikutnya\n",
              idx, TEMP_NAMES[idx]);
        return;
    }

    // self_test — jalankan diagnostik lengkap & publish hasil terstruktur
    if (strstr(payload, "self_test")) {
        runSelfTest();
        mqtt.publishSelfTestResult(selfTestResults, selfTestCount);
        return;
    }
}

void addSelfTestCheck(const char* category, bool pass, const char* name, const char* detailFmt, ...) {
    if (selfTestCount >= SELFTEST_MAX_CHECKS) return;
    uint8_t idx = selfTestCount++;
    strncpy(selfTestNameBuf[idx], name, sizeof(selfTestNameBuf[idx]) - 1);
    selfTestNameBuf[idx][sizeof(selfTestNameBuf[idx]) - 1] = '\0';

    SelfTestResult& r = selfTestResults[idx];
    r.name     = selfTestNameBuf[idx];
    r.category = category;
    r.pass     = pass;

    va_list args;
    va_start(args, detailFmt);
    vsnprintf(r.detail, SELFTEST_DETAIL_SIZE, detailFmt, args);
    va_end(args);
}

void runSelfTest() {
    selfTestCount = 0;
    esp_task_wdt_reset();

    // ── Konektivitas ──
    addSelfTestCheck("connectivity", mqtt.isConnected(), "MQTT Connected",
                      mqtt.isConnected() ? "terhubung ke broker" : "tidak terhubung ke broker");
    addSelfTestCheck("connectivity", mqtt.isNtpSynced(), "NTP Synced",
                      mqtt.isNtpSynced() ? "waktu tersinkron via NTP" : "belum sync — pakai uptime sebagai timestamp");

    // ── Sensor arus: sanity + verifikasi logika respons alarm ──
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        esp_task_wdt_reset();
        char name[32];
        CurrentReading r = sensors[i].read(cal.getData(i));

        snprintf(name, sizeof(name), "Current[%d] sanity", i);
        bool sane = evaluateRange(r.ampere, -25.0f, 25.0f);
        addSelfTestCheck("current", sane, name, "%s raw=%.2fA", CURRENT_NAMES[i], r.ampere);

        snprintf(name, sizeof(name), "Current[%d] alarm-logic", i);
        float injected = CURRENT_ALARM[i] * 1.5f;
        bool  respond   = evaluateAlarmResponse(injected, CURRENT_ALARM[i]);
        addSelfTestCheck("current", respond, name, "%s inject=%.2fA thr=%.2fA",
                          CURRENT_NAMES[i], injected, CURRENT_ALARM[i]);
    }

    // ── Sensor suhu: sanity + verifikasi logika respons alarm ──
    for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
        esp_task_wdt_reset();
        char name[32];
        TempReading r = tempSensors.read(i);

        snprintf(name, sizeof(name), "Temp[%d] sanity", i);
        bool sane = !r.sensorError && evaluateRange(r.celsius, -10.0f, 120.0f);
        addSelfTestCheck("temp", sane, name, "%s %.1fC%s",
                          TEMP_NAMES[i], r.celsius, r.sensorError ? " (disconnect)" : "");

        snprintf(name, sizeof(name), "Temp[%d] alarm-logic", i);
        float injected = TEMP_ALARM[i] * 1.2f;
        bool  respond   = evaluateAlarmResponse(injected, TEMP_ALARM[i]);
        addSelfTestCheck("temp", respond, name, "%s inject=%.1fC thr=%.1fC",
                          TEMP_NAMES[i], injected, TEMP_ALARM[i]);
    }

    // ── Relay actuation — hanya jika mesin idle & tidak dalam safe mode ──
    esp_task_wdt_reset();
    if (safeModeLatch) {
        addSelfTestCheck("relay", false, "Relay Actuation", "dilewati: safe mode aktif, reboot untuk reset");
    } else if (relay.isOn()) {
        addSelfTestCheck("relay", false, "Relay Actuation", "dilewati: mesin sedang aktif (relay ON)");
    } else {
        relay.on();
        bool step1 = relay.isOn();
        relay.off();
        bool step2 = !relay.isOn();
        addSelfTestCheck("relay", step1 && step2, "Relay Actuation",
                          "toggle on->off berhasil, state kembali idle");
    }

    // ── Kalibrasi NVS roundtrip ──
    esp_task_wdt_reset();
    {
        double before = cal.getData(0).vMid;
        cal.save();
        cal.load();
        double after = cal.getData(0).vMid;
        addSelfTestCheck("calibration", fabs(before - after) < 0.01, "Calibration NVS Roundtrip",
                          "vMid before=%.3f after=%.3f", before, after);
    }

    // ── EventLog ring-buffer roundtrip — pakai instance sementara terpisah,
    //    supaya trip log asli di eventLog tidak tercemar entry palsu ──
    esp_task_wdt_reset();
    {
        EventLogBuffer tempLog;
        EventEntry dummy{0, SensorType::CURRENT, 0, 0.0f, AlarmType::OVERCURRENT};
        tempLog.add(dummy);
        bool ok = (tempLog.count() == 1) && (tempLog.get(0).value == 0.0f);
        addSelfTestCheck("eventlog", ok, "EventLog Roundtrip", "ring buffer add+get OK");
    }
}

void setup() {
    Serial.begin(115200);

    // [SPRINT-1] Aktifkan Hardware Watchdog Timer
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // panic=true → reboot otomatis
    esp_task_wdt_add(NULL);
    LOG_I("[WDT] Hardware watchdog aktif — timeout %ds\n", WDT_TIMEOUT_S);

    cal.load();
    eventLog.load();
    tempSensors.begin();
    relay.begin();      // NC: relay OFF saat boot = kontak NC terhubung = mesin bisa jalan
    cli.setSelfTestCallback([]() {
        runSelfTest();
        mqtt.publishSelfTestResult(selfTestResults, selfTestCount);
    });
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

    latestPayload = payload;
    hasLatestPayload = true;

    // [TEST] Inject nilai arus palsu jika test_overcurrent aktif
    if (testOvercurrentActive) {
        int si = testOvercurrentSensor;
        payload.current[si].ampere = CURRENT_ALARM[si] * 1.5f;  // 150% batas alarm
        payload.current[si].alarm  = true;
        testOvercurrentActive      = false;   // one-shot: reset setelah satu siklus
        LOG_D("[TEST] Inject arus %.2fA ke sensor[%d] %s (alarm threshold: %.2fA)\n",
              payload.current[si].ampere, si, CURRENT_NAMES[si], CURRENT_ALARM[si]);
    }

    // [TEST] Inject nilai suhu palsu jika test_overtemp aktif
    if (testOvertempActive) {
        int si = testOvertempSensor;
        payload.temp[si].celsius = TEMP_ALARM[si] * 1.2f;  // 120% batas alarm
        payload.temp[si].alarm   = true;
        testOvertempActive       = false;   // one-shot: reset setelah satu siklus
        LOG_D("[TEST] Inject suhu %.1fC ke sensor[%d] %s (alarm threshold: %.1fC)\n",
              payload.temp[si].celsius, si, TEMP_NAMES[si], TEMP_ALARM[si]);
    }

    checkAlarms(payload);   // alarm dicek dari nilai raw

    // Terapkan EMA setelah alarm check — hanya untuk nilai tampilan dashboard
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        float raw     = payload.current[i].ampere;
        emaAmpere[i]  = emaUpdate(raw, emaAmpere[i], emaInit[i], EMA_ALPHA);
        emaInit[i]    = true;
        payload.current[i].ampere = emaAmpere[i];
    }     // cek alarm SEBELUM publish
    payload.relayOn = relay.isOn();

    // [SPRINT-2] Track publish sukses untuk heartbeat fail-safe
    bool ok = mqtt.publish(payload);
    if (ok) lastPublishSuccess = millis();

    cli.printLive(payload.current, NUM_CURRENT_SENSORS);
}
