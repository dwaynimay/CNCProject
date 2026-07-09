#pragma once
#ifdef UNIT_TEST
#include <stdint.h>
#else
#include <Arduino.h>
#endif

enum ACS712Type { ACS712_20A, ACS712_30A };

struct CurrentReading {
    uint8_t  sensorIndex;
    float    ampere;
    float    vAvg;
    float    vMid;
    bool     alarm;
};

struct CalibrationData {
    double vMid;
    double mVpA;
    bool   offsetDone;
};

struct TempReading {
    uint8_t sensorIndex;
    float   celsius;
    bool    alarm;
    bool    sensorError;  // true jika sensor lepas/tidak terbaca (-127°C)
};

struct MqttPayload {
    CurrentReading current[5];
    TempReading    temp[2];
    uint32_t       timestamp;
    bool           relayOn;
};
