#pragma once

/**
 * @file midi_handler.h
 * @brief MIDI note processing with probability gating
 *
 * Uses fast math approximations from float_math.h for performance
 */

#include <stdint.h>
#include "prng.h"
#include "fm_voices.h"
#include "float_math.h"  // Include KORG's fast math library

// MIDI note to frequency conversion (A4 = 440Hz)
#define A4_MIDI 69
#define A4_FREQ 440.0f

// Semitone ratio = 2^(1/12) - precomputed for speed
#define SEMITONE_RATIO 1.0594630943592953f  // 2^(1/12)

// 12-tone equal temperament ratios (precomputed for direct lookup)
static const float NOTE_RATIOS[12] = {
    1.0f,           // C    (0 semitones)
    1.059463094f,   // C#   (1 semitone)
    1.122462048f,   // D    (2 semitones)
    1.189207115f,   // D#   (3 semitones)
    1.259921050f,   // E    (4 semitones)
    1.334839854f,   // F    (5 semitones)
    1.414213562f,   // F#   (6 semitones)
    1.498307077f,   // G    (7 semitones)
    1.587401052f,   // G#   (8 semitones)
    1.681792830f,   // A    (9 semitones)
    1.781797436f,   // A#   (10 semitones)
    1.887748625f    // B    (11 semitones)
};

/**
 * Convert MIDI note to frequency (Hz) using fast approximation
 *
 * OPTIMIZED: Uses either:
 * 1. Direct table lookup for notes within one octave of reference
 * 2. Fast pow2 approximation for larger intervals
 * 3. Or combination of both for best accuracy/speed tradeoff
 */
fast_inline float midi_note_to_freq(uint8_t note) {
    int32_t semitone_offset = (int32_t)note - A4_MIDI;

    // Method 1: Direct table lookup for notes within one octave
    if (semitone_offset >= -12 && semitone_offset <= 12) {
        // Map to table index (0-11)
        int idx = (semitone_offset + 12) % 12;
        float ratio = NOTE_RATIOS[idx];

        // Multiply by octave factor
        int octave_shift = (semitone_offset + 12) / 12 - 1;
        if (octave_shift >= 0) {
            return A4_FREQ * ratio * (float)(1 << octave_shift);
        } else {
            return A4_FREQ * ratio / (float)(1 << -octave_shift);
        }
    }

    // Method 2: Fast pow2 approximation for larger intervals
    // 2^(semitone_offset/12) = 2^(semitone_offset * (1/12))
    float exponent = semitone_offset * (1.0f/12.0f);

    // Use KORG's fast pow2 approximation from float_math.h
    float ratio = fastpow2f(exponent);  // or fasterpow2f for even faster but less accurate

    return A4_FREQ * ratio;
}

/**
 * Even faster version using only fastpow2f
 * Slightly less accurate but much faster for real-time use
 */
fast_inline float midi_note_to_freq_fast(uint8_t note) {
    float exponent = ((float)note - A4_MIDI) * (1.0f/12.0f);
    return A4_FREQ * fasterpow2f(exponent);  // Using faster version
}

/**
 * Alternative method using direct multiplication for small intervals
 * Useful when processing many notes in a loop
 */
fast_inline void midi_notes_to_freqs_batch(const uint8_t* notes,
                                           float* freqs,
                                           uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        freqs[i] = midi_note_to_freq_fast(notes[i]);
    }
}

/**
 * Convert frequency to MIDI note (approximate)
 * Useful for tuning feedback or display
 */
fast_inline float freq_to_midi_note(float freq_hz) {
    // MIDI note = 69 + 12 * log2(freq/440)
    float ratio = freq_hz / A4_FREQ;

    // Use fastlog2f from float_math.h
    return 69.0f + 12.0f * fastlog2f(ratio);
}

/**
 * Voice trigger event
 */
typedef struct {
    uint8_t voice;      // 0=Kick, 1=Snare, 2=Metal, 3=Perc
    uint8_t note;       // MIDI note number
    uint8_t velocity;   // 0-127
    float frequency;    // Pre-calculated frequency (for engines)
} trigger_event_t;

/**
 * MIDI handler state
 */
typedef struct {
    // Note-to-voice mapping (which note triggers which voice)
    uint8_t kick_note;     // Default: 36 (C2)
    uint8_t snare_note;    // Default: 38 (D2)
    uint8_t metal_note;    // Default: 42 (F#2)
    uint8_t perc_note;     // Default: 45 (A2)

    // Active notes (for release detection)
    uint8_t active_notes[128];

    // Cached frequencies for active notes
    float note_freqs[128];
} midi_handler_t;

/**
 * Initialize MIDI handler with default mappings
 */
fast_inline void midi_handler_init(midi_handler_t* mh) {
    mh->kick_note = 36;   // C2 - Kick
    mh->snare_note = 38;  // D2 - Snare
    mh->metal_note = 42;  // F#2 - Metal
    mh->perc_note = 45;   // A2 - Perc

    for (int i = 0; i < 128; i++) {
        mh->active_notes[i] = 0;
        mh->note_freqs[i] = midi_note_to_freq_fast(i);  // Pre-cache all frequencies
    }
}

