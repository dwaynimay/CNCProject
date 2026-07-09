// Unit tests — resume-safety hysteresis logic (pure C++, no hardware)
// Run: pio test -e native
#include <unity.h>
#include "SafetyLogic.h"

void setUp()    {}
void tearDown() {}

// Setup mirip SensorConfig.h: 2 sensor suhu (alarm 60/55, hyst 5), 5 sensor arus (alarm 3/3/3/2/3, hyst 0.5)
static const float TEMP_ALARM_T[2]    = {60.0f, 55.0f};
static const float CURRENT_ALARM_T[5] = {3.0f, 3.0f, 3.0f, 2.0f, 3.0f};

void test_safe_when_all_readings_well_below_threshold() {
    float temp[2] = {20.0f, 20.0f};
    float cur[5]  = {0.1f, 0.1f, 0.1f, 0.1f, 0.1f};
    TEST_ASSERT_TRUE(isSafeToResume(temp, TEMP_ALARM_T, 5.0f, 2, cur, CURRENT_ALARM_T, 0.5f, 5));
}

void test_unsafe_when_temp_still_in_hysteresis_band() {
    // Alarm suhu[0]=60, hyst=5 -> aman jika <=55. 56 masih di zona bahaya.
    float temp[2] = {56.0f, 20.0f};
    float cur[5]  = {0.1f, 0.1f, 0.1f, 0.1f, 0.1f};
    TEST_ASSERT_FALSE(isSafeToResume(temp, TEMP_ALARM_T, 5.0f, 2, cur, CURRENT_ALARM_T, 0.5f, 5));
}

void test_safe_exactly_at_hysteresis_boundary() {
    // Tepat di batas (55.0 == 60-5) -> tidak > threshold, jadi aman
    float temp[2] = {55.0f, 20.0f};
    float cur[5]  = {0.1f, 0.1f, 0.1f, 0.1f, 0.1f};
    TEST_ASSERT_TRUE(isSafeToResume(temp, TEMP_ALARM_T, 5.0f, 2, cur, CURRENT_ALARM_T, 0.5f, 5));
}

void test_unsafe_when_current_still_in_hysteresis_band() {
    // Alarm arus[3]=2.0 (Stepper_Z), hyst=0.5 -> aman jika <=1.5. 1.6 masih bahaya.
    float temp[2] = {20.0f, 20.0f};
    float cur[5]  = {0.1f, 0.1f, 0.1f, 1.6f, 0.1f};
    TEST_ASSERT_FALSE(isSafeToResume(temp, TEMP_ALARM_T, 5.0f, 2, cur, CURRENT_ALARM_T, 0.5f, 5));
}

void test_negative_current_uses_absolute_value() {
    float temp[2] = {20.0f, 20.0f};
    float cur[5]  = {0.1f, 0.1f, 0.1f, -1.6f, 0.1f};
    TEST_ASSERT_FALSE(isSafeToResume(temp, TEMP_ALARM_T, 5.0f, 2, cur, CURRENT_ALARM_T, 0.5f, 5));
}

void test_one_bad_sensor_among_many_blocks_resume() {
    // Semua sensor aman kecuali satu suhu -> tetap ditolak
    float temp[2] = {20.0f, 51.0f};  // stepper_Z alarm=55, hyst=5 -> aman<=50, 51 bahaya
    float cur[5]  = {0.1f, 0.1f, 0.1f, 0.1f, 0.1f};
    TEST_ASSERT_FALSE(isSafeToResume(temp, TEMP_ALARM_T, 5.0f, 2, cur, CURRENT_ALARM_T, 0.5f, 5));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_safe_when_all_readings_well_below_threshold);
    RUN_TEST(test_unsafe_when_temp_still_in_hysteresis_band);
    RUN_TEST(test_safe_exactly_at_hysteresis_boundary);
    RUN_TEST(test_unsafe_when_current_still_in_hysteresis_band);
    RUN_TEST(test_negative_current_uses_absolute_value);
    RUN_TEST(test_one_bad_sensor_among_many_blocks_resume);
    return UNITY_END();
}
