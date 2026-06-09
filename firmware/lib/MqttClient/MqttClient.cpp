#include "MqttClient.h"

// Topic: cnc/<clientId>/telemetry  (ESP32 → broker)
//        cnc/<clientId>/command    (broker → ESP32)

MqttClient* MqttClient::_instance = nullptr;

MqttClient::MqttClient() : _client(_wifi) { _instance = this; }

void MqttClient::setCommandCallback(MqttCommandCallback cb) { _cmdCb = cb; }
bool MqttClient::isConnected() { return _client.connected(); }

void MqttClient::begin(const char* ssid, const char* pass,
                       const char* host, uint16_t port, const char* clientId) {
    _clientId = clientId;
    WiFi.begin(ssid, pass);
    Serial.print("WiFi");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.printf(" OK: %s\n", WiFi.localIP().toString().c_str());
    _client.setBufferSize(512);   // safety net — pastikan buffer cukup walau build flag gagal
    _client.setServer(host, port);
    _client.setCallback(_onMessage);
    reconnect();
}

void MqttClient::loop() {
    if (!_client.connected()) reconnect();
    _client.loop();
}

void MqttClient::publish(const MqttPayload& p) {
    // JSON manual (hindari ArduinoJson dependency di sini)
    char buf[512];
    int  pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "{\"ts\":%lu,\"current\":[", p.timestamp);
    for (int i = 0; i < 5; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"id\":\"%s\",\"a\":%.3f,\"alarm\":%s}",
            i ? "," : "", CURRENT_NAMES[i], p.current[i].ampere,
            p.current[i].alarm ? "true" : "false");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "],\"temp\":[");
    for (int i = 0; i < 2; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos,
            "%s{\"id\":\"%s\",\"c\":%.1f,\"alarm\":%s}",
            i ? "," : "", TEMP_NAMES[i], p.temp[i].celsius,
            p.temp[i].alarm ? "true" : "false");
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "]}");

    char topic[64];
    snprintf(topic, sizeof(topic), "cnc/%s/telemetry", _clientId);

    Serial.printf("[MQTT] publish %d bytes ke %s\n", pos, topic);
    bool ok = _client.publish(topic, buf);
    if (!ok) Serial.printf("[MQTT] ✗ publish GAGAL! (payload=%d, bufSize=%d)\n", pos, _client.getBufferSize());
}

void MqttClient::reconnect() {
    while (!_client.connected()) {
        Serial.print("MQTT...");
        if (_client.connect(_clientId)) {
            Serial.println("OK");
            char topic[64];
            snprintf(topic, sizeof(topic), "cnc/%s/command", _clientId);
            _client.subscribe(topic);
        } else {
            Serial.printf("gagal rc=%d, retry 3s\n", _client.state());
            delay(3000);
        }
    }
}

void MqttClient::_onMessage(char* topic, byte* payload, unsigned int len) {
    if (!_instance || !_instance->_cmdCb) return;
    char buf[256];
    memcpy(buf, payload, min(len, 255u));
    buf[min(len, 255u)] = '\0';
    _instance->_cmdCb(topic, buf);
}
