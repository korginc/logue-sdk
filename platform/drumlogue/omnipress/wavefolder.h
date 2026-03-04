#pragma once
/*
 * File: wavefolder.h
 * 
 * NEON-optimized wavefolder and overdrive stages
 * Implements multiple distortion types:
 * 0 = Soft clip (tanh approximation)
 * 1 = Hard clip (limiter)
 * 2 = Triangle folder
 * 3 = Sine folder
 */

#include <arm_neon.h>
#include <math.h>

typedef struct {
    uint32_t mode;  // 0-3
    float32x4_t state;  // For hysteresis (future use)
} wavefolder_t;

fast_inline void wavefolder_init(wavefolder_t* wf) {
    wf->mode = 0;
    wf->state = vdupq_n_f32(0.0f);
}

fast_inline void wavefolder_reset(wavefolder_t* wf) {
    wavefolder_init(wf);
}

// Fast tanh approximation using polynomial
// tanh(x) ≈ x - x^3/3 for small x, clamped to ±1 for large x
fast_inline float32x4_t fast_tanh(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t three = vdupq_n_f32(3.0f);
    
    // x^3
    float32x4_t x2 = vmulq_f32(x, x);
    float32x4_t x3 = vmulq_f32(x, x2);
    
    // x - x^3/3
    float32x4_t approx = vsubq_f32(x, vdivq_f32(x3, three));
    
    // Clamp to ±1
    return vmaxq_f32(vminq_f32(approx, one), vnegq_f32(one));
}

// Triangle wavefolder
// y = |(x + 1) % 4 - 2| - 1
fast_inline float32x4_t triangle_folder(float32x4_t x) {
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t two = vdupq_n_f32(2.0f);
    float32x4_t four = vdupq_n_f32(4.0f);
    
    // Shift to [0, 4) range
    float32x4_t shifted = vaddq_f32(x, one);
    
    // Modulo 4 using floor division
    float32x4_t div = vmulq_f32(shifted, vdupq_n_f32(0.25f));
    int32x4_t floor_div = vcvtq_s32_f32(div);
    float32x4_t mod = vsubq_f32(shifted, 
                                 vmulq_f32(vcvtq_f32_s32(floor_div), four));
    
    // |mod - 2| - 1
    float32x4_t abs_diff = vabsq_f32(vsubq_f32(mod, two));
    return vsubq_f32(abs_diff, one);
}

// Sine folder using polynomial approximation
// sin(π * x/2) for x in [-2, 2]
fast_inline float32x4_t sine_folder(float32x4_t x) {
    float32x4_t half_pi = vdupq_n_f32(M_PI_2);
    float32x4_t two = vdupq_n_f32(2.0f);
    
    // Clamp input to [-2, 2]
    x = vmaxq_f32(vminq_f32(x, two), vnegq_f32(two));
    
    // Scale to [-π/2, π/2]
    float32x4_t angle = vmulq_f32(x, half_pi);
    
    // Use fast sine approximation
    float32x4_t x2 = vmulq_f32(angle, angle);
    float32x4_t x3 = vmulq_f32(angle, x2);
    
    // sin(x) ≈ x - x^3/6
    return vsubq_f32(angle, vmulq_f32(x3, vdupq_n_f32(1.0f/6.0f)));
}

// Process 4 stereo samples through wavefolder
fast_inline float32x4x2_t wavefolder_process_4(wavefolder_t* wf,
                                                float32x4_t in_l,
                                                float32x4_t in_r,
                                                float drive) {
    float32x4x2_t result;
    float32x4_t drive_vec = vdupq_n_f32(drive * 4.0f);  // Scale drive
    
    // Apply drive gain
    float32x4_t driven_l = vmulq_f32(in_l, drive_vec);
    float32x4_t driven_r = vmulq_f32(in_r, drive_vec);
    
    switch (wf->mode) {
        case 0:  // Soft clip (tanh)
            result.val[0] = fast_tanh(driven_l);
            result.val[1] = fast_tanh(driven_r);
            break;
            
        case 1:  // Hard clip
            result.val[0] = vmaxq_f32(vminq_f32(driven_l, vdupq_n_f32(1.0f)),
                                       vdupq_n_f32(-1.0f));
            result.val[1] = vmaxq_f32(vminq_f32(driven_r, vdupq_n_f32(1.0f)),
                                       vdupq_n_f32(-1.0f));
            break;
            
        case 2:  // Triangle folder
            result.val[0] = triangle_folder(driven_l);
            result.val[1] = triangle_folder(driven_r);
            break;
            
        case 3:  // Sine folder
            result.val[0] = sine_folder(driven_l);
            result.val[1] = sine_folder(driven_r);
            break;
            
        default:
            result.val[0] = driven_l;
            result.val[1] = driven_r;
    }
    
    // Blend dry/wet based on drive (subtle at low drive)
    float32x4_t wet_mix = vdupq_n_f32(drive);
    float32x4_t dry_mix = vsubq_f32(vdupq_n_f32(1.0f), wet_mix);
    
    result.val[0] = vaddq_f32(vmulq_f32(in_l, dry_mix),
                              vmulq_f32(result.val[0], wet_mix));
    result.val[1] = vaddq_f32(vmulq_f32(in_r, dry_mix),
                              vmulq_f32(result.val[1], wet_mix));
    
    return result;
}