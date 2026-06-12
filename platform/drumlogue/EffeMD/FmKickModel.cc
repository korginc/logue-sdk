#include "FmKickModel.h"

const float FmKickModel::ratios[FmKickModel::num_ratios][2] = {
    // Integer multiples 2:1 to 40:1
    {2.0f, 1.0f}, {3.0f, 1.0f}, {4.0f, 1.0f}, {5.0f, 1.0f}, {6.0f, 1.0f}, {7.0f, 1.0f}, {8.0f, 1.0f}, {9.0f, 1.0f},
    {10.0f, 1.0f}, {11.0f, 1.0f}, {12.0f, 1.0f}, {13.0f, 1.0f}, {14.0f, 1.0f}, {15.0f, 1.0f}, {16.0f, 1.0f}, {17.0f, 1.0f},
    {18.0f, 1.0f}, {19.0f, 1.0f}, {20.0f, 1.0f}, {21.0f, 1.0f}, {22.0f, 1.0f}, {23.0f, 1.0f}, {24.0f, 1.0f}, {25.0f, 1.0f},
    {26.0f, 1.0f}, {27.0f, 1.0f}, {28.0f, 1.0f}, {29.0f, 1.0f}, {30.0f, 1.0f}, {31.0f, 1.0f}, {32.0f, 1.0f}, {33.0f, 1.0f},
    {34.0f, 1.0f}, {35.0f, 1.0f}, {36.0f, 1.0f}, {37.0f, 1.0f}, {38.0f, 1.0f}, {39.0f, 1.0f}, {40.0f, 1.0f},
    // Odd/weird ratios >1 and <=40 (25 more)
    {8.0f, 5.0f}, {11.0f, 3.0f}, {13.0f, 7.0f}, {17.0f, 5.0f}, {19.0f, 4.0f}, {23.0f, 7.0f}, {31.0f, 9.0f}, {37.0f, 8.0f},
    {15.0f, 4.0f}, {21.0f, 5.0f}, {25.0f, 6.0f}, {27.0f, 8.0f}, {35.0f, 9.0f}, {39.0f, 10.0f}, {29.0f, 7.0f}, {33.0f, 8.0f},
    {22.0f, 3.0f}, {34.0f, 5.0f}, {38.0f, 7.0f}, {5.0f, 2.0f}, {7.0f, 3.0f}, {9.0f, 2.0f}, {12.0f, 5.0f}, {14.0f, 3.0f},
    {16.0f, 5.0f}
};

void FmKickModel::Init() {
    amp_env = 1.0f;
    mod_env = 1.0f;
    freq_env = 1.0f;
    fmo_reset(&mod_op_);
    fmo_reset(&car_op_);
}

void FmKickModel::Trigger() {
    Init();
    // Decay multipliers per sample: for small x, exp(-x) ~= 1 - x
    float dt = 1.0f / SAMPLE_RATE;
    amp_decay_const = 1.0f - (dt / d_b);
    mod_decay_const = 1.0f - (dt / d_m);
    freq_decay_const = 1.0f - (dt / d_f);
    if (amp_decay_const < 0.0f) amp_decay_const = 0.0f;
    if (mod_decay_const < 0.0f) mod_decay_const = 0.0f;
    if (freq_decay_const < 0.0f) freq_decay_const = 0.0f;
    // DX7-style feedback on the modulator (0..7, clamped by the operator)
    fmo_set_feedback(&mod_op_, b_m);
}

float FmKickModel::Process() {
    // Iterative decay
    amp_env *= amp_decay_const;
    mod_env *= mod_decay_const;
    freq_env *= freq_decay_const;
    const float freq_env_scaled = A_f * freq_env;

    // Modulator frequency selection
    float mod_freq = f_m;
    if (use_ratio_mode) {
        mod_freq = f_b * (ratios[ratio_index][0] / ratios[ratio_index][1]);
    }
    // Sync modulator freq envelope to carrier if enabled
    if (mod_env_sync) {
        mod_freq += freq_env_scaled;
    }

    fmo_set_freq(&mod_op_, mod_freq * pitch_ratio_);
    fmo_set_freq(&car_op_, (f_b + freq_env_scaled) * pitch_ratio_);

    // Modulator (feedback applied inside the operator), unit amplitude
    const float mod_raw = fmo_render_raw(&mod_op_, 0.0f, 1.0f);
    // Carrier phase-modulated by I * mod_env (normalized-phase units, as in
    // the original EFM model). FMO_MOD_RANGE compensates fmo_advance scaling.
    const float pm = (I * mod_env * mod_raw) * (1.0f / FMO_MOD_RANGE);
    const float out = fmo_render_raw(&car_op_, pm, 1.0f);

    return out * amp_env;
}

void FmKickModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
            f_b = 59.016f; d_b = 0.091f; f_m = 116.189f; I = 0.02f; d_m = 0.23f; b_m = 0.012f; A_f = 149.18f; d_f = 0.04f;
            use_ratio_mode = false; ratio_index = 0; mod_env_sync = false;
            break;
        case 1:
            f_b = 52.549f; d_b = 0.253f; f_m = 461.03f; I = 0.052f; d_m = 0.295f; b_m = 2.196f; A_f = 529.412f; d_f = 0.038f;
            use_ratio_mode = true; ratio_index = 57; mod_env_sync = true;
            break;
        // case 2: - maybe in the future
    }
    fmo_set_feedback(&mod_op_, b_m);
}

void FmKickModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
            f_b = 20.0f + value * 0.8f;
            break;
        case K_Modulation_Frequency:
            f_m = 50.0f + value * 19.5f;
            break;
        case K_Modulation_Feedback:
            b_m = value * 0.07f;  // 0..7 DX7 feedback range
            fmo_set_feedback(&mod_op_, b_m);
            break;
        case K_Modulation_Index:
            I = value * 0.1f;
            ratio_index = (int)(value * 0.639f); // index 0..63
            break;
        case K_Modulation_Decay:
            d_m = 0.001f + value * 0.01999f;
            break;
        case K_Decay_A:
            d_b = 0.01f + value * 0.0199f;
            break;
        case K_Count:
            mod_env_sync = ((int)value % 2) == 0;
            break;
        case K_UseRatio:
            use_ratio_mode = (int)value == 1;
            break;
        case K_Frequency_Sweep:
            A_f = value * 10.0f;
            break;
        case K_Sweep_Decay:
            d_f = 0.001f + value * 0.01999f;
            break;
        default:
            break;
    }
}

float FmKickModel::getParameter(fm_param_index_t param_index) {
    switch (param_index) {
        case K_Base_Frequency:      return f_b;
        case K_Modulation_Frequency: return f_m;
        case K_Modulation_Feedback: return b_m;
        case K_Modulation_Index:    return I;
        case K_Modulation_Decay:    return d_m;
        case K_Decay_A:             return d_b;
        case K_Count:               return mod_env_sync ? 1.0f : 0.0f; // "mod env sync"
        case K_UseRatio:            return use_ratio_mode ? 1.0f : 0.0f;
        case K_Frequency_Sweep:     return A_f; // Hz
        case K_Sweep_Decay:         return d_f;
        default:                    return 255.0f;  // invalid
    }
}
