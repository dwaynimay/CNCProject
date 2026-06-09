#include "DS18B20.h"

DS18B20Manager::DS18B20Manager()
    : _wire(ONEWIRE_PIN), _sensors(&_wire) {}

void DS18B20Manager::begin()        { _sensors.begin(); }
uint8_t DS18B20Manager::count() { return _sensors.getDeviceCount(); }

TempReading DS18B20Manager::read(uint8_t index) {
    _sensors.requestTemperatures();
    float c = _sensors.getTempCByIndex(index);
    return { index, c, (c > TEMP_ALARM[index]) };
}
