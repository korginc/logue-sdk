#include "TRXHiHat.h"
#include "constants.h"

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

void TRXHiHat::Init() {
    drum_rng_seed(&rng_, 0x44000001u);
    env = 0.0f;
    t = 0.0f;
    lp_y = hp_y = hp_x = 0.0f;
    for (int i = 0; i < 6; ++i) phase_[i] = 0.0f;
}

void TRXHiHat::Trigger() {
    env = 1.0f;
    t = 0.0f;
    env_mul   = expf(-INV_SAMPLE_RATE / decay);
    lpf_alpha = expf(-2.0f * M_PI * lpfFreq * INV_SAMPLE_RATE);
    hpf_alpha = expf(-2.0f * M_PI * hpfFreq * INV_SAMPLE_RATE);
}

float TRXHiHat::get_value(const float fadeTime){
    return fadeTime;
}

float TRXHiHat::Process() {
    constexpr float fadeTime = 0.005f; // 5 ms crossfade
    t += 1.0f * INV_SAMPLE_RATE;

    if (env <= 0.0f) return 0.0f;

    // Generate metallic noise
    float n = generateMetallicNoise();

    // Apply low-pass filter
    lp_y = (1.0f - lpf_alpha) * n + lpf_alpha * lp_y;

    // Apply high-pass filter
    float hp = hpf_alpha * (hp_y + lp_y - hp_x);
    hp_y = lp_y;
    hp_x = hp;

    // Envelope decay
    env *= env_mul;

    // GAP crossfade
    if (t > gap) {
        float excess = t - gap;
        if (excess >= get_value(fadeTime)) {
            env = 0.0f;
            return 0.0f;
        } else {
            float fadeFactor = 1.0f - (excess / fadeTime);
            env *= fadeFactor;
        }
    }

    return hp * env;
}


// void TRXHiHat::RenderControls() {
//     ImGui::SliderFloat("Gap", &gap, 0.0f, 1.0f);
//     ImGui::SliderFloat("Decay", &decay, 0.01f, 1.0f);
//     ImGui::SliderFloat("LPF Freq", &lpfFreq, 1000.0f, 12000.0f);
//     ImGui::SliderFloat("HPF Freq", &hpfFreq, 100.0f, 10000.0f);
//     ImGui::SliderFloat("Peak", &peak, 0.0f, 1.0f);
//     ImGui::SliderFloat("Metal", &metal, 0.0f, 1.0f);
// }

float TRXHiHat::generateMetallicNoise() {
    // Square wave harmonic mix — crude but efficient
    float result = 0.0f;
    static const float freqs[6] = { 306.0f, 512.0f, 551.0f, 743.0f, 826.0f, 900.0f };

    for (int i = 0; i < 6; ++i) {
        phase_[i] += freqs[i] * pitch_ratio_ * INV_SAMPLE_RATE;
        if (phase_[i] >= 1.0f) phase_[i] -= 1.0f;
        result += (phase_[i] < 0.5f ? 1.0f : -1.0f);
    }

    float white = drum_rng_bipolar(&rng_);
    return metal * (result / 6.0f) + (1.0f - metal) * white;
}


void TRXHiHat::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
            gap = 0.819f; decay = 0.034f; lpfFreq = 10112.7f; hpfFreq = 1604.41f; peak = 1.0f; metal = 0.569f;
            break;
        case 1:
            gap = 0.489f; decay = 0.084f; lpfFreq = 8990.23f; hpfFreq = 5678.83f; peak = 0.759f; metal = 0.583f;
            break;
        // case 2: - maybe in the future
    }
};
void TRXHiHat::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Gap:
        // SliderFloat("Gap", &gap, 0.0f, 1.0f);
            gap = value * 0.01f;
            break;
        case K_Decay_A:
        // SliderFloat("Decay", &decay, 0.01f, 1.0f);
            decay = 0.01f + value * 0.0099f;
            break;
        case K_HPF:
        // SliderFloat("LPF Freq", &lpfFreq, 1000.0f, 12000.0f);
            hpfFreq = 100.0f + value * 99.0f;
            break;
        case K_LPF:
        // SliderFloat("HPF Freq", &hpfFreq, 100.0f, 10000.0f);
            lpfFreq = 1000.0f + value * 110.0f;
            break;
        case K_Count:
        // SliderFloat("Peak", &peak, 0.0f, 1.0f);
            peak = value * 0.01f;
            break;
        case K_Noise_Level:
        // SliderFloat("Metal", &metal, 0.0f, 1.0f);
            metal = value * 0.01f;
            break;
        default:
            break;
    }
};
float TRXHiHat::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Decay_A:
            return decay;
            break;
        case K_Gap:
            return gap;  // seconds
            break;
        case K_Count:
            return peak;
            break;
        case K_HPF:
            return hpfFreq; // Hz
            break;
        case K_LPF:
            return lpfFreq; // Hz
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void TRXHiHat::RenderControls() {
//     ImGui::SliderFloat("Gap", &gap, 0.0f, 1.0f);
//     ImGui::SliderFloat("Decay", &decay, 0.01f, 1.0f);
//     ImGui::SliderFloat("LPF Freq", &lpfFreq, 1000.0f, 12000.0f);
//     ImGui::SliderFloat("HPF Freq", &hpfFreq, 100.0f, 10000.0f);
//     ImGui::SliderFloat("Peak", &peak, 0.0f, 1.0f);
//     ImGui::SliderFloat("Metal", &metal, 0.0f, 1.0f);
// }