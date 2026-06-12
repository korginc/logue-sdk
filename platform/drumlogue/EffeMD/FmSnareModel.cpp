// FmSnareModel.cpp
#include "FmSnareModel.h"


void FmSnareModel::Init() {
    t = 0.0f;
    modulator_.Reset();
    carrier_.Reset();
    x_prev = y_prev = 0.0f;
    fb_state_[0] = fb_state_[1] = 0.0f;
    amp_env = 1.0f;
    mod_env = 1.0f;
    noise_env = 1.0f;
    // Calculate decay constants for iterative envelopes WITHOUT std::expf
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
    float dt = 1.0f / SAMPLE_RATE;
    // Iterative envelope decay
    amp_env *= amp_decay_const;
    mod_env *= mod_decay_const;
    noise_env *= noise_decay_const;

    // Prepare frequency and amplitude for operators (normalized to [0, 0.5] for Nyquist)
    float mod_freq = f_m / SAMPLE_RATE;
    float car_freq = f_b / SAMPLE_RATE;
    float mod_amp = I * mod_env; // Modulation index as amplitude
    float car_amp = amp_env;

    float mod_out = 0.0f;
    float car_out = 0.0f;
    // Render modulator (no feedback, no external modulation)
    plaits::fm::Operator* mod_ops[1] = { &modulator_ }; // TODO move to fm_operator
    float mod_f[1] = { mod_freq };
    float mod_a[1] = { mod_amp };
    float dummy_fb[2] = {0.0f, 0.0f};
    float dummy_mod[1] = {0.0f};
    float mod_buf[1] = {0.0f};
    plaits::fm::RenderOperators<1, plaits::fm::Operator::MODULATION_SOURCE_NONE, false>( // TODO move to fm_operator
        mod_ops[0], mod_f, mod_a, dummy_fb, 0, dummy_mod, mod_buf, 1);
    mod_out = mod_buf[0];

    // Render carrier (external modulation from modulator)
    plaits::fm::Operator* car_ops[1] = { &carrier_ };
    float car_f[1] = { car_freq };
    float car_a[1] = { car_amp };
    float car_mod[1] = { mod_out };
    float car_buf[1] = {0.0f};
    plaits::fm::RenderOperators<1, plaits::fm::Operator::MODULATION_SOURCE_EXTERNAL, false>( // TODO move to fm_operator
        car_ops[0], car_f, car_a, dummy_fb, 0, car_mod, car_buf, 1);
    float tone = car_buf[0];

    float white = ((float(rand()) / RAND_MAX) * 2.0f - 1.0f) * Abrus * noise_env;
    float x = tone + white;
    float alpha = 1.0f / (1.0f + 2.0f * PI * fhp * dt);
    float y = alpha * (y_prev + x - x_prev);
    x_prev = x;
    y_prev = y;
    t += dt;
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
};
void FmSnareModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            f_b = 100.0f + value * 3.0f;
            break;
        case K_Modulation_Frequency:
            f_m = 500.0f + value * 25.0f;
            break;
        case K_Modulation_Index:
            I = value * 0.5;
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
};
float FmSnareModel::getParameter(fm_param_index_t param_index) {
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
        case K_Noise_Level:
            return Abrus;  // seconds
            break;
        case K_Noise_Decay:
            return dbrus;
            break;
        case K_HPF:
            return fhp; // Hz
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmSnareModel::RenderControls() {
//     CustomControls::ParameterSlider("f_b (Tone Freq)", &f_b, 100.0f, 400.0f);
//     CustomControls::ParameterSlider("d_b (Tone Decay)", &d_b, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("f_m (Mod Freq)", &f_m, 500.0f, 3000.0f);
//     CustomControls::ParameterSlider("I (Mod Index)", &I, 0.0f, 50.0f);
//     CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("Abrus (Noise Level)", &Abrus, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("dbrus (Noise Decay)", &dbrus, 0.01f, 1.0f);
//     CustomControls::ParameterSlider("fhp (HPF Cutoff)", &fhp, 20.0f, 2000.0f);
// }
