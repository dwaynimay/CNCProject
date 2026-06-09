#pragma once
#include <Arduino.h>
#include "types.h"
#include "SensorConfig.h"

class ACS712 {
public:
    ACS712(uint8_t index, int pin, ACS712Type type);
    CurrentReading read(const CalibrationData& cal, int nSamples = SAMPLE_RUNTIME);
    double         sampleAvgADC(int nSamples);
    uint8_t        getIndex() const { return _index; }
    ACS712Type     getType()  const { return _type; }
private:
    uint8_t    _index;
    int        _pin;
    ACS712Type _type;
    double     mVpA() const;
};
