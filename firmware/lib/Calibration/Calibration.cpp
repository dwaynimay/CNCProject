#include "Calibration.h"

Calibration::Calibration() { reset(); }

void Calibration::reset() {
    for (int i = 0; i < 5; i++) {
        double mvpa = (CURRENT_TYPES[i] == ACS712_20A) ? 100.0 : 66.0;
        _cal[i] = { (VCC_ACS_MV / 2.0) * VD_RATIO, mvpa, false, false };
    }
}

bool Calibration::autoOffset(ACS712& sensor) {
    uint8_t idx = sensor.getIndex();
    double avg  = sensor.sampleAvgADC(SAMPLE_CALIBRATION);
    _cal[idx].vMid       = (avg / ADC_MAX) * ADC_VREF_MV;
    _cal[idx].offsetDone = true;
    return true;
}

bool Calibration::autoScale(ACS712& sensor, float ampereRef) {
    if (ampereRef <= 0.0f) return false;
    uint8_t idx  = sensor.getIndex();
    double avg   = sensor.sampleAvgADC(SAMPLE_CALIBRATION);
    double vAvg  = (avg / ADC_MAX) * ADC_VREF_MV;
    double vDelta = _cal[idx].vMid - vAvg;
    _cal[idx].mVpA      = (vDelta / VD_RATIO) / (double)ampereRef;
    _cal[idx].scaleDone = true;
    return true;
}

const CalibrationData& Calibration::getData(uint8_t index) const {
    return _cal[index];
}

void Calibration::save() {
    _prefs.begin("cnc_cal", false);
    for (int i = 0; i < 5; i++) {
        _prefs.putDouble(("vMid"    + String(i)).c_str(), _cal[i].vMid);
        _prefs.putDouble(("mVpA"    + String(i)).c_str(), _cal[i].mVpA);
        _prefs.putBool(  ("offDone" + String(i)).c_str(), _cal[i].offsetDone);
        _prefs.putBool(  ("sclDone" + String(i)).c_str(), _cal[i].scaleDone);
    }
    _prefs.end();
    Serial.println(">> Kalibrasi tersimpan");
}

void Calibration::load() {
    _prefs.begin("cnc_cal", true);
    for (int i = 0; i < 5; i++) {
        String k = "vMid" + String(i);
        if (_prefs.isKey(k.c_str())) {
            _cal[i].vMid       = _prefs.getDouble(k.c_str(), _cal[i].vMid);
            _cal[i].mVpA       = _prefs.getDouble(("mVpA"    + String(i)).c_str(), _cal[i].mVpA);
            _cal[i].offsetDone = _prefs.getBool(  ("offDone" + String(i)).c_str(), false);
            _cal[i].scaleDone  = _prefs.getBool(  ("sclDone" + String(i)).c_str(), false);
        }
    }
    _prefs.end();
    Serial.println(">> Kalibrasi diload dari NVS");
}
