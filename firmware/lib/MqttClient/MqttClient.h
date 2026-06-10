#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include "types.h"
#include "SensorConfig.h"

using MqttCommandCallback = std::function<void(const char* topic, const char* payload)>;

// Timeout WiFi connect sebelum reboot (ms)
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS  = 30000;
// Interval antar percobaan reconnect MQTT (ms) — non-blocking
constexpr uint32_t MQTT_RECONNECT_INTERVAL  = 5000;

class MqttClient {
public:
    MqttClient();
    void begin(const char* ssid, const char* pass,
               const char* host, uint16_t port, const char* clientId);
    void loop();
    bool publish(const MqttPayload& payload);  // true = publish berhasil
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
