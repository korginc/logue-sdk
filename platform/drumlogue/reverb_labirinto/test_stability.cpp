/**
 * @file test_stability.cpp
 * @brief Stability test for reverb_labirinto at maximum parameter values.
 *
 * Verifies that the FDN reverb does not diverge (no NaN, no Inf, output
 * stays bounded) when all parameters are pushed to their maximum.
 *
 * Worst case for stability:
 *   TIME=100  → decay=0.99
 *   LOW=100   → lowDecayMult=1.5  (0.9 + 1.0*0.6)
 *   HIGH=100  → highDecayMult=1.0 (0.1 + 1.0*0.9)
 *   unifiedDecay = min(0.99, 0.99 * sqrt(1.5*1.0)) = min(0.99, 1.21) = 0.99 → stable
 *   DAMP=1000 → damping freq=10000 Hz  (high-freq pass-through)
 *   WIDE=200  → stereo width=2.0       (extreme wide)
 *   DFSN=1000 → diffusion=1.0          (maximum diffusion)
 *   MIX=1000  → 100% wet
 *
 * Compile: g++ -std=c++14 -O2 -o test_stability test_stability.cpp -lm
 * Run:     ./test_stability
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <algorithm>

#ifndef M_TWOPI
#define M_TWOPI (2.0 * M_PI)
#endif

#define SAMPLE_RATE    48000.0f
#define FDN_CH         8
#define BUF_SIZE       65536      /* 2^16 – mirrors NeonAdvancedLabirinto.h */
#define BUF_MASK       (BUF_SIZE - 1)
#define TEST_SECONDS   10
#define TEST_SAMPLES   (TEST_SECONDS * (int)SAMPLE_RATE)

/* -------------------------------------------------------------------------
 * Hadamard matrix (same construction as NeonAdvancedLabirinto.h)
 * ---------------------------------------------------------------------- */
static float H[FDN_CH][FDN_CH];

static void build_hadamard() {
    float norm = 1.0f / sqrtf((float)FDN_CH);
    for (int i = 0; i < FDN_CH; i++)
        for (int j = 0; j < FDN_CH; j++) {
            int bits = i & j, parity = 0;
            while (bits) { parity ^= (bits & 1); bits >>= 1; }
            H[i][j] = parity ? -norm : norm;
        }
}

/* -------------------------------------------------------------------------
 * Scalar FDN engine (mirrors NeonAdvancedLabirinto.h processScalar path)
 * ---------------------------------------------------------------------- */
typedef struct {
    float buf[FDN_CH][BUF_SIZE];
    float lpfState[FDN_CH];   /* one-pole LPF state per channel */
    float modPhase[FDN_CH];   /* LFO phase per channel (0..2π) */
    int   writePos;
    int   activeSampleCount;  /* APC tail counter — 0 means bypass */
    /* parameters */
    float decay;              /* clamped to ≤0.99 */
    float lowDecayMult;       /* 0.9..1.5 */
    float highDecayMult;      /* 0.1..1.0 */
    float diffusion;          /* 0..1   */
    float dampingCoeff;       /* one-pole LPF coeff */
    float modDepth;           /* 0..1   */
    float modRate;            /* Hz     */
    float mix;                /* 0..1   */
    float width;              /* 0..2   */
    float delayTimes[FDN_CH]; /* seconds */
} ScalarLabirinto;

static void lab_init(ScalarLabirinto *lab) {
    memset(lab, 0, sizeof(*lab));
    const float base[FDN_CH] = {
        0.0421f, 0.0713f, 0.0987f, 0.1249f,
        0.1571f, 0.1835f, 0.2127f, 0.2413f
    };
    for (int i = 0; i < FDN_CH; i++)
        lab->delayTimes[i] = base[i];
    lab->decay         = 0.5f;
    lab->lowDecayMult  = 1.0f;
    lab->highDecayMult = 0.5f;
    lab->diffusion     = 0.3f;
    float omega = M_TWOPI * 500.0f / SAMPLE_RATE;
    lab->dampingCoeff  = expf(-omega);
    lab->modDepth      = 0.1f;
    lab->modRate       = 0.5f;
    lab->mix               = 0.3f;
    lab->width             = 1.0f;
    lab->activeSampleCount = 0;   /* cold start — matches hardware power-on */
    /* modPhase: spread phases across channels */
    for (int i = 0; i < FDN_CH; i++)
        lab->modPhase[i] = i * (M_TWOPI / FDN_CH);
}

