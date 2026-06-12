#pragma once

/**
 * @file midi_handler.h
 * @brief Lightweight MIDI helper layer for selector-based Sonaglio
 *
 * Current Sonaglio routing is handled in fm_perc_synth_note_on().
 * This file should not contain the old four-probability trigger model anymore.
 *
 * Responsibilities kept here:
 * - MIDI note <-> frequency helpers
 * - default drum note constants
 * - active-note bookkeeping
 * - cached MIDI frequencies
 */

#include <stdint.h>
#include "constants.h"
#include "float_math.h"

#ifndef MIDI_HANDLER_INLINE
#define MIDI_HANDLER_INLINE fast_inline
#endif

/**
 * Default drum notes.
 *
 * These are still useful for host / external MIDI conventions, even though
 * the reduced selector model no longer maps each note to a separate parallel
 * probability voice.
 */
#define MIDI_NOTE_KICK   36  // C2
#define MIDI_NOTE_SNARE  38  // D2
#define MIDI_NOTE_METAL  42  // F#2 / closed-hat convention
#define MIDI_NOTE_PERC   45  // A2 / tom-perc convention

/**
 * 12-tone equal temperament ratios within one octave.
 *
 * Kept for the accurate scalar helper. The hot engine path usually uses the
 * NEON exp2 helper from fm_voices.h after Euclidean/scatter offsets are applied.
 */
static const float MIDI_NOTE_RATIOS[12] = {
    1.0f,           // C
    1.059463094f,   // C#
    1.122462048f,   // D
    1.189207115f,   // D#
    1.259921050f,   // E
    1.334839854f,   // F
    1.414213562f,   // F#
    1.498307077f,   // G
    1.587401052f,   // G#
    1.681792830f,   // A
    1.781797436f,   // A#
    1.887748625f    // B
};

/**
 * Accurate-ish scalar MIDI note to frequency for integer notes.
 *
 * This is mostly for cached display/helper use. The active engines generally
 * receive the MIDI note and convert it with exp2_neon() internally.
 */
MIDI_HANDLER_INLINE float midi_note_to_freq(uint8_t note) {
    int32_t semitone_offset = (int32_t)note - (int32_t)A4_MIDI;

    if (semitone_offset >= -12 && semitone_offset <= 12) {
        int idx = (semitone_offset + 12) % 12;
        float ratio = MIDI_NOTE_RATIOS[idx];

        int octave_shift = (semitone_offset + 12) / 12 - 1;
        if (octave_shift >= 0) {
            return A4_FREQ * ratio * (float)(1 << octave_shift);
        }

        return A4_FREQ * ratio / (float)(1 << -octave_shift);
    }

    float exponent = (float)semitone_offset * (1.0f / 12.0f);
    return A4_FREQ * fastpow2f(exponent);
}

/**
 * Fast scalar MIDI note to frequency.
 */
MIDI_HANDLER_INLINE float midi_note_to_freq_fast(uint8_t note) {
    float exponent = ((float)note - A4_MIDI) * (1.0f / 12.0f);
    return A4_FREQ * fasterpow2f(exponent);
}

/**
 * Fractional MIDI note to frequency.
 *
 * Useful for Euclidean offsets, scatter detune, and future display tools.
 */
MIDI_HANDLER_INLINE float midi_note_float_to_freq_fast(float note) {
    float exponent = (note - A4_MIDI) * (1.0f / 12.0f);
    return A4_FREQ * fasterpow2f(exponent);
}

/**
 * Batch helper for tests/tools.
 */
MIDI_HANDLER_INLINE void midi_notes_to_freqs_batch(const uint8_t* notes,
                                                   float* freqs,
                                                   uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        freqs[i] = midi_note_to_freq_fast(notes[i]);
    }
}

/**
 * Convert frequency to MIDI note.
 *
 * Guard against zero/negative input.
 */
MIDI_HANDLER_INLINE float freq_to_midi_note(float freq_hz) {
    if (freq_hz <= 0.0f) {
        return 0.0f;
    }

    float ratio = freq_hz / A4_FREQ;
    return A4_MIDI + 12.0f * fastlog2f(ratio);
}

/**
 * MIDI handler state.
 *
 * The selector-based synth does not need per-voice probability state here.
 * active_notes[] remains useful for note-off / pending-trigger cancellation.
 */
typedef struct {
    uint8_t kick_note;
    uint8_t snare_note;
    uint8_t metal_note;
    uint8_t perc_note;

    // 1 if the MIDI note is currently held.
    uint8_t active_notes[128];

    // Cached scalar frequencies for integer MIDI notes.
    float note_freqs[128];
} midi_handler_t;

/**
 * Initialize MIDI handler with default mappings and frequency cache.
 */
