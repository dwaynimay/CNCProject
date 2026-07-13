#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include "types.h"
#include "SensorConfig.h"
#include "SelfTest.h"

using MqttCommandCallback = std::function<void(const char* topic, const char* payload)>;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS  = 30000;   // timeout WiFi sebelum reboot (ms)
constexpr uint32_t MQTT_RECONNECT_INTERVAL  = 5000;    // interval reconnect non-blocking (ms)
constexpr uint16_t MQTT_PAYLOAD_BUF_SIZE    = 512;     // buffer JSON telemetry (bytes)
constexpr uint16_t MQTT_SELFTEST_BUF_SIZE   = 3072;    // buffer JSON hasil self-test (bytes) — bisa berisi banyak check
constexpr uint16_t MQTT_CMD_BUF_SIZE        = 256;     // buffer command inbound (bytes)
constexpr uint8_t  MQTT_TOPIC_BUF_SIZE      = 64;      // buffer nama topic (bytes)

class MqttClient {
public:
    MqttClient();
    void begin(const char* ssid, const char* pass,
               const char* host, uint16_t port, const char* clientId);
    void loop();
    bool publish(const MqttPayload& payload);  // true = publish berhasil
    bool publishSelfTestResult(const SelfTestResult* results, uint8_t count);  // true = publish berhasil
    bool isNtpSynced();                         // true = waktu sudah disinkronisasi
    void setCommandCallback(MqttCommandCallback cb);
    bool isConnected();
private:
    WiFiClient          _wifi;
    PubSubClient        _client;
    MqttCommandCallback _cmdCb;
    const char*         _clientId;
    unsigned long       _lastReconnectAttempt = 0;  // untuk non-blocking backoff
    bool                _attemptReconnect();         // single non-blocking attempt
    static void _onMessage(char* topic, byte* payload, unsigned int len);
    static MqttClient* _instance;
};