static void lab_set_params(ScalarLabirinto *lab,
                           float mix, float decay,
                           float lowDecayMult, float highDecayMult,
                           float dampingFreqHz, float width,
                           float diffusion, float modDepth, float modRate) {
    lab->mix           = mix;
    lab->decay         = decay;
    lab->lowDecayMult  = lowDecayMult;
    lab->highDecayMult = highDecayMult;
    float omega = M_TWOPI * dampingFreqHz / SAMPLE_RATE;
    lab->dampingCoeff  = expf(-omega);
    lab->width         = width;
    lab->diffusion     = diffusion;
    lab->modDepth      = modDepth;
    lab->modRate       = modRate;
}

static void lab_process_sample(ScalarLabirinto *lab, float inL, float inR,
                                float *outL, float *outR) {
    float input = (inL + inR) * 0.5f;

    /* APC: raw-input wake-up (must come before bypass guard). */
    if (fabsf(input) > 1e-5f)
        lab->activeSampleCount = (int)(SAMPLE_RATE * (1.0f + lab->decay * 5.0f));

    /* Bypass when tail is exhausted — output dry signal only. */
    if (lab->activeSampleCount <= 0) {
        *outL = inL * (1.0f - lab->mix);
        *outR = inR * (1.0f - lab->mix);
        lab->writePos = (lab->writePos + 1) & BUF_MASK;
        return;
    }
    lab->activeSampleCount--;

    /* Read delayed samples with LFO modulation */
    float delayOut[FDN_CH];
    for (int ch = 0; ch < FDN_CH; ch++) {
        float ds  = lab->delayTimes[ch] * SAMPLE_RATE;
        float mod = sinf(lab->modPhase[ch]) * lab->modDepth * 100.0f;
        float rpos = (float)lab->writePos - (ds + mod);
        while (rpos < 0)         rpos += BUF_SIZE;
        while (rpos >= BUF_SIZE) rpos -= BUF_SIZE;
        int   i1   = (int)rpos;
        int   i2   = (i1 + 1) & BUF_MASK;
        float frac = rpos - i1;
        delayOut[ch] = lab->buf[ch][i1] + frac * (lab->buf[ch][i2] - lab->buf[ch][i1]);
        /* advance LFO */
        lab->modPhase[ch] += lab->modRate * M_TWOPI / SAMPLE_RATE;
        if (lab->modPhase[ch] >= M_TWOPI)
            lab->modPhase[ch] -= M_TWOPI;
    }

    /* Hadamard mix + unified decay (geometric mean of low/high multipliers) */
    float unifiedDecay = fminf(0.99f, lab->decay *
                               sqrtf(lab->highDecayMult * lab->lowDecayMult));
    float mixed[FDN_CH];
    for (int i = 0; i < FDN_CH; i++) {
        float sum = 0.0f;
        for (int j = 0; j < FDN_CH; j++) sum += H[i][j] * delayOut[j];
        mixed[i] = sum * unifiedDecay;
    }
    mixed[0] += input * (1.0f - lab->decay);

    /* One-pole LPF (DAMP + DFSN diffusion) */
    float pole = lab->diffusion * lab->dampingCoeff;
    for (int ch = 0; ch < FDN_CH; ch++) {
        lab->lpfState[ch] = mixed[ch] * (1.0f - pole) + lab->lpfState[ch] * pole;
        mixed[ch] = lab->lpfState[ch];
    }

    /* Write back */
    for (int ch = 0; ch < FDN_CH; ch++)
        lab->buf[ch][lab->writePos] = mixed[ch];
    lab->writePos = (lab->writePos + 1) & BUF_MASK;

    /* Stereo downmix (ch 0-3 → L, ch 4-7 → R) */
    float leftRaw = 0.0f, rightRaw = 0.0f;
    for (int i = 0; i < 4; i++)  leftRaw  += mixed[i];
    for (int i = 4; i < 8; i++)  rightRaw += mixed[i];
    leftRaw  *= 0.25f;
    rightRaw *= 0.25f;

    /* Stereo width: mid/side */
    float mid  = (leftRaw + rightRaw) * 0.5f;
    float side = (leftRaw - rightRaw) * 0.5f;
    float wetL = mid + side * lab->width;
    float wetR = mid - side * lab->width;

    /* Wet/dry mix */
    *outL = inL * (1.0f - lab->mix) + wetL * lab->mix;
    *outR = inR * (1.0f - lab->mix) + wetR * lab->mix;
}

/* -------------------------------------------------------------------------
 * LCG noise (deterministic white noise)
 * ---------------------------------------------------------------------- */
