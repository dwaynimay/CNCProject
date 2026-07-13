#pragma once
#include <Arduino.h>
#include <functional>
#include "Calibration.h"
#include "ACS712.h"
#include "EventLog.h"

using SelfTestCallback = std::function<void()>;

class SerialCLI {
public:
    SerialCLI(Calibration& cal, ACS712* sensors, uint8_t n, EventLog& log);
    void begin();
    void loop();
    void printLive(const CurrentReading* readings, uint8_t n);
    void setSelfTestCallback(SelfTestCallback cb);
private:
    Calibration& _cal;
    ACS712*      _sensors;
    uint8_t      _n;
    uint8_t      _active;
    EventLog&    _log;
    bool         _showLive;
    String       _cmdBuf;
    SelfTestCallback _selfTestCb;

    void handleCommand(String cmd);
    void doOffset(int idx);
    void printStatus();
    void printHelp();
};
