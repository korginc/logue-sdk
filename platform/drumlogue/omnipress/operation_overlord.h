// New file: operation_overlord.h

#pragma once
/**
 * @file operation_overlord.h
 * @brief EHX Operation Overlord drum machine overdrive emulation
 *
 * Features:
 * - Tube-like asymmetric clipping
 * - Bass/Treble EQ (pre-drive)
 * - Blend control (parallel drive)
 * - Presence control (post-drive high-end)
 */

#include <arm_neon.h>
#include <filters.h>
#include <cmath>

typedef struct {
    // Tube emulation stages
    float32x4_t tube_state;      // First tube stage
    float32x4_t tube_state2;     // Second tube stage

    // EQ filter states — separate per channel (L/R biquad histories must not be shared)
    biquad_state_t bass_boost_l;
    biquad_state_t treble_boost_l;
    biquad_state_t presence_l;
    biquad_state_t bass_boost_r;
    biquad_state_t treble_boost_r;
    biquad_state_t presence_r;

    // Blend
    float32x4_t dry_wet;

    // Parameters
    float drive;          // 0-100%
    float bass;           // 0-100% (-12 to +12 dB)
    float treble;         // 0-100% (-12 to +12 dB)
    float blend;          // 0-100% (dry/wet)
    float presence;       // 0-100% (high-end sparkle)
} overlord_t;

// Initialize Operation Overlord emulation
fast_inline void overlord_init(overlord_t* ov, float sample_rate) {
    ov->drive = 0.0f;
    ov->bass = 0.5f;
    ov->treble = 0.5f;
    ov->blend = 1.0f;
    ov->presence = 0.5f;

    ov->tube_state = vdupq_n_f32(0.0f);
    ov->tube_state2 = vdupq_n_f32(0.0f);

    // Initialize EQ filter states (separate for L and R channels)
    biquad_init_state(&ov->bass_boost_l);
    biquad_init_state(&ov->treble_boost_l);
    biquad_init_state(&ov->presence_l);
    biquad_init_state(&ov->bass_boost_r);
    biquad_init_state(&ov->treble_boost_r);
    biquad_init_state(&ov->presence_r);
}

/**
 * Set drive amount (0-100%)
 */
fast_inline void overlord_set_drive(overlord_t* ov, float drive_percent) {
    float drive = drive_percent * 0.01f;
    ov->drive = drive;
}

// Tube saturation stage (asymmetric clipping)
fast_inline float32x4_t tube_saturate(float32x4_t in, float drive) {
    // Asymmetric clipping: positive side clips later than negative
    // Simulates 12AX7 tube characteristics

    float32x4_t pos_clip = vdupq_n_f32(0.8f);
    float32x4_t neg_clip = vdupq_n_f32(0.6f);
    float32x4_t one = vdupq_n_f32(1.0f);

    // Apply drive gain
    float32x4_t driven = vmulq_f32(in, vaddq_f32(one, vmulq_f32(vdupq_n_f32(drive), vdupq_n_f32(3.0f))));

    // Asymmetric clipping
    uint32x4_t pos = vcgtq_f32(driven, pos_clip);
    uint32x4_t neg = vcltq_f32(driven, vnegq_f32(neg_clip));

    float32x4_t clipped = driven;
    clipped = vbslq_f32(pos, pos_clip, clipped);
    clipped = vbslq_f32(neg, vnegq_f32(neg_clip), clipped);

    // Soft knee: quadratic roll-off from 0.9*pos_clip to pos_clip.
    // t=0 at knee start (0.72), t=1 at clip level (0.80).
    // output = knee_start + knee_width * t*(2-t)  → smooth S from 0.72 to 0.80.
    // The old 10:1 linear slope amplified in the knee region instead of compressing.
    uint32x4_t knee_region = vandq_u32(
        vcgtq_f32(driven, vmulq_f32(pos_clip, vdupq_n_f32(0.9f))),
        vcltq_f32(driven, pos_clip)
    );
    float32x4_t knee_start = vmulq_f32(pos_clip, vdupq_n_f32(0.9f));
    float32x4_t knee_width = vsubq_f32(pos_clip, knee_start);
    float32x4_t t = fast_div_neon(vsubq_f32(driven, knee_start), knee_width);
    float32x4_t knee_out = vaddq_f32(knee_start,
        vmulq_f32(knee_width, vmulq_f32(t, vsubq_f32(vdupq_n_f32(2.0f), t))));
    clipped = vbslq_f32(knee_region, knee_out, clipped);

    return clipped;
}

