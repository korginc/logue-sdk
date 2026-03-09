#pragma once

/**
 * @file midi_handler.h
 * @brief MIDI note processing with probability gating
 */

#include <stdint.h>
#include "prng.h"
#include "constants.h"

// MIDI note to frequency conversion (A4 = 440Hz)
#define A4_MIDI 69
#define A4_FREQ 440.0f

// 12-tone equal temperament ratios
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
 * Convert MIDI note to frequency (Hz)
 */
fast_inline float midi_note_to_freq(uint8_t note) {
    int32_t semitone_offset = (int32_t)note - A4_MIDI;
    float exponent = semitone_offset * (1.0f/12.0f);
    return A4_FREQ * fasterpow2f(exponent);
}

/**
 * Voice trigger event
 */
typedef struct {
    uint8_t voice;      // 0-3
    uint8_t note;       // MIDI note number
    uint8_t velocity;   // 0-127
    float frequency;    // Pre-calculated frequency
} trigger_event_t;

/**
 * MIDI handler state
 */
typedef struct {
    // Note-to-voice mapping (default MIDI notes)
    uint8_t kick_note;     // Default: 36 (C2)
    uint8_t snare_note;    // Default: 38 (D2)
    uint8_t metal_note;    // Default: 42 (F#2)
    uint8_t perc_note;     // Default: 45 (A2)

    // Active notes (for release detection)
    uint8_t active_notes[128];

    // Cached frequencies
    float note_freqs[128];
} midi_handler_t;

/**
 * Initialize MIDI handler
 */
fast_inline void midi_handler_init(midi_handler_t* mh) {
    mh->kick_note = 36;
    mh->snare_note = 38;
    mh->metal_note = 42;
    mh->perc_note = 45;

    for (int i = 0; i < 128; i++) {
        mh->active_notes[i] = 0;
        mh->note_freqs[i] = midi_note_to_freq(i);
    }
}

/**
 * Process MIDI note on - triggers all 4 voices simultaneously
 * Returns number of voices triggered (0-4)
 */
fast_inline uint32_t midi_note_on(
    midi_handler_t* mh,
    neon_prng_t* prng,
    uint8_t note,
    uint8_t velocity,
    uint32_t voice_probs[4],  // [voice0, voice1, voice2, voice3]
    trigger_event_t* triggers
) {
    // All notes trigger all voices (probability determines if they sound)
    uint32x4_t gate = probability_gate_neon(prng,
                                            voice_probs[0],
                                            voice_probs[1],
                                            voice_probs[2],
                                            voice_probs[3]);

    // Extract gate results
    uint32_t gate_bits = 0;
    gate_bits |= (vgetq_lane_u32(gate, 0) ? 1 : 0) << 0;
    gate_bits |= (vgetq_lane_u32(gate, 1) ? 1 : 0) << 1;
    gate_bits |= (vgetq_lane_u32(gate, 2) ? 1 : 0) << 2;
    gate_bits |= (vgetq_lane_u32(gate, 3) ? 1 : 0) << 3;

    if (gate_bits == 0) return 0;

    // Generate triggers for active voices
    uint32_t count = 0;
    float freq = mh->note_freqs[note];

    for (int v = 0; v < 4; v++) {
        if (gate_bits & (1 << v)) {
            triggers[count].voice = v;
            triggers[count].note = note;
            triggers[count].velocity = velocity;
            triggers[count].frequency = freq;
            count++;

            mh->active_notes[note] |= (1 << v);
        }
    }

    return count;
}

/**
 * Process MIDI note off
 */
fast_inline uint32_t midi_note_off(
    midi_handler_t* mh,
    uint8_t note,
    uint8_t* releasing_voices
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