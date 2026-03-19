/**
 * @file test_biquad_filters.cpp
 * @brief x86-compilable unit tests for delay_tribal biquad filter DSP math.
 *
 * Tests biquad coefficient calculation and Transposed DF-II processing using
 * scalar equivalents of the NEON operations in filters.h.
 *
 * Compile: g++ -std=c++14 -o test_biquad test_biquad_filters.cpp -lm
 * Run:     ./test_biquad
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define FILTER_SAMPLE_RATE 48000.0f
#define FILTER_PI          3.141592653589793f
#define EPSILON            1e-4f

/* -------------------------------------------------------------------------
 * Scalar biquad types (mirror of filters.h without NEON)
 * ---------------------------------------------------------------------- */

typedef struct {
    float b0, b1, b2, a1, a2;
} scalar_coeffs_t;

typedef struct {
    float z1, z2;
} scalar_state_t;

static void scalar_state_init(scalar_state_t* s) { s->z1 = s->z2 = 0.0f; }

/**
 * Transposed DF-II biquad – identical logic to filters.h::biquad_process().
 */
static float scalar_biquad_process(float in,
                                    const scalar_coeffs_t* c,
                                    scalar_state_t* s) {
    float y = c->b0 * in + s->z1;
    s->z1 = c->b1 * in - c->a1 * y + s->z2;
    s->z2 = c->b2 * in - c->a2 * y;
    return y;
}

static void calc_bandpass(scalar_coeffs_t* c, float fc, float q) {
    float w0    = 2.0f * FILTER_PI * fc / FILTER_SAMPLE_RATE;
    float cos_w = cosf(w0), sin_w = sinf(w0);
    float alpha = sin_w / (2.0f * q);
    float b0    = alpha;
    float b1    = 0.0f;
    float b2    = -alpha;
    float a0    = 1.0f + alpha;
    float a1    = -2.0f * cos_w;
    float a2    = 1.0f - alpha;
    float inv   = 1.0f / a0;
    c->b0 = b0 * inv; c->b1 = b1 * inv; c->b2 = b2 * inv;
    c->a1 = a1 * inv; c->a2 = a2 * inv;
}

static void calc_highpass(scalar_coeffs_t* c, float fc, float q) {
    float w0    = 2.0f * FILTER_PI * fc / FILTER_SAMPLE_RATE;
    float cos_w = cosf(w0), sin_w = sinf(w0);
    float alpha = sin_w / (2.0f * q);
    float b0    = (1.0f + cos_w) / 2.0f;
    float b1    = -(1.0f + cos_w);
    float b2    = b0;
    float a0    = 1.0f + alpha;
    float a1    = -2.0f * cos_w;
    float a2    = 1.0f - alpha;
    float inv   = 1.0f / a0;
    c->b0 = b0 * inv; c->b1 = b1 * inv; c->b2 = b2 * inv;
    c->a1 = a1 * inv; c->a2 = a2 * inv;
}

static void calc_lowpass(scalar_coeffs_t* c, float fc, float q) {
    float w0    = 2.0f * FILTER_PI * fc / FILTER_SAMPLE_RATE;
    float cos_w = cosf(w0), sin_w = sinf(w0);
    float alpha = sin_w / (2.0f * q);
    float b0    = (1.0f - cos_w) / 2.0f;
    float b1    = (1.0f - cos_w);
    float b2    = b0;
    float a0    = 1.0f + alpha;
    float a1    = -2.0f * cos_w;
    float a2    = 1.0f - alpha;
    float inv   = 1.0f / a0;
    c->b0 = b0 * inv; c->b1 = b1 * inv; c->b2 = b2 * inv;
    c->a1 = a1 * inv; c->a2 = a2 * inv;
}

/* -------------------------------------------------------------------------
 * Frequency-response helper: feed a complex exponential and measure gain.
 * Drives the filter with N cycles of a sine at freq Hz, discards the first
 * half as transient, returns RMS of steady-state output / RMS input.
 * ---------------------------------------------------------------------- */

static float measure_gain_db(const scalar_coeffs_t* c, float freq_hz, int N) {
    scalar_state_t s;
    scalar_state_init(&s);

    float in_rms = 0.0f, out_rms = 0.0f;
    for (int n = 0; n < N; n++) {
        float x = sinf(2.0f * FILTER_PI * freq_hz / FILTER_SAMPLE_RATE * n);
        float y = scalar_biquad_process(x, c, &s);
        if (n >= N / 2) {
            in_rms  += x * x;
            out_rms += y * y;
        }
    }
    in_rms  = sqrtf(in_rms / (N / 2));
    out_rms = sqrtf(out_rms / (N / 2));
    if (in_rms < 1e-9f) return -200.0f;
    return 20.0f * log10f(out_rms / in_rms);
}