// Baxandall bass/treble EQ: low shelf into high shelf in series
fast_inline float32x4_t baxandall_eq(float32x4_t in,
                                     biquad_state_t* bass_state,
                                     biquad_state_t* treble_state,
                                     float bass,
                                     float treble,
                                     float sample_rate) {
    float bass_gain = -12.0f + bass * 24.0f;    // -12 to +12 dB
    float treble_gain = -12.0f + treble * 24.0f;

    // Low shelf at 100 Hz — bass cut/boost
    float32x4_t bass_shelf = low_shelf_filter(in, bass_state, 100.0f, bass_gain, 1.0f, sample_rate);

    // High shelf at 10 kHz — treble cut/boost (fed from bass shelf output)
    float32x4_t treble_shelf = high_shelf_filter(bass_shelf, treble_state, 10000.0f, treble_gain, 0.0f, sample_rate);

    return treble_shelf;
}

// EQ-only path (no tube saturation) — for use in distressor mode
// Applies bass/treble Baxandall EQ and presence shelf without adding drive distortion
fast_inline float32x4x2_t overlord_apply_eq(overlord_t* ov,
                                             float32x4_t in_l,
                                             float32x4_t in_r,
                                             float sample_rate) {
    float active_threshold = 0.01f;
    if (fabsf(ov->bass - 0.5f) < active_threshold &&
        fabsf(ov->treble - 0.5f) < active_threshold &&
        fabsf(ov->presence - 0.5f) < active_threshold) {
        float32x4x2_t bypass;
        bypass.val[0] = in_l;
        bypass.val[1] = in_r;
        return bypass;
    }

    float32x4_t eq_l = baxandall_eq(in_l, &ov->bass_boost_l, &ov->treble_boost_l, ov->bass, ov->treble, sample_rate);
    float32x4_t eq_r = baxandall_eq(in_r, &ov->bass_boost_r, &ov->treble_boost_r, ov->bass, ov->treble, sample_rate);

    // presence=0.5 → flat (0 dB), 0.0 → −12 dB, 1.0 → +12 dB
    float presence_gain = (ov->presence - 0.5f) * 24.0f;
    float32x4_t out_l = high_shelf_filter(eq_l, &ov->presence_l, 5000.0f,
                                           presence_gain, 1.0f, sample_rate);
    float32x4_t out_r = high_shelf_filter(eq_r, &ov->presence_r, 5000.0f,
                                           presence_gain, 1.0f, sample_rate);

    float32x4x2_t out;
    out.val[0] = out_l;
    out.val[1] = out_r;
    return out;
}

// Main Overlord processing
fast_inline float32x4x2_t overlord_process(overlord_t* ov,
                                           float32x4_t in_l,
                                           float32x4_t in_r,
                                           float sample_rate) {
    float active_threshold = 0.01f;
    if (ov->drive < active_threshold &&
        fabsf(ov->bass - 0.5f) < active_threshold &&  // 0.5 = flat
        fabsf(ov->treble - 0.5f) < active_threshold &&
        fabsf(ov->presence - 0.5f) < active_threshold) {
        // Bypass - return dry signal
        float32x4x2_t bypass;
        bypass.val[0] = in_l;
        bypass.val[1] = in_r;
        return bypass;
    }

    // Save dry signal for blend
    float32x4_t dry_l = in_l;
    float32x4_t dry_r = in_r;

    // 1. Tube saturation (dual stage)
    float32x4_t tube1_l = tube_saturate(dry_l, ov->drive);
    float32x4_t tube1_r = tube_saturate(dry_r, ov->drive);

    float32x4_t tube2_l = tube_saturate(tube1_l, ov->drive * 0.7f);
    float32x4_t tube2_r = tube_saturate(tube1_r, ov->drive * 0.7f);

    // 2. Post-drive EQ (bass/treble boost/cut) - after distortion to avoid boost going to consume all the saturation headroom
    float32x4_t eq_l =
        baxandall_eq(tube2_l, &ov->bass_boost_l, &ov->treble_boost_l, ov->bass,
                     ov->treble, sample_rate);
    float32x4_t eq_r =
        baxandall_eq(tube2_r, &ov->bass_boost_r, &ov->treble_boost_r, ov->bass,
                     ov->treble, sample_rate);

    // 3. Presence control (high shelf ±12 dB, flat at presence=0.5)
    float presence_gain = (ov->presence - 0.5f) * 24.0f;
    float32x4_t presence_l = high_shelf_filter(
        eq_l, &ov->presence_l, 5000.0f, presence_gain, 1.0f, sample_rate);
    float32x4_t presence_r = high_shelf_filter(
        eq_r, &ov->presence_r, 5000.0f, presence_gain, 1.0f, sample_rate);

    // 4. Blend (parallel processing)
    float32x4_t wet_gain = vdupq_n_f32(ov->blend);
    float32x4_t dry_gain = vdupq_n_f32(1.0f - ov->blend);

    float32x4x2_t out;
    out.val[0] = vaddq_f32(vmulq_f32(dry_l, dry_gain), vmulq_f32(presence_l, wet_gain));
    out.val[1] = vaddq_f32(vmulq_f32(dry_r, dry_gain), vmulq_f32(presence_r, wet_gain));

    return out;
}