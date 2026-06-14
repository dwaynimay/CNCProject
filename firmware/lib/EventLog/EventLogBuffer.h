#pragma once
// Pure C++ ring buffer — no Arduino dependency. Testable in native/host env.
#ifdef UNIT_TEST
#include <stdint.h>
#else
#include <Arduino.h>
#endif

constexpr uint8_t EVENT_LOG_SIZE = 10;

enum class AlarmType : uint8_t {
    OVERCURRENT       = 0,
    OVERTEMP          = 1,
    SENSOR_DISCONNECT = 2,
};

enum class SensorType : uint8_t {
    CURRENT = 0,
    TEMP    = 1,
};

struct EventEntry {
    uint32_t   timestamp;    // Unix epoch (UTC+7 WIB), 0 if NTP not synced
    SensorType sensorType;
    uint8_t    sensorIndex;
    float      value;        // ampere for CURRENT, celsius for TEMP
    AlarmType  alarmType;
};

class EventLogBuffer {
public:
    EventLogBuffer();
    void              add(const EventEntry& e);
    uint8_t           count() const { return _count; }
    const EventEntry& get(uint8_t i) const;  // i=0 = newest, i=count-1 = oldest
protected:
    EventEntry _buf[EVENT_LOG_SIZE];
    uint8_t    _count;
    uint8_t    _head;   // next write slot (ring buffer)
};
