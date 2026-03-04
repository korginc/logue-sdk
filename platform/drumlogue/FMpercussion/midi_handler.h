/**
 *  @file midi_handler.h
 *  @brief MIDI note processing with probability gating
 */

#pragma once

#include <stdint.h>
#include "prng.h"
#include "fm_voices.h"

// MIDI note to frequency conversion (A4 = 440Hz)
#define A4_MIDI 69
#define A4_FREQ 440.0f

/**
 * Convert MIDI note to frequency (Hz)
 */
fast_inline float midi_note_to_freq(uint8_t note) {
    return A4_FREQ * powf(2.0f, (note - A4_MIDI) / 12.0f);
}

/**
 * Voice trigger event
 */
typedef struct {
    uint8_t voice;      // 0=Kick, 1=Snare, 2=Metal, 3=Perc
    uint8_t note;       // MIDI note number
    uint8_t velocity;   // 0-127
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
    
    // Generate triggers for active voices
    for (int v = 0; v < 4; v++) {
        if (active & (1 << v)) {
            triggers[trigger_count].voice = v;
            triggers[trigger_count].note = note;
            triggers[trigger_count].velocity = velocity;
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

// ========== UNIT TEST ==========
#ifdef TEST_MIDI

void test_midi_mapping() {
    midi_handler_t mh;
    midi_handler_init(&mh);
    
    assert(midi_note_to_freq(69) == 440.0f);  // A4
    assert(midi_note_to_freq(60) == 261.63f); // C4 (approx)
    
    printf("MIDI mapping test PASSED\n");
}

#endif