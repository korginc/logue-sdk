// FmRimshotModel.h
#pragma once
#include "DrumModel.h"

class FmRimshotModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;

    // void loadParameters(std::istream& is) override {
    //     is >> f_bB >> d_bB >> I_B >> f_bA >> d_bA >> I_A >> A_A >> d_m >> f_hp;
    // }
    FmRimshotModel(void) : f_bB(600.0f), d_bB(0.05f), I_B(15.0f),
                            f_bA(200.0f), d_bA(0.25f), I_A(10.0f),
                            A_A(0.4f), d_m(0.05f), f_hp(400.0f),
                            mod_phase(0.0f), carB_phase(0.0f), carA_phase(0.0f),
                            prev_mod(0.0f), t(0.0f), x_prev(0.0f), y_prev(0.0f) {};
    ~FmRimshotModel (void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;
private:
 /**
  * f_bB (rim freq)",
 d_bB (rim decay)",
 I_B (rim mod index
 f_bA (body freq)",
 d_bA (body decay)",
 I_A (body mod index
 A_A (body mix)",
 d_m (mod env decay
 f_hp (K_HPF cutoff)",
  */
 float f_bB, d_bB, I_B;
 float f_bA, d_bA, I_A;
 float A_A;
 float d_m;
 float f_hp;

 float mod_phase, carB_phase, carA_phase, prev_mod, t;
 float x_prev, y_prev;
};
