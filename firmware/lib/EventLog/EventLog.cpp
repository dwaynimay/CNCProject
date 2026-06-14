#include "EventLogBuffer.h"
#ifndef UNIT_TEST
#include "EventLog.h"

static const char* NVS_NS = "cnc_ev";

// ── NVS slot helpers ──────────────────────────────────────────────────────────
// Key naming: "ts0".."ts9", "st0".."st9", "si0".."si9", "vl0".."vl9", "at0".."at9"
// Max NVS key length is 15 chars — these are all ≤4 chars.

void EventLog::_writeSlot(Preferences& p, uint8_t slot, const EventEntry& e) {
    char key[6];
    snprintf(key, sizeof(key), "ts%d", slot); p.putUInt( key, e.timestamp);
    snprintf(key, sizeof(key), "st%d", slot); p.putUChar(key, (uint8_t)e.sensorType);
    snprintf(key, sizeof(key), "si%d", slot); p.putUChar(key, e.sensorIndex);
    snprintf(key, sizeof(key), "vl%d", slot); p.putFloat(key, e.value);
    snprintf(key, sizeof(key), "at%d", slot); p.putUChar(key, (uint8_t)e.alarmType);
}

void EventLog::_readSlot(Preferences& p, uint8_t slot) {
    char key[6];
    snprintf(key, sizeof(key), "ts%d", slot); _buf[slot].timestamp   = p.getUInt( key, 0);
    snprintf(key, sizeof(key), "st%d", slot); _buf[slot].sensorType  = (SensorType)p.getUChar(key, 0);
    snprintf(key, sizeof(key), "si%d", slot); _buf[slot].sensorIndex = p.getUChar(key, 0);
    snprintf(key, sizeof(key), "vl%d", slot); _buf[slot].value       = p.getFloat(key, 0.0f);
    snprintf(key, sizeof(key), "at%d", slot); _buf[slot].alarmType   = (AlarmType)p.getUChar(key, 0);
}

// ── Public API ────────────────────────────────────────────────────────────────

void EventLog::load() {
    _prefs.begin(NVS_NS, true);
    _head  = _prefs.getUChar("head", 0);
    _count = _prefs.getUChar("cnt",  0);
    if (_count > EVENT_LOG_SIZE) _count = 0;  // corrupt NVS — reset
    for (uint8_t slot = 0; slot < EVENT_LOG_SIZE; slot++) _readSlot(_prefs, slot);
    _prefs.end();
    Serial.printf(">> EventLog: %d entri dimuat dari NVS\n", _count);
}

void EventLog::save() {
    _prefs.begin(NVS_NS, false);
    _prefs.putUChar("head", _head);
    _prefs.putUChar("cnt",  _count);
    for (uint8_t slot = 0; slot < EVENT_LOG_SIZE; slot++) _writeSlot(_prefs, slot, _buf[slot]);
    _prefs.end();
}

void EventLog::addAndSave(const EventEntry& e) {
    add(e);
    uint8_t slot = (_head == 0) ? EVENT_LOG_SIZE - 1 : _head - 1;
    _prefs.begin(NVS_NS, false);
    _prefs.putUChar("head", _head);
    _prefs.putUChar("cnt",  _count);
    _writeSlot(_prefs, slot, e);
    _prefs.end();
}

void EventLog::print() const {
    if (_count == 0) {
        Serial.println(">> Trip log kosong");
        return;
    }
    Serial.printf(">> Trip log (%d entri, terbaru lebih dulu):\n", _count);
    static const char* typeStr[]  = {"CURRENT", "TEMP"};
    static const char* alarmStr[] = {"OVERCURRENT", "OVERTEMP", "SENSOR_DISC"};
    for (uint8_t i = 0; i < _count; i++) {
        const EventEntry& e = get(i);
        Serial.printf("  [%d] ts=%lu  %-7s sensor[%d]  %.2f  %s\n",
            i,
            (unsigned long)e.timestamp,
            typeStr[(uint8_t)e.sensorType],
            e.sensorIndex,
            e.value,
            alarmStr[(uint8_t)e.alarmType]);
    }
}
#endif // !UNIT_TEST
