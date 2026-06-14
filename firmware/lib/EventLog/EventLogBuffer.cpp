#include "EventLogBuffer.h"

EventLogBuffer::EventLogBuffer() : _count(0), _head(0) {}

void EventLogBuffer::add(const EventEntry& e) {
    _buf[_head] = e;
    _head = (_head + 1) % EVENT_LOG_SIZE;
    if (_count < EVENT_LOG_SIZE) _count++;
}

const EventEntry& EventLogBuffer::get(uint8_t i) const {
    // i=0 is newest: slot = (_head - 1 - i) with wraparound
    // Adding EVENT_LOG_SIZE*2 guarantees positive before modulo
    uint8_t slot = (uint8_t)((_head - 1 - i + EVENT_LOG_SIZE * 2) % EVENT_LOG_SIZE);
    return _buf[slot];
}
