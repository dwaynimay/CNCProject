#pragma once
#include <Preferences.h>
#include "types.h"
#include "SensorConfig.h"
#include "ACS712.h"

class Calibration {
public:
    Calibration();
    bool autoOffset(ACS712& sensor);
    const CalibrationData& getData(uint8_t index) const;
    void save();
    void load();
    void reset();
private:
    CalibrationData _cal[5];
    Preferences     _prefs;
};
