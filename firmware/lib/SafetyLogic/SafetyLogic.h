#pragma once
// Pure C++ resume-safety check — no Arduino dependency. Testable in native/host env.
#ifdef UNIT_TEST
#include <stdint.h>
#else
#include <Arduino.h>
#endif

// True jika mesin aman untuk resume (relay_off): semua suhu & arus sudah turun
// di bawah (ALARM - HYSTERESIS). Mirror dari cek resume di main.cpp::onCommand().
bool isSafeToResume(const float* tempCelsius, const float* tempAlarm, float tempHysteresis, uint8_t nTemp,
                     const float* currentAmpere, const float* currentAlarm, float currentHysteresis, uint8_t nCurrent);
