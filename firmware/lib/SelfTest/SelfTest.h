#pragma once
// Pure C++ self-test evaluation logic — no Arduino dependency. Testable in native/host env.
// The hardware runner that drives sensors/relay/etc lives in src/main.cpp (it needs access
// to the global sensor/relay/calibration/eventlog instances) and uses these pure helpers
// to decide pass/fail for each check.
#ifdef UNIT_TEST
#include <stdint.h>
#include <string.h>
#else
#include <Arduino.h>
#endif

constexpr uint8_t SELFTEST_MAX_CHECKS  = 24;
constexpr uint8_t SELFTEST_DETAIL_SIZE = 48;

struct SelfTestResult {
    const char* name;
    const char* category;
    bool        pass;
    char        detail[SELFTEST_DETAIL_SIZE];
};

// True jika value dalam rentang [min, max] wajar (dipakai utk sanity current/temp reading).
bool evaluateRange(float value, float min, float max);

// True jika nilai yang diinjeksi memang melewati ambang alarm — dipakai utk memverifikasi
// bahwa injeksi overcurrent/overtemp sintetis seharusnya memicu alarm.
bool evaluateAlarmResponse(float injectedValue, float threshold);

// AND semua hasil check — false jika ada satu saja yang gagal (atau count == 0).
bool overallPass(const SelfTestResult* results, uint8_t count);
