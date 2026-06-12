// FmTomModel.h
#pragma once
#include "DrumModel.h"

class FmTomModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    // void loadParameters(std::istream& is) override {
    //     is >> f_b >> d_b >> f_m >> I >> d_m >> A_f >> d_f;
    // }
    FmTomModel(void) :  f_b(150.0f), d_b(0.7f), f_m(300.0f), I(15.0f), d_m(0.2f),
                        A_f(30.0f), d_f(0.1f), start_phase(3.14159f / 2.0f),
                        mod_phase(0.0f), car_phase(0.0f), prev_mod(0.0f), t(0.0f) {};
    ~FmTomModel (void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;
private:
    float f_b, d_b, f_m, I, d_m;
    float A_f, d_f, start_phase;
    float mod_phase, car_phase, prev_mod, t;
    /**
    f_b (Base Frequency)",
    d_b (Amp Decay)",
    f_m (Modulator Freq)",
    I (Mod Index)",
    d_m (Mod Decay)",

    A_f (Freq Sweep Amt
    d_f (Freq Sweep Decay

    Start Phase", &start_phase

*/
};