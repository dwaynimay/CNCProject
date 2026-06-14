#include "Calibration.h"

Calibration::Calibration() { reset(); }

void Calibration::reset() {
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        double mvpa = (CURRENT_TYPES[i] == ACS712_20A) ? ACS712_20A_MVPA : ACS712_30A_MVPA;
        _cal[i] = { (VCC_ACS_MV / 2.0) * VD_RATIO, mvpa, false };
    }
}

bool Calibration::autoOffset(ACS712& sensor) {
    uint8_t idx = sensor.getIndex();
    _cal[idx].vMid       = sensor.sampleAvgMV(SAMPLE_CALIBRATION);  // calibrated mV
    _cal[idx].offsetDone = true;
    return true;
}


const CalibrationData& Calibration::getData(uint8_t index) const {
    return _cal[index];
}

void Calibration::save() {
    _prefs.begin("cnc_cal", false);
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        _prefs.putDouble(("vMid"    + String(i)).c_str(), _cal[i].vMid);
        _prefs.putDouble(("mVpA"    + String(i)).c_str(), _cal[i].mVpA);
        _prefs.putBool(  ("offDone" + String(i)).c_str(), _cal[i].offsetDone);
    }
    _prefs.end();
    Serial.println(">> Kalibrasi tersimpan");
}

void Calibration::load() {
    _prefs.begin("cnc_cal", true);
    for (int i = 0; i < NUM_CURRENT_SENSORS; i++) {
        String k = "vMid" + String(i);
        if (_prefs.isKey(k.c_str())) {
            _cal[i].vMid       = _prefs.getDouble(k.c_str(), _cal[i].vMid);
            _cal[i].mVpA       = _prefs.getDouble(("mVpA"    + String(i)).c_str(), _cal[i].mVpA);
            _cal[i].offsetDone = _prefs.getBool(  ("offDone" + String(i)).c_str(), false);
        }
    }
    _prefs.end();
    Serial.println(">> Kalibrasi diload dari NVS");
}
