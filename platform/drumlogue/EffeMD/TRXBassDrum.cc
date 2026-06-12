#include "TRXBassDrum.h"

void TRXBassDrum::Init() {
    phase = t = env = rampEnv = 0.0f;
    prevSample = 0.0f;
    drum_rng_seed(&rng_, 0xBD000001u);
}

void TRXBassDrum::Trigger() {
    t = 0.0f;
    env = 1.0f;
    rampEnv = 1.0f;
    phase = 0.0f;
    env_mul  = expf(-INV_SAMPLE_RATE / decay);
    ramp_mul = expf(-INV_SAMPLE_RATE / rampDecay);
}

float TRXBassDrum::Process() {
    if (env <= 0.0001f) return 0.0f;

    t += 1.0f * INV_SAMPLE_RATE;

    // Envelope decay
    env *= env_mul;
    rampEnv *= ramp_mul;

    // Frequency modulation
    float freq = (pitch + ramp * rampEnv * 1000.0f) * pitch_ratio_;
    phase += freq * INV_SAMPLE_RATE;
    if (phase > 1.0f) phase -= 1.0f;

    float sineOut = fastersinfullf(phase * 2.0f * M_PI);
    float value = sineOut * env * start;

    // Add harmonic distortion
    if (harmonics > 0.0f) {
        value += harmonics * fastertanhf(sineOut * 3.0f) * env;
    }

    // Add noise burst
    if (noise > 0.0f && t < 0.01f) {
        value += noise * drum_rng_bipolar(&rng_) * env;
    }

    // Soft clip
    if (clip > 0.0f) {
        value = fastertanhf(value * (1.0f + clip * 5.0f));
    }

    return value;
}


void TRXBassDrum::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
          pitch = 50.0f; decay = 0.117f; ramp = 0.152f; rampDecay = 0.034f; start = 0.794f; noise = 0.201f; harmonics = 0.279f; clip = 0.137f;
          break;
        case 1:
          pitch = 30.098f; decay = 0.088f; ramp = 0.075f; rampDecay = 0.033f; start = 1.785f; noise = 0.088f; harmonics = 0.583f; clip = 0.368f;
          break;
        // case 2: - maybe in the future
    }
};
void TRXBassDrum::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            pitch = 20.0f + value;
            break;
        case K_Modulation_Feedback:
            ramp = value * 0.01f;
            break;
        case K_Modulation_Index:
            harmonics = value * 0.01f;
            break;
        case K_Modulation_Decay:
            rampDecay = value * 0.01f;
            break;
        case K_Decay_A:
            decay = 0.01f + value * 0.0199f;
            break;
        case K_Noise_Level:
            noise = value * 0.01f;
            break;
        case K_Gap:
            start = value * 0.0199f;
            break;
        case K_Count:
            clip = value * 0.01f;
            break;
        default:
            break;
    }
};
float TRXBassDrum::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            return pitch;
            break;
        case K_Modulation_Feedback:
            return ramp;
            break;
        case K_Modulation_Index:
            return harmonics;
            break;
        case K_Modulation_Decay:
            return rampDecay;
            break;
        case K_Decay_A:
            return decay;
            break;
        case K_Noise_Level:
            return noise;
            break;
        case K_Gap:
            return start;  // seconds
            break;
        case K_Count:
            return clip;
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};


// void TRXBassDrum::RenderControls() {
//     ImGui::SliderFloat("Pitch", &pitch, 20.0f, 120.0f);
//     ImGui::SliderFloat("Decay", &decay, 0.01f, 2.0f);
//     ImGui::SliderFloat("Ramp", &ramp, 0.0f, 1.0f);
//     ImGui::SliderFloat("Ramp Decay", &rampDecay, 0.01f, 1.0f);
//     ImGui::SliderFloat("Start", &start, 0.0f, 2.0f);
//     ImGui::SliderFloat("Noise", &noise, 0.0f, 1.0f);
//     ImGui::SliderFloat("Harmonics", &harmonics, 0.0f, 1.0f);
//     ImGui::SliderFloat("Clip", &clip, 0.0f, 1.0f);
// }

