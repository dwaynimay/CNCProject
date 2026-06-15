#pragma once
#ifndef UNIT_TEST
#include <Arduino.h>
#endif

// Log levels — set via -DLOG_LEVEL=N in platformio.ini build_flags
#define LOG_SILENT  0
#define LOG_ERROR   1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_DEBUG   4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

#if LOG_LEVEL >= LOG_ERROR
#  define LOG_E(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#  define LOG_E(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_WARN
#  define LOG_W(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#  define LOG_W(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_INFO
#  define LOG_I(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#  define LOG_I(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_DEBUG
#  define LOG_D(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#  define LOG_D(fmt, ...) ((void)0)
#endif
