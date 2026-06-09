#pragma once
#include <Arduino.h>
#include "SensorConfig.h"

class RelayControl {
public:
    RelayControl();
    void begin();
    void on();
    void off();
    bool isOn() const;
private:
    bool _state;
};
