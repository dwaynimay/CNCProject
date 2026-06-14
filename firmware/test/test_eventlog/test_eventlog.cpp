// Unit tests — EventLogBuffer ring buffer (pure C++, no hardware)
// Run: pio test -e native
#include <unity.h>
#define UNIT_TEST
#include "EventLogBuffer.h"

void setUp()    {}
void tearDown() {}

static EventEntry makeEntry(uint32_t ts, float val, SensorType st = SensorType::CURRENT) {
    EventEntry e;
    e.timestamp   = ts;
    e.sensorType  = st;
    e.sensorIndex = 0;
    e.value       = val;
    e.alarmType   = AlarmType::OVERCURRENT;
    return e;
}

void test_empty_log_has_zero_count() {
    EventLogBuffer log;
    TEST_ASSERT_EQUAL(0, log.count());
}

void test_add_single_entry() {
    EventLogBuffer log;
    log.add(makeEntry(1000, 4.5f));
    TEST_ASSERT_EQUAL(1, log.count());
    TEST_ASSERT_EQUAL_FLOAT(4.5f, log.get(0).value);
    TEST_ASSERT_EQUAL(1000UL, (unsigned long)log.get(0).timestamp);
}

void test_newest_is_index_zero() {
    EventLogBuffer log;
    log.add(makeEntry(1000, 1.0f));
    log.add(makeEntry(2000, 2.0f));
    // get(0) = newest = 2.0
    TEST_ASSERT_EQUAL_FLOAT(2.0f, log.get(0).value);
    // get(1) = older = 1.0
    TEST_ASSERT_EQUAL_FLOAT(1.0f, log.get(1).value);
}

void test_ring_buffer_wraps_at_max_size() {
    EventLogBuffer log;
    for (uint8_t i = 0; i < 15; i++)
        log.add(makeEntry((uint32_t)i * 1000, (float)i));

    TEST_ASSERT_EQUAL(EVENT_LOG_SIZE, log.count());
    TEST_ASSERT_EQUAL_FLOAT(14.0f, log.get(0).value);  // newest
    TEST_ASSERT_EQUAL_FLOAT(5.0f,  log.get(9).value);  // oldest (entries 0..4 evicted)
}

void test_sensor_type_preserved() {
    EventLogBuffer log;
    log.add(makeEntry(1000, 65.0f, SensorType::TEMP));
    TEST_ASSERT_EQUAL((uint8_t)SensorType::TEMP, (uint8_t)log.get(0).sensorType);
}

void test_alarm_type_preserved() {
    EventLogBuffer log;
    EventEntry e = makeEntry(1000, -127.0f);
    e.alarmType   = AlarmType::SENSOR_DISCONNECT;
    log.add(e);
    TEST_ASSERT_EQUAL((uint8_t)AlarmType::SENSOR_DISCONNECT, (uint8_t)log.get(0).alarmType);
}

void test_exact_max_entries_no_overflow() {
    EventLogBuffer log;
    for (uint8_t i = 0; i < EVENT_LOG_SIZE; i++)
        log.add(makeEntry((uint32_t)i, (float)i));
    TEST_ASSERT_EQUAL(EVENT_LOG_SIZE, log.count());
    TEST_ASSERT_EQUAL_FLOAT((float)(EVENT_LOG_SIZE - 1), log.get(0).value);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, log.get(EVENT_LOG_SIZE - 1).value);
}

void test_single_overwrite() {
    EventLogBuffer log;
    for (uint8_t i = 0; i < EVENT_LOG_SIZE + 1; i++)
        log.add(makeEntry((uint32_t)i, (float)i));
    TEST_ASSERT_EQUAL(EVENT_LOG_SIZE, log.count());
    TEST_ASSERT_EQUAL_FLOAT((float)EVENT_LOG_SIZE, log.get(0).value);  // newest = 10
    TEST_ASSERT_EQUAL_FLOAT(1.0f, log.get(9).value);                   // oldest = 1 (0 evicted)
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_empty_log_has_zero_count);
    RUN_TEST(test_add_single_entry);
    RUN_TEST(test_newest_is_index_zero);
    RUN_TEST(test_ring_buffer_wraps_at_max_size);
    RUN_TEST(test_sensor_type_preserved);
    RUN_TEST(test_alarm_type_preserved);
    RUN_TEST(test_exact_max_entries_no_overflow);
    RUN_TEST(test_single_overwrite);
    return UNITY_END();
}
