#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include "types.h"
#include "SensorConfig.h"

class DS18B20Manager {
public:
    DS18B20Manager();
    void        begin();
    TempReading read(uint8_t index);
    uint8_t     count();
private:
    OneWire           _wire;
    DallasTemperature _sensors;
};
