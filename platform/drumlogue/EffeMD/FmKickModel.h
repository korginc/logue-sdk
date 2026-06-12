#pragma once
#include "DrumModel.h"

/**
 * Two-operator FM kick (EFM bass drum model).
 *
 * Ported from ctag-fh-kiel/md-drum-synth FmKickModel.
 * The original used the Mutable Instruments (plaits::fm) operator; this
 * version uses the Sonaglio scalar FM operator (fm_operator.h, fm_op_t),
 * preserving the original math:
 *   - modulator with DX7-style feedback b_m feeds the carrier phase
 *   - phase modulation depth = I * mod_env (normalized-phase units)
 *   - carrier frequency swept by A_f * exp-decay(d_f)
 */
class FmKickModel : public DrumModel {
public:
    FmKickModel(void) : f_b(50.0f), d_b(0.5f), f_m(180.0f), I(20.0f), d_m(0.15f),
                        b_m(0.5f), A_f(60.0f), d_f(0.1f),
                        use_ratio_mode(false), ratio_index(0), mod_env_sync(false),
                        amp_env(1.0f), mod_env(1.0f), freq_env(1.0f),
                        amp_decay_const(0.0f), mod_decay_const(0.0f), freq_decay_const(0.0f) {
        fmo_init(&mod_op_);
        fmo_init(&car_op_);
    }
    ~FmKickModel(void) override {}

    void Init() override;
    void Trigger() override;
    float Process() override;

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

private:
    float f_b, d_b, f_m, I;
    float d_m, b_m, A_f, d_f;

    // Ratio mode for modulator frequency
    bool use_ratio_mode;
    int ratio_index; // Index into ratio array
    static constexpr int num_ratios = 64;
    static const float ratios[num_ratios][2];

    bool mod_env_sync; // Sync modulator freq envelope to carrier sweep

    // Iterative decay state
    float amp_env, mod_env, freq_env;
    float amp_decay_const, mod_decay_const, freq_decay_const;

    // Sonaglio scalar FM operators
    fm_op_t mod_op_;
    fm_op_t car_op_;
};
