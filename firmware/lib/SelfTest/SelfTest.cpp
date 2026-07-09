#include "SelfTest.h"
#include <math.h>

bool evaluateRange(float value, float min, float max) {
    if (isnan(value)) return false;
    return value >= min && value <= max;
}

bool evaluateAlarmResponse(float injectedValue, float threshold) {
    return fabsf(injectedValue) > threshold;
}

bool overallPass(const SelfTestResult* results, uint8_t count) {
    if (count == 0) return false;
    for (uint8_t i = 0; i < count; i++) {
        if (!results[i].pass) return false;
    }
    return true;
}
