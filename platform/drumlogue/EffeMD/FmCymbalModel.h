// FmCymbalModel.h
#pragma once
#include "DrumModel.h"

class FmCymbalModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void Release() override;

    // void loadParameters(std::istream& is) override {
    //     is >> fb >> fm >> d_b >> I >> d_m >> bb >> sustain >> f_hp;
    // }
    FmCymbalModel(void) : fb(400.0f), fm(800.0f), d_b(1.0f), I(10.0f), d_m(0.2f),
                            bb(0.5f), sustain(0.3f), f_hp(300.0f), t(0.0f), x_prev(0.0f), y_prev(0.0f),
                            choke_(1.0f), choke_mul_(1.0f)
                            {};
    ~FmCymbalModel (void) override {};

    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

private:
    static constexpr int NUM_PAIRS = 4;
    float fb;     // base carrier frequency
    float fm;     // base modulator frequency
    float d_b;      // amp decay
    float I;       // FM index
    float d_m;      // mod env decay
    float bb;       // mod feedback
    float sustain;  // constant bias
    float f_hp;   // high-pass filter

    float car_phase[NUM_PAIRS] = {};
    float mod_phase[NUM_PAIRS] = {};
    float prev_mod[NUM_PAIRS] = {};
    float t;
    float x_prev, y_prev;

    // Note-off choke: 1.0 while held, ramps to 0 after Release() so the
    // sustaining tail (sustain > 0) can fall silent instead of ringing on.
    float choke_;
    float choke_mul_;
};
