// FmClapModel.cpp
#include "FmClapModel.h"

void FmClapModel::Init() {
    t = 0.0f;
    clap_stage = 0;
    clap_timer = 0.0f;
    mod_phase = car_phase = PI / 2.0f;
    prev_mod = 0.0f;
    x_prev = y_prev = 0.0f;
    active = true;
}

void FmClapModel::Trigger() {
    Init();
}

float FmClapModel::Process() {
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

void FmClapModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
          f_b = 234.804f;  f_m = 1066.67f;  I = 3.431f;  d_m = 0.17f;  d1 = 0.023f;  d2 = 0.3f;  clap_count = 2.0f;  clap_interval = 0.028f;  fhp = 786.765f;  bm = 1.0f;
          break;
        case 1:
          f_b = 176.64f; f_m = 1585.66f; I = 15.164f; d_m = 0.095f; d1 = 0.01f; d2 = 0.09f; clap_count = 30.0f; clap_interval = 0.034f; fhp = 953.197f; bm = 0.018f;
          break;
        // case 2: - maybe in the future
    }
};
void FmClapModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            f_b = 100.0f + value * 11.0f;
            break;
        case K_Modulation_Frequency:
            f_m = 100.0f + value * 29.0f;
            break;
        case K_Modulation_Feedback:
            bm = 0.01f + value * 0.0099f;
            break;
        case K_Modulation_Index:
            I = value;
            break;
        case K_Modulation_Decay:
            d_m = 0.01f + value * 0.0099f;
            break;
        case K_Decay_A:
            d1 = 0.005f + value * 0.00595f;
            break;
        case K_Decay_B:
            d2 = 0.01f + value * 0.0099f;
            break;
        case K_Gap:
            clap_interval = 0.005f + value * 0.00045f;
            break;
        case K_Count:
            clap_count = value;
            break;
        case K_HPF:
            fhp = 20.0f + value * 19.80f;
            break;
        default:
            break;
    }
};
float FmClapModel::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            return f_b;
            break;
        case K_Modulation_Frequency:
            return f_m;
            break;
        case K_Modulation_Feedback:
            return bm;
            break;
        case K_Modulation_Index:
            return I;
            break;
        case K_Modulation_Decay:
            return d_m;
            break;
        case K_Decay_A:
            return d1;
            break;
        case K_Decay_B:
            return d2;
            break;
        case K_Gap:
            return clap_interval;  // seconds
            break;
        case K_Count:
            return clap_count;
            break;
        case K_HPF:
            return fhp; // Hz
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmClapModel::RenderControls() {
//     CustomControls::ParameterSlider("f_b (Base Freq)", &f_b, 100.0f, 1200.0f);
//     CustomControls::ParameterSlider("f_m (Mod Freq)", &f_m, 100.0f, 3000.0f);
//     CustomControls::ParameterSlider("bm (Mod Feedback)", &bm, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 100.0f);
//     CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("d1 (Pre-Clap Decay)", &d1, 0.005f, 0.6f);
//     CustomControls::ParameterSlider("d2 (Final Clap Decay)", &d2, 0.01f, 0.9f);
//     CustomControls::ParameterSliderInt("clap_count", &clap_count, 1, 6);
//     CustomControls::ParameterSlider("clap_interval (s)", &clap_interval, 0.005f, 0.05f);
//     CustomControls::ParameterSlider("fhp (HPF Cutoff)", &fhp, 20.0f, 2000.0f);
// }