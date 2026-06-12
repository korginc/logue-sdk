#include "TRXSnareDrum.h"

void TRXSnareDrum::Init() {
    t = ampEnv = snapEnv = 0.0f;
    phase1 = phase2 = 0.0f;
    hp_x = hp_y = 0.0f;
    drum_rng_seed(&rng_, 0x5D000001u);
}

void TRXSnareDrum::Trigger() {
    t = 0.0f;
    ampEnv = 1.0f;
    snapEnv = 1.0f;
    phase1 = phase2 = 0.0f;
    amp_mul  = expf(-INV_SAMPLE_RATE / decay);
    snap_mul = expf(-INV_SAMPLE_RATE / 0.02f); // 20ms snap noise decay
}

float TRXSnareDrum::Process() {
    if (ampEnv <= 0.0001f) return 0.0f;

    t += INV_SAMPLE_RATE;

    // Decay envelopes
    ampEnv *= amp_mul;
    snapEnv *= snap_mul;

    // Oscillators (tuned with interval)
    float freq1 = pitch + bump * 80.0f;
    float freq2 = pitch + tune;

    phase1 += freq1 * pitch_ratio_ * INV_SAMPLE_RATE;
    if (phase1 > 1.0f) phase1 -= 1.0f;
    float osc1 = fastersinfullf(phase1 * 2.0f * M_PI);

    phase2 += freq2 * pitch_ratio_ * INV_SAMPLE_RATE;
    if (phase2 > 1.0f) phase2 -= 1.0f;
    float osc2 = fastersinfullf(phase2 * 2.0f * M_PI);

    float tonePart = (tone * osc1 + (1.0f - tone) * osc2) * ampEnv;

    // Snap noise burst
    float snapNoise = drum_rng_bipolar(&rng_) * snap * snapEnv;

    // Sustained filtered noise (high-pass)
    float rawNoise = drum_rng_bipolar(&rng_);
    const float hp_a = 0.9490f; // = exp(-2*pi*400/48000), fixed 400 Hz HPF
    float hp = hp_a * (hp_y + rawNoise - hp_x);
    hp_y = rawNoise;
    hp_x = hp;

    float noisePart = noise * hp * ampEnv;

    // Sum
    float out = tonePart + snapNoise + noisePart;

    // Clip
    if (clip > 0.0f) {
        out = fastertanhf(out * (1.0f + clip * 5.0f));
    }

    return out;
}


void TRXSnareDrum::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
            pitch = 180.0f; decay = 0.05f; snap = 0.211f; noise = 0.417f; tone = 0.348f; tune = 105.882f; bump = 0.108f; clip = 0.078f;
            break;
        case 1:
            pitch = 176.287f; decay = 0.065f; snap = 0.733f; noise = 0.169f; tone = 0.388f; tune = 105.537f; bump = 0.1f; clip = 0.2f;
            break;
        // case 2: - maybe in the future
    }
};

void TRXSnareDrum::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 1..100 - TODO 1..1000
    switch (param_index) {
        case K_Base_Frequency:
            pitch = 60.0f + value * 3.4f;
            break;
        case K_Frequency_Sweep:
            snap = value * 0.01f;
            break;
        case K_Noise_Level:
            noise = value * 0.01f;
            break;
        case K_Mix:
            tone = value * 0.01f;
            break;
        case K_Sweep_Decay:
            bump = value * 0.01f;
            break;
        case K_Decay_A:
            decay = 0.05f + value * 0.0095f;
            break;
        case K_Gap:
            tune = value * 4.0f;
            break;
        case K_Count:
            clip = value * 0.01f;
            break;
        default:
            break;
    }
};
float TRXSnareDrum::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Base_Frequency:
            return pitch;
            break;
        case K_Frequency_Sweep:
            return snap;
            break;
        case K_Noise_Level:
            return noise;
            break;
        case K_Mix:
            return tone;
            break;
        case K_Decay_A:
            return decay;
            break;
        case K_Gap:
            return tune;
            break;
        case K_Count:
            return clip;
            break;
        case K_Sweep_Decay:
            return bump;
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void TRXSnareDrum::RenderControls() {
//     ImGui::SliderFloat("Pitch", &pitch, 60.0f, 400.0f);
//     ImGui::SliderFloat("Decay", &decay, 0.05f, 1.0f);
//     ImGui::SliderFloat("Snap", &snap, 0.0f, 1.0f);
//     ImGui::SliderFloat("Noise", &noise, 0.0f, 1.0f);
//     ImGui::SliderFloat("Tone Balance", &tone, 0.0f, 1.0f);
//     ImGui::SliderFloat("Tune Interval", &tune, 0.0f, 400.0f);
//     ImGui::SliderFloat("Bump", &bump, 0.0f, 1.0f);
//     ImGui::SliderFloat("Clip", &clip, 0.0f, 1.0f);
// }