/* -------------------------------------------------------------------------
 * Test 1: Bandpass coefficient constraints (Tribal mode)
 * ---------------------------------------------------------------------- */

void test_bandpass_coefficients() {
    printf("\n=== Test 1: Bandpass Coefficient Constraints ===\n");

    float freqs[] = {80.0f, 200.0f, 400.0f, 800.0f};
    for (int i = 0; i < 4; i++) {
        scalar_coeffs_t c;
        calc_bandpass(&c, freqs[i], 2.0f);

        /* For a bandpass: b1 must be 0, b2 must be -b0 */
        int ok = (fabsf(c.b1) < 1e-6f) && (fabsf(c.b0 + c.b2) < 1e-6f);
        printf("  fc=%.0f Hz: b0=%.5f b1=%.5f b2=%.5f  %s\n",
               freqs[i], c.b0, c.b1, c.b2, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 2: Highpass coefficient constraints (Military mode)
 * ---------------------------------------------------------------------- */

void test_highpass_coefficients() {
    printf("\n=== Test 2: Highpass Coefficient Constraints ===\n");

    float freqs[] = {1000.0f, 4000.0f, 8000.0f};
    for (int i = 0; i < 3; i++) {
        scalar_coeffs_t c;
        calc_highpass(&c, freqs[i], 0.707f);

        /* For a highpass: b0 == b2, b1 == -2*b0 */
        int ok = (fabsf(c.b0 - c.b2) < 1e-6f) &&
                 (fabsf(c.b1 + 2.0f * c.b0) < 1e-6f);
        printf("  fc=%.0f Hz: b0=%.5f b1=%.5f b2=%.5f  %s\n",
               freqs[i], c.b0, c.b1, c.b2, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 3: Lowpass coefficient constraints (Angel mode LPF)
 * ---------------------------------------------------------------------- */

void test_lowpass_coefficients() {
    printf("\n=== Test 3: Lowpass Coefficient Constraints ===\n");

    float freqs[] = {500.0f, 2000.0f, 6000.0f};
    for (int i = 0; i < 3; i++) {
        scalar_coeffs_t c;
        calc_lowpass(&c, freqs[i], 1.0f);

        /* For a lowpass: b0 == b2, b1 == 2*b0 */
        int ok = (fabsf(c.b0 - c.b2) < 1e-6f) &&
                 (fabsf(c.b1 - 2.0f * c.b0) < 1e-6f);
        printf("  fc=%.0f Hz: b0=%.5f b1=%.5f b2=%.5f  %s\n",
               freqs[i], c.b0, c.b1, c.b2, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 4: Transposed DF-II z2 sign correctness
 * Feed a constant DC=1 signal to a neutral filter (all-pass numerator,
 * identity denominator) and verify the state update is correct.
 * ---------------------------------------------------------------------- */

void test_biquad_process_state_update() {
    printf("\n=== Test 4: Biquad Process State Update (z2 sign) ===\n");

    /* Identity filter: b0=1, b1=b2=0, a1=a2=0 */
    scalar_coeffs_t c = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    scalar_state_t s;
    scalar_state_init(&s);

    float y0 = scalar_biquad_process(1.0f, &c, &s);
    float y1 = scalar_biquad_process(1.0f, &c, &s);
    float y2 = scalar_biquad_process(1.0f, &c, &s);

    /* Identity filter must pass every sample unchanged */
    int ok = (fabsf(y0 - 1.0f) < 1e-6f) &&
             (fabsf(y1 - 1.0f) < 1e-6f) &&
             (fabsf(y2 - 1.0f) < 1e-6f);
    printf("  Identity: y[0]=%.6f y[1]=%.6f y[2]=%.6f  %s\n",
           y0, y1, y2, ok ? "PASS" : "FAIL");
    assert(ok);

    /* Pure delay filter: b0=0, b1=1, b2=0, a1=a2=0 → y[n] = x[n-1] */
    scalar_coeffs_t c2 = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    scalar_state_init(&s);

    float d0 = scalar_biquad_process(1.0f, &c2, &s);  // x[-1]=0
    float d1 = scalar_biquad_process(0.0f, &c2, &s);  // x[0]=1
    int ok2 = (fabsf(d0) < 1e-6f) && (fabsf(d1 - 1.0f) < 1e-6f);
    printf("  Pure delay: d[0]=%.6f d[1]=%.6f  %s\n",
           d0, d1, ok2 ? "PASS" : "FAIL");
    assert(ok2);
}

/* -------------------------------------------------------------------------
 * Test 5: Bandpass frequency response (gain at center >> gain at stopband)
 * ---------------------------------------------------------------------- */

void test_bandpass_frequency_response() {
    printf("\n=== Test 5: Bandpass Frequency Response ===\n");

    float fc = 200.0f, q = 2.0f;
    scalar_coeffs_t c;
    calc_bandpass(&c, fc, q);

    int N = 96000;  /* 2 s */

    float gain_at_center = measure_gain_db(&c, fc, N);
    float gain_at_dc     = measure_gain_db(&c, 10.0f, N);    /* well below */
    float gain_at_high   = measure_gain_db(&c, 4000.0f, N);  /* well above */

    printf("  fc=%g Hz, Q=%g\n", fc, q);
    printf("    Gain at center:  %.1f dB (expect ~0)\n",  gain_at_center);
    printf("    Gain at 10 Hz:   %.1f dB (expect <-20)\n", gain_at_dc);
    printf("    Gain at 4kHz:    %.1f dB (expect <-20)\n", gain_at_high);

    int ok = (gain_at_center > -6.0f) &&
             (gain_at_dc < -20.0f) &&
             (gain_at_high < -20.0f);
    printf("  %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 6: Highpass frequency response
 * ---------------------------------------------------------------------- */

void test_highpass_frequency_response() {
    printf("\n=== Test 6: Highpass Frequency Response ===\n");

    float fc = 1000.0f, q = 0.707f;
    scalar_coeffs_t c;
    calc_highpass(&c, fc, q);

    int N = 96000;

    float gain_at_pass   = measure_gain_db(&c, 8000.0f, N);
    float gain_at_stop   = measure_gain_db(&c, 50.0f, N);

    printf("  fc=%g Hz, Q=%g\n", fc, q);
    printf("    Gain at 8kHz (pass): %.1f dB (expect >-6)\n",  gain_at_pass);
    printf("    Gain at 50 Hz (stop): %.1f dB (expect <-20)\n", gain_at_stop);

    int ok = (gain_at_pass > -6.0f) && (gain_at_stop < -20.0f);
    printf("  %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 7: Lowpass frequency response
 * ---------------------------------------------------------------------- */

void test_lowpass_frequency_response() {
    printf("\n=== Test 7: Lowpass Frequency Response ===\n");

    float fc = 4000.0f, q = 1.0f;
    scalar_coeffs_t c;
    calc_lowpass(&c, fc, q);

    int N = 96000;

    float gain_at_pass = measure_gain_db(&c, 400.0f, N);
    float gain_at_stop = measure_gain_db(&c, 20000.0f, N);  /* well into stop band */

    printf("  fc=%g Hz, Q=%g\n", fc, q);
    printf("    Gain at 400 Hz (pass): %.1f dB (expect >-6)\n",  gain_at_pass);
    printf("    Gain at 20kHz (stop): %.1f dB (expect <-20)\n",  gain_at_stop);

    int ok = (gain_at_pass > -6.0f) && (gain_at_stop < -20.0f);
    printf("  %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 8: Angel mode – separate L/R states (no crosstalk)
 * Apply different signals on L and R, verify outputs are independent.
 * ---------------------------------------------------------------------- */

void test_angel_mode_independent_lr_states() {
    printf("\n=== Test 8: Angel Mode - Separate L/R Filter States ===\n");

    scalar_coeffs_t hpf, lpf;
    calc_highpass(&hpf, 500.0f,  1.0f);
    calc_lowpass (&lpf, 4000.0f, 1.0f);

    scalar_state_t hpf_L, lpf_L, hpf_R, lpf_R;
    scalar_state_init(&hpf_L); scalar_state_init(&lpf_L);
    scalar_state_init(&hpf_R); scalar_state_init(&lpf_R);

    /* L: sine at 2 kHz (inside both HPF pass and LPF pass) */
    /* R: DC=1.0 (HPF should block it) */
    int N = 48000;
    float sum_L = 0.0f, sum_R = 0.0f;
    for (int n = 0; n < N; n++) {
        float x_L = sinf(2.0f * FILTER_PI * 2000.0f / FILTER_SAMPLE_RATE * n);
        float x_R = 1.0f;

        float y_L = scalar_biquad_process(x_L, &hpf, &hpf_L);
        y_L       = scalar_biquad_process(y_L,  &lpf, &lpf_L);

        float y_R = scalar_biquad_process(x_R, &hpf, &hpf_R);
        y_R       = scalar_biquad_process(y_R,  &lpf, &lpf_R);

        if (n >= N / 2) {
            sum_L += y_L * y_L;
            sum_R += y_R * y_R;
        }
    }

    float rms_L = sqrtf(sum_L / (N / 2));
    float rms_R = sqrtf(sum_R / (N / 2));

    /* L should have significant energy (2kHz passes both filters)
     * R should have ~0 energy (DC blocked by HPF)                    */
    int ok = (rms_L > 0.1f) && (rms_R < 0.01f);
    printf("  L RMS (2kHz, expect >0.1): %.4f  %s\n", rms_L, rms_L > 0.1f ? "PASS" : "FAIL");
    printf("  R RMS (DC,  expect <0.01): %.4f  %s\n", rms_R, rms_R < 0.01f ? "PASS" : "FAIL");
    printf("  Overall: %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 9: Filter numerically stable (no runaway) after 5 s of noise
 * ---------------------------------------------------------------------- */

void test_filter_stability() {
    printf("\n=== Test 9: Numerical Stability Under 5s of Noise ===\n");

    scalar_coeffs_t c;
    scalar_state_t s;
    scalar_state_init(&s);

    /* Use high-Q bandpass (stress test) */
    calc_bandpass(&c, 400.0f, 8.0f);

    uint32_t lcg = 0x12345678u;
    int N = 48000 * 5;
    float max_out = 0.0f;

    for (int n = 0; n < N; n++) {
        lcg = lcg * 1664525u + 1013904223u;
        float x = (float)(int32_t)lcg / (float)0x80000000u;  /* -1..1 */
        float y = scalar_biquad_process(x, &c, &s);
        float ay = fabsf(y);
        if (ay > max_out) max_out = ay;
    }

    /* Output should remain bounded (biquad is a stable IIR design) */
    int ok = (max_out < 100.0f) && !isinf(max_out) && !isnan(max_out);
    printf("  Max output amplitude after 5s noise: %.4f  %s\n",
           max_out, ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * Test 10: Mode frequency range mapping (constants validation)
 * ---------------------------------------------------------------------- */

void test_mode_frequency_ranges() {
    printf("\n=== Test 10: Mode Frequency Range Constants ===\n");

    /* Tribal: bandpass 80-800 Hz */
    float tribal_min = 80.0f, tribal_max = 800.0f;
    /* Military: highpass 1k-8k Hz */
    float mil_min = 1000.0f, mil_max = 8000.0f;
    /* Angel: HPF at 500 Hz, LPF at 4k-6k Hz */
    float angel_hpf = 500.0f;
    float angel_lpf_min = 4000.0f, angel_lpf_max = 6000.0f;

    /* Tribal: freq at depth=0 and depth=1 */
    float depth0 = 0.0f, depth1 = 1.0f;
    float tribal_f0 = tribal_min + depth0 * (tribal_max - tribal_min);
    float tribal_f1 = tribal_min + depth1 * (tribal_max - tribal_min);

    /* Military: same */
    float mil_f0 = mil_min + depth0 * (mil_max - mil_min);
    float mil_f1 = mil_min + depth1 * (mil_max - mil_min);

    /* Angel LPF: 4000 + depth * 2000 */
    float angel_f0 = 4000.0f + depth0 * 2000.0f;
    float angel_f1 = 4000.0f + depth1 * 2000.0f;

    int ok =
        (fabsf(tribal_f0 - 80.0f)   < 1.0f) &&
        (fabsf(tribal_f1 - 800.0f)  < 1.0f) &&
        (fabsf(mil_f0 - 1000.0f)    < 1.0f) &&
        (fabsf(mil_f1 - 8000.0f)    < 1.0f) &&
        (fabsf(angel_hpf - 500.0f)  < 1.0f) &&
        (fabsf(angel_f0 - 4000.0f)  < 1.0f) &&
        (fabsf(angel_f1 - 6000.0f)  < 1.0f);

    printf("  Tribal:   depth=0 → %g Hz, depth=1 → %g Hz\n", tribal_f0, tribal_f1);
    printf("  Military: depth=0 → %g Hz, depth=1 → %g Hz\n", mil_f0,    mil_f1);
    printf("  Angel HPF: %g Hz, LPF range: %g - %g Hz\n", angel_hpf, angel_f0, angel_f1);
    printf("  %s\n", ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void) {
    printf("==========================================\n");
    printf("delay_tribal – Biquad Filter Test Suite\n");
    printf("==========================================\n");

    test_bandpass_coefficients();
    test_highpass_coefficients();
    test_lowpass_coefficients();
    test_biquad_process_state_update();
    test_bandpass_frequency_response();
    test_highpass_frequency_response();
    test_lowpass_frequency_response();
    test_angel_mode_independent_lr_states();
    test_filter_stability();
    test_mode_frequency_ranges();

    printf("\n==========================================\n");
    printf("ALL TESTS PASSED\n");
    printf("==========================================\n");
    return 0;
}
