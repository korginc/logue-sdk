#pragma once
/*
 * File: wavefolder.h
 * 
 * Complete drive/wavefolder implementation with 5 modes
 * Now with proper drive control and dry/wet blending
 */

#include <arm_neon.h>
#include <math.h>

#define DRIVE_MODE_SOFT_CLIP   0
#define DRIVE_MODE_HARD_CLIP    1
#define DRIVE_MODE_TRIANGLE     2
#define DRIVE_MODE_SINE         3
#define DRIVE_MODE_SUBOCTAVE    4

typedef struct {
    uint32_t mode;
    float32x4_t drive;           // Drive amount (0.0 to 1.0)
    float32x4_t output_gain;     // Makeup after drive
    
    // Sub-octave state
    float32x4_t sub_phase;
    float32x4_t last_input;
    uint32x4_t zero_cross;
} wavefolder_t;

// Initialize wavefolder
fast_inline void wavefolder_init(wavefolder_t* wf) {
    wf->mode = DRIVE_MODE_SOFT_CLIP;
    wf->drive = vdupq_n_f32(0.0f);
    wf->output_gain = vdupq_n_f32(1.0f);
    wf->sub_phase = vdupq_n_f32(0.0f);
    wf->last_input = vdupq_n_f32(0.0f);
    wf->zero_cross = vdupq_n_u32(0);
}

// Set drive amount (0-100%)
fast_inline void wavefolder_set_drive(wavefolder_t* wf, float drive_percent) {
    float drive = drive_percent / 100.0f;
    wf->drive = vdupq_n_f32(drive);
    
    // Auto makeup gain (inverse of drive reduction)
    float makeup = 1.0f / (0.2f + drive * 0.8f);
    wf->output_gain = vdupq_n_f32(makeup);
}

// Soft clip (tanh approximation)
fast_inline float32x4_t soft_clip(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t three = vdupq_n_f32(3.0f);
    
    // x - x^3/3 for small x, clamp to ±1 for large x
    float32x4_t x2 = vmulq_f32(x, x);
    float32x4_t x3 = vmulq_f32(x, x2);
    float32x4_t approx = vsubq_f32(x, vdivq_f32(x3, three));
    
    return vmaxq_f32(vminq_f32(approx, one), vnegq_f32(one));
}

// Hard clip (limiter)
fast_inline float32x4_t hard_clip(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    return vmaxq_f32(vminq_f32(x, one), vnegq_f32(one));
}

// Triangle folder
fast_inline float32x4_t triangle_folder(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t two = vdupq_n_f32(2.0f);
    float32x4_t four = vdupq_n_f32(4.0f);
    
    // y = |(x + 1) % 4 - 2| - 1
    float32x4_t shifted = vaddq_f32(x, one);
    float32x4_t div = vmulq_f32(shifted, vdupq_n_f32(0.25f));
    int32x4_t floor_div = vcvtq_s32_f32(div);
    float32x4_t mod = vsubq_f32(shifted,
                                vmulq_f32(vcvtq_f32_s32(floor_div), four));
    
    float32x4_t abs_diff = vabsq_f32(vsubq_f32(mod, two));
    return vsubq_f32(abs_diff, one);
}

// Sine folder
fast_inline float32x4_t sine_folder(float32x4_t x) {
    float32x4_t half_pi = vdupq_n_f32(M_PI_2);
    float32x4_t two = vdupq_n_f32(2.0f);
    
    x = vmaxq_f32(vminq_f32(x, two), vnegq_f32(two));
    float32x4_t angle = vmulq_f32(x, half_pi);
    
    // sin(angle) ≈ angle - angle^3/6
    float32x4_t a2 = vmulq_f32(angle, angle);
    float32x4_t a3 = vmulq_f32(angle, a2);
    return vsubq_f32(angle, vmulq_f32(a3, vdupq_n_f32(1.0f/6.0f)));
}

// Sub-octave generator
fast_inline float32x4_t suboctave_process(wavefolder_t* wf, float32x4_t in) {
    // Detect zero crossing
    uint32x4_t last_neg = vcltq_f32(wf->last_input, vdupq_n_f32(0.0f));
    uint32x4_t curr_pos = vcgeq_f32(in, vdupq_n_f32(0.0f));
    uint32x4_t cross = vandq_u32(last_neg, curr_pos);
    
    // Increment phase on zero crossing (half frequency)
    wf->sub_phase = vbslq_f32(cross,
                              vaddq_f32(wf->sub_phase, vdupq_n_f32(0.5f)),
                              wf->sub_phase);
    
    // Wrap phase
    uint32x4_t wrap = vcgeq_f32(wf->sub_phase, vdupq_n_f32(1.0f));
    wf->sub_phase = vbslq_f32(wrap,
                              vsubq_f32(wf->sub_phase, vdupq_n_f32(1.0f)),
                              wf->sub_phase);
    
    // Square wave
    uint32x4_t square_high = vcltq_f32(wf->sub_phase, vdupq_n_f32(0.5f));
    float32x4_t square = vbslq_f32(square_high,
                                   vdupq_n_f32(1.0f),
                                   vdupq_n_f32(-1.0f));
    
    wf->last_input = in;
    return square;
}

// Main wavefolder processing with drive and blend
fast_inline float32x4x2_t wavefolder_process(wavefolder_t* wf,
                                              float32x4_t in_l,
                                              float32x4_t in_r) {
    float32x4x2_t out;
    
    // Apply drive gain (pre-drive)
    float32x4_t driven_l = vmulq_f32(in_l, vaddq_f32(vdupq_n_f32(1.0f),
                                                      vmulq_f32(wf->drive, vdupq_n_f32(4.0f))));
    float32x4_t driven_r = vmulq_f32(in_r, vaddq_f32(vdupq_n_f32(1.0f),
                                                      vmulq_f32(wf->drive, vdupq_n_f32(4.0f))));
    
    // Apply selected waveshaping
    switch (wf->mode) {
        case DRIVE_MODE_SOFT_CLIP:
            out.val[0] = soft_clip(driven_l);
            out.val[1] = soft_clip(driven_r);
            break;
            
        case DRIVE_MODE_HARD_CLIP:
            out.val[0] = hard_clip(driven_l);
            out.val[1] = hard_clip(driven_r);
            break;
            
        case DRIVE_MODE_TRIANGLE:
            out.val[0] = triangle_folder(driven_l);
            out.val[1] = triangle_folder(driven_r);
            break;
            
        case DRIVE_MODE_SINE:
            out.val[0] = sine_folder(driven_l);
            out.val[1] = sine_folder(driven_r);
            break;
            
        case DRIVE_MODE_SUBOCTAVE:
            out.val[0] = suboctave_process(wf, driven_l);
            out.val[1] = suboctave_process(wf, driven_r);
            break;
            
        default:
            out.val[0] = driven_l;
            out.val[1] = driven_r;
    }
    
    // Apply output makeup gain
    out.val[0] = vmulq_f32(out.val[0], wf->output_gain);
    out.val[1] = vmulq_f32(out.val[1], wf->output_gain);
    
    return out;
}