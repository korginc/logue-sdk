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
}

float TRXClaves::Process() {
    if (env < 0.0001f) return 0.0f;

    t += INV_SAMPLE_RATE;
    env *= e_expff(-INV_SAMPLE_RATE / (decay));

    phase1 += pitch * INV_SAMPLE_RATE;
    phase2 += (pitch + interval) * INV_SAMPLE_RATE;
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
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            pitch = 200.0f + value * 38.0f;
            break;
        case K_Decay_A:
            decay = 0.01f + value * 0.0499f;
            break;
        case K_Mix:
            balance = value * 0.01f;
            break;
        case K_Gap:
            interval = value * 4.0f;
            break;
        case K_Count:
            clip = value * 0.01f;
            break;
        default:
            break;
    }
};
float TRXClaves::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
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
    return fasterfullsinf(x);
}
