#pragma once
#include <Arduino.h>
#include "Calibration.h"
#include "ACS712.h"
#include "EventLog.h"

class SerialCLI {
public:
    SerialCLI(Calibration& cal, ACS712* sensors, uint8_t n, EventLog& log);
    void begin();
    void loop();
    void printLive(const CurrentReading* readings, uint8_t n);
private:
    Calibration& _cal;
    ACS712*      _sensors;
    uint8_t      _n;
    uint8_t      _active;
    EventLog&    _log;
    void handleChar(char c);
    void doOffset();
    void printStatus();
    void printHelp();
    void selectSensor();
};
