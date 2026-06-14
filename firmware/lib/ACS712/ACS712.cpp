#include "ACS712.h"

ACS712::ACS712(uint8_t index, int pin, ACS712Type type)
    : _index(index), _pin(pin), _type(type) {}

double ACS712::mVpA() const {
    return (_type == ACS712_20A) ? ACS712_20A_MVPA : ACS712_30A_MVPA;
}

// analogReadMilliVolts() uses esp_adc_cal internally — corrects ESP32 ADC non-linearity
// at the extremes (0-150 mV, 3100-3300 mV range) that analogRead() cannot.
double ACS712::sampleAvgMV(int nSamples) {
    double sum = 0;
    for (int i = 0; i < nSamples; i++) {
        sum += analogReadMilliVolts(_pin);
        delayMicroseconds(ADC_SAMPLE_DELAY_US);
    }
    return sum / nSamples;
}

double ACS712::sampleAvgADC(int nSamples) {
    double sum = 0;
    for (int i = 0; i < nSamples; i++) {
        sum += analogRead(_pin);
        delayMicroseconds(ADC_SAMPLE_DELAY_US);
    }
    return sum / nSamples;
}

CurrentReading ACS712::read(const CalibrationData& cal, int nSamples) {
    double vAvg    = sampleAvgMV(nSamples);     // calibrated mV, no linear approximation
    double vDelta  = cal.vMid - vAvg;
    double vActual = vDelta / VD_RATIO;
    float  ampere  = (float)(vActual / cal.mVpA);
    return { _index, ampere, (float)vAvg, (float)cal.vMid,
             (abs(ampere) > CURRENT_ALARM[_index]) };
}
