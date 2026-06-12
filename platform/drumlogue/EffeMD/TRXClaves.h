#pragma once
#include "DrumModel.h"

class TRXClaves : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    // void loadParameters(std::istream& is) override {
    //     is >> pitch >> interval >> decay >> balance >> clip;
    // }
    TRXClaves(void) : pitch(600.0f), interval(200.0f), decay(0.1f), balance(0.5f), clip(0.2f),
                       phase1(0.0f), phase2(0.0f), env(0.0f), t(0.0f) {};
    ~TRXClaves(void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

    // void RenderControls() override;

private:
    float pitch;     // Base pitch (Hz)
    float interval;  // Interval between the two oscillators
    float decay;       // Envelope decay
    float balance;     // Balance between osc1 and osc2
    float clip;        // Clipping/saturation

    float phase1;
    float phase2;
    float env;
    float t;

    float sine(float x);
};