MIDI_HANDLER_INLINE void midi_handler_init(midi_handler_t* mh) {
    mh->kick_note  = MIDI_NOTE_KICK;
    mh->snare_note = MIDI_NOTE_SNARE;
    mh->metal_note = MIDI_NOTE_METAL;
    mh->perc_note  = MIDI_NOTE_PERC;

    for (int i = 0; i < 128; ++i) {
        mh->active_notes[i] = 0;
        mh->note_freqs[i] = midi_note_to_freq_fast((uint8_t)i);
    }
}

/**
 * Mark a note as active.
 */
MIDI_HANDLER_INLINE void midi_handler_note_on(midi_handler_t* mh,
                                              uint8_t note) {
    if (note < 128) {
        mh->active_notes[note] = 1;
    }
}

/**
 * Mark a note as inactive.
 *
 * Returns non-zero if the note had been active.
 */
MIDI_HANDLER_INLINE uint8_t midi_handler_note_off(midi_handler_t* mh,
                                                  uint8_t note) {
    if (note >= 128) {
        return 0;
    }

    uint8_t was_active = mh->active_notes[note];
    mh->active_notes[note] = 0;
    return was_active;
}

/**
 * Check whether a note is active.
 */
MIDI_HANDLER_INLINE uint8_t midi_handler_is_note_active(const midi_handler_t* mh,
                                                        uint8_t note) {
    return (note < 128) ? mh->active_notes[note] : 0;
}

/**
 * Get cached integer-note frequency.
 */
MIDI_HANDLER_INLINE float midi_handler_note_freq(const midi_handler_t* mh,
                                                 uint8_t note) {
    return (note < 128) ? mh->note_freqs[note] : 0.0f;
}

/**
 * Refresh cached note frequencies.
 */
MIDI_HANDLER_INLINE void midi_handler_update_freqs(midi_handler_t* mh) {
    for (int i = 0; i < 128; ++i) {
        mh->note_freqs[i] = midi_note_to_freq_fast((uint8_t)i);
    }
}

/**
 * Optional helper: classify a MIDI note against default drum note constants.
 *
 * This is not used for core selector routing, but can be useful if a future
 * mode wants external drum notes to influence presets or instrument selection.
 *
 * Return:
 *   0 = kick note
 *   1 = snare note
 *   2 = metal note
 *   3 = perc note
 *   255 = generic/unmatched note
 */
MIDI_HANDLER_INLINE uint8_t midi_handler_classify_note(const midi_handler_t* mh,
                                                       uint8_t note) {
    if (note == mh->kick_note)  return 0;
    if (note == mh->snare_note) return 1;
    if (note == mh->metal_note) return 2;
    if (note == mh->perc_note)  return 3;
    return 255;
}

// ========== UNIT TEST ==========
#ifdef TEST_MIDI

#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_midi_to_freq() {
    float f = midi_note_to_freq(69);
    assert(fabsf(f - 440.0f) < 0.1f);

    f = midi_note_to_freq(60);
    assert(fabsf(f - 261.63f) < 0.5f);

    f = midi_note_to_freq(72);
    assert(fabsf(f - 523.25f) < 1.0f);

    float f_fast = midi_note_to_freq_fast(69);
    assert(fabsf(f_fast - 440.0f) < 1.0f);

    printf("MIDI conversion tests PASSED\n");
}

void test_freq_to_midi() {
    float midi = freq_to_midi_note(440.0f);
    assert(fabsf(midi - 69.0f) < 0.1f);

    midi = freq_to_midi_note(261.63f);
    assert(fabsf(midi - 60.0f) < 0.2f);

    printf("Freq-to-MIDI test PASSED\n");
}

void test_midi_handler_state() {
    midi_handler_t mh;
    midi_handler_init(&mh);

    assert(midi_handler_is_note_active(&mh, 36) == 0);
    midi_handler_note_on(&mh, 36);
    assert(midi_handler_is_note_active(&mh, 36) == 1);
    assert(midi_handler_note_off(&mh, 36) == 1);
    assert(midi_handler_is_note_active(&mh, 36) == 0);

    assert(midi_handler_classify_note(&mh, MIDI_NOTE_KICK) == 0);
    assert(midi_handler_classify_note(&mh, MIDI_NOTE_SNARE) == 1);
    assert(midi_handler_classify_note(&mh, MIDI_NOTE_METAL) == 2);
    assert(midi_handler_classify_note(&mh, MIDI_NOTE_PERC) == 3);
    assert(midi_handler_classify_note(&mh, 64) == 255);

    printf("MIDI handler state tests PASSED\n");
}

int main() {
    test_midi_to_freq();
    test_freq_to_midi();
    test_midi_handler_state();
    return 0;
}

#endif // TEST_MIDI
