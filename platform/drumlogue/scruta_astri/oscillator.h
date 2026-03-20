#pragma once
#include <cmath>
#include <cstdint>

struct WavetableOsc {
    float phase = 0.0f;
    float phase_inc = 0.0f;

    // Pointer to the active waveform array
    const float* current_table = nullptr;
    size_t table_length = 256;

    // Call this from NoteOn or when the drone pitch is modulated
    inline void set_frequency(float hz, float srate) {
        phase_inc = hz / srate;
    }

    inline float process() {
        if (!current_table) return 0.0f;

        // Advance phase
        phase += phase_inc;
        if (phase >= 1.0f) phase -= 1.0f;

        // Find exact position in the wavetable
        float exact_pos = phase * (float)table_length;
        uint32_t index_A = (uint32_t)exact_pos;
        uint32_t index_B = (index_A + 1) & 255u; // table is always 256 samples (power-of-2)

        // Linear Interpolation for smooth sub-octave pitching
        float frac = exact_pos - (float)index_A;

        return (current_table[index_A] * (1.0f - frac)) + (current_table[index_B] * frac);
    }
};