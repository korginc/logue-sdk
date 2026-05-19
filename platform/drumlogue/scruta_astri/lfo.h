#pragma once
#include <cmath>
#include "float_math.h"

enum LFOWaveform {
    LFO_TRIANGLE = 0,
    LFO_SAW_DOWN,
    LFO_SAW_UP,
    LFO_SQUARE,
    LFO_RANDOM,
    LFO_EXP_DECAY, // ADSR-like percussive strike
    LFO_SINE,
    LFO_PULSE_25,
    LFO_SMOOTH_RANDOM
};

struct FastLFO {
    float phase = 0.0f;
    float phase_inc = 0.0f;
    float current_val = 0.0f;
    int wave_type = LFO_TRIANGLE;
    uint32_t rand_seed = 2463534242UL; // Per-instance RNG state for LFO_RANDOM

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
                    rand_seed ^= rand_seed << 13; rand_seed ^= rand_seed >> 17; rand_seed ^= rand_seed << 5;
                    current_val = ((float)(int32_t)rand_seed / 2147483648.0f);
                }
                break;
            case LFO_EXP_DECAY: { // ADSR-style percussive strike
                // Avoid expf(). Using a cubic polynomial curve (1-x)^3 creates a sharp,
                // CPU-efficient exponential decay from 1.0 down to -1.0.
                float inv = 1.0f - phase;
                current_val = (inv * inv * inv) * 2.0f - 1.0f;
                break;
            }
            case LFO_SINE:
                current_val = fastsinf((phase * 2.0f * 3.14159265359f) - 1.57079632679f);
                break;
            case LFO_PULSE_25:
                current_val = (phase < 0.25f) ? 1.0f : -1.0f;
                break;
            case LFO_SMOOTH_RANDOM: {
                if (phase < phase_inc) {
                    rand_seed ^= rand_seed << 13; rand_seed ^= rand_seed >> 17; rand_seed ^= rand_seed << 5;
                    float target = ((float)(int32_t)rand_seed / 2147483648.0f);
                    current_val = (current_val * 0.7f) + (target * 0.3f);
                }
                break;
            }
            default:
                current_val =  0.0f;
        }
        return current_val;
    }
};
