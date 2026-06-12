// FmSnareModel.cc
#include "FmSnareModel.h"

void FmSnareModel::Init() {
    fmo_reset(&mod_op_);
    fmo_reset(&car_op_);
    drum_rng_seed(&rng_, 0x5A4E0001u);
    x_prev = y_prev = 0.0f;
    amp_env = 1.0f;
    mod_env = 1.0f;
    noise_env = 1.0f;
    // Decay multipliers per sample: for small x, exp(-x) ~= 1 - x
    float dt = 1.0f / SAMPLE_RATE;
    amp_decay_const = 1.0f - (dt / d_b);
    mod_decay_const = 1.0f - (dt / d_m);
    noise_decay_const = 1.0f - (dt / dbrus);
    if (amp_decay_const < 0.0f) amp_decay_const = 0.0f;
    if (mod_decay_const < 0.0f) mod_decay_const = 0.0f;
    if (noise_decay_const < 0.0f) noise_decay_const = 0.0f;
}

void FmSnareModel::Trigger() {
    Init();
}

float FmSnareModel::Process() {
    const float dt = 1.0f / SAMPLE_RATE;
    // Iterative envelope decay
    amp_env *= amp_decay_const;
    mod_env *= mod_decay_const;
    noise_env *= noise_decay_const;

    fmo_set_freq(&mod_op_, f_m * pitch_ratio_);
    fmo_set_freq(&car_op_, f_b * pitch_ratio_);

    // Modulator (no feedback), then carrier phase-modulated by I * mod_env
    // (normalized-phase units, as in the original EFM model).
    const float mod_raw = fmo_render_raw(&mod_op_, 0.0f, 1.0f);
    const float pm = (I * mod_env * mod_raw) * (1.0f / FMO_MOD_RANGE);
    const float tone = fmo_render_raw(&car_op_, pm, 1.0f) * amp_env;

    const float white = drum_rng_bipolar(&rng_) * Abrus * noise_env;
    const float x = tone + white;
    // One-pole high pass
    const float alpha = 1.0f / (1.0f + 2.0f * PI * fhp * dt);
    const float y = alpha * (y_prev + x - x_prev);
    x_prev = x;
    y_prev = y;
    return y * amp_env;
}

void FmSnareModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
            f_b = 200.0f; d_b = 0.175f; f_m = 500.0f; I = 0.586f; d_m = 0.01f; Abrus = 0.529f; dbrus = 0.151f; fhp = 291.765f;
            break;
        case 1:
            f_b = 160.247f; d_b = 0.091f; f_m = 551.229f; I = 1.025f; d_m = 0.01f; Abrus = 0.5f; dbrus = 0.124f; fhp = 799.016f;
            break;
        // case 2: - maybe in the future
    }
}

void FmSnareModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
            f_b = 100.0f + value * 3.0f;
            break;
        case K_Modulation_Frequency:
            f_m = 500.0f + value * 25.0f;
            break;
        case K_Modulation_Index:
            I = value * 0.5f;
            break;
        case K_Modulation_Decay:
            d_m = 0.01f + value * 0.0099f;
            break;
        case K_Decay_A:
            d_b = 0.01f + value * 0.0099f;
            break;
        case K_Noise_Level:
            Abrus = value * 0.01f;
            break;
        case K_Noise_Decay:
            dbrus = value * 0.01f;
            break;
        case K_HPF:
            fhp = 20.0f + value * 19.80f;
            break;
        default:
            break;
    }
}

float FmSnareModel::getParameter(fm_param_index_t param_index) {
    switch (param_index) {
        case K_Base_Frequency:       return f_b;
        case K_Modulation_Frequency: return f_m;
        case K_Modulation_Index:     return I;
        case K_Modulation_Decay:     return d_m;
        case K_Decay_A:              return d_b;
        case K_Noise_Level:          return Abrus;
        case K_Noise_Decay:          return dbrus;
        case K_HPF:                  return fhp; // Hz
        default:                     return 255.0f;  // invalid
    }
}
