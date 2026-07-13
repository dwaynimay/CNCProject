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

// ── [TEST] Penahanan nilai arus palsu — untuk uji histeresis end-to-end ──
// Beda dari testOvercurrentActive (one-shot): nilai ini TERUS dipakai tiap
// siklus selama testHoldActive[i]==true, sehingga bisa disimulasikan
// beberapa tahap nilai berurutan (di atas ambang -> zona histeresis ->
// di bawah ambang resume) tanpa perlu arus fisik sungguhan mengalir.
bool  testHoldActive[NUM_CURRENT_SENSORS] = {};
float testHoldValue[NUM_CURRENT_SENSORS]  = {};

// ── [TEST] Penahanan nilai suhu palsu — sama seperti testHoldActive di atas,
// tapi untuk kanal suhu (Spindle, Stepper_Z).
bool  testHoldTempActive[NUM_TEMP_SENSORS] = {};
float testHoldTempValue[NUM_TEMP_SENSORS]  = {};

// ── [TEST] Helper waktu untuk korelasi presisi Serial Monitor ↔ CSV/API ──
// Mencetak "epoch=<unix_ts> uptime=<ms>" di tiap log pengujian, sehingga
// screenshot Serial Monitor/Plotter bisa dicocokkan ke kolom waktu di CSV
// hasil skrip Python (yang membaca field sama: data.ts dari payload MQTT).
// Catatan: isNtpSynced() ada di kelas MqttClient (private ke instance mqtt),
// jadi di sini pakai pengecekan setara langsung ke time(nullptr).
static void logTestTimestamp() {
    time_t now = time(nullptr);
    bool   ntpOk = now > 1600000000;  // > tahun 2020, sama seperti NTP_EPOCH_2020
    uint32_t epoch = ntpOk ? (uint32_t)now : 0;
    LOG_D("        [waktu] epoch=%u uptime_ms=%lu ntpOk=%s\n",
          epoch, millis(), ntpOk ? "true" : "false");
}

