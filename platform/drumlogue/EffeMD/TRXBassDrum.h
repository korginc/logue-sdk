#pragma once
#include "DrumModel.h"

class TRXBassDrum : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    // void loadParameters(std::istream& is) override {
    //     is >> pitch >> decay >> ramp >> rampDecay >> start >> noise >> harmonics >> clip;
    // }
    TRXBassDrum(void) : pitch(50.0f), decay(0.4f), ramp(0.3f), rampDecay(0.1f),
                        start(1.0f), noise(0.0f), harmonics(0.0f), clip(0.0f),
                        phase(0.0f), t(0.0f), env(0.0f), rampEnv(0.0f), prevSample(0.0f) {};
    ~TRXBassDrum(void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

    // void RenderControls() override;

private:
    // User parameters
    float pitch;        // Base pitch in Hz
    float decay;        // Envelope decay time
    float ramp;         // Frequency ramp amount
    float rampDecay;    // Ramp decay time
    float start;        // Start amplitude multiplier
    float noise;        // Noise at attack
    float harmonics;    // Adds clipped harmonic content
    float clip;         // Soft clipping amount

    // Internal state
    float phase;
    float t;
    float env;
    float rampEnv;
    float prevSample;

    // Helpers
    float sine(float x);
};
