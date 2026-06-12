// FmCowbellModel.cpp
#include "FmCowbellModel.h"


void FmCowbellModel::Init() {
    t = 0.0f;
    mod_phase = 0.0f;
    carA_phase = carB_phase = PI / 2.0f;
    prev_mod = 0.0f;
    fbB = fbA * 1.48f;
    Ab2 = 1.0f - Ab1;
}

void FmCowbellModel::Trigger() {
    Init();
}

float FmCowbellModel::Process() {
    float dt = 1.0f / SAMPLE_RATE;

    float env1 = ExpDecay(t, d_b1);
    float env2 = ExpDecay(t, db2);
    float amp_env = Ab1 * env1 + Ab2 * env2;

    float mod_env = ExpDecay(t, dm);
    float mod_feedback = bm * prev_mod;
    mod_phase = WrapPhase(mod_phase + TWO_PI * (fm * pitch_ratio_) * dt + mod_feedback);
    float mod_out = fastersinfullf(mod_phase);
    prev_mod = mod_out;

    float mod_signal = I * mod_env * mod_out;

    carA_phase = WrapPhase(carA_phase + TWO_PI * (fbA * pitch_ratio_) * dt + mod_signal);
    carB_phase = WrapPhase(carB_phase + TWO_PI * (fbB * pitch_ratio_) * dt + mod_signal);

    float outA = fastersinfullf(carA_phase);
    float outB = fastersinfullf(carB_phase);

    float out = (outA + outB) * 0.5f * amp_env;

    t += dt;
    return out;
}


void FmCowbellModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
        fbA = 324.314f; d_b1 = 0.078f; db2 = 0.224f; fm = 1529.41f; I = 0.06f; dm = 0.076f; bm = 0.0f; Ab1 = 0.946f;
          break;
        case 1:
        fbA = 281.967f; d_b1 = 0.03f; db2 = 0.099f; fm = 879.098f; I = 0.02f; dm = 0.042f; bm = 0.008f; Ab1 = 0.279f;
          break;
        // case 2: - maybe in the future
    }
    fbB = fbA * 1.48f;
    Ab2 = 1.0f - Ab1;
};
void FmCowbellModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
        // ParameterSlider("fbA (Base Freq)", &fbA, 200.0f, 1000.0f);
            fbA = 200.0f + value * 4.0f;    //0..200
            fbB = fbA * 1.48f;
            break;
        case K_Modulation_Frequency:
        // ParameterSlider("fm (Mod Freq)", &fm, 500.0f, 3000.0f);
        fm = 500.0f + value * 25.0f;
        break;
        case K_Modulation_Feedback:
        // ParameterSlider("bm (Mod Feedback)", &bm, 0.0f, 1.0f);
        bm = value * 0.01f;
        break;
        case K_Modulation_Index:
        // ParameterSlider("I (Mod Index)", &I, 0.0f, 100.0f);
        I = value * 0.5f;   //0..200
        break;
        case K_Modulation_Decay:
        // ParameterSlider("dm (Mod Decay)", &dm, 0.01f, 1.0f);
        dm = 0.01f + value * 0.0099f;
        break;
        case K_Decay_A:
        // ParameterSlider("d_b1 (Decay A)", &d_b1, 0.005f, 0.2f);
        d_b1 = 0.005f + value * 0.00195f;
        break;
        case K_Decay_B:
        // ParameterSlider("db2 (Decay B)", &db2, 0.01f, 1.0f);
            db2 = 0.01f + value * 0.0099f;
            break;
        case K_Mix:
        // ParameterSlider("Ab1 (Envelope Mix A)", &Ab1, 0.0f, 1.0f);
            Ab1 = value * 0.01f;
            Ab2 = 1.0f - Ab1;
            break;
        default:
            break;
    }
};
float FmCowbellModel::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
            return fbA;
            break;
        case K_Modulation_Frequency:
            return fm;
            break;
        case K_Modulation_Feedback:
            return bm;
            break;
        case K_Modulation_Index:
            return I;
            break;
        case K_Modulation_Decay:
            return dm;
            break;
        case K_Decay_A:
            return d_b1;
            break;
        case K_Decay_B:
            return db2;
            break;
        case K_Mix:
            return Ab1;
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmCowbellModel::RenderControls() {
//     CustomControls::ParameterSlider("fbA (Base Freq)", &fbA, 200.0f, 1000.0f);
//     CustomControls::ParameterSlider("d_b1 (Decay A)", &d_b1, 0.005f, 0.2f);
//     CustomControls::ParameterSlider("db2 (Decay B)", &db2, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("fm (Mod Freq)", &fm, 500.0f, 3000.0f);
//     CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 100.0f);
//     CustomControls::ParameterSlider("dm (Mod Decay)", &dm, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("bm (Mod Feedback)", &bm, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("Ab1 (Envelope Mix A)", &Ab1, 0.0f, 1.0f);
// }