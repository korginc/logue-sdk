#pragma once
#include <cmath>
#include "float_math.h"

struct MorphingFilter {
    int mode = 0; // 0 = LP, 1 = BP, 2 = HP
    float f = 0.0f, q = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    // Morphing Parameters
    float drive = 0.0f;           // 0.0 (Clean) to 5.0 (Screaming)
    float sherman_asym = 0.0f;    // 0.0 (Symmetrical) to 1.0 (Asymmetrical)
    float lfo_res_mod = 0.0f;     // How much LFO3 rips the resonance apart

    inline void set_coeffs(float cutoff_hz, float resonance, float srate) {
        f = 2.0f * sinf(3.14159265f * cutoff_hz / srate);
        q = 1.0f / resonance;
    }

    inline float process(float in, float lfo3_val) {
        // 1. Sherman Asymmetry (Pushes the wave off-center before clipping)
        float driven_in = in * (1.0f + drive);
        driven_in += sherman_asym * driven_in * driven_in * 0.15f;

        // 2. Input Soft-Clipping (Moog-style symmetrical saturation)
        driven_in = driven_in / (1.0f + fabsf(driven_in));

        // 3. Sherman Resonance Destabilization (LFO3 modulates the Q factor)
        float current_q = q - (lfo_res_mod * lfo3_val * 0.8f);
        if (current_q < 0.05f) current_q = 0.05f; // Prevent total math explosion

        // 4. State Variable Filter with Non-Linear Integrators
        float hp = driven_in - z1 * current_q - z2;

        // The integrators (v1, v2) get saturated inside the loop based on drive
        float v1 = hp * f;
        z1 += v1 / (1.0f + fabsf(v1 * drive * 0.25f));

        float v2 = z1 * f;
        z2 += v2 / (1.0f + fabsf(v2 * drive * 0.25f));

        if (mode == 0) return z2; // LP
        if (mode == 1) return z1; // BP
        return hp; // HP
    }
};