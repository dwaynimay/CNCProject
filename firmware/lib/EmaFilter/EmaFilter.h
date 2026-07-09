#pragma once
// Pure C++ EMA (Exponential Moving Average) filter — no Arduino dependency.
// Testable in native/host env.

inline float emaUpdate(float raw, float prevFiltered, bool initialized, float alpha) {
    if (!initialized) return raw;
    return alpha * raw + (1.0f - alpha) * prevFiltered;
}
