#pragma once

/**
 * @file fm_perc_synth_process.h
 * @brief Core EffeMD synth state and rendering functions
 *
 * Structure follows Sonaglio's fm_perc_synth_process.h, adapted to the
 * md-drum-synth model architecture:
 *   - one Instrument selector chooses among 11 self-contained drum models
 *   - the selected model owns its parameters and envelopes
 *   - note-on applies velocity and Euclidean tuning, then triggers the model
 *   - the render path is scalar per sample (models are scalar);
 *     block-level NEON optimizations live in synth.h
 *
 * This header is intentionally free of NEON intrinsics so the whole
 * integration layer can be unit-tested on a host machine (see
 * test_effemd.cc / run_effemd_tests.sh).
 */

#include <stdint.h>
#include <string.h>

#include "constants.h"
#include "float_math.h"

#include "DrumModel.h"
#include "FmKickModel.h"
#include "FmSnareModel.h"
#include "FmTomModel.h"
#include "FmClapModel.h"
#include "FmRimshotModel.h"
#include "FmCowbellModel.h"
#include "FmCymbalModel.h"
#include "TRXBassDrum.h"
#include "TRXSnareDrum.h"
#include "TRXClaves.h"
#include "TRXHiHat.h"
#include "FmWhistleModel.h"
#include "TRXGongModel.h"

// ------------------------
// MACROS
// ------------------------
#define NUM_OF_PRESETS (2)

// Root note: GateOn / sequencer default. Played at this MIDI note (with
// Euclidean mode Off) every model sounds at its dialed base frequency.
#define EFFEMD_ROOT_NOTE (36)

// Idle detection: after this many consecutive near-silent samples the
// synth reports idle and synth.h can skip whole blocks.
#define EFFEMD_SILENCE_SAMPLES (2400)   /* 50 ms @ 48 kHz */
#define EFFEMD_SILENCE_THRESH  (1.0e-4f)

static const char* const FM_PRESET_NAMES[NUM_OF_PRESETS] = {
    "MD Init",
    "MD Alt",
};

// ------------------------
// STRUCTURES
// ------------------------

/**
 * Complete synthesizer state
 */
typedef struct fm_perc_synth {
    // User parameters as last sent by the UI (raw 0..100 style values)
    int16_t params[PARAM_TOTAL];

    // Selector routing model
    uint8_t instrument;      // 0..INST_COUNT-1
    uint8_t euclid_mode;     // 0..EUCLID_MODE_COUNT-1

    // Per-trigger gain (velocity), output gain
    float velocity_gain;
    float master_gain;

    // Idle tracking for block skipping
    uint32_t silence_count;
    uint8_t  idle;

    // Static instance of each instrument model
    FmKickModel     kick;
    FmSnareModel    snare;
    FmTomModel      tom;
    FmClapModel     clap;
    FmRimshotModel  rimshot;
    FmCowbellModel  cowbell;
    FmCymbalModel   cymbal;
    TRXBassDrum     bass;
    TRXSnareDrum    snare_drum;
    TRXClaves       claves;
    TRXHiHat        hi_hat;
    FmWhistleModel  whistle;
    TRXGongModel    gong;

    // Array of instruments through the virtual common base class
    DrumModel* models[INST_COUNT];
} fm_perc_synth_t;

// ============================================================================
// Utility helpers
// ============================================================================

/* Fast scalar soft clip, same curve as Sonaglio's fm_soft_clip:
 * y = x / (1 + |x|) */
static fast_inline float fm_soft_clip_scalar(float x) {
    return x / (1.0f + si_fabsf(x));
}

// ============================================================================
// Initialization / presets
// ============================================================================

fast_inline void load_fm_preset(fm_perc_synth_t* synth, uint8_t idx) {
    if (idx >= NUM_OF_PRESETS) idx = 0;
    for (int i = 0; i < INST_COUNT; ++i) {
        synth->models[i]->loadPreset(idx);
    }
}