/**
 * Process MIDI note on message
 * Returns trigger events for voices that pass probability gate
 */
fast_inline uint32_t midi_note_on(
    midi_handler_t* mh,
    neon_prng_t* prng,
    uint8_t note,
    uint8_t velocity,
    uint32_t probabilities[4],  // [kick, snare, metal, perc]
    trigger_event_t* triggers   // Array to fill (max 4)
) {
    uint32_t trigger_count = 0;

    // Check if this note maps to any voice
    uint32_t voice_mask = 0;
    if (note == mh->kick_note) voice_mask |= 1 << 0;
    if (note == mh->snare_note) voice_mask |= 1 << 1;
    if (note == mh->metal_note) voice_mask |= 1 << 2;
    if (note == mh->perc_note) voice_mask |= 1 << 3;

    if (voice_mask == 0) return 0;  // No matching voice

    // Apply probability gate to all 4 voices at once
    uint32x4_t gate = probability_gate_neon(
        prng,
        probabilities[0],
        probabilities[1],
        probabilities[2],
        probabilities[3]
    );

    // Extract gate results
    uint32_t gate_bits = 0;
    gate_bits |= (vgetq_lane_u32(gate, 0) ? 1 : 0) << 0;
    gate_bits |= (vgetq_lane_u32(gate, 1) ? 1 : 0) << 1;
    gate_bits |= (vgetq_lane_u32(gate, 2) ? 1 : 0) << 2;
    gate_bits |= (vgetq_lane_u32(gate, 3) ? 1 : 0) << 3;

    // Combine with voice mask
    uint32_t active = voice_mask & gate_bits;

    // Get pre-cached frequency
    float freq = mh->note_freqs[note];

    // Generate triggers for active voices
    for (int v = 0; v < 4; v++) {
        if (active & (1 << v)) {
            triggers[trigger_count].voice = v;
            triggers[trigger_count].note = note;
            triggers[trigger_count].velocity = velocity;
            triggers[trigger_count].frequency = freq;
            trigger_count++;

            mh->active_notes[note] |= (1 << v);
        }
    }

    return trigger_count;
}

/**
 * Process MIDI note off message
 * Returns voices that should release
 */
fast_inline uint32_t midi_note_off(
    midi_handler_t* mh,
    uint8_t note,
    uint8_t* releasing_voices  // Array to fill
) {
    uint32_t release_count = 0;
    uint8_t active = mh->active_notes[note];

    for (int v = 0; v < 4; v++) {
        if (active & (1 << v)) {
            releasing_voices[release_count] = v;
            release_count++;
        }
    }

    mh->active_notes[note] = 0;
    return release_count;
}

/**
 * Pre-calculate all frequencies (call after sample rate changes)
 * Not needed for 440Hz reference, but kept for completeness
 */
fast_inline void midi_handler_update_freqs(midi_handler_t* mh) {
    for (int i = 0; i < 128; i++) {
        mh->note_freqs[i] = midi_note_to_freq_fast(i);
    }
}

// ========== UNIT TEST ==========
#ifdef TEST_MIDI

#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_midi_to_freq() {
    printf("Testing MIDI to frequency conversion:\n");
    printf("--------------------------------------\n");

    // Test A4 (69) should be 440 Hz
    float f = midi_note_to_freq(69);
    printf("A4 (69): %f Hz (expected 440.0)\n", f);
    assert(fabsf(f - 440.0f) < 0.1f);

    // Test C4 (60) should be ~261.63 Hz
    f = midi_note_to_freq(60);
    printf("C4 (60): %f Hz (expected ~261.63)\n", f);
    assert(fabsf(f - 261.63f) < 0.5f);

    // Test C5 (72) should be ~523.25 Hz
    f = midi_note_to_freq(72);
    printf("C5 (72): %f Hz (expected ~523.25)\n", f);
    assert(fabsf(f - 523.25f) < 1.0f);

    // Test fast version accuracy
    float f_fast = midi_note_to_freq_fast(69);
    printf("Fast A4: %f Hz\n", f_fast);
    assert(fabsf(f_fast - 440.0f) < 1.0f);

    printf("\nAll MIDI conversion tests PASSED\n");
}

void test_freq_to_midi() {
    float midi = freq_to_midi_note(440.0f);
    printf("440 Hz -> MIDI note: %f (expected 69.0)\n", midi);
    assert(fabsf(midi - 69.0f) < 0.1f);

    midi = freq_to_midi_note(261.63f);
    printf("261.63 Hz -> MIDI note: %f (expected 60.0)\n", midi);
    assert(fabsf(midi - 60.0f) < 0.1f);

    printf("Freq to MIDI test PASSED\n");
}

int main() {
    test_midi_to_freq();
    test_freq_to_midi();
    return 0;
}

#endif // TEST_MIDI