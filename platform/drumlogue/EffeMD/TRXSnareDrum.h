#pragma once
#include "DrumModel.h"

class TRXSnareDrum : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    // void loadParameters(std::istream& is) override {
    //     is >> pitch >> decay >> snap >> noise >> tone >> tune >> bump >> clip;
    // }
    TRXSnareDrum(void) : pitch(180.0f), decay(0.4f), snap(0.6f), noise(0.5f),
                        tone(0.5f), tune(100.0f), bump(0.1f), clip(0.2f),
                        t(0.0f), ampEnv(0.0f), snapEnv(0.0f), phase1(0.0f),
                        phase2(0.0f), hp_x(0.0f), hp_y(0.0f) {};
    ~TRXSnareDrum (void)  override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;


private:
    // Parameters
    float pitch;      // Base pitch (Hz)
    float decay;        // Amplitude decay
    float snap;         // Extra noisy attack
    float noise;        // Noise level
    float tone;         // Balance between oscillators
    float tune;       // Frequency interval between osc1 and osc2
    float bump;         // Small pitch rise at start
    float clip;         // Clipping intensity

    // Envelope
    float t;
    float ampEnv;
    float snapEnv;

    // Oscillator phase
    float phase1;
    float phase2;

    // Filter state for noise
    float hp_x, hp_y;

    float sine(float x);
};
