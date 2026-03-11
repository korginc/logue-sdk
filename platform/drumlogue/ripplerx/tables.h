#pragma once
#include "float_math.h"

// Global static table for instant pitch lookups
struct FastTables {
    float note_to_delay_length[128] = {0.0f};

    // Called ONCE during Init()
    inline void generate(float srate) {
        for (int i = 0; i < 128; ++i) {
            // Standard MIDI to Hz formula
            float freq = 440.0f * fasterpowf(2.0f, ((float)i - 69.0f) / 12.0f);

            // Protect against divide-by-zero or sub-sonic frequencies
            // [UT2: DELAY BOUNDS FIX] - Prevent buffer overflow wrap-around
            // 48000 / 12Hz = 4000 samples (safely inside our 4096 buffer)
            if (freq < 12.0f) freq = 12.0f;

            note_to_delay_length[i] = srate / freq;
        }
    }
};

// Expose the global instance
extern FastTables g_tables;