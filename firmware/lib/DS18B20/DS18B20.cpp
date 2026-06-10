#include "DS18B20.h"
#include <DallasTemperature.h>   // untuk DEVICE_DISCONNECTED_C

DS18B20Manager::DS18B20Manager()
    : _wire(ONEWIRE_PIN), _sensors(&_wire) {}

void DS18B20Manager::begin()        { _sensors.begin(); }
uint8_t DS18B20Manager::count() { return _sensors.getDeviceCount(); }

TempReading DS18B20Manager::read(uint8_t index) {
    _sensors.requestTemperatures();
    float c = _sensors.getTempCByIndex(index);

    // DEVICE_DISCONNECTED_C = -127.0 → sensor lepas atau gagal baca
    bool sensorErr = (c == DEVICE_DISCONNECTED_C || c < -50.0f);
    bool alarm     = sensorErr || (c > TEMP_ALARM[index]);

    return { index, c, alarm, sensorErr };
}
