// FmCymbalModel.cc
#include "FmCymbalModel.h"

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#include "sine_neon.h"
#endif

void FmCymbalModel::Init() {
    t = 0.0f;
    for (int i = 0; i < NUM_PAIRS; ++i) {
        car_phase[i] = mod_phase[i] = PI / 2.0f;
        prev_mod[i] = 0.0f;
    }
    x_prev = y_prev = 0.0f;
    choke_ = 1.0f;
    choke_mul_ = 1.0f;
}

void FmCymbalModel::Trigger() {
    Init();
    choke_ = 1.0f;
    choke_mul_ = 1.0f;  // 1.0 = not releasing
}

void FmCymbalModel::Release() {
    // Start a fast (~60 ms) fade. Only meaningful when sustain > 0, where the
    // amp envelope otherwise floors at `sustain` and never reaches silence.
    choke_mul_ = expf(-INV_SAMPLE_RATE / 0.06f);
}

float FmCymbalModel::Process() {
    float dt = INV_SAMPLE_RATE;
    float amp_env = sustain + ExpDecay(t, d_b);
    float mod_env = ExpDecay(t, d_m);

    static const float ratios[NUM_PAIRS] = {1.0f, 1.411f, 1.8f, 2.7f};
    float sample = 0.0f;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    // NEON v7: process the 4 modulator/carrier pairs in parallel lanes.
    const float32x4_t ratio_v = vld1q_f32(ratios);
    const float32x4_t two_pi_dt = vdupq_n_f32(TWO_PI * dt);

    float32x4_t mod_p  = vld1q_f32(mod_phase);
    float32x4_t car_p  = vld1q_f32(car_phase);
    float32x4_t prev_m = vld1q_f32(prev_mod);

    // mod_phase += 2*pi*(fm*ratio)*dt + bb*prev_mod
    float32x4_t mod_inc = vmulq_f32(vmulq_n_f32(ratio_v, fm * pitch_ratio_), two_pi_dt);
    mod_p = vaddq_f32(mod_p, vaddq_f32(mod_inc, vmulq_n_f32(prev_m, bb)));
    // Keep phases bounded: subtract 2*pi*round(phase/2*pi)
    {
        int32x4_t n = round_to_nearest_int(vmulq_n_f32(mod_p, kInvTwoPi));
        mod_p = vmlsq_f32(mod_p, vcvtq_f32_s32(n), vdupq_n_f32(kTwoPi));
    }
    float32x4_t mod_out = neon_sin(mod_p);

    // car_phase += 2*pi*(fb*ratio)*dt + I*mod_env*mod_out
    float32x4_t car_inc = vmulq_f32(vmulq_n_f32(ratio_v, fb * pitch_ratio_), two_pi_dt);
    car_p = vaddq_f32(car_p, vaddq_f32(car_inc, vmulq_n_f32(mod_out, I * mod_env)));
    {
        int32x4_t n = round_to_nearest_int(vmulq_n_f32(car_p, kInvTwoPi));
        car_p = vmlsq_f32(car_p, vcvtq_f32_s32(n), vdupq_n_f32(kTwoPi));
    }
    float32x4_t car_out = neon_sin(car_p);

    vst1q_f32(mod_phase, mod_p);
    vst1q_f32(car_phase, car_p);
    vst1q_f32(prev_mod, mod_out);

    // Horizontal sum (ARMv7-compatible)
    float32x2_t s2 = vpadd_f32(vget_low_f32(car_out), vget_high_f32(car_out));
    s2 = vpadd_f32(s2, s2);
    sample = vget_lane_f32(s2, 0);
#else
    for (int i = 0; i < NUM_PAIRS; ++i) {
        mod_phase[i] = WrapPhase(mod_phase[i] + TWO_PI * (fm * pitch_ratio_ * ratios[i]) * dt + bb * prev_mod[i]);
        float mod_out = fastersinfullf(mod_phase[i]);
        prev_mod[i] = mod_out;

        car_phase[i] = WrapPhase(car_phase[i] + TWO_PI * (fb * pitch_ratio_ * ratios[i]) * dt + I * mod_env * mod_out);
        float car_out = fastersinfullf(car_phase[i]);

        sample += car_out;
    }
#endif

    float mixed = sample * 0.25f * amp_env;

    float alpha = 1.0f / (1.0f + 2.0f * PI * f_hp * dt);
    float y = alpha * (y_prev + mixed - x_prev);
    x_prev = mixed;
    y_prev = y;

    t += dt;
    choke_ *= choke_mul_;  // 1.0 until Release(), then fades to 0
    return y * choke_;
}


