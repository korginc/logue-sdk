// TRXGongModel.h
#pragma once
#include "DrumModel.h"

/**
 * TRX Gong — two detuned sines with a long (up to ~5 s) decay and clip.
 *
 * This is an exact clone of the original TRXClaves DSP, keeping the wide
 * decay-time mapping that made it ring like a gong. TRXClaves itself was
 * retuned to a short, click-like decay so it sounds like real claves; this
 * model preserves the long, beating, gong-like voice as its own instrument.
 */
class TRXGongModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    TRXGongModel(void) : pitch(600.0f), interval(200.0f), decay(0.1f), balance(0.5f), clip(0.2f),
                         phase1(0.0f), phase2(0.0f), env(0.0f), t(0.0f), env_mul(0.0f) {};
    ~TRXGongModel(void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

private:
    float pitch;     // Base pitch (Hz)
    float interval;  // Interval between the two oscillators
    float decay;     // Envelope decay (long range → gong)
    float balance;   // Balance between osc1 and osc2
    float clip;      // Clipping/saturation

    float phase1;
    float phase2;
    float env;
    float t;
    float env_mul;   // per-sample decay multiplier, computed at Trigger

    float sine(float x);
};
