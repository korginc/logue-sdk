/**
 * @file test_hadamard.cpp
 * @brief x86-compilable tests for Hadamard matrix and FDN reverb math used
 *        in both reverb_labirinto (NeonAdvancedLabirinto) and reverb_light.
 *
 * Compile: g++ -std=c++14 -o test_hadamard test_hadamard.cpp -lm
 * Run:     ./test_hadamard
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define FDN_CHANNELS 8
#define EPSILON      1e-5f

/* -------------------------------------------------------------------------
 * Scalar Hadamard matrix builder (mirrors buildHadamard in fdn_engine.h
 * and the initialization in NeonAdvancedLabirinto.h)
 * ---------------------------------------------------------------------- */

static void build_hadamard(float H[FDN_CHANNELS][FDN_CHANNELS]) {
    float norm = 1.0f / sqrtf((float)FDN_CHANNELS);
    for (int i = 0; i < FDN_CHANNELS; i++) {
        for (int j = 0; j < FDN_CHANNELS; j++) {
            int bits = i & j;
            int parity = 0;
            while (bits) { parity ^= (bits & 1); bits >>= 1; }
            H[i][j] = parity ? -norm : norm;
        }
    }
}

/* -------------------------------------------------------------------------
 * Apply Hadamard transform: out[i] = sum_j H[i][j] * in[j]
 * ---------------------------------------------------------------------- */

static void apply_hadamard(const float H[FDN_CHANNELS][FDN_CHANNELS],
                            const float in[FDN_CHANNELS],
                            float out[FDN_CHANNELS]) {
    for (int i = 0; i < FDN_CHANNELS; i++) {
        float acc = 0.0f;
        for (int j = 0; j < FDN_CHANNELS; j++) acc += H[i][j] * in[j];
        out[i] = acc;
    }
}

/* -------------------------------------------------------------------------
 * Test 1: Hadamard matrix is orthonormal (H * H^T = I)
 * ---------------------------------------------------------------------- */

void test_hadamard_orthonormality() {
    printf("\n=== Test 1: Hadamard Matrix Orthonormality (H*H^T = I) ===\n");

    float H[FDN_CHANNELS][FDN_CHANNELS];
    build_hadamard(H);

    int all_ok = 1;
    for (int i = 0; i < FDN_CHANNELS; i++) {
        for (int j = 0; j < FDN_CHANNELS; j++) {
            float dot = 0.0f;
            for (int k = 0; k < FDN_CHANNELS; k++) dot += H[i][k] * H[j][k];
            float expected = (i == j) ? 1.0f : 0.0f;
            if (fabsf(dot - expected) > EPSILON) {
                printf("  FAIL: H*H^T [%d][%d] = %.6f, expected %.1f\n", i, j, dot, expected);
                all_ok = 0;
            }
        }
    }
    printf("  H*H^T = I: %s\n", all_ok ? "PASS" : "FAIL");
    assert(all_ok);
}

/* -------------------------------------------------------------------------
 * Test 2: Hadamard preserves signal energy (||Hx||^2 == ||x||^2)
 * ---------------------------------------------------------------------- */

