#pragma once
#include <cmath>
#include "float_math.h"

enum LFOWaveform {
    LFO_TRIANGLE = 0,
    LFO_SAW_DOWN,
    LFO_SAW_UP,
    LFO_SQUARE,
    LFO_RANDOM,
    LFO_EXP_DECAY // ADSR-like percussive strike
};

struct FastLFO {
    float phase = 0.0f;
    float phase_inc = 0.0f;
    float current_val = 0.0f;
    int wave_type = LFO_TRIANGLE;

    inline void set_rate(float hz, float srate) {
        phase_inc = hz / srate;
    }

    inline float process() {
        phase += phase_inc;
        if (phase >= 1.0f) phase -= 1.0f;

        switch (wave_type) {
            case LFO_TRIANGLE:
                current_val = 2.0f * fabsf(2.0f * phase - 1.0f) - 1.0f; // -1.0 to 1.0
                break;
            case LFO_SAW_DOWN:
                current_val = 1.0f - (2.0f * phase); // 1.0 to -1.0
                break;
            case LFO_SAW_UP:
                current_val = (2.0f * phase) - 1.0f; // -1.0 to 1.0
                break;
            case LFO_SQUARE:
                current_val = (phase < 0.5f) ? 1.0f : -1.0f; // 1.0 or -1.0
                break;
            case LFO_RANDOM:
                if (phase < phase_inc) {
                    static uint32_t seed = 2463534242UL;
                    seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                    current_val = ((float)(int32_t)seed / 2147483648.0f);
                }
                break;
            case LFO_EXP_DECAY:
                // ADSR-like shape: Snaps to 1.0 at phase 0, then exponentially decays to 0.0
                current_val = fasterpowf(2.71828f, -5.0f * phase);
                break;
        }
        return current_val;
    }
};