// ── [SAFE] Cek apakah semua kanal arus & suhu sudah di bawah ambang resume ──
// (ambang alarm - histeresis). Dipakai onCommand() sebelum mengizinkan
// relay_off, supaya logika reject/accept resume bisa diuji terpisah dari
// pencatatan log.
static bool isSafeToResume(const float* tempC, const float* tempAlarm, float tempHyst, int numTemp,
                            const float* curA, const float* curAlarm, float curHyst, int numCur) {
    for (int i = 0; i < numTemp; i++) {
        if (tempC[i] > (tempAlarm[i] - tempHyst)) return false;
    }
    for (int i = 0; i < numCur; i++) {
        if (fabsf(curA[i]) > (curAlarm[i] - curHyst)) return false;
    }
    return true;
}

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
                LOG_E("[ALARM] epoch=%u Arus sensor[%d] %s %.2fA — relay ON (matikan mesin)\n",
                      (uint32_t)now, i, CURRENT_NAMES[i], p.current[i].ampere);
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
                LOG_E("[ALARM] epoch=%u Sensor suhu[%d] %s DISCONNECT — relay ON (matikan mesin)!\n",
                      (uint32_t)now, i, TEMP_NAMES[i]);
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
                LOG_E("[ALARM] epoch=%u Suhu sensor[%d] %s %.1f°C — relay ON (matikan mesin)\n",
                      (uint32_t)now, i, TEMP_NAMES[i], p.temp[i].celsius);
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
                time_t nowReject; time(&nowReject);
                LOG_W("[SAFE] epoch=%u Tolak resume! Sensor masih di zona bahaya (arus/suhu di atas ambang - hysteresis).\n",
                      (uint32_t)nowReject);
                return;
            }
        }
        relay.off();
        time_t nowAccept; time(&nowAccept);
        LOG_I("[SAFE] epoch=%u Resume mesin berhasil.\n", (uint32_t)nowAccept);
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

    // test_hold:<index>:<nilai_ampere>
    // Tahan pembacaan arus sensor ke-N pada nilai tertentu, terus-menerus,
    // sampai diubah lewat perintah test_hold lain atau dihentikan test_clear.
    // Dipakai untuk menguji perilaku resume di berbagai zona nilai
    // (di atas ambang alarm, di zona histeresis, di bawah ambang resume)
    // secara berurutan tanpa perlu arus fisik sungguhan.
    static constexpr char CMD_TEST_HOLD[] = "test_hold:";
    if (strncmp(payload, CMD_TEST_HOLD, sizeof(CMD_TEST_HOLD) - 1) == 0) {
        char* rest = (char*)payload + sizeof(CMD_TEST_HOLD) - 1;
        int   idx  = atoi(rest);
        char* colon2 = strchr(rest, ':');
        float nilai  = colon2 ? atof(colon2 + 1) : 0.0f;
        idx = constrain(idx, 0, (int)NUM_CURRENT_SENSORS - 1);
        testHoldActive[idx] = true;
        testHoldValue[idx]  = nilai;
        LOG_D("[TEST] Tahan arus sensor[%d] %s di %.2fA (ambang=%.2fA, resume<=%.2fA)\n",
              idx, CURRENT_NAMES[idx], nilai, CURRENT_ALARM[idx],
              CURRENT_ALARM[idx] - CURRENT_HYSTERESIS);
        logTestTimestamp();
        return;
    }

    // test_clear:<index>
    // Hentikan penahanan pada sensor ke-N, kembali membaca nilai asli.
    static constexpr char CMD_TEST_CLEAR[] = "test_clear:";
    if (strncmp(payload, CMD_TEST_CLEAR, sizeof(CMD_TEST_CLEAR) - 1) == 0) {
        int idx = atoi(payload + sizeof(CMD_TEST_CLEAR) - 1);
        idx = constrain(idx, 0, (int)NUM_CURRENT_SENSORS - 1);
        testHoldActive[idx] = false;
        LOG_D("[TEST] Hentikan penahanan sensor[%d] %s, kembali ke pembacaan asli\n",
              idx, CURRENT_NAMES[idx]);
        logTestTimestamp();
        return;
    }

    // test_hold_temp:<index>:<nilai_celsius>
    // Sama seperti test_hold, tapi untuk kanal suhu (0=Spindle, 1=Stepper_Z).
    static constexpr char CMD_TEST_HOLD_T[] = "test_hold_temp:";
    if (strncmp(payload, CMD_TEST_HOLD_T, sizeof(CMD_TEST_HOLD_T) - 1) == 0) {
        char* rest = (char*)payload + sizeof(CMD_TEST_HOLD_T) - 1;
        int   idx  = atoi(rest);
        char* colon2 = strchr(rest, ':');
        float nilai  = colon2 ? atof(colon2 + 1) : 0.0f;
        idx = constrain(idx, 0, (int)NUM_TEMP_SENSORS - 1);
        testHoldTempActive[idx] = true;
        testHoldTempValue[idx]  = nilai;
        LOG_D("[TEST] Tahan suhu sensor[%d] %s di %.1fC (ambang=%.1fC, resume<=%.1fC)\n",
              idx, TEMP_NAMES[idx], nilai, TEMP_ALARM[idx],
              TEMP_ALARM[idx] - TEMP_HYSTERESIS);
        logTestTimestamp();
        return;
    }

    // test_clear_temp:<index>
    // Hentikan penahanan suhu pada sensor ke-N, kembali membaca nilai asli.
    static constexpr char CMD_TEST_CLEAR_T[] = "test_clear_temp:";
    if (strncmp(payload, CMD_TEST_CLEAR_T, sizeof(CMD_TEST_CLEAR_T) - 1) == 0) {
        int idx = atoi(payload + sizeof(CMD_TEST_CLEAR_T) - 1);
        idx = constrain(idx, 0, (int)NUM_TEMP_SENSORS - 1);
        testHoldTempActive[idx] = false;
        LOG_D("[TEST] Hentikan penahanan suhu sensor[%d] %s, kembali ke pembacaan asli\n",
              idx, TEMP_NAMES[idx]);
        logTestTimestamp();
        return;
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
        LOG_D("[TEST] Inject arus %.2fA ke sensor[%d] %s (alarm threshold: %.2fA)\n",
              payload.current[si].ampere, si, CURRENT_NAMES[si], CURRENT_ALARM[si]);
    }

    // [TEST] Injeksi tertahan — dibaca tiap siklus selama testHoldActive[i] true.
    // Diterapkan SETELAH one-shot test di atas, supaya test_hold bisa
    // menimpa/melanjutkan simulasi tanpa konflik.
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        if (testHoldActive[i]) {
            payload.current[i].ampere = testHoldValue[i];
            payload.current[i].alarm  = fabsf(testHoldValue[i]) > CURRENT_ALARM[i];
        }
    }

    // [TEST] Injeksi tertahan untuk kanal suhu — pola identik dengan arus.
    for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
        if (testHoldTempActive[i]) {
            payload.temp[i].celsius = testHoldTempValue[i];
            payload.temp[i].alarm   = testHoldTempValue[i] > TEMP_ALARM[i];
            payload.temp[i].sensorError = false;  // pastikan tidak dibaca sebagai sensor lepas
        }
    }

    // [FIX] latestPayload dipindah ke SINI (setelah seluruh injeksi test),
    // supaya isSafeToResume() di onCommand() benar-benar melihat nilai yang
    // sedang disimulasikan/tertahan, bukan nilai asli sebelum injeksi.
    // Sebelumnya latestPayload di-set SEBELUM blok injeksi test dijalankan,
    // sehingga test_overcurrent/test_overtemp tidak pernah memengaruhi
    // keputusan resume sama sekali — celah ini turut diperbaiki oleh diff ini.
    latestPayload     = payload;
    hasLatestPayload   = true;

    checkAlarms(payload);   // alarm dicek dari nilai raw

    // Terapkan EMA setelah alarm check — hanya untuk nilai tampilan dashboard
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        float raw     = payload.current[i].ampere;
        emaAmpere[i]  = emaInit[i] ? EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * emaAmpere[i] : raw;
        emaInit[i]    = true;
        payload.current[i].ampere = emaAmpere[i];
    }     // cek alarm SEBELUM publish
    payload.relayOn = relay.isOn();

    // [SPRINT-2] Track publish sukses untuk heartbeat fail-safe
    bool ok = mqtt.publish(payload);
    if (ok) lastPublishSuccess = millis();

    cli.printLive(payload.current, NUM_CURRENT_SENSORS);
}
