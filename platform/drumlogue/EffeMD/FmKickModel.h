#pragma once
#include "DrumModel.h"

class FmKickModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    // void loadParameters(std::istream& is) override {
    //     is >> f_b >> d_b >> f_m >> I >> d_m >> b_m >> A_f >> d_f >> use_ratio_mode >> ratio_index >> mod_env_sync;
    // }
    FmKickModel(void) : f_b(50.0f), d_b(0.5f), f_m(180.0f), I(20.0f), d_m(0.15f),
                        b_m(0.5f), A_f(60.0f), d_f(0.1f), t(0.0f),
                        use_ratio_mode(false), ratio_index(0), mod_env_sync(false),
                        amp_env(1.0f), mod_env(1.0f), freq_env(1.0f),
                        amp_decay_const(0.0f), mod_decay_const(0.0f), freq_decay_const(0.0f),
                        fb_state{0.0f, 0.0f} {};
    ~FmKickModel (void) override {};

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
    static constexpr float ratios[num_ratios][2] = {
        // Integer multiples 2:1 to 40:1
        {2.0f, 1.0f}, {3.0f, 1.0f}, {4.0f, 1.0f}, {5.0f, 1.0f}, {6.0f, 1.0f}, {7.0f, 1.0f}, {8.0f, 1.0f}, {9.0f, 1.0f},
        {10.0f, 1.0f}, {11.0f, 1.0f}, {12.0f, 1.0f}, {13.0f, 1.0f}, {14.0f, 1.0f}, {15.0f, 1.0f}, {16.0f, 1.0f}, {17.0f, 1.0f},
        {18.0f, 1.0f}, {19.0f, 1.0f}, {20.0f, 1.0f}, {21.0f, 1.0f}, {22.0f, 1.0f}, {23.0f, 1.0f}, {24.0f, 1.0f}, {25.0f, 1.0f},
        {26.0f, 1.0f}, {27.0f, 1.0f}, {28.0f, 1.0f}, {29.0f, 1.0f}, {30.0f, 1.0f}, {31.0f, 1.0f}, {32.0f, 1.0f}, {33.0f, 1.0f},
        {34.0f, 1.0f}, {35.0f, 1.0f}, {36.0f, 1.0f}, {37.0f, 1.0f}, {38.0f, 1.0f}, {39.0f, 1.0f}, {40.0f, 1.0f},
        // Odd/weird ratios >1 and <=40 (25 more)
        {8.0f, 5.0f}, {11.0f, 3.0f}, {13.0f, 7.0f}, {17.0f, 5.0f}, {19.0f, 4.0f}, {23.0f, 7.0f}, {31.0f, 9.0f}, {37.0f, 8.0f},
        {15.0f, 4.0f}, {21.0f, 5.0f}, {25.0f, 6.0f}, {27.0f, 8.0f}, {35.0f, 9.0f}, {39.0f, 10.0f}, {29.0f, 7.0f}, {33.0f, 8.0f},
        {22.0f, 3.0f}, {34.0f, 5.0f}, {38.0f, 7.0f}, {5.0f, 2.0f}, {7.0f, 3.0f}, {9.0f, 2.0f}, {12.0f, 5.0f}, {14.0f, 3.0f},
        {16.0f, 5.0f}
    };

    // Iterative decay state
    float amp_env, mod_env, freq_env;
    float amp_decay_const, mod_decay_const, freq_decay_const;

    // Plaits FM operator state
    plaits::fm::Operator ops[2]; // [0]=modulator, [1]=carrier - TODO change to fm_operator
    float fb_state[2];
    float t;

    bool mod_env_sync; // New: sync modulator freq envelope to carrier
};
