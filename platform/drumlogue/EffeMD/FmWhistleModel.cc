// FmWhistleModel.cc
//
// Exact clone of the original (noise-free) FmClapModel DSP. See
// FmWhistleModel.h for why this exists as its own instrument.
#include "FmWhistleModel.h"

void FmWhistleModel::Init() {
    t = 0.0f;
    clap_stage = 0;
    clap_timer = 0.0f;
    mod_phase = car_phase = PI / 2.0f;
    prev_mod = 0.0f;
    x_prev = y_prev = 0.0f;
    active = true;
}

void FmWhistleModel::Trigger() {
    Init();
}

float FmWhistleModel::Process() {
    if (!active) return 0.0f;

    float dt = 1.0f / SAMPLE_RATE;
    float decay = (clap_stage < clap_count) ? d1 : d2;
    float amp_env = ExpDecay(t, decay);
    float mod_env = ExpDecay(t, d_m);

    // FM synthesis
    float mod_feedback = bm * prev_mod;
    mod_phase = WrapPhase(mod_phase + TWO_PI * (f_m * pitch_ratio_) * dt + mod_feedback);
    float mod_out = fastersinfullf(mod_phase);
    prev_mod = mod_out;

    car_phase = WrapPhase(car_phase + TWO_PI * (f_b * pitch_ratio_) * dt + I * mod_env * mod_out);
    float tone = fastersinfullf(car_phase);
    float x = tone * amp_env;

    // High-pass filter
    float alpha = 1.0f / (1.0f + 2.0f * PI * fhp * dt);
    float y = alpha * (y_prev + x - x_prev);
    x_prev = x;
    y_prev = y;

    t += dt;
    clap_timer += dt;

    if (clap_timer >= clap_interval) {
        ++clap_stage;
        t = 0.0f;
        clap_timer = 0.0f;
        if (clap_stage >= clap_count + 1)
            active = false;
    }

    return y;
}

void FmWhistleModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
          f_b = 234.804f;  f_m = 1066.67f;  I = 3.431f;  d_m = 0.17f;  d1 = 0.023f;  d2 = 0.3f;  clap_count = 2;  clap_interval = 0.028f;  fhp = 786.765f;  bm = 1.0f;
          break;
        case 1:
          f_b = 176.64f; f_m = 1585.66f; I = 15.164f; d_m = 0.095f; d1 = 0.01f; d2 = 0.09f; clap_count = 30; clap_interval = 0.034f; fhp = 953.197f; bm = 0.018f;
          break;
    }
}

void FmWhistleModel::setParameter(fm_param_index_t param_index, float value) {
    switch (param_index) {
        case K_Base_Frequency:      f_b = 100.0f + value * 11.0f; break;
        case K_Modulation_Frequency: f_m = 100.0f + value * 29.0f; break;
        case K_Modulation_Feedback: bm = 0.01f + value * 0.0099f; break;
        case K_Modulation_Index:    I = value; break;
        case K_Modulation_Decay:    d_m = 0.01f + value * 0.0099f; break;
        case K_Decay_A:             d1 = 0.005f + value * 0.00595f; break;
        case K_Decay_B:             d2 = 0.01f + value * 0.0099f; break;
        case K_Gap:                 clap_interval = 0.005f + value * 0.00045f; break;
        case K_Count:               clap_count = (int)value; break;
        case K_HPF:                 fhp = 20.0f + value * 19.80f; break;
        default: break;
    }
}

float FmWhistleModel::getParameter(fm_param_index_t param_index) {
    switch (param_index) {
        case K_Base_Frequency:      return f_b;
        case K_Modulation_Frequency: return f_m;
        case K_Modulation_Feedback: return bm;
        case K_Modulation_Index:    return I;
        case K_Modulation_Decay:    return d_m;
        case K_Decay_A:             return d1;
        case K_Decay_B:             return d2;
        case K_Gap:                 return clap_interval;
        case K_Count:               return (float)clap_count;
        case K_HPF:                 return fhp;
        default:                    return 255.0f;
    }
}
