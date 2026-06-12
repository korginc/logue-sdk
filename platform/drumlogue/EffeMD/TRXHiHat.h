#pragma once
#include "DrumModel.h"

class TRXHiHat : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float get_value(float fadeTime);
    float Process() override;

    // void loadParameters(std::istream& is) override {
    //     is >> gap >> decay >> lpfFreq >> hpfFreq >> peak >> metal;
    // }
    TRXHiHat(void) : gap(0.5f), decay(0.2f), lpfFreq(8000.0f), hpfFreq(4000.0f),
                     peak(0.5f), metal(0.7f), env(0.0f), t(0.0f),
                     lp_y(0.0f), hp_y(0.0f), hp_x(0.0f) {};
    ~TRXHiHat (void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

    // void RenderControls() override;
private:
    // Parameters
    float gap;
    float decay;
    float lpfFreq;
    float hpfFreq;
    float peak;
    float metal;

    // Envelope
    float env;
    float t;

    // Filters
    float lp_y;
    float hp_y;
    float hp_x;

    // Per-sample coefficients, computed at Trigger with accurate expf
    float env_mul;
    float lpf_alpha;
    float hpf_alpha;

    // Metallic square-stack oscillator phases (per instance)
    float phase_[6];

    // Noise source (deterministic, RT-safe)
    drum_rng_t rng_;

    float generateMetallicNoise();
};