void FmCymbalModel::loadPreset(uint8_t idx) {
    switch (idx) {
        case 0:
        fb = 395.588f; fm = 2000.0f; d_b = 0.108f; I = 14.559f; d_m = 0.461f; bb = 0.088f; sustain = 0.0f; f_hp = 1031.37f;
        break;
        case 1:
        fb = 295.492f; fm = 745.902f; d_b = 0.066f; I = 15.123f; d_m = 0.234f; bb = 0.049f; sustain = 0.0f; f_hp = 1197.95f;
          break;
        // case 2: - maybe in the future
    }
};
void FmCymbalModel::setParameter(fm_param_index_t param_index, float value) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
        // ParameterSlider("fb (Base Carrier)", &fb, 100.0f, 1000.0f);
            fb = 100.0f + value * 4.5f; //0..200
            break;
        case K_Modulation_Frequency:
        // ParameterSlider("fm (Base Mod)", &fm, 200.0f, 2000.0f);
            fm = 200.0f + value * 18.0f;
            break;
        case K_Modulation_Feedback:
        // ParameterSlider("bb (Mod Feedback)", &bb, 0.0f, 1.0f);
        bb = 0.01f + value * 0.0099f;
        break;
        case K_Modulation_Index:
        // ParameterSlider("I (FM Index)", &I, 0.0f, 30.0f);
        I = value * 0.15f;   //0..200
        break;
        case K_Modulation_Decay:
        // ParameterSlider("d_m (Mod Decay)", &d_m, 0.05f, 2.0f);
        d_m = 0.01f + value * 0.0099f;
        break;
        case K_Decay_A:
        // ParameterSlider("d_b (Amp Decay)", &d_b, 0.05f, 4.0f);
            d_b = 0.05f + value * 0.0495f;
            break;
        case K_Sustain:
        // ParameterSlider("sustain", &sustain, 0.0f, 1.0f);
            sustain = value * 0.01f;
            break;
        case K_HPF:
        // ParameterSlider("f_hp (HPF Cutoff)", &f_hp, 100.0f, 2000.0f);
            f_hp = 100.0f + value * 19.9f;
            break;
        default:
            break;
    }
};
float FmCymbalModel::getParameter(fm_param_index_t param_index) {
    // user editable parameters are in range 0..100
    switch (param_index) {
        case K_Base_Frequency:
            return fb;
            break;
        case K_Modulation_Frequency:
            return fm;
            break;
        case K_Modulation_Feedback:
            return bb;
            break;
        case K_Modulation_Index:
            return I;
            break;
        case K_Modulation_Decay:
            return d_m;
            break;
        case K_Decay_A:
            return d_b;
            break;
        case K_Sustain:
            return sustain;
            break;
        case K_HPF:
            return f_hp; // Hz
            break;
        default:
            return 255.0f;  // invalid
            break;
    }
};

// void FmCymbalModel::RenderControls() {
//     CustomControls::ParameterSlider("fb (Base Carrier)", &fb, 100.0f, 1000.0f);
//     CustomControls::ParameterSlider("fm (Base Mod)", &fm, 200.0f, 2000.0f);
//     CustomControls::ParameterSlider("d_b (Amp Decay)", &d_b, 0.05f, 4.0f);
//     CustomControls::ParameterSlider("I (FM Index)", &I, 0.0f, 30.0f);
//     CustomControls::ParameterSlider("d_m (Mod Decay)", &d_m, 0.05f, 2.0f);
//     CustomControls::ParameterSlider("bb (Mod Feedback)", &bb, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("sustain", &sustain, 0.0f, 1.0f);
//     CustomControls::ParameterSlider("f_hp (HPF Cutoff)", &f_hp, 100.0f, 2000.0f);
// }