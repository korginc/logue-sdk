/**
 * @file fm_presets.h
 * @brief Factory presets for FM Percussion Synth
 *
 * Rewritten for the fixed 4-engine model:
 * - Kick, Snare, Metal, Perc
 * - 2 controls per engine: Attack / Body
 * - 3 global controls: HitShape / BodyTilt / Drive
 *
 * Resonant / voice-allocation data removed from the active preset model.
 */

#pragma once

#include <stdint.h>
#include "constants.h"

#define NUM_OF_PRESETS (26)
#define NAME_LENGTH    (12)



// Parameter indices matching the fixed 24-parameter synth contract.
typedef enum {
    PARAM_INSTRUMENT = 0,
    PARAM_BLEND,
    PARAM_GAP,
    PARAM_SCATTER,

    PARAM_KICK_ATK,
    PARAM_KICK_BODY,
    PARAM_SNARE_ATK,
    PARAM_SNARE_BODY,

    PARAM_METAL_ATK,
    PARAM_METAL_BODY,
    PARAM_PERC_ATK,
    PARAM_PERC_BODY,

    PARAM_LFO1_SHAPE,
    PARAM_LFO1_RATE,
    PARAM_LFO1_TARGET,
    PARAM_LFO1_DEPTH,

    PARAM_EUCL_TUN,
    PARAM_LFO2_RATE,
    PARAM_LFO2_TARGET,
    PARAM_LFO2_DEPTH,

    PARAM_ENV_SHAPE,
    PARAM_HIT_SHAPE,
    PARAM_BODY_TILT,
    PARAM_DRIVE,

    PARAM_TOTAL = 24
} fm_param_index_t;

typedef struct {
    char name[NAME_LENGTH];

    // Page 1: Probabilities
    uint8_t instrument_sel;
    uint8_t blend;
    uint8_t gap;
    uint8_t scatter;

    // Page 2: Kick + Snare (Attack / Body)
    uint8_t kick_attack;
    uint8_t kick_body;
    uint8_t snare_attack;
    uint8_t snare_body;

    // Page 3: Metal + Perc (Attack / Body)
    uint8_t metal_attack;
    uint8_t metal_body;
    uint8_t perc_attack;
    uint8_t perc_body;

    // Page 4: LFO1
    uint8_t lfo1_shape;   // 0-8
    uint8_t lfo1_rate;    // 0-100
    uint8_t lfo1_target;  // 0-10
    int8_t  lfo1_depth;  // -100..100

    // Page 5: EuclTun + LFO2
    uint8_t eucl_tun;     // 0-8 (Euclidean pitch spread)
    uint8_t lfo2_rate;    // 0-100
    uint8_t lfo2_target;  // 0-10
    int8_t  lfo2_depth;  // -100..100

    // Page 6: Envelope + global shaping
    uint8_t env_shape;    // 0-255: bit7=metal character, bits[6:0]=envelope index
    uint8_t hit_shape;    // 0-100
    uint8_t body_tilt;    // 0-100
    uint8_t drive;        // 0-100
} fm_preset_t;

#ifdef __cplusplus
extern "C" {
#endif
extern const fm_preset_t FM_PRESETS[NUM_OF_PRESETS];
void load_fm_preset(uint8_t idx, int8_t *params);
#ifdef __cplusplus
}
#endif