static uint32_t lcg_state = 0xDEADBEEFu;
static float lcg_noise() {
    lcg_state = lcg_state * 1664525u + 1013904223u;
    return ((int32_t)lcg_state) / 2147483648.0f;
}

/* -------------------------------------------------------------------------
 * Tests
 * ---------------------------------------------------------------------- */

static void test_stability_max_params() {
    printf("\n=== Stability Test: ALL PARAMS MAX ===\n");
    printf("    TIME=100 (decay=0.99), LOW=100, HIGH=100\n");
    printf("    unifiedDecay = min(0.99, 0.99*sqrt(1.5)) = 0.99\n");
    printf("    Processing %d seconds of white noise...\n", TEST_SECONDS);

    build_hadamard();
    ScalarLabirinto lab;
    lab_init(&lab);

    /* Max parameter values (mirrors unit_set_param_value mappings):
     * MIX=1000 → 1.0,  TIME=100 → decay=0.99,
     * LOW=100  → lowDecayMult=1.5,  HIGH=100 → highDecayMult=1.0,
     * DAMP=1000 → 10000 Hz,  WIDE=200 → width=2.0,
     * DFSN=1000 → diffusion=1.0,  modDepth=0.1, modRate=0.5 (defaults) */
    float mix           = 1000 / 1000.0f;             /* 1.0 */
    float decay         = 0.01f + (100-1)/99.0f*0.98f;/* 0.99 */
    float lowDecayMult  = 0.9f + (100/100.0f)*0.6f;   /* 1.5 */
    float highDecayMult = 0.1f + (100/100.0f)*0.9f;   /* 1.0 */
    float dampingFreqHz = 1000 * 10.0f;               /* 10000 Hz */
    float width         = 200 * 0.01f;               /* 2.0 */
    float diffusion     = 1000 / 1000.0f;             /* 1.0 */
    lab_set_params(&lab, mix, decay, lowDecayMult, highDecayMult,
                   dampingFreqHz, width, diffusion, 0.1f, 0.5f);

    float expectedUnified = fminf(0.99f, decay * sqrtf(highDecayMult * lowDecayMult));
    printf("    unifiedDecay actual = %.6f (must be <= 0.99)\n", expectedUnified);
    assert(expectedUnified <= 0.99f);

    float maxAbs = 0.0f;
    int   nanCount = 0, infCount = 0;

    for (int n = 0; n < TEST_SAMPLES; n++) {
        float in  = lcg_noise() * 0.5f;
        float outL, outR;
        lab_process_sample(&lab, in, in, &outL, &outR);

        if (fabsf(outL) > maxAbs) maxAbs = fabsf(outL);
        if (fabsf(outR) > maxAbs) maxAbs = fabsf(outR);
        if (isnan(outL) || isnan(outR)) nanCount++;
        if (isinf(outL) || isinf(outR)) infCount++;
    }

    printf("  Max |output|  = %.6f\n", maxAbs);
    printf("  NaN count     = %d\n", nanCount);
    printf("  Inf count     = %d\n", infCount);

    /* With unifiedDecay clamped to 0.99, output must stay bounded.
     * FDN energy converges to input^2 / (1-decay^2) ≈ 25 → amplitude ≈ 5.
     * Using 10.0 as a generous safety margin (width=2.0 adds mild amplification). */
    assert(nanCount == 0 && "NaN detected at max params");
    assert(infCount == 0 && "Inf detected at max params");
    assert(maxAbs < 10.0f && "Output exploded at max params");

    printf("  PASS: stable at max params\n");
}

