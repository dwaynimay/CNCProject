#pragma once
#include "EventLogBuffer.h"
#ifndef UNIT_TEST
#include <Preferences.h>

// Extends EventLogBuffer with NVS persistence and Serial print.
class EventLog : public EventLogBuffer {
public:
    void load();
    void save();
    void addAndSave(const EventEntry& e);   // add to ring buffer + persist new entry only
    void print() const;
private:
    Preferences _prefs;
    void _writeSlot(Preferences& p, uint8_t slot, const EventEntry& e);
    void _readSlot(Preferences& p, uint8_t slot);
};
#endif // !UNIT_TEST
