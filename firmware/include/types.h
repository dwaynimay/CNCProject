#pragma once
#include <Arduino.h>

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
    bool   scaleDone;
};

struct TempReading {
    uint8_t sensorIndex;
    float   celsius;
    bool    alarm;
};

struct MqttPayload {
    CurrentReading current[5];
    TempReading    temp[2];
    uint32_t       timestamp;
};
