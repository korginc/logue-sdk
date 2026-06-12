// FmRimshotModel.cpp
#include "FmRimshotModel.h"


void FmRimshotModel::Init() {
    t = 0.0f;
    carB_phase = carA_phase = mod_phase = PI / 2.0f;
    prev_mod = x_prev = y_prev = 0.0f;
}

void FmRimshotModel::Trigger() {
    Init();
}

float FmRimshotModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;

    float mod_env = ExpDecay(t, d_m);
    float mod_out = fastersinfullf(mod_phase);
    mod_phase = WrapPhase(mod_phase + TWO_PI * (1000.0f * pitch_ratio_) * dt);  // fixed mod freq
    prev_mod = mod_out;

    float envB = ExpDecay(t, d_bB);
    float carB = fastersinfullf(WrapPhase(carB_phase + I_B * mod_env * mod_out));
    carB_phase = WrapPhase(carB_phase + TWO_PI * (f_bB * pitch_ratio_) * dt);

    float envA = ExpDecay(t, d_bA);
    float carA = fastersinfullf(WrapPhase(carA_phase + I_A * mod_env * mod_out));
    carA_phase = WrapPhase(carA_phase + TWO_PI * (f_bA * pitch_ratio_) * dt);

    float mixed = (1.0f - A_A) * (carB * envB) + A_A * (carA * envA);

    float alpha = 1.0f / (1.0f + 2.0f * PI * f_hp * dt);
    float y = alpha * (y_prev + mixed - x_prev);
    x_prev = mixed;
    y_prev = y;

    t += dt;
    return y;
}


void FmRimshotModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
            f_bB = 615.686f; d_bB = 0.036f; I_B = 0.03f; f_bA = 210.196f; d_bA = 0.05f; I_A = 7.843f; A_A = 0.537f; d_m = 0.01f; f_hp = 696.078f;
            break;
        case 1:
            f_bB = 322.476f; d_bB = 0.015f; I_B = 3.42f; f_bA = 120.651f; d_bA = 0.05f; I_A = 2.911f; A_A = 0.5f; d_m = 0.02f; f_hp = 742.684f;
            break;
        // case 2: - maybe in the future
    }
};
void FmRimshotModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
        // ParameterSlider("f_bA (body freq)", &f_bA, 80.0f, 400.0f);
            f_bA = 80.0f + value * 1.6f;    //0..200
            break;
        case K_Modulation_Frequency_B:
        // ParameterSlider("f_bB (rim freq)", &f_bB, 200.0f, 1000.0f);
            f_bB = 200.0f + value * 8.0f;
            break;
        case K_Modulation_Index:
        // ParameterSlider("I_A (body mod index)", &I_A, 0.0f, 50.0f);
            I_A = value * 0.25f;    //0..200
            break;
        case K_Modulation_Decay:
        // ParameterSlider("d_m (mod env decay)", &d_m, 0.01f, 0.5f);
            d_m = 0.01f + value * 0.0049f;
            break;
        case K_Decay_A:
        // ParameterSlider("d_bA (body decay)", &d_bA, 0.05f, 1.0f);
            d_bA = 0.05f + value * 0.0095f;
            break;
        case K_Decay_B:
        // ParameterSlider("d_bB (rim decay)", &d_bB, 0.01f, 0.5f);
            d_bB = 0.01f + value * 0.0049f;
            break;
            case K_Modulation_Index_B:
        // ParameterSlider("I_B (rim mod index)", &I_B, 0.0f, 50.0f);
            I_B = value * 0.25f;
            break;
        case K_Mix:
        // ParameterSlider("A_A (body mix)", &A_A, 0.0f, 1.0f);
            A_A = value * 0.01f;
            break;
        case K_HPF:
        // ParameterSlider("f_hp (HPF cutoff)", &f_hp, 100.0f, 2000.0f);
            f_hp = 100.0f + value * 19.0f;
            break;
        default:
            break;
    }
};
float FmRimshotModel::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
            return f_bA;
            break;
        case K_Modulation_Frequency_B:
            return f_bB;
            break;
        case K_Modulation_Index:
            return I_A;
            break;
        case K_Modulation_Decay:
            return d_m;
            break;
        case K_Decay_A:
            return d_bA;
            break;
        case K_Decay_B:
            return d_bB;
            break;
        case K_Modulation_Index_B:
            return I_B;  //
            break;
        case K_Mix:
            return A_A;
            break;
        case K_HPF:
            return f_hp; // Hz
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmRimshotModel::RenderControls() {
//     CustomControls::ParameterSlider("f_bB (rim freq)", &f_bB, 200.0f, 1000.0f);
//     CustomControls::ParameterSlider("d_bB (rim decay)", &d_bB, 0.01f, 0.5f);
//     CustomControls::ParameterSlider("I_B (rim mod index)", &I_B, 0.0f, 50.0f);
//     CustomControls::ParameterSlider("f_bA (body freq)", &f_bA, 80.0f, 400.0f);
//     CustomControls::ParameterSlider("d_bA (body decay)", &d_bA, 0.05f, 1.0f);
//     CustomControls::ParameterSlider("I_A (body mod index)", &I_A, 0.0f, 50.0f);
//     CustomControls::ParameterSlider("A_A (body mix)", &A_A, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("d_m (mod env decay)", &d_m, 0.01f, 0.5f);
//     CustomControls::ParameterSlider("f_hp (HPF cutoff)", &f_hp, 100.0f, 2000.0f);
// }
