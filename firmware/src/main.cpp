#include <Arduino.h>
#include "types.h"
#include "SensorConfig.h"
#include "ACS712.h"
#include "DS18B20.h"
#include "Calibration.h"
#include "RelayControl.h"
#include "MqttClient.h"
#include "SerialCLI.h"

// ── Credentials — dipisah agar tidak masuk ke git ─────────
#include "credentials.h"

// Interval kirim data ke broker (ms)
#define PUBLISH_INTERVAL 2000

// ── Instance semua modul ──────────────────────────────────
ACS712 sensors[5] = {
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
#undef cli
SerialCLI      cli(cal, sensors, 5);

unsigned long lastPublish = 0;

// ── Proteksi otomatis ─────────────────────────────────────
void checkAlarms(const MqttPayload& p) {
    for (int i = 0; i < 5; i++) if (p.current[i].alarm) { relay.off(); return; }
    for (int i = 0; i < 2; i++) if (p.temp[i].alarm)    { relay.off(); return; }
}

// ── Perintah dari dashboard via MQTT ─────────────────────
void onCommand(const char* topic, const char* payload) {
    if (strstr(payload, "relay_on"))  { relay.on();  return; }
    if (strstr(payload, "relay_off")) { relay.off(); return; }
    if (strstr(payload, "cal_save"))  { cal.save();  return; }
    if (strstr(payload, "cal_reset")) { cal.reset(); Serial.println(">> Reset default"); return; }

    // cal_offset:<index>  — kalibrasi offset sensor ke-N (mesin harus mati)
    if (strncmp(payload, "cal_offset:", 11) == 0) {
        int idx = atoi(payload + 11);
        if (idx >= 0 && idx < 5) {
            cal.autoOffset(sensors[idx]);
            Serial.printf(">> cal_offset[%d] done vMid=%.3f\n", idx, cal.getData(idx).vMid);
        }
        return;
    }

    // cal_scale:<index>:<ampere_ref>  — kalibrasi skala sensor ke-N dengan referensi arus
    if (strncmp(payload, "cal_scale:", 10) == 0) {
        char buf[32];
        strncpy(buf, payload + 10, sizeof(buf) - 1);
        char* sep = strchr(buf, ':');
        if (sep) {
            *sep = '\0';
            int   idx = atoi(buf);
            float ref = atof(sep + 1);
            if (idx >= 0 && idx < 5 && ref > 0) {
                cal.autoScale(sensors[idx], ref);
                Serial.printf(">> cal_scale[%d] done mVpA=%.3f\n", idx, cal.getData(idx).mVpA);
            }
        }
        return;
    }
}

void setup() {
    Serial.begin(9600);
    cal.load();
    tempSensors.begin();
    relay.begin();
    cli.begin();
    mqtt.setCommandCallback(onCommand);
    mqtt.begin(WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_PORT, MQTT_ID);
}

void loop() {
    mqtt.loop();
    cli.loop();

    if (millis() - lastPublish < PUBLISH_INTERVAL) return;
    lastPublish = millis();

    MqttPayload payload;
    payload.timestamp = millis();
    for (int i = 0; i < 5; i++)
        payload.current[i] = sensors[i].read(cal.getData(i));
    for (int i = 0; i < 2; i++)
        payload.temp[i] = tempSensors.read(i);

    checkAlarms(payload);
    mqtt.publish(payload);
    cli.printLive(payload.current, 5);
}
