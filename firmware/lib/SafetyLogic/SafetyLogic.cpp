#include "SafetyLogic.h"
#include <math.h>

bool isSafeToResume(const float* tempCelsius, const float* tempAlarm, float tempHysteresis, uint8_t nTemp,
                     const float* currentAmpere, const float* currentAlarm, float currentHysteresis, uint8_t nCurrent) {
    for (uint8_t i = 0; i < nTemp; i++) {
        if (tempCelsius[i] > (tempAlarm[i] - tempHysteresis)) return false;
    }
    for (uint8_t i = 0; i < nCurrent; i++) {
        if (fabsf(currentAmpere[i]) > (currentAlarm[i] - currentHysteresis)) return false;
    }
    return true;
}
