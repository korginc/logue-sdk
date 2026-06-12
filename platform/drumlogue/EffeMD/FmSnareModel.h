// FmSnareModel.h
#pragma once
#include "DrumModel.h"

/**
 * Two-operator FM snare with filtered noise (EFM snare model).
 *
 * Ported from ctag-fh-kiel/md-drum-synth FmSnareModel.
 * The original used the Mutable Instruments (plaits::fm) operator; this
 * version uses the Sonaglio scalar FM operator (fm_operator.h, fm_op_t)
 * and a deterministic xorshift noise source instead of rand().
 */
class FmSnareModel : public DrumModel {
public:
    FmSnareModel(void) : f_b(200.0f), d_b(0.4f), f_m(1500.0f), I(15.0f), d_m(0.1f),
                         Abrus(0.5f), dbrus(0.3f), fhp(400.0f),
                         y_prev(0.0f), x_prev(0.0f), amp_env(1.0f), mod_env(1.0f), noise_env(1.0f),
                         amp_decay_const(0.0f), mod_decay_const(0.0f), noise_decay_const(0.0f) {
        fmo_init(&mod_op_);
        fmo_init(&car_op_);
        drum_rng_seed(&rng_, 0x5A4E0001u);
    }
    ~FmSnareModel(void) override {}

    void Init() override;
    void Trigger() override;
    float Process() override;

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

private:
    // FM parameters
    float f_b;     // Carrier frequency
    float d_b;     // Amplitude envelope decay
    float f_m;     // Modulator frequency
    float I;       // Modulation index
    float d_m;     // Modulator envelope decay

    // Noise and filter
    float Abrus;     // Noise level
    float dbrus;     // Noise envelope decay
    float fhp;       // High-pass filter cutoff (Hz)

    // Internal state
    float y_prev, x_prev; // HPF state

    // Envelope states and decay constants
    float amp_env;
    float mod_env;
    float noise_env;
    float amp_decay_const;
    float mod_decay_const;
    float noise_decay_const;

    // Sonaglio scalar FM operators
    fm_op_t mod_op_;
    fm_op_t car_op_;

    drum_rng_t rng_;
};
