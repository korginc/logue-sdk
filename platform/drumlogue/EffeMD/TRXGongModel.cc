// TRXGongModel.cc
//
// Exact clone of the original TRXClaves DSP (wide decay range → gong).
// See TRXGongModel.h for why this exists as its own instrument.
#include "TRXGongModel.h"

void TRXGongModel::Init() {
    env = 0.0f;
    t = 0.0f;
    phase1 = phase2 = 0.0f;
}

void TRXGongModel::Trigger() {
    env = 1.0f;
    t = 0.0f;
    phase1 = phase2 = 0.0f;
    env_mul = expf(-INV_SAMPLE_RATE / decay);
}

float TRXGongModel::Process() {
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

void TRXGongModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
          pitch = 1410.78f; interval = 200.0f; decay = 0.5f; balance = 0.5f; clip = 0.211f;
          break;
        case 1:
          pitch = 880.782f; interval = 280.13f; decay = 1.2f; balance = 0.912f; clip = 0.0f;
          break;
    }
}

void TRXGongModel::setParameter(fm_param_index_t param_index, float value) {
    switch (param_index) {
        case K_Base_Frequency: pitch = 200.0f + value * 38.0f; break;
        case K_Decay_A:        decay = 0.01f + value * 0.0499f; break;  // up to ~5 s
        case K_Mix:            balance = value * 0.01f; break;
        case K_Gap:            interval = value * 4.0f; break;
        case K_Count:          clip = value * 0.01f; break;
        default: break;
    }
}

float TRXGongModel::getParameter(fm_param_index_t param_index) {
    switch (param_index) {
        case K_Base_Frequency: return pitch;
        case K_Decay_A:        return decay;
        case K_Mix:            return balance;
        case K_Gap:            return interval;
        case K_Count:          return clip;
        default:               return 255.0f;
    }
}

float TRXGongModel::sine(float x) {
    return fastersinfullf(x);
}
