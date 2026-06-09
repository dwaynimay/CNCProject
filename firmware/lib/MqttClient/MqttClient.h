#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include "types.h"
#include "SensorConfig.h"

using MqttCommandCallback = std::function<void(const char* topic, const char* payload)>;

class MqttClient {
public:
    MqttClient();
    void begin(const char* ssid, const char* pass,
               const char* host, uint16_t port, const char* clientId);
    void loop();
    void publish(const MqttPayload& payload);
    void setCommandCallback(MqttCommandCallback cb);
    bool isConnected();
private:
    WiFiClient          _wifi;
    PubSubClient        _client;
    MqttCommandCallback _cmdCb;
    const char*         _clientId;
    void reconnect();
    static void _onMessage(char* topic, byte* payload, unsigned int len);
    static MqttClient* _instance;
};