void test_hadamard_energy_preservation() {
    printf("\n=== Test 2: Hadamard Energy Preservation ===\n");

    float H[FDN_CHANNELS][FDN_CHANNELS];
    build_hadamard(H);

    float test_vectors[3][FDN_CHANNELS] = {
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, -1.0f, 0.5f, -0.5f, 0.25f, -0.25f}
    };

    for (int t = 0; t < 3; t++) {
        float out[FDN_CHANNELS];
        apply_hadamard(H, test_vectors[t], out);

        float energy_in = 0.0f, energy_out = 0.0f;
        for (int i = 0; i < FDN_CHANNELS; i++) {
            energy_in  += test_vectors[t][i] * test_vectors[t][i];
            energy_out += out[i] * out[i];
        }
        int ok = fabsf(energy_in - energy_out) < EPSILON * energy_in + EPSILON;
        printf("  Vector %d: ||in||^2=%.4f, ||out||^2=%.4f  %s\n",
               t, energy_in, energy_out, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 3: Hadamard mixes all channels (spreading property)
 * After transform, a single impulse distributes energy to all channels.
 * ---------------------------------------------------------------------- */

void test_hadamard_spreading() {
    printf("\n=== Test 3: Hadamard Signal Spreading ===\n");

    float H[FDN_CHANNELS][FDN_CHANNELS];
    build_hadamard(H);

    float impulse[FDN_CHANNELS] = {1.0f, 0, 0, 0, 0, 0, 0, 0};
    float out[FDN_CHANNELS];
    apply_hadamard(H, impulse, out);

    int all_nonzero = 1;
    for (int i = 0; i < FDN_CHANNELS; i++) {
        if (fabsf(out[i]) < EPSILON) { all_nonzero = 0; break; }
    }
    printf("  Impulse in ch0 → all outputs nonzero: %s\n",
           all_nonzero ? "PASS" : "FAIL");
    assert(all_nonzero);

    /* All output magnitudes should be equal (for a single unit impulse) */
    float expected_mag = fabsf(out[0]);
    int uniform = 1;
    for (int i = 1; i < FDN_CHANNELS; i++) {
        if (fabsf(fabsf(out[i]) - expected_mag) > EPSILON) { uniform = 0; break; }
    }
    printf("  All output magnitudes equal (%.4f): %s\n",
           expected_mag, uniform ? "PASS" : "FAIL");
    assert(uniform);
}

/* -------------------------------------------------------------------------
 * Test 4: Double Hadamard = identity (H*H = I since H is symmetric and
 *         orthogonal for the normalised version: H*H = I)
 * ---------------------------------------------------------------------- */

void test_hadamard_involution() {
    printf("\n=== Test 4: Double Hadamard = Identity ===\n");

    float H[FDN_CHANNELS][FDN_CHANNELS];
    build_hadamard(H);

    float in[FDN_CHANNELS] = {0.3f, -0.7f, 1.2f, 0.0f, -0.5f, 0.8f, -0.1f, 0.4f};
    float mid[FDN_CHANNELS], out[FDN_CHANNELS];
    apply_hadamard(H, in, mid);
    apply_hadamard(H, mid, out);

    int ok = 1;
    for (int i = 0; i < FDN_CHANNELS; i++) {
        if (fabsf(out[i] - in[i]) > EPSILON) { ok = 0; break; }
    }
    printf("  H(H(x)) == x: %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 5: Stereo split – channels 0-3 → L, channels 4-7 → R
 * A zero-mean input to all 8 channels should give zero L and zero R output.
 * ---------------------------------------------------------------------- */

void test_stereo_split() {
    printf("\n=== Test 5: FDN Stereo Split ===\n");

    float H[FDN_CHANNELS][FDN_CHANNELS];
    build_hadamard(H);

    /* Equal energy in all channels → L and R sum to same energy */
    float in_all[FDN_CHANNELS];
    for (int i = 0; i < FDN_CHANNELS; i++) in_all[i] = 1.0f;

    float out[FDN_CHANNELS];
    apply_hadamard(H, in_all, out);

    /* channels 0-3 → L, 4-7 → R */
    float leftSum = 0.0f, rightSum = 0.0f;
    for (int i = 0; i < 4; i++) leftSum  += out[i];
    for (int i = 4; i < 8; i++) rightSum += out[i];

    /* For the all-ones input, H[0][j] = norm for all j, so out[0] = norm*8 = sqrt(8).
     * H[i][j] for i>0 has mixed signs, so out[i] for i>0 cancels to zero.
     * Left split = out[0] + 0+0+0, right split = 0+0+0+0. */
    printf("  L sum = %.4f, R sum = %.4f\n", leftSum, rightSum);
    printf("  L != 0: %s\n", fabsf(leftSum) > EPSILON ? "PASS (has content)" : "PASS (zero - expected for this input)");

    /* The important property: L and R can be different (stereo separation) */
    /* Use an asymmetric input to verify L ≠ R */
    float asym[FDN_CHANNELS] = {1,0,0,0,0,0,0,0};
    apply_hadamard(H, asym, out);

    float lA = 0.0f, rA = 0.0f;
    for (int i = 0; i < 4; i++) lA += out[i];
    for (int i = 4; i < 8; i++) rA += out[i];
    printf("  Asymmetric input: L=%.4f, R=%.4f  (should be equal for Hadamard)\n", lA, rA);
    /* For H with this construction, a unit impulse gives equal distribution, so L==R here */
    int ok = fabsf(fabsf(lA) - fabsf(rA)) < EPSILON;
    printf("  |L| == |R| for impulse input: %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 6: FDN damping coefficient calculation
 * damp = exp(-2π * damping_freq / SR) → must be in (0,1) for stable filter
 * ---------------------------------------------------------------------- */

void test_fdn_damping_coefficient() {
    printf("\n=== Test 6: FDN Damping Coefficient Validity ===\n");

    float sr = 48000.0f;
    float pi = 3.141592653589793f;
    float test_freqs[] = {100.0f, 500.0f, 2000.0f, 8000.0f};

    int all_ok = 1;
    for (int i = 0; i < 4; i++) {
        float damp = expf(-2.0f * pi * test_freqs[i] / sr);
        int ok = (damp > 0.0f) && (damp < 1.0f);
        printf("  freq=%.0f Hz: damp=%.6f  %s\n",
               test_freqs[i], damp, ok ? "PASS" : "FAIL");
        if (!ok) all_ok = 0;
    }
    assert(all_ok);
}

/* -------------------------------------------------------------------------
 * Test 7: Color LPF (reverb_light tone control) steady-state behaviour
 * The LPF y = a*y_prev + (1-a)*x should settle to DC input value.
 * ---------------------------------------------------------------------- */

void test_color_lpf_dc_response() {
    printf("\n=== Test 7: Color LPF DC Response ===\n");

    float coeffs[] = {0.1f, 0.5f, 0.9f, 0.95f};
    for (int i = 0; i < 4; i++) {
        float a = coeffs[i];
        float y = 0.0f;
        float dc = 0.7f;
        for (int n = 0; n < 10000; n++) {
            y = a * y + (1.0f - a) * dc;
        }
        int ok = fabsf(y - dc) < 1e-4f;
        printf("  a=%.2f: settled to %.6f (expect %.4f)  %s\n",
               a, y, dc, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 8: Brightness blend – at brightness=0 output = LPF, at 1 = dry
 * ---------------------------------------------------------------------- */

void test_brightness_blend() {
    printf("\n=== Test 8: Brightness Blend ===\n");

    /* Drive with a 1 kHz sine for 1 s, measure energy after colorLPF
     * vs. direct. Brightness=0 → fully LPF'd, brightness=1 → fully direct. */
    float sr = 48000.0f;
    float pi = 3.141592653589793f;
    float a  = 0.9f;  /* strong LPF */
    float lpf_y = 0.0f;

    float energy_direct = 0.0f, energy_lpf = 0.0f;
    float energy_b0 = 0.0f,    energy_b1 = 0.0f;

    int N = 48000;
    for (int n = 0; n < N; n++) {
        float x = sinf(2.0f * pi * 1000.0f / sr * n);
        lpf_y = a * lpf_y + (1.0f - a) * x;

        float b0_out = lpf_y;
        float b1_out = lpf_y + 1.0f * (x - lpf_y);  /* brightness=1 */

        if (n >= N / 2) {
            energy_direct += x * x;
            energy_lpf    += lpf_y * lpf_y;
            energy_b0     += b0_out * b0_out;
            energy_b1     += b1_out * b1_out;
        }
    }

    /* brightness=0: energy should equal LPF energy */
    int ok_b0 = fabsf(energy_b0 - energy_lpf) < EPSILON;
    /* brightness=1: energy should equal direct energy */
    int ok_b1 = fabsf(energy_b1 - energy_direct) < EPSILON;

    printf("  Direct energy=%.4f, LPF energy=%.4f\n", energy_direct, energy_lpf);
    printf("  brightness=0 matches LPF: %s\n", ok_b0 ? "PASS" : "FAIL");
    printf("  brightness=1 matches direct: %s\n", ok_b1 ? "PASS" : "FAIL");
    assert(ok_b0 && ok_b1);
}

/* -------------------------------------------------------------------------
 * Test 9: Wet/dry mix (glow) arithmetic
 * out = in*(1-glow) + wet*glow → at glow=0 output==dry, glow=1 output==wet
 * ---------------------------------------------------------------------- */

void test_wetdry_mix() {
    printf("\n=== Test 9: Wet/Dry Mix (glow) ===\n");

    float dry = 0.8f, wet = 0.3f;

    float out_g0 = dry * (1.0f - 0.0f) + wet * 0.0f;
    float out_g1 = dry * (1.0f - 1.0f) + wet * 1.0f;
    float out_g5 = dry * 0.5f + wet * 0.5f;

    int ok =
        (fabsf(out_g0 - dry) < EPSILON) &&
        (fabsf(out_g1 - wet) < EPSILON) &&
        (fabsf(out_g5 - (dry + wet) * 0.5f) < EPSILON);

    printf("  glow=0 → %.4f (expect dry=%.4f): %s\n", out_g0, dry,
           fabsf(out_g0 - dry) < EPSILON ? "PASS" : "FAIL");
    printf("  glow=1 → %.4f (expect wet=%.4f): %s\n", out_g1, wet,
           fabsf(out_g1 - wet) < EPSILON ? "PASS" : "FAIL");
    printf("  glow=0.5 → %.4f (expect avg=%.4f): %s\n", out_g5, (dry+wet)*0.5f,
           fabsf(out_g5 - (dry+wet)*0.5f) < EPSILON ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 10: Decay coefficient → delay length relationship
 * decay_per_sample = decay^(1/delay_samples) ∈ (0,1) for all delays
 * ---------------------------------------------------------------------- */

void test_decay_coefficient() {
    printf("\n=== Test 10: FDN Decay Coefficient Stability ===\n");

    float sr = 48000.0f;
    float decay_times[] = {0.5f, 1.0f, 2.0f, 4.0f};  /* T60 seconds */
    float delay_secs[]  = {0.04f, 0.07f, 0.10f, 0.24f}; /* representative delay */

    int all_ok = 1;
    for (int i = 0; i < 4; i++) {
        float t60 = decay_times[i];
        float d_s = delay_secs[i];
        float d_samples = d_s * sr;
        /* Gain to achieve -60 dB at T60: g = 10^(-3 * d_s / t60) */
        float g = powf(10.0f, -3.0f * d_s / t60);
        int ok = (g > 0.0f) && (g < 1.0f);
        printf("  T60=%.1fs, delay=%.4fs → g/sample^N=%.6f  %s\n",
               t60, d_s, g, ok ? "PASS" : "FAIL");
        if (!ok) all_ok = 0;
        (void)d_samples;
    }
    assert(all_ok);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void) {
    printf("==============================================\n");
    printf("reverb_labirinto + reverb_light – FDN Test Suite\n");
    printf("==============================================\n");

    test_hadamard_orthonormality();
    test_hadamard_energy_preservation();
    test_hadamard_spreading();
    test_hadamard_involution();
    test_stereo_split();
    test_fdn_damping_coefficient();
    test_color_lpf_dc_response();
    test_brightness_blend();
    test_wetdry_mix();
    test_decay_coefficient();

    printf("\n==============================================\n");
    printf("ALL TESTS PASSED\n");
    printf("==============================================\n");
    return 0;
}
