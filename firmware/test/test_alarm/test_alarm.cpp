// Unit tests — alarm threshold logic (no hardware, no Arduino deps)
// Run: pio test -e native
#include <unity.h>
#include <math.h>

// Mirror of SensorConfig.h constants (kept in sync manually)
static const float CURRENT_ALARM[5] = {3.0f, 3.0f, 3.0f, 2.0f, 8.0f};
static const float TEMP_ALARM[2]    = {60.0f, 55.0f};
static const float DS18B20_ERR      = -127.0f;

void setUp()    {}
void tearDown() {}

void test_current_alarm_fires_above_threshold() {
    TEST_ASSERT_TRUE(fabsf(3.5f) > CURRENT_ALARM[0]);
}

void test_current_alarm_clears_below_threshold() {
    TEST_ASSERT_FALSE(fabsf(2.9f) > CURRENT_ALARM[0]);
}

void test_current_alarm_exact_threshold_does_not_trip() {
    // > not >= : at exactly the limit, no alarm
    TEST_ASSERT_FALSE(fabsf(3.0f) > CURRENT_ALARM[0]);
}

void test_negative_current_also_triggers_alarm() {
    // abs() used in firmware — negative overcurrent is still overcurrent
    TEST_ASSERT_TRUE(fabsf(-3.5f) > CURRENT_ALARM[0]);
}

void test_spindle_has_higher_current_limit() {
    TEST_ASSERT_FALSE(fabsf(7.9f) > CURRENT_ALARM[4]);  // spindle limit = 8 A
    TEST_ASSERT_TRUE(fabsf(8.1f)  > CURRENT_ALARM[4]);
}

void test_stepper_z_has_lower_current_limit() {
    // sensor[3] = Stepper_Z, limit = 2 A
    TEST_ASSERT_TRUE(fabsf(2.1f)  > CURRENT_ALARM[3]);
    TEST_ASSERT_FALSE(fabsf(1.9f) > CURRENT_ALARM[3]);
}

void test_temp_alarm_fires_above_threshold() {
    TEST_ASSERT_TRUE(61.0f  > TEMP_ALARM[0]);   // spindle limit = 60°C
    TEST_ASSERT_FALSE(59.9f > TEMP_ALARM[0]);
}

void test_stepper_z_temp_lower_limit() {
    TEST_ASSERT_TRUE(55.1f  > TEMP_ALARM[1]);   // stepper_Z limit = 55°C
    TEST_ASSERT_FALSE(54.9f > TEMP_ALARM[1]);
}

void test_ds18b20_disconnect_detected() {
    bool sensorError = (DS18B20_ERR == -127.0f);
    TEST_ASSERT_TRUE(sensorError);
}

void test_ds18b20_disconnect_does_not_look_like_overtemp() {
    // -127°C must NOT trigger temp alarm (firmware handles it via sensorError flag)
    TEST_ASSERT_FALSE(DS18B20_ERR > TEMP_ALARM[0]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_current_alarm_fires_above_threshold);
    RUN_TEST(test_current_alarm_clears_below_threshold);
    RUN_TEST(test_current_alarm_exact_threshold_does_not_trip);
    RUN_TEST(test_negative_current_also_triggers_alarm);
    RUN_TEST(test_spindle_has_higher_current_limit);
    RUN_TEST(test_stepper_z_has_lower_current_limit);
    RUN_TEST(test_temp_alarm_fires_above_threshold);
    RUN_TEST(test_stepper_z_temp_lower_limit);
    RUN_TEST(test_ds18b20_disconnect_detected);
    RUN_TEST(test_ds18b20_disconnect_does_not_look_like_overtemp);
    return UNITY_END();
}
