#pragma once
#ifdef UNIT_TEST
#include <stdint.h>
#else
#include <Arduino.h>
#endif
#include "types.h"

// ── Edit file ini untuk sesuaikan dengan hardware ──────────

constexpr int CURRENT_PINS[5]          = {33, 32, 35, 34, 36};
constexpr const char* CURRENT_NAMES[5] = {
    "Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"
};
constexpr ACS712Type CURRENT_TYPES[5]  = {
    ACS712_20A, ACS712_20A, ACS712_20A, ACS712_20A, ACS712_20A
};
constexpr float CURRENT_ALARM[5]       = {3.0f, 3.0f, 3.0f, 2.0f, 3.0f};
constexpr float CURRENT_HYSTERESIS     = 0.5f;   // Mesin baru bisa resume jika arus <= ALARM - HYSTERESIS

constexpr int   ONEWIRE_PIN            = 4;
constexpr const char* TEMP_NAMES[2]    = {"Spindle", "Stepper_Z"};
constexpr float TEMP_ALARM[2]          = {60.0f, 55.0f};
constexpr float TEMP_HYSTERESIS        = 5.0f;   // Mesin baru bisa resume jika suhu <= ALARM - HYSTERESIS

constexpr int  RELAY_PIN               = 5;
constexpr bool RELAY_ACTIVE_LOW        = true;

constexpr int  ESTOP_PIN               = 25;   // HIGH = E-Stop aktif ke CNC Shield

constexpr double VD_RATIO              = 20.0 / 30.0;
constexpr double ADC_MAX               = 4095.0;
constexpr double ADC_VREF_MV           = 3300.0;
constexpr double VCC_ACS_MV            = 5000.0;

constexpr double ACS712_20A_MVPA       = 100.0;  // mV per Ampere — ACS712-20A
constexpr double ACS712_30A_MVPA       = 66.0;   // mV per Ampere — ACS712-30A

constexpr int SAMPLE_CALIBRATION       = 2000;
constexpr int SAMPLE_RUNTIME           = 300;
constexpr uint32_t ADC_SAMPLE_DELAY_US = 200;    // delay antar sample ADC (µs)

constexpr uint32_t DS18B20_CONVERSION_MS = 800;  // waktu konversi DS18B20 12-bit (ms)

constexpr uint32_t NTP_EPOCH_2020      = 1577836800UL;  // 2020-01-01 00:00:00 UTC

// ── [SPRINT-2] Jaringan & Waktu ─────────────────────────────
constexpr const char* NTP_SERVER       = "pool.ntp.org";
constexpr long        TZ_OFFSET_SEC    = 7L * 3600L;  // WIB — UTC+7
constexpr int         DST_OFFSET_SEC   = 0;            // Indonesia tidak pakai DST
constexpr uint32_t    NTP_TIMEOUT_MS   = 10000;        // 10 detik max wait NTP sync

// ── [SPRINT-2] Fail-safe Heartbeat ──────────────────────────
// Jika relay ON dan tidak berhasil publish selama ini → relay OFF (safe state)
constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 60000;  // 60 detik

// ── Named sensor counts (ganti magic number) ─────────────────
constexpr uint8_t NUM_CURRENT_SENSORS  = 5;
constexpr uint8_t NUM_TEMP_SENSORS     = 2;

