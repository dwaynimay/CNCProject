#include "MqttClient.h"
#include <time.h>        // [SPRINT-2] NTP timestamp

// Topic: cnc/<clientId>/telemetry  (ESP32 → broker)
//        cnc/<clientId>/command    (broker → ESP32)
//        cnc/<clientId>/status     (LWT: "online"/"offline")

MqttClient* MqttClient::_instance = nullptr;

MqttClient::MqttClient() : _client(_wifi) { _instance = this; }

void MqttClient::setCommandCallback(MqttCommandCallback cb) { _cmdCb = cb; }
bool MqttClient::isConnected() { return _client.connected(); }

// [SPRINT-2] Cek apakah NTP sudah tersinkronisasi (timestamp > tahun 2020)
bool MqttClient::isNtpSynced() {
    time_t now = time(nullptr);
    return (now > 1577836800UL);  // 2020-01-01 00:00:00 UTC
}

// ── [SPRINT-1 FIX #2] WiFi connect dengan timeout 30s → ESP.restart() ──────
// ── [SPRINT-2 FIX #5] Tambah NTP sync setelah WiFi connect ─────────────────
void MqttClient::begin(const char* ssid, const char* pass,
                       const char* host, uint16_t port, const char* clientId) {
    _clientId = clientId;
    WiFi.begin(ssid, pass);
    Serial.print("[WiFi] Menghubungkan");

    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startMs > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.printf("\n[WiFi] TIMEOUT %lus — reboot!\n",
                          WIFI_CONNECT_TIMEOUT_MS / 1000);
            delay(500);
            ESP.restart();
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] OK: %s  RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());

    // [SPRINT-2] Sinkronisasi waktu via NTP (non-blocking dengan timeout)
    Serial.print("[NTP] Sync");
    configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
    struct tm timeinfo;
    unsigned long ntpStart = millis();
    while (!getLocalTime(&timeinfo)) {
        if (millis() - ntpStart > NTP_TIMEOUT_MS) {
            Serial.println("\n[NTP] TIMEOUT — timestamp akan pakai millis()/1000 sebagai fallback");
            break;
        }
        delay(200);
        Serial.print(".");
    }
    if (isNtpSynced()) {
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("\n[NTP] OK: %s WIB\n", timeBuf);
    }

    _client.setBufferSize(512);
    _client.setServer(host, port);
    _client.setCallback(_onMessage);

    // Percobaan connect pertama (blocking hanya sekali di boot)
    _attemptReconnect();
}

// ── [SPRINT-1 FIX #4] loop() non-blocking ────────────────────────────────
void MqttClient::loop() {
    if (!_client.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
            _lastReconnectAttempt = now;
            _attemptReconnect();
        }
        // TIDAK blocking — langsung return agar checkAlarms() tetap dieksekusi
        return;
    }
    _client.loop();
}

// ── Single non-blocking reconnect attempt ─────────────────────────────────
bool MqttClient::_attemptReconnect() {
    Serial.print("[MQTT] Connect...");
    char statusTopic[64];
    snprintf(statusTopic, sizeof(statusTopic), "cnc/%s/status", _clientId);

    // LWT: jika ESP32 disconnect paksa → broker kirim "offline"
    if (_client.connect(_clientId, nullptr, nullptr,
                        statusTopic, 0, true, "offline")) {
        Serial.println(" OK");
        _client.publish(statusTopic, "online", true);
        char cmdTopic[64];
        snprintf(cmdTopic, sizeof(cmdTopic), "cnc/%s/command", _clientId);
        _client.subscribe(cmdTopic);
        return true;
    } else {
        Serial.printf(" GAGAL rc=%d (retry %lus)\n",
                      _client.state(), MQTT_RECONNECT_INTERVAL / 1000);
        return false;
    }
}

// ── [SPRINT-2 FIX #5] Publish telemetry — return true jika berhasil ────────
bool MqttClient::publish(const MqttPayload& p) {
    char buf[512];
    int  pos = 0;

    // [SPRINT-2] Gunakan NTP timestamp (Unix epoch) jika sudah sync,
    //            fallback ke millis()/1000 jika NTP belum ready
    uint32_t ts;
    if (isNtpSynced()) {
        ts = (uint32_t)time(nullptr);
    } else {
        ts = millis() / 1000;   // detik sejak boot (bukan waktu nyata)
        Serial.println("[NTP] Belum sync — pakai uptime detik sebagai timestamp");
    }

    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "{\"ts\":%u,\"ntpOk\":%s,\"current\":[",
                    ts, isNtpSynced() ? "true" : "false");

    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"id\":\"%s\",\"a\":%.3f,\"alarm\":%s}",
            i ? "," : "", CURRENT_NAMES[i], p.current[i].ampere,
            p.current[i].alarm ? "true" : "false");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "],\"temp\":[");
    for (int i = 0; i < NUM_TEMP_SENSORS; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"id\":\"%s\",\"c\":%.1f,\"alarm\":%s,\"err\":%s}",
            i ? "," : "", TEMP_NAMES[i], p.temp[i].celsius,
            p.temp[i].alarm       ? "true" : "false",
            p.temp[i].sensorError ? "true" : "false");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "],\"relayOn\":%s}", p.relayOn ? "true" : "false");

    char topic[64];
    snprintf(topic, sizeof(topic), "cnc/%s/telemetry", _clientId);

    bool ok = _client.publish(topic, buf);
    if (ok) {
        Serial.printf("[MQTT] publish OK — %d bytes  ts=%u\n", pos, ts);
    } else {
        Serial.printf("[MQTT] ✗ publish GAGAL! (payload=%d, bufSize=%d)\n",
                      pos, _client.getBufferSize());
    }
    return ok;
}

// ── Static MQTT message callback ─────────────────────────────────────────
void MqttClient::_onMessage(char* topic, byte* payload, unsigned int len) {
    if (!_instance || !_instance->_cmdCb) return;
    char buf[256];
    memcpy(buf, payload, min(len, 255u));
    buf[min(len, 255u)] = '\0';
    _instance->_cmdCb(topic, buf);
}
