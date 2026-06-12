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

    // Noise source - TODO use prng.h
    std::default_random_engine rng;
    std::uniform_real_distribution<float> noiseDist{-1.0f, 1.0f};

    float generateMetallicNoise();
};
