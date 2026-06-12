// FmCymbalModel.cpp
#include "FmCymbalModel.h"

void FmCymbalModel::Init() {
    t = 0.0f;
    for (int i = 0; i < NUM_PAIRS; ++i) {
        car_phase[i] = mod_phase[i] = PI / 2.0f;
        prev_mod[i] = 0.0f;
    }
    x_prev = y_prev = 0.0f;
}

void FmCymbalModel::Trigger() {
    Init();
}

float FmCymbalModel::Process() {
    float dt = INV_SAMPLE_RATE;
    float amp_env = sustain + ExpDecay(t, d_b);
    float mod_env = ExpDecay(t, d_m);

    float ratios[NUM_PAIRS] = {1.0f, 1.411f, 1.8f, 2.7f};
    float sample = 0.0f;

    for (int i = 0; i < NUM_PAIRS; ++i) {
        mod_phase[i] = WrapPhase(mod_phase[i] + TWO_PI * (fm * ratios[i]) * dt + bb * prev_mod[i]);
        float mod_out = fasterfullsinf(mod_phase[i]);
        prev_mod[i] = mod_out;

        car_phase[i] = WrapPhase(car_phase[i] + TWO_PI * (fb * ratios[i]) * dt + I * mod_env * mod_out);
        float car_out = fasterfullsinf(car_phase[i]);

        sample += car_out;
    }

    float mixed = sample * 0.25f * amp_env;

    float alpha = 1.0f / (1.0f + 2.0f * PI * f_hp * dt);
    float y = alpha * (y_prev + mixed - x_prev);
    x_prev = mixed;
    y_prev = y;

    t += dt;
    return y;
}


void FmCymbalModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
        fb = 395.588f; fm = 2000.0f; d_b = 0.108f; I = 14.559f; d_m = 0.461f; bb = 0.088f; sustain = 0.0f; f_hp = 1031.37f;
        break;
        case 1:
        fb = 295.492f; fm = 745.902f; d_b = 0.066f; I = 15.123f; d_m = 0.234f; bb = 0.049f; sustain = 0.0f; f_hp = 1197.95f;
          break;
        // case 2: - maybe in the future
    }
};
void FmCymbalModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            fb = 100.0f + value * 9.9f;
            break;
        case K_Modulation_Frequency:
            fm = 200.0f + value * 18.0f;
            break;
        case K_Modulation_Feedback:
            bb = 0.01f + value * 0.0099f;
            break;
        case K_Modulation_Index:
            I = value * 0.3f;
            break;
        case K_Modulation_Decay:
            d_m = 0.01f + value * 0.0099f;
            break;
        case K_Decay_A:
            d_b = 0.05f + value * 0.0495f;
            break;
        case K_Sustain:
            sustain = value * 0.01f;
            break;
        case K_HPF:
            f_hp = 100.0f + value * 19.9f;
            break;
        default:
            break;
    }
};
float FmCymbalModel::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            return fb;
            break;
        case K_Modulation_Frequency:
            return fm;
            break;
        case K_Modulation_Feedback:
            return bb;
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
        case K_Sustain:
            return sustain;
            break;
        case K_HPF:
            return f_hp; // Hz
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmCymbalModel::RenderControls() {
//     CustomControls::ParameterSlider("fb (Base Carrier)", &fb, 100.0f, 1000.0f);
//     CustomControls::ParameterSlider("fm (Base Mod)", &fm, 200.0f, 2000.0f);
//     CustomControls::ParameterSlider("d_b (Amp Decay)", &d_b, 0.05f, 4.0f);
//     CustomControls::ParameterSlider("I (FM Index)", &I, 0.0f, 30.0f);
//     CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.05f, 2.0f);
//     CustomControls::ParameterSlider("bb (Mod Feedback)", &bb, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("sustain", &sustain, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("f_hp (HPF Cutoff)", &f_hp, 100.0f, 2000.0f);
// }