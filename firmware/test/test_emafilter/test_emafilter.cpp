// Unit tests — EMA (Exponential Moving Average) current filter (pure C++, no hardware)
// Run: pio test -e native
#include <unity.h>
#include "EmaFilter.h"

void setUp()    {}
void tearDown() {}

void test_first_sample_uses_raw_value() {
    // initialized=false -> filter tidak boleh smoothing sample pertama
    TEST_ASSERT_EQUAL_FLOAT(4.2f, emaUpdate(4.2f, 0.0f, false, 0.3f));
}

void test_alpha_one_tracks_raw_exactly() {
    // alpha=1 -> filtered = raw sepenuhnya, tidak ada smoothing
    TEST_ASSERT_EQUAL_FLOAT(5.0f, emaUpdate(5.0f, 2.0f, true, 1.0f));
}

void test_alpha_zero_never_changes() {
    // alpha=0 -> filtered tetap di nilai sebelumnya, raw diabaikan
    TEST_ASSERT_EQUAL_FLOAT(2.0f, emaUpdate(5.0f, 2.0f, true, 0.0f));
}

void test_partial_alpha_blends_raw_and_previous() {
    // alpha=0.3 -> 0.3*5 + 0.7*2 = 2.9
    TEST_ASSERT_EQUAL_FLOAT(2.9f, emaUpdate(5.0f, 2.0f, true, 0.3f));
}

void test_converges_towards_stable_input() {
    float filtered = 0.0f;
    bool  init      = false;
    for (int i = 0; i < 200; i++) {
        filtered = emaUpdate(3.0f, filtered, init, 0.3f);
        init = true;
    }
    // setelah banyak sample dengan raw konstan, filter harus konvergen ke raw
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, filtered);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_first_sample_uses_raw_value);
    RUN_TEST(test_alpha_one_tracks_raw_exactly);
    RUN_TEST(test_alpha_zero_never_changes);
    RUN_TEST(test_partial_alpha_blends_raw_and_previous);
    RUN_TEST(test_converges_towards_stable_input);
    return UNITY_END();
}
