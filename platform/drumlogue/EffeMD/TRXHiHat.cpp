#include "TRXHiHat.h"


void TRXHiHat::Init() {
    rng.seed(std::random_device{}());
    env = 0.0f;
    t = 0.0f;
    lp_y = hp_y = hp_x = 0.0f;
}

void TRXHiHat::Trigger() {
    env = 1.0f;
    t = 0.0f;
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
    float lpfAlpha = e_expff(-2.0f * M_PI * lpfFreq * INV_SAMPLE_RATE);
    lp_y = (1.0f - lpfAlpha) * n + lpfAlpha * lp_y;

    // Apply high-pass filter
    float hpfAlpha = e_expff(-2.0f * M_PI * hpfFreq * INV_SAMPLE_RATE);
    float hp = hpfAlpha * (hp_y + lp_y - hp_x);
    hp_y = lp_y;
    hp_x = hp;

    // Envelope decay
    env *= e_expff(-INV_SAMPLE_RATE / (decay));

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
    static float phase[6] = { 0 };
    static const float freqs[6] = { 306.0f, 512.0f, 551.0f, 743.0f, 826.0f, 900.0f };

    for (int i = 0; i < 6; ++i) {
        phase[i] += freqs[i] * INV_SAMPLE_RATE;
        if (phase[i] >= 1.0f) phase[i] -= 1.0f;
        result += (phase[i] < 0.5f ? 1.0f : -1.0f);
    }

    float white = noiseDist(rng);
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
    // user editable parameters are in range 1..100
    switch (param_index) {
        case K_Decay_A:
            decay = 0.01f + value * 0.0099f;
            break;
        case K_Gap:
            gap = value * 0.01f;
            break;
        case K_Count:
            peak = value * 0.01f;
            break;
        case K_HPF:
            hpfFreq = 100.0f + value * 99.0f;
            break;
        case K_LPF:
            lpfFreq = 1000.0f + value * 110.0f;
            break;
        default:
            break;
    }
};
float TRXHiHat::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 1..100
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