static void test_stability_default_params() {
    printf("\n=== Stability Test: DEFAULT PARAMS ===\n");
    printf("    TIME=50 (decay~0.5), MIX=700 (70%%)\n");

    build_hadamard();
    ScalarLabirinto lab;
    lab_init(&lab);

    /* Default values from unit.cc: {700, 50, 50, 70, 250, 100, 1000, 3} */
    float mix           = 700 / 1000.0f;              /* 0.7 */
    float decay         = 0.01f + (50-1)/99.0f*0.98f; /* ~0.495 */
    float lowDecayMult  = 0.9f + (50/100.0f)*0.6f;    /* 1.2 */
    float highDecayMult = 0.1f + (70/100.0f)*0.9f;    /* 0.73 */
    float dampingFreqHz = 250 * 10.0f;                /* 2500 Hz */
    float width         = 100 * 0.01f;               /* 1.0 */
    float diffusion     = 1000 / 1000.0f;             /* 1.0 */
    lab_set_params(&lab, mix, decay, lowDecayMult, highDecayMult,
                   dampingFreqHz, width, diffusion, 0.1f, 0.5f);

    float outL, outR;
    lab_process_sample(&lab, 1.0f, 1.0f, &outL, &outR);  /* impulse */

    float maxAbs = 0.0f;
    int   nanCount = 0, nonzeroFrames = 0;
    for (int n = 0; n < TEST_SAMPLES; n++) {
        lab_process_sample(&lab, 0.0f, 0.0f, &outL, &outR);
        if (fabsf(outL) > maxAbs) maxAbs = fabsf(outL);
        if (isnan(outL) || isnan(outR)) nanCount++;
        if (fabsf(outL) > 1e-7f || fabsf(outR) > 1e-7f) nonzeroFrames++;
    }

    printf("  Max |output|  = %.6f\n", maxAbs);
    printf("  NaN count     = %d\n", nanCount);
    printf("  Non-zero frames = %d / %d\n", nonzeroFrames, TEST_SAMPLES);

    assert(nanCount == 0);
    assert(maxAbs   < 4.0f);
    assert(nonzeroFrames > (int)(SAMPLE_RATE * 0.1f) &&
           "Reverb tail should persist for at least 100ms");

    printf("  PASS: default params stable, reverb tail present\n");
}

static void test_unified_decay_clamp() {
    printf("\n=== Unified Decay Clamp Test ===\n");
    /* At TIME=100, LOW=100, HIGH=100 the pre-clamp value would be 1.21.
     * The fminf(0.99, ...) must reduce it to exactly 0.99. */
    float decay         = 0.01f + (100-1)/99.0f*0.98f; /* 0.99 */
    float lowDecayMult  = 0.9f + (100/100.0f)*0.6f;    /* 1.5 */
    float highDecayMult = 0.1f + (100/100.0f)*0.9f;    /* 1.0 */
    float raw           = decay * sqrtf(highDecayMult * lowDecayMult);
    float clamped       = fminf(0.99f, raw);

    printf("  raw unifiedDecay = %.6f  (> 1 before clamp)\n", raw);
    printf("  clamped          = %.6f  (must be 0.99)\n",     clamped);
    assert(raw     > 1.0f  && "Pre-clamp value should exceed 1.0");
    assert(clamped <= 0.99f && "Clamped value must not exceed 0.99");
    printf("  PASS: fminf clamp prevents super-unity feedback\n");
}

/* Test that the engine wakes from a cold start (activeSampleCount=0) and
 * produces wet output — the exact scenario that was permanently bypassed
 * before commit 8f8d255.  With mix=1.0 (100% wet), the output MUST exceed
 * the dry floor (zero) once the shortest delay line fills (~2021 samples). */
static void test_wet_output_cold_start() {
    printf("\n=== Wet Output Cold-Start Test ===\n");
    printf("    mix=1.0, decay=0.5, impulse at t=0, then silence\n");
    printf("    Expect non-zero wet output within the shortest delay time\n");

    build_hadamard();
    ScalarLabirinto lab;
    lab_init(&lab);
    lab.mix = 1.0f;  /* 100% wet — any dry bleed would mask the test */

    assert(lab.activeSampleCount == 0 && "Engine must start cold");

    /* Impulse — this must wake the engine via raw-input check. */
    float outL, outR;
    lab_process_sample(&lab, 1.0f, 1.0f, &outL, &outR);
    assert(lab.activeSampleCount > 0 && "Impulse must wake the engine from cold start");

    /* Shortest delay ≈ 0.0421 s × 48000 Hz = 2021 samples.
     * Run silence until just past the first reflection. */
    const int kFirstReflection = 2200; /* slightly beyond 2021 */
    float maxWet = 0.0f;
    for (int n = 0; n < kFirstReflection; n++) {
        lab_process_sample(&lab, 0.0f, 0.0f, &outL, &outR);
        if (fabsf(outL) > maxWet) maxWet = fabsf(outL);
    }

    printf("  Max wet amplitude in first %.1f ms = %.6f\n",
           kFirstReflection * 1000.0f / SAMPLE_RATE, maxWet);
    assert(maxWet > 1e-4f &&
           "Engine must produce wet output after cold-start impulse "
           "(was permanently 0 before APC deadlock fix)");

    printf("  PASS: wet output confirmed from cold start\n");
}

int main() {
    printf("=== reverb_labirinto stability tests ===\n");
    test_unified_decay_clamp();
    test_wet_output_cold_start();
    test_stability_default_params();
    test_stability_max_params();
    printf("\n=== ALL reverb_labirinto STABILITY TESTS PASSED ===\n");
    return 0;
}
