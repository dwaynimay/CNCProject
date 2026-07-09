// Unit tests — self-test evaluation logic (pure C++, no hardware)
// Run: pio test -e native
#include <unity.h>
#include "SelfTest.h"

void setUp()    {}
void tearDown() {}

void test_range_within_bounds_passes() {
    TEST_ASSERT_TRUE(evaluateRange(25.0f, 0.0f, 100.0f));
}

void test_range_below_min_fails() {
    TEST_ASSERT_FALSE(evaluateRange(-1.0f, 0.0f, 100.0f));
}

void test_range_above_max_fails() {
    TEST_ASSERT_FALSE(evaluateRange(101.0f, 0.0f, 100.0f));
}

void test_range_at_exact_bounds_passes() {
    TEST_ASSERT_TRUE(evaluateRange(0.0f, 0.0f, 100.0f));
    TEST_ASSERT_TRUE(evaluateRange(100.0f, 0.0f, 100.0f));
}

void test_range_nan_fails() {
    float nanValue = 0.0f / 0.0f;
    TEST_ASSERT_FALSE(evaluateRange(nanValue, 0.0f, 100.0f));
}

void test_ds18b20_disconnect_value_fails_range_check() {
    // -127 = kode disconnect DS18B20 — harus gagal terhadap rentang suhu wajar
    TEST_ASSERT_FALSE(evaluateRange(-127.0f, 0.0f, 100.0f));
}

void test_alarm_response_fires_when_injected_above_threshold() {
    TEST_ASSERT_TRUE(evaluateAlarmResponse(4.5f, 3.0f));
}

void test_alarm_response_does_not_fire_below_threshold() {
    TEST_ASSERT_FALSE(evaluateAlarmResponse(2.0f, 3.0f));
}

void test_alarm_response_uses_absolute_value() {
    TEST_ASSERT_TRUE(evaluateAlarmResponse(-4.5f, 3.0f));
}

void test_overall_pass_true_when_all_checks_pass() {
    SelfTestResult results[2] = {
        {"Current[0]", "current", true,  ""},
        {"Temp[0]",    "temp",    true,  ""},
    };
    TEST_ASSERT_TRUE(overallPass(results, 2));
}

void test_overall_pass_false_when_one_check_fails() {
    SelfTestResult results[2] = {
        {"Current[0]", "current", true,  ""},
        {"Temp[0]",    "temp",    false, "disconnect"},
    };
    TEST_ASSERT_FALSE(overallPass(results, 2));
}

void test_overall_pass_false_when_no_checks_ran() {
    TEST_ASSERT_FALSE(overallPass(nullptr, 0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_range_within_bounds_passes);
    RUN_TEST(test_range_below_min_fails);
    RUN_TEST(test_range_above_max_fails);
    RUN_TEST(test_range_at_exact_bounds_passes);
    RUN_TEST(test_range_nan_fails);
    RUN_TEST(test_ds18b20_disconnect_value_fails_range_check);
    RUN_TEST(test_alarm_response_fires_when_injected_above_threshold);
    RUN_TEST(test_alarm_response_does_not_fire_below_threshold);
    RUN_TEST(test_alarm_response_uses_absolute_value);
    RUN_TEST(test_overall_pass_true_when_all_checks_pass);
    RUN_TEST(test_overall_pass_false_when_one_check_fails);
    RUN_TEST(test_overall_pass_false_when_no_checks_ran);
    return UNITY_END();
}
