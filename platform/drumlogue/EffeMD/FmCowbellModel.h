// FmCowbellModel.h
#pragma once
#include "DrumModel.h"
#include "midi_handler.h"

class FmCowbellModel : public DrumModel {
public:
    void Init() override;
    void Trigger() override;
    float Process() override;
    void Update() { fbB = fbA * 1.48f; Ab2 = 1.0f - Ab1; };

    // void loadParameters(std::istream& is) override {
    //     is >> fbA >> d_b1 >> db2 >> fm >> I >> dm >> bm >> Ab1;
    //     fbB = fbA * 1.48f;
    //     Ab2 = 1.0f - Ab1;
    // }
    FmCowbellModel(void) :  fbA(540.0f), fbB(540.0f * 1.48f),
                            d_b1(0.015f), db2(0.1f), fm(2000.0f),
                            I(15.0f), dm(0.1f), bm(0.3f),
                            Ab1(0.7f), Ab2(1.0f - 0.7f), mod_phase(0.0f),
                            carA_phase(0.0f),
                            carB_phase(0.0f), prev_mod(0.0f), t(0.0f) {};
    ~FmCowbellModel (void) override {};


    void loadPreset(uint8_t idx) override;
    void setParameter(fm_param_index_t param_index, float value) override;
    float getParameter(fm_param_index_t param_index) override;

    private : float fbA, fbB;
    float d_b1, db2;
    float fm, I, dm, bm;
    float Ab1, Ab2;
    float mod_phase, carA_phase, carB_phase, prev_mod, t;
};
