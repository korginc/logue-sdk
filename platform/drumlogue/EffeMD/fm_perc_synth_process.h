#pragma once

/**
 * @file fm_perc_synth_process.h
 * @brief Core EffeMD synth state and rendering functions
 *
 * Taken inspiration from Sonaglio's fm_perc_synth_process.h with adjustments for the EffeMD model:
 * adapting the ctag-fh-kiel/md-drum-synth
 */

#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

#include "constants.h"
#include "engine_mapping.h"
#include "fm_voices.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "prng.h"
#include "midi_handler.h"

// NEW
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

// ------------------------
// MACROS
// ------------------------
#define NUM_OF_PRESETS (2)
#define NAME_LENGTH    (12)


// ------------------------
// STRUCTURES
// ------------------------

/**
 * Complete synthesizer state
 */
typedef struct {
    // PRNG
    neon_prng_t prng;

    // MIDI handler
    midi_handler_t midi;

    // User parameters
    int16_t params[PARAM_TOTAL];

    // Note tuning
    float32x4_t euclid_offsets;

    // Output gain
    float engine_gain[INST_COUNT];
    float master_gain;

    // static instance of each instrument model
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

    // array of intruments derived from virtual common class
    DrumModel* models[INST_COUNT];

} fm_perc_synth_t;

typedef struct {
    char    name[NAME_LENGTH];
    uint8_t params[PARAM_TOTAL];
} fm_preset_t;


// ============================================================================
// Initialization
// ============================================================================

fast_inline void load_fm_preset(fm_perc_synth_t * synth, uint8_t idx) {
    synth->models[synth->models].loadPreset(idx);
}

fast_inline void fm_perc_synth_init(fm_perc_synth_t * synth) {
    // lfo_enhanced_init(&synth->lfo);
    // lfo_smoother_init(&synth->lfo_smooth);

    neon_prng_init(&synth->prng, RAND_DEFAULT_SEED);
    midi_handler_init(&synth->midi);

    synth->euclid_offsets = vdupq_n_f32(0.0f);
    synth->master_gain = 1.30f;

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

    for (int i = 0; i < INST_COUNT; ++i) {
      synth->engine_gain[i] = 0.0f;
      synth->models.Init();
    }

    load_fm_preset(synth, 0);
    // fm_perc_synth_update_params(synth);
}

fast_inline void fm_perc_synth_update_params(fm_perc_synth_t * synth, uint8_t index, int32_t value) {
    uint8_t instrument_id = synth->params[K_Instrument];
    return synth->models[instrument_id]->setParameter(index, value);
}

// ============================================================================
// Render
// ============================================================================
fast_inline float fm_perc_synth_process(fm_perc_synth_t * synth) {
    uint8_t instrument_id = synth->params[K_Instrument];
    float sample = synth->models[instrument_id]->Process();
    return sample;
}

// ============================================================================
// Utility helpers
// ============================================================================

  fast_inline float neon_horizontal_sum(float32x4_t v) {
    float32x2_t sum_low = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
    float32x2_t sum_total = vpadd_f32(sum_low, sum_low);
    return vget_lane_f32(sum_total, 0);
  }

  fast_inline float neon_horizontal_sum_alt(float32x4_t v) {
#if defined(__aarch64__)
  return vaddvq_f32(v);
#else
  float32x2_t sum_low = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
  float32x2_t sum_total = vpadd_f32(sum_low, sum_low);
  return vget_lane_f32(sum_total, 0);
#endif
}

// Use only lane 0 in the reduced selector model. This avoids summing four
// duplicated lanes after removing the four-probability voice model.
static fast_inline uint32x4_t active_lane_mask() {
  uint32_t lanes[4] = {0xFFFFFFFFu, 0u, 0u, 0u};
  return vld1q_u32(lanes);
}

static constexpr float k_u32_to_unit = 1.0f / 4294967295.0f;

static fast_inline float rand_bipolar_from_prng(fm_perc_synth_t * synth) {
  uint32x4_t rnd = neon_prng_rand_u32(&synth->prng);
  uint32_t r0 = vgetq_lane_u32(rnd, 0);
  return ((float)r0 * k_u32_to_unit) * 2.0f - 1.0f;
}
