// FmTomModel.cpp
#include "FmTomModel.h"

void FmTomModel::Init() {
    t = 0.0f;
    mod_phase = car_phase = start_phase;
    prev_mod = 0.0f;
}

void FmTomModel::Trigger() {
    Init();
}

float FmTomModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;
    float amp_env = ExpDecay(t, d_b);
    float mod_env = ExpDecay(t, d_m);
    float freq_env = A_f * ExpDecay(t, d_f);

    float mod_feedback = 1.0f * prev_mod;
    mod_phase = WrapPhase(mod_phase + TWO_PI * (f_m * pitch_ratio_) * dt + mod_feedback);
    float mod_out = fastersinfullf(mod_phase);
    prev_mod = mod_out;

    car_phase = WrapPhase(car_phase + TWO_PI * ((f_b + freq_env) * pitch_ratio_) * dt + I * mod_env * mod_out);
    float out = fastersinfullf(car_phase) * amp_env;

    t += dt;
    return out;
}


void FmTomModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
            f_b = 214.902f; d_b = 0.127f; f_m = 435.294f; I = 1.716f; d_m = 0.01f; A_f = 21.569f; d_f = 0.039f;
            break;
        case 1:
            f_b = 146.885f; d_b = 0.024f; f_m = 100.04f; I = 0.04f; d_m = 1.0f; A_f = 17.213f; d_f = 0.027f;
            break;
        // case 2: - maybe in the future
    }
};
void FmTomModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 1..100 - TODO extend to 1 1000, so the following shall be divided by additional 10
    switch (param_index) {
        case K_Base_Frequency:
            f_b = 80.0f + value * 3.2f;
            break;
        case K_Modulation_Frequency:
            f_m = 100.0f + value * 29.0f;
            break;
        case K_Modulation_Index:
            I = 100.0f + value * 19.0f;
            break;
        case K_Modulation_Decay:
            d_m = 0.01f + value * 0.0099f;
            break;
        case K_Decay_A:
            d_b = 0.01f + value * 0.0199f;
            break;
        case K_Frequency_Sweep:
            A_f = value;
            break;
        case K_Sweep_Decay:
            d_f = 0.01f + value * 0.0199f;
            break;
        case K_Phase:
            start_phase = value * 0.01 * PI;
            break;
        default:
            break;
    }
};
float FmTomModel::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            return f_b;
            break;
        case K_Modulation_Frequency:
            return f_m;
            break;
        case K_Modulation_Index:
            return I;
            break;
        case K_Modulation_Decay:
            return d_m;
            break;
        case K_Decay_A:
            return d_b;
            break;
        case K_Frequency_Sweep:
            return A_f;
            break;
        case K_Sweep_Decay:
            return d_f;  // seconds
            break;
        case K_Phase:
            return start_phase;
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmTomModel::RenderControls() {
//     CustomControls::ParameterSlider("f_b (Base Frequency)", &f_b, 80.0f, 400.0f);
//     CustomControls::ParameterSlider("d_b (Amp Decay)", &d_b, 0.01f, 2.0f);
//     CustomControls::ParameterSlider("f_m (Modulator Freq)", &f_m, 100.0f, 2000.0f);
//     CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 50.0f);
//     CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("A_f (Freq Sweep Amt)", &A_f, 0.0f, 100.0f);
//     CustomControls::ParameterSlider("d_f (Freq Sweep Decay)", &d_f, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("Start Phase", &start_phase, 0.0f, PI);
// }