fast_inline void fm_perc_synth_init(fm_perc_synth_t* synth) {
    memset(synth->params, 0, sizeof(synth->params));

    synth->instrument    = ID_FmKickModel;
    synth->euclid_mode   = EUCLID_MODE_OFF;
    synth->velocity_gain = 1.0f;
    synth->master_gain   = 1.30f;
    synth->silence_count = EFFEMD_SILENCE_SAMPLES;
    synth->idle          = 1;

    synth->models[ID_FmKickModel]    = &synth->kick;
    synth->models[ID_FmSnareModel]   = &synth->snare;
    synth->models[ID_FmTomModel]     = &synth->tom;
    synth->models[ID_FmClapModel]    = &synth->clap;
    synth->models[ID_FmRimshotModel] = &synth->rimshot;
    synth->models[ID_FmCowbellModel] = &synth->cowbell;
    synth->models[ID_FmCymbalModel]  = &synth->cymbal;
    synth->models[ID_TRXBassDrum]    = &synth->bass;
    synth->models[ID_TRXSnareDrum]   = &synth->snare_drum;
    synth->models[ID_TRXClaves]      = &synth->claves;
    synth->models[ID_TRXHiHat]       = &synth->hi_hat;
    synth->models[ID_FmWhistle]      = &synth->whistle;
    synth->models[ID_TRXGong]        = &synth->gong;

    for (int i = 0; i < INST_COUNT; ++i) {
        synth->models[i]->Init();
    }

    load_fm_preset(synth, 0);
}

// ============================================================================
// Parameter handling
// ============================================================================

fast_inline void fm_perc_synth_update_params(fm_perc_synth_t* synth,
                                             uint8_t index,
                                             int32_t value) {
    switch (index) {
        case K_Instrument: {
            int32_t v = value;
            if (v < 0) v = 0;
            if (v >= INST_COUNT) v = INST_COUNT - 1;
            if ((uint8_t)v != synth->instrument) {
                synth->instrument = (uint8_t)v;
                // Reset the newly selected model so it starts clean
                synth->models[synth->instrument]->Init();
            }
            break;
        }
        case K_Euclidean_Tuning: {
            int32_t v = value;
            if (v < 0) v = 0;
            if (v >= EUCLID_MODE_COUNT) v = EUCLID_MODE_COUNT - 1;
            synth->euclid_mode = (uint8_t)v;
            break;
        }
        default:
            // Per-instrument parameter: routed to the active model.
            // Models ignore parameters they do not implement.
            synth->models[synth->instrument]->setParameter(
                (fm_param_index_t)index, (float)value);
            break;
    }
}

// ============================================================================
// MIDI note handling
// ============================================================================

fast_inline void fm_perc_synth_note_on(fm_perc_synth_t* synth,
                                       uint8_t note,
                                       uint8_t velocity) {
    /* Velocity latched per trigger (Sonaglio convention): linear with a
     * 0.3 floor so soft hits stay audible. */
    synth->velocity_gain = 0.3f + 0.7f * ((float)velocity / 127.0f);

    DrumModel* model = synth->models[synth->instrument];

    /* Euclidean tuning, as in Sonaglio: the instrument's engine class picks
     * a lane in the EUCLID_OFFSETS table and the offset (in semitones) is
     * added to the incoming MIDI note before it becomes a pitch ratio.
     * With mode Off and note == root the model plays at its dialed pitch. */
    const float offset =
        EUCLID_OFFSETS[synth->euclid_mode]
                      [fm_engine_to_euclid_lane((engine_id_t)synth->instrument)];
    const float semis = (float)note - (float)EFFEMD_ROOT_NOTE + offset;
    model->setPitchRatio(fasterpow2f(semis * (1.0f / 12.0f)));

    model->Trigger();

    synth->idle = 0;
    synth->silence_count = 0;
}

fast_inline void fm_perc_synth_note_off(fm_perc_synth_t* synth, uint8_t note) {
    // Most models are one-shot and ignore Release(); sustaining voices (the
    // cymbal with sustain > 0) use it to start a fast fade so they go silent
    // instead of ringing until the next trigger.
    (void)note;
    synth->models[synth->instrument]->Release();
}

// ============================================================================
// Audio render
// ============================================================================

fast_inline bool fm_perc_synth_is_idle(const fm_perc_synth_t* synth) {
    return synth->idle != 0;
}

fast_inline float fm_perc_synth_process(fm_perc_synth_t* synth) {
    float sample = synth->models[synth->instrument]->Process();
    sample *= synth->velocity_gain;
    // Master gain into soft clip, as in Sonaglio's output stage
    sample = fm_soft_clip_scalar(sample * synth->master_gain);

    // Idle detection: a long run of near-silence marks the voice idle so
    // the block renderer can skip work until the next trigger.
    if (si_fabsf(sample) < EFFEMD_SILENCE_THRESH) {
        if (++synth->silence_count >= EFFEMD_SILENCE_SAMPLES) {
            synth->silence_count = EFFEMD_SILENCE_SAMPLES;
            synth->idle = 1;
        }
    } else {
        synth->silence_count = 0;
    }

    return sample;
}
