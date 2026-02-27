#pragma once
#include <cmath>
#include "float_math.h" // For our fast math approximations

/**
 * Fast Chamberlin State Variable Filter (SVF)
 * Provides 12dB/octave Lowpass, Highpass, and Bandpass outputs.
 */
struct FastSVF {
    // Filter State (Memory)
    float lp = 0.0f;
    float bp = 0.0f;
    float hp = 0.0f;

    // Fast-math coefficients (Calculated in UI thread)
    float f = 0.0f; // Cutoff coefficient
    float q = 0.0f; // Resonance coefficient

    // Mode: 0 = Lowpass, 1 = Bandpass, 2 = Highpass
    int mode = 0;

    // Called by the UI Thread (setParameter) to keep the Audio Thread fast
    inline void set_coeffs(float cutoff_hz, float resonance, float srate) {
        // Clamp cutoff to prevent SVF explosion near Nyquist (srate / 2)
        float safe_cutoff = fminf(cutoff_hz, srate * 0.45f);

        // Chamberlin tuning formula: f = 2 * sin(pi * cutoff / srate)
        // We use our float_math.h fast approximation for speed
        f = 2.0f * fastercosfullf(0.25f - (safe_cutoff / srate)); // cos(0.25 - x) == sin(x*pi) approx

        // Resonance (Q). Lower value = higher resonance peak.
        // Clamp resonance between 1.0 (no peak) and 10.0 (self-oscillation boundary)
        float safe_res = fmaxf(1.0f, fminf(resonance, 10.0f));
        q = 1.0f / safe_res;
    }

    // Called by the Audio Thread (processBlock)
    inline float process(float input) {
        // The core SVF math (requires zero division or trig!)
        hp = input - lp - (q * bp);
        bp += f * hp;
        lp += f * bp;

        if (mode == 0) return lp;
        if (mode == 1) return bp;
        return hp;
    }
};