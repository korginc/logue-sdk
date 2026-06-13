// FmWhistleModel.h
#pragma once
#include "DrumModel.h"

/**
 * FM Whistle — the tonal two-operator voice that the FM Clap model produces
 * when no noise layer is present.
 *
 * This is an exact clone of the original FmClapModel DSP (multi-burst FM with
 * feedback + HPF). FmClapModel itself was given a noise layer so it sounds
 * like a hand clap; this model preserves the pure-FM "whistle" character as a
 * standalone instrument.
 */
class FmWhistleModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    FmWhistleModel(void) : f_b(800.0f), f_m(800.0f), I(40.0f), d_m(0.05f),
                           d1(0.02f), d2(0.3f), clap_count(3), clap_interval(0.012f),
                           fhp(400.0f), bm(0.9f), mod_phase(0.0f), car_phase(0.0f),
                           prev_mod(0.0f), t(0.0f), y_prev(0.0f), x_prev(0.0f), active(false) {};
    ~FmWhistleModel(void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

private:
    float f_b, f_m, I, d_m;
    float d1, d2;
    int clap_count, clap_stage;
    float clap_interval;  // seconds between bursts
    float clap_timer;
    float fhp;
    float bm;  // mod feedback

    float mod_phase, car_phase, prev_mod, t;
    float y_prev, x_prev;
    bool active;
};
