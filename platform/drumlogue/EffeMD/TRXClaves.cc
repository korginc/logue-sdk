#include "TRXClaves.h"

void TRXClaves::Init() {
    env = 0.0f;
    t = 0.0f;
    phase1 = phase2 = 0.0f;
}

void TRXClaves::Trigger() {
    env = 1.0f;
    t = 0.0f;
    phase1 = phase2 = 0.0f;
    env_mul = expf(-INV_SAMPLE_RATE / decay);
}

float TRXClaves::Process() {
    if (env < 0.0001f) return 0.0f;

    t += INV_SAMPLE_RATE;
    env *= env_mul;

    phase1 += pitch * pitch_ratio_ * INV_SAMPLE_RATE;
    phase2 += (pitch + interval) * pitch_ratio_ * INV_SAMPLE_RATE;
    if (phase1 > 1.0f) phase1 -= 1.0f;
    if (phase2 > 1.0f) phase2 -= 1.0f;

    float osc1 = sine(phase1 * 2.0f * M_PI);
    float osc2 = sine(phase2 * 2.0f * M_PI);
    float mix = balance * osc1 + (1.0f - balance) * osc2;
    float out = mix * env;

    if (clip > 0.0f) {
        out = fastertanhf(out * (1.0f + clip * 5.0f));
    }

    return out;
}


void TRXClaves::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
          pitch = 1410.78f; interval = 200.0f; decay = 0.017f; balance = 0.5f; clip = 0.211f;
          break;
        case 1:
          pitch = 880.782f; interval = 280.13f; decay = 0.01f; balance = 0.912f; clip = 0.0f;
          break;
        // case 2: - maybe in the future
    }
};
void TRXClaves::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
        // SliderFloat("Pitch", &pitch, 200.0f, 4000.0f);
            pitch = 100.0f + value * 19.5f; //0..200
            break;
        case K_Decay_A:
            // Claves are a short click. The original 0.01..5 s range turned the
            // two beating sines into a gong at the default value; clamp to a
            // clave-length range (~5..100 ms). The long-decay voice lives on as
            // the separate "TRX Gong" instrument.
        // SliderFloat("Decay", &decay, 0.01f, 0.5f);
        decay = 0.005f + value * 0.00095f;
        break;
        case K_Mix:
        // SliderFloat("Balance", &balance, 0.0f, 1.0f);
        balance = value * 0.01f;
        break;
        case K_Gap:
        // SliderFloat("Interval", &interval, 0.0f, 400.0f);
            interval = value * 4.0f;
            break;
        case K_Count:
        // SliderFloat("Clip", &clip, 0.0f, 1.0f);
            clip = value * 0.01f;
            break;
        default:
            break;
    }
};
float TRXClaves::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
            return pitch;
            break;
        case K_Decay_A:
            return decay;
            break;
        case K_Mix:
            return balance;
            break;
        case K_Gap:
            return interval;  // seconds
            break;
        case K_Count:
            return clip;
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void TRXClaves::RenderControls() {
//     ImGui::SliderFloat("Pitch", &pitch, 200.0f, 4000.0f);
//     ImGui::SliderFloat("Interval", &interval, 0.0f, 400.0f);
//     ImGui::SliderFloat("Decay", &decay, 0.01f, 0.5f);
//     ImGui::SliderFloat("Balance", &balance, 0.0f, 1.0f);
//     ImGui::SliderFloat("Clip", &clip, 0.0f, 1.0f);
// }

float TRXClaves::sine(float x) {
    return fastersinfullf(x);
}
