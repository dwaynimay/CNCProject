#include "ACS712.h"

ACS712::ACS712(uint8_t index, int pin, ACS712Type type)
    : _index(index), _pin(pin), _type(type) {}

double ACS712::mVpA() const {
    return (_type == ACS712_20A) ? 100.0 : 66.0;
}

double ACS712::sampleAvgADC(int nSamples) {
    double sum = 0;
    for (int i = 0; i < nSamples; i++) {
        sum += analogRead(_pin);
        delayMicroseconds(200);
    }
    return sum / nSamples;
}

CurrentReading ACS712::read(const CalibrationData& cal, int nSamples) {
    double avgAdc  = sampleAvgADC(nSamples);
    double vAvg    = (avgAdc / ADC_MAX) * ADC_VREF_MV;
    double vDelta  = cal.vMid - vAvg;
    double vActual = vDelta / VD_RATIO;
    float  ampere  = (float)(vActual / cal.mVpA);
    return { _index, ampere, (float)vAvg, (float)cal.vMid,
             (abs(ampere) > CURRENT_ALARM[_index]) };
}
