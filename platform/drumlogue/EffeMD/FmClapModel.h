// FmClapModel.h
#pragma once
#include "DrumModel.h"

class FmClapModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    // void loadParameters() override {
    //     is >> f_b >> f_m >> I >> d_m >> d1 >> d2 >> clap_count >> clap_interval >> fhp >> bm;
    // }
    FmClapModel(void) : f_b(800.0f), f_m(800.0f), I(40.0f), d_m(0.05f),
                        d1(0.02f), d2(0.3f), clap_count(3), clap_interval(0.012f),
                        fhp(400.0f), bm(0.9f), mod_phase(0.0f), car_phase(0.0f),
                        prev_mod(0.0f), t(0.0f), y_prev(0.0f), x_prev(0.0f), active(false){};
    ~FmClapModel (void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

   private:
    /** f_b (Base Freq)
       f_m (Mod Freq)
       bm (Mod Feedback)
       I (Mod Index)
       d_m (Mod Decay)
       d1 (Pre-Clap Decay)
       d2 (Final Clap Decay)
       clap_count
       clap_interval
       fhp (HPF Cutoff) */
    float f_b, f_m, I, d_m;
    float d1, d2;
    int clap_count, clap_stage;
    float clap_interval;  // seconds between claps
    float clap_timer;
    float fhp;
    float bm;  // now user-controllable mod feedback

    float mod_phase, car_phase, prev_mod, t;
    float y_prev, x_prev;
    bool active
};
