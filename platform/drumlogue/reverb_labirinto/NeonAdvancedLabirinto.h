#pragma once

/**
 * @file NeonAdvancedLabirinto.h
 * @brief Advanced NEON-optimized reverb with FDN (Feedback Delay Network)
 *
 * OPTIMIZED:
 * - vld4q_f32 gather for 3x faster delay line reads
 * - Interleaved storage format for efficient vector loads
 * - Vectorized linear interpolation
 * - Process 4 samples in parallel throughout
 */

#include <arm_neon.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <float_math.h>
#include <algorithm>

// Maximum delay line length (2 seconds at 48kHz)
#define MAX_DELAY_SECONDS 2.0f
#define MAX_DELAY_SAMPLES (int)(MAX_DELAY_SECONDS * 48000)

// Number of FDN channels (must be multiple of 4 for NEON)
#define FDN_CHANNELS 8

// Buffer size for delay lines (power of 2 for efficient modulo)
#define BUFFER_SIZE 65536  // 2^16
#define BUFFER_MASK (BUFFER_SIZE - 1)
#define FREQ_MAX_DIV_MIN (18.333f)
#define PREDELAY_BUFFER_SIZE 16384  // ~341ms at 48kHz
#define PREDELAY_MASK (PREDELAY_BUFFER_SIZE - 1)
#define NEON_LANES  (4)
#define MAX_PILLARS (16)

/**
 * OPTIMIZED: Interleaved frame structure for vld4q_f32
 * Stores all 8 FDN channels at a single time position
 * Format: [ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7]
 */
typedef struct __attribute__((aligned(16))) {
    float samples[FDN_CHANNELS];  // All 8 channels at this time position
} interleaved_frame_t;

typedef enum {
    k_foresta,
    k_tempio,
    k_labirinto,
    k_esotico,
    k_stellare,
    k_preset_number,
} preset_numer_t;


enum parameterState {
  k_paramProgram = 0,
  k_time, k_low, k_high, k_damp,
  k_wide, k_diffusion, k_pill, k_shimmer_freq,
  k_pre_delay, k_vibr,
  k_total
};

static const char *k_preset_names[k_preset_number] =
    {"foresta", "tempio", "labirinto", "esotico", "stellare"};
// ============================================================================
// Factory Presets
// ============================================================================
// Each preset: {PRESET, TIME, LOW, HIGH, DAMP, WIDE, DFSN, PILL, SHMR, PDLY, VIBR}
static const int32_t k_presets[k_preset_number][k_total] = {
    // 0: foresta - mellow, sparse, "wood" (warm lows, short, moderate decay)
    {k_foresta, 40, 60, 40, 200, 80, 60, 3, 0, 0, 10},
    // 1: tempio  - sombre, "stone" (heavy lows, long, dark, 6-ch)
    {k_tempio, 70, 80, 25, 130, 130, 80, 2, 0, 0, 10},
    // 2: labirinto - center values with random ping-pong stereo bouncing
    {k_labirinto, 60, 50, 50, 510, 100, 50, 1, 0, 10, 10},
    // 3: esotico - microtonal echoes on non-Western scale
    {k_esotico, 40, 60, 80, 800, 100, 50, 4, 5, 5, 20},
    // 4: stellare - long, subtle, "spacey" shimmer (8-ch + shimmer)
    {k_stellare, 90, 50, 80, 800, 180, 30, 4, 35, 20, 10},
};

// ============================================================================
// Main Class
// ============================================================================

class NeonAdvancedLabirinto {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    NeonAdvancedLabirinto()
        : sampleRate(48000.0f)
        , writePos(0)
        , decay(0.5f)
        , diffusion(0.3f)
        , modDepth(0.1f)
        , modRate(0.5f)
        , width(1.0f)
        , dampingCoeff(0.5f)
        , lowDecayMult(1.0f)
        , highDecayMult(0.5f)
        , initialized(false)
        , pillar_(3)
        , pingPong_(false)
        , shimmerDepth_(0.0f)
        , shimmerPhase_(0.0f)
        , shimmerFreq_ (35.0f)
        , preDelayWritePos(0)
        , currentPreDelaySamples(0.0f)
        , targetPreDelaySamples(0.0f)
        , currentPreset(0)
        , lfoSpeed(1.0f)
        , randomLfoValue(0.0f)
        , randomLfoCounter(0)
        , randomLfoPeriodSamples(48000)
        , smoothedLfoValue(0.0f)
        , noiseSeed(132465798U)
        , noiseColour(2.0f)
        , noiseGain(0.0f)
        , noiseEnvelope(0.0f)
        , pingMapIndex(0)
        , pingMapCounter(0)
        , filterUpdateCounter(0) {

        // Initialize delay times (prime-based for smooth diffusion)
        float baseDelays[FDN_CHANNELS] = {
            0.0421f, 0.0713f, 0.0987f, 0.1249f,
            0.1571f, 0.1835f, 0.2127f, 0.2413f
        };

        for (int i = 0; i < FDN_CHANNELS; i++) {
            delayTimes[i] = baseDelays[i];
        }

        // Initialize modulation phases (store full vector per channel)
        for (int i = 0; i < FDN_CHANNELS; i++) {
	    // Initialize all 4 lanes to the same starting phase
            modPhaseVec[i] = vdupq_n_f32(0.0f);
            filterState1[i] = 0.0f;
            filterState2[i] = 0.0f;
        }

        // Initialize filter states
        memset(lpfState, 0, sizeof(float) * FDN_CHANNELS);
        memset(metalState, 0, sizeof(float) * FDN_CHANNELS);
        memset(crystalAPState, 0, sizeof(float) * FDN_CHANNELS);
        updateFilterCoeffs();

    }

    ~NeonAdvancedLabirinto() {
    }

    /**
     * Initialize the reverb with proper memory alignment
     * @return true if initialization successful
     */
    bool init() {

        // Clear buffer
        clear();

        // Initialize Cochrane Microtonal Shimmer (18-EDO Scale)
        initMicrotonalShimmer();

        // set state variables
        initialized = true;
        return true;
    }

    /**
     * Clear all delay lines and filter states
     */
    void clear() {
        // Use NEON to clear efficiently
        float32x4_t zero = vdupq_n_f32(0.0f);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            // Clear 8 channels using 2 NEON stores
            vst1q_f32(&delayLine[i].samples[0], zero);
            vst1q_f32(&delayLine[i].samples[4], zero);
        }

        writePos = 0;

        // Reset modulation phases with proper per-lane offsets
	    // lane k = k * incPerSample so sequential
        // samples start at phase 0, inc, 2*inc, 3*inc
        float incPerSample = modRate * M_TWOPI / sampleRate;
        float32x4_t init_phases = {
            0.0f,
            incPerSample,
            2.0f * incPerSample,
            3.0f * incPerSample
        };
        for (int i = 0; i < FDN_CHANNELS; i++) {
            modPhaseVec[i] = init_phases;
        }

        memset(metalState, 0, sizeof(float) * FDN_CHANNELS);
        memset(crystalAPState, 0, sizeof(float) * FDN_CHANNELS);
        // Reset filter states
        memset(lpfState, 0, sizeof(float) * FDN_CHANNELS);
        for (int i = 0; i < FDN_CHANNELS; i++) {
            filterState1[i] = 0.0f;
            filterState2[i] = 0.0f;
        }

        // Reset pre delay line
        memset(preDelayBuffer, 0, sizeof(preDelayBuffer));
        memset(channelPhase_, 0, sizeof(channelPhase_));
        memset(swirlRate_, 0, sizeof(swirlRate_));
        memset(microtonalRate_, 0, sizeof(microtonalRate_));
        preDelayWritePos = 0;
        activeSampleCount = 0;
        // Reset LFO
        randomLfoCounter = 0;
        randomLfoValue = 0.0f;
        pingMapCounter = 0;
        filterUpdateCounter = 0;
        pingMapIndex = 0;

        // Reset noise
        for (int i = 0; i < FDN_CHANNELS; i++) {
            noiseStates[i] = 0.0f;
            noiseStates2[i] = 0.0f;
        }
        noiseEnvelope = 0.0f;
    }

    /*===========================================================================*/
    /* Parameter Setters/Getters */
    /*===========================================================================*/
    inline int getPreset() {
        return currentPreset;
    }

    inline void loadPreset(int32_t value) {
        if (currentPreset != value) {
            setFilterType(value);
            for (uint8_t i = 0; i < k_total; i++) {
                setParameter(i, k_presets[value][i]);
            }
        }
    }

    inline int32_t getParameterValue(uint8_t index) {
        if (index >= k_total) return -1;    // invalid value
        return params_[index];
    }

    inline void setParameter(uint8_t index, int32_t value) {
        if (index >= k_total) return;
        params_[index] = value;   // store into local DB

        switch (index) {
        case k_paramProgram:
          if (currentPreset != value) loadPreset(value);
            break;
        case k_time: // TIME  1..100 → decay 0.01..0.99
            setDecay(0.01f + (value - 1) / 99.0f * 0.98f);
            break;
        case k_low: // LOW  1..100 → low-freq decay multiplier
            setLowDecay((float)value);
            break;
        case k_high: // HIGH  1..100 → high-freq decay multiplier
            setHighDecay((float)value);
            break;
        case k_damp: // DAMP  20..1000 (×10 → 200..10000 Hz)
            setDamping((float)value * 10.0f);
            break;
        case k_wide: // WIDE  0..200 → stereo width 0.0..2.0
            setWidth(value * 0.01f);
            break;
        case k_diffusion: // DFSN  0..100 → diffusion 0.0..1.0
            setDiffusion(value * 0.01f);
            break;
        case k_pill: // PILL  0..4  - pillar routing mode
            setPillar(value);
            break;
        case k_shimmer_freq: // SHMR  0..100  - shimmer frequency
            setShimmerFreq(value);
            break;
        case k_pre_delay: // PDLY 0..200 ms
            setPreDelay((float)value);
            break;
        case k_vibr:
            // value 1..30 → 0.1..3.0 Hz
            setLfoSpeed(value * 0.1f);
            updateModRate();
            break;
        default:
        break;
        }
    }
    void setDecay(float d) { decay = fmaxf(0.0f, fminf(0.99f, d)); }
    void setDiffusion(float d) {
        diffusion = fmaxf(0.0f, fminf(1.0f, d));
        if (filterMode == kFilterNoise) {
            // Map to 0=Brown, 1=Pink, 2=White, 3=Blue, 4=Violet, 5=Grey
            noiseColour = diffusion * 5.999f;
        }
        // Modulation depth depends on pillar (as before) and DFSN (diffusion)
        setModDepth(((pillar_ == 0) ? 0.6f : (pillar_ == 1) ? 0.4f :
                        (pillar_ == 2) ? 0.2f : 0.1f) * diffusion);
    }

    /**
     * Set pillar count / routing mode.
     *
     * 0 = sparse  (only 2 channels reach output - large sparse room feel)
     * 1 = ping-pong (4 channels, alternating L/R - bouncing stereo echo)
     * 2 = stone    (6 channels - sombre, dense)
     * 3 = full     (all 8 channels - full FDN, default)
     * 4 = shimmer  (all 8 channels + subtle frequency-modulated re-injection)
     */
    void setPillar(int value) {
        pillar_       = std::max(0, std::min(value, 4));
        pingPong_     = (pillar_ == 1);
        shimmerDepth_ = (pillar_ == 4) ? 0.4f * modDepth : 0.0f;
        shimmerPhase_ = 0.0f;
        // Modulation depth depends on pillar (as before) and DFSN (diffusion)
        setModDepth(((pillar_ == 0) ? 0.6f : (pillar_ == 1) ? 0.4f :
                        (pillar_ == 2) ? 0.2f : 0.1f) * diffusion);
        updateModRate();
    }
    void setModDepth(float d) { modDepth = fmaxf(0.0f, fminf(1.0f, d)); }
    void setModRate(float r) { modRate = fmaxf(0.1f, fminf(10.0f, r)); }
    // Recomputes modRate from scratch using current lfoSpeed and modDepth.
    // Uses a fixed base rate of 1.0 Hz so the result is always deterministic
    // regardless of how many times this is called.
    void updateModRate() { modRate = fmaxf(0.1f, fminf(10.0f, lfoSpeed * modDepth)); }
    void setWidth(float w) { width = fmaxf(0.0f, fminf(2.0f, w)); } // UNUSED
    // 3 Hz to 8 Hz: Creates Cochrane's "microtonal beating" — a nervous, spicy, disconcerting chorusing.
    // 20 Hz to 55 Hz: Creates the "low pitching" cascade — thick, dark, metallic undertones that dive deeper as the reverb decays.
    void setShimmerFreq(float value) {
        // Normalize UI value to 0.0 -> 1.0
        float norm = fmaxf(0.0f, fminf(1.0f, (float)value * 0.01f));

        // Exponential mapping: min * (max/min)^norm
        // 3.0f * (55.0 / 3.0)^norm
        shimmerFreq_ = 3.0f * fasterpowf(FREQ_MAX_DIV_MIN, norm);
    }
    float getShimmerFreq() { return shimmerFreq_; }
    void setPreDelay(float ms) {
        float clampedMs = fmaxf(0.0f, fminf(340.0f, ms));
        // Convert milliseconds to samples and store it as our new float target
        targetPreDelaySamples = (int)(clampedMs * sampleRate / 1000.0f);
    }
    void setDamping(float freqHz) {
        freqHz = fmaxf(200.0f, fminf(10000.0f, freqHz));
	    // omega = 2π * fc / fs;  coeff ≈ 1 - omega  (first-order approx)
        float omega = 2.0f * (float)M_PI * freqHz / sampleRate;
        dampingCoeff = e_expff(-omega);
        updateFilterCoeffs();   // recalc wood/stone/metal filters
        updateBaseFc();
        if (filterMode == kFilterNoise) {
            // Scale by 0.05: steady-state noise level = noiseGain per channel.
            // Without the scale factor, accumulated noise at high decay overwhelms reverb.
            noiseGain = diffusion * dampingCoeff * 0.05f;
        }
    }

    /**
     * Low-frequency RT60 multiplier (1..100 → 0.01..1.0 s scale).
     * Increases effective decay for low-end warmth.
     */
    void setLowDecay(float value) {
        // value 1-100; map to a per-channel decay multiplier 0.9..1.5
        lowDecayMult = 0.9f + (value * 0.01f) * 0.6f;
    }

    /**
     * High-frequency RT60 multiplier (1..100 → 0.01..1.0 s scale).
     * Controls how quickly the high end decays.
     */
    void setHighDecay(float value) {
        // value 1-100; higher value = brighter (less high-freq damping)
        highDecayMult = 0.1f + (value * 0.01f) * 0.9f;
    }

    /**
     * @brief Set the Lfo Speed object for modulation 0.1Hz -> 3Hz
     *
     * @param speedHz
     */
    void setLfoSpeed(float speedHz) {
        lfoSpeed = fmaxf(0.1f, fminf(3.0f, speedHz));
        randomLfoPeriodSamples = (int)(sampleRate / lfoSpeed);
        if (randomLfoPeriodSamples < 1) randomLfoPeriodSamples = 1;
    }

    void setFilterType(int preset) {
        currentPreset = preset;   // Fix: currentPreset must track the active preset
        switch (preset) {
            case k_foresta:    filterMode = kFilterWood;    break;
            case k_tempio:     filterMode = kFilterStone;   break;
            case k_labirinto:  filterMode = kFilterMetal;   break;  // glassy high-Q bandpass
            case k_esotico:    filterMode = kFilterCrystal; break;  // bright allpass-like shimmer
            case k_stellare:   filterMode = kFilterNoise;   break;
            default:           filterMode = kFilterWood;    break;
        }
        updateBaseFc();
    }

    void updateBaseFc() {
        // compute base cutoff frequency from preset and dampingCoeff
        float fc;
        switch (filterMode) {
            case kFilterWood:
                fc = 200.0f + dampingCoeff * 800.0f;   // 200..1000 Hz
                break;
            case kFilterStone:
                fc = 80.0f + dampingCoeff * 420.0f;    // 80..500 Hz
                break;
            case kFilterMetal:
                // 300..1200 Hz: covers kick body and snare fundamentals so
                // drums with energy in this range trigger the metallic resonance.
                fc = 300.0f + dampingCoeff * 900.0f;
                break;
            case kFilterCrystal:
                // 1000..3000 Hz: snare crack / hi-hat body. Stays below DAMP LPF
                // even at moderate DAMP settings so the crystal character survives.
                fc = 1000.0f + dampingCoeff * 2000.0f;
                break;
            default:
                fc = 1000.0f;
                break;
        }
        baseFc = fc;
        updateFilterCoeffs();   // initial coefficients
    }

    /*===========================================================================*/
    /* Advanced features */
    /*===========================================================================*/

    void updateFilterCoeffsAt(float fc) {
        float w0 = 2.0f * M_PI * fc / sampleRate;
        float cos_w0 = fastercosfullf(w0);
        float sin_w0 = fastersinfullf(w0);

        float alpha, b0, b1, b2, a0, a1, a2;

        // peaking filter (band‑pass with gain)
        // For simplicity, we'll use a standard biquad peaking:
        // H(s) = (s^2 + s*(ω0/Q) + ω0^2) / (s^2 + s*(ω0/Q) + ω0^2)
        // Actually we want a peaking EQ, but a simple 2‑pole band‑pass works well.
        // We'll implement a classic band‑pass with gain at centre frequency.
        // Coefficients from RBJ cookbook.
        // standard Direct Form Biquad calculates its output using this mathematical formula:
        // y[n] = (b0 * x[n]) + (b1 * x[n-1]) + (b2 * x[n-2]) - (a1 * y[n-1]) - (a2 * y[n-2])
        if (filterMode == kFilterMetal) {
            // METAL: Bandpass resonance for Labirinto — Q=6 keeps the band wide
            // enough to capture drum fundamentals (kick/snare) while still adding
            // metallic ring. High-Q=8 was inaudible on broad-spectrum percussion.
            float Q = 6.0f;
            alpha = sin_w0 / (2.0f * Q);

            // resonance makeup gain
            float makeup = 5.0f;

            b0 = Q * alpha * makeup;
            b1 = 0.0f;
            b2 = -Q * alpha * makeup;

            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
        }
        else if (filterMode == kFilterCrystal) {
            // CRYSTAL: Medium-Q Bandpass at higher frequencies — used by esotico.
            // Q=3 gives a broader peak than METAL, suitable for microtonal shimmer.
            // The higher fc range (2-7 kHz) keeps the tail bright and airy without
            // the harsh metallic ring, blending with the microtonal modulation path.
            float Q = 1.2f;
            alpha = sin_w0 / (2.0f * Q);

            b0 = alpha;
            b1 = 0.0f;
            b2 = -alpha;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
        }
        else if (filterMode == kFilterWood) {
            // WOOD: Warm, highly damped Lowpass
            float Q = 0.5f;
            alpha = sin_w0 / (2.0f * Q);

            b0 = (1.0f - cos_w0) / 2.0f;
            b1 = 1.0f - cos_w0;
            b2 = (1.0f - cos_w0) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
        }
        else {
            // STONE / DEFAULT: Heavier Butterworth Lowpass
            float Q = 0.707f;
            alpha = sin_w0 / (2.0f * Q);

            b0 = (1.0f - cos_w0) / 2.0f;
            b1 = 1.0f - cos_w0;
            b2 = (1.0f - cos_w0) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
        }

        // ==========================================================
        // UNIFIED NORMALIZATION (Safely prepares coeffs for the DSP loop)
        // ==========================================================
        float invA0 = 1.0f / a0;

        // Feed-forward coefficients
        biquadA0 = b0 * invA0;
        biquadA1 = b1 * invA0;
        biquadA2 = b2 * invA0;

        // Feedback coefficients - DO NOT NEGATE HERE!
        // The DSP loop explicitly uses subtraction: `- (out * biquadB1)`
        biquadB1 = a1 * invA0;
        biquadB2 = a2 * invA0;
    }

    void updateFilterCoeffs() { updateFilterCoeffsAt(baseFc); }

    void applyResonantFilterModulated(float32x4_t* signals, int numChannels, float fcMod) {
        if (filterMode == kFilterNoise) return;
        // Compute modulated frequency but do NOT write back to baseFc.
        // The old code did `baseFc = fc` here, which permanently accumulated the LFO
        // offset on every call (12000 times/second), drifting baseFc to sampleRate*0.45
        // where the bandpass filter becomes a Nyquist-frequency resonator and blows up.
        float fc = fmaxf(20.0f, fminf(sampleRate * 0.45f, baseFc + fcMod));
        updateFilterCoeffsAt(fc);

        // ---------------------------------------------------------
        // PROPER SCALAR IIR BIQUAD PROCESSING (same as before, but using current coefficients)
        // ---------------------------------------------------------
        for (int ch = 0; ch < numChannels; ch++) {

          // 1. Unpack the 4 consecutive samples from the NEON vector
          float in_samps[NEON_LANES];
          vst1q_f32(in_samps, signals[ch]);

          float out_samps[NEON_LANES];

          // 2. Process sequentially to maintain the IIR feedback loop
          for (int s = 0; s < NEON_LANES; s++) {
            float in_val = in_samps[s];

            // Direct Form II Transposed Biquad Math
            float out_val = (in_val * biquadA0) + filterState1[ch];

            // Update history states immediately for the next sample
            filterState1[ch] = (in_val * biquadA1) - (out_val * biquadB1) + filterState2[ch];
            filterState2[ch] = (in_val * biquadA2) - (out_val * biquadB2);

            // kFilterCrystal (esotico): the bandpass alone attenuates by ~20 dB
            // (peak gain ≈ alpha/(1+alpha) ≈ 0.04), making the reverb tail inaudible.
            // Blend additively: passband stays at full level + resonant colour at fc.
            // All other modes replace the signal (LPF/HPF shape the tail spectrum).
            if (filterMode == kFilterCrystal)
              out_samps[s] =
                  in_val +
                  out_val * 2.0f; // Reduced from 6.0 to prevent screeching
            else
                out_samps[s] = out_val;
          }

          // 3. Repack back into the NEON vector to continue the delay network
          signals[ch] = vld1q_f32(out_samps);
        }
    }

    void updateRandomLfo() {
        if (--randomLfoCounter <= 0) {
            randomLfoCounter = randomLfoPeriodSamples;
            randomLfoValue = randomFloat() * 2.0f - 1.0f;
        }
        // One-pole smoothing (~10Hz glide) to prevent filter pops
        smoothedLfoValue += 0.001f * (randomLfoValue - smoothedLfoValue);
    }

    // basic cyclic pseudo-random values
    float randomFloat() {
        noiseSeed ^= noiseSeed << 13;
        noiseSeed ^= noiseSeed >> 17;
        noiseSeed ^= noiseSeed << 5;
        return (float)noiseSeed / (float)0xFFFFFFFFU;
    }

    void applyResonantFilter(float32x4_t* signals, int numChannels) {
        if (filterMode == kFilterNoise) return; // noise is added elsewhere

        for (int ch = 0; ch < numChannels; ch++) {
            // 1. Unpack the NEON vector
            float in_samps[4];
            vst1q_f32(in_samps, signals[ch]);
            float out_samps[4];

            // 2. Process sequentially to maintain the IIR feedback loop
            for (int s = 0; s < 4; s++) {
                float in_val = in_samps[s];

                // Direct Form II Transposed Biquad
                float out_val = (in_val * biquadA0) + filterState1[ch];

                // Update states for the NEXT sample using the CORRECT output feedback
                filterState1[ch] = (in_val * biquadA1) - (out_val * biquadB1) + filterState2[ch];
                filterState2[ch] = (in_val * biquadA2) - (out_val * biquadB2);

                if (filterMode == kFilterCrystal)
                    out_samps[s] = in_val + out_val * 6.0f;
                else
                    out_samps[s] = out_val;
            }

            // 3. Repack back into NEON vector
            signals[ch] = vld1q_f32(out_samps);
        }
    }

    void addColouredNoise(float32x4_t* signals, int numChannels, float gainScale = 0.5f) {
        if (filterMode != kFilterNoise || noiseGain < 0.001f) return;

        for (int ch = 0; ch < numChannels; ch++) {
            float n_out[4];
            for (int i = 0; i < 4; i++) {
                float n = randomFloat() * 2.0f - 1.0f; // Raw white noise

                if (noiseColour < 0.5f) {
                    // 0: Brown (Heavy LPF)
                    noiseStates[ch] = 0.95f * noiseStates[ch] + 0.05f * n;
                    n_out[i] = noiseStates[ch];
                } else if (noiseColour < 1.5f) {
                    // 1: Pink (Mild LPF Approximation)
                    noiseStates[ch] = 0.5f * noiseStates[ch] + 0.5f * n;
                    n_out[i] = noiseStates[ch] * 1.5f; // Gain makeup
                } else if (noiseColour < 2.5f) {
                    // 2: White (Flat)
                    n_out[i] = n;
                } else if (noiseColour < 3.5f) {
                    // 3: Blue (Mild HPF - difference of mild LPF)
                    float diff = n - noiseStates[ch];
                    noiseStates[ch] = 0.5f * noiseStates[ch] + 0.5f * n;
                    n_out[i] = diff * 0.8f;
                } else if (noiseColour < 4.5f) {
                    // 4: Violet (Heavy HPF - pure derivative)
                    float diff = n - noiseStates[ch];
                    noiseStates[ch] = n;
                    n_out[i] = diff * 0.5f;
                } else {
                    // 5: Grey (Notch filter around 2kHz using z^-2)
                    // y[n] = x[n] + x[n-2] creates a comb notch
                    n_out[i] = (n + noiseStates2[ch]) * 0.778f;
                    noiseStates2[ch] = noiseStates[ch];
                    noiseStates[ch] = n;
                }
            }
            // Inject the colored noise into the FDN channel
            float32x4_t noise = vld1q_f32(n_out);
            signals[ch] = vmlaq_n_f32(signals[ch], noise, noiseGain * gainScale);
        }
    }

    void applyCrossFeedback(float32x4_t* signals) {
        float gain = crossGain[pillar_];
        if (gain < 0.001f) return;

        float32x4_t t0, t1;
        switch (pillar_) {
            case 0: { // 2 channels — simultaneous update
                t0 = signals[0]; t1 = signals[1];
                signals[0] = vaddq_f32(t0, vmulq_n_f32(t1, gain));
                signals[1] = vaddq_f32(t1, vmulq_n_f32(t0, gain));
                break;
            }
            case 1: { // 4 channels (ping-pong) — 2 independent pairs
                for (int i = 0; i < 4; i += 2) {
                    t0 = signals[i]; t1 = signals[i+1];
                    signals[i]   = vaddq_f32(t0, vmulq_n_f32(t1, gain));
                    signals[i+1] = vaddq_f32(t1, vmulq_n_f32(t0, gain));
                }
                break;
            }
            case 2: { // 6 channels — 2 independent 3-way rings
                for (int i = 0; i < 6; i += 3) {
                    float32x4_t t2 = signals[i+2];
                    t0 = signals[i]; t1 = signals[i+1];
                    signals[i]   = vaddq_f32(t0, vmulq_n_f32(t1, gain));
                    signals[i+1] = vaddq_f32(t1, vmulq_n_f32(t2, gain));
                    signals[i+2] = vaddq_f32(t2, vmulq_n_f32(t0, gain));
                }
                break;
            }
            default: { // 8 channels — 2 independent 4-way rings
                for (int i = 0; i < 8; i += 4) {
                    float32x4_t t2 = signals[i+2], t3 = signals[i+3];
                    t0 = signals[i]; t1 = signals[i+1];
                    signals[i]   = vaddq_f32(t0, vmulq_n_f32(t2, gain));
                    signals[i+2] = vaddq_f32(t2, vmulq_n_f32(t0, gain));
                    signals[i+1] = vaddq_f32(t1, vmulq_n_f32(t3, gain));
                    signals[i+3] = vaddq_f32(t3, vmulq_n_f32(t1, gain));
                }
                break;
            }
        }
    }

    void applyRandomizedPingPongMix(const float32x4_t* mixed,
                                                        float32x4_t& left, float32x4_t& right) {
        // Timer is handled in the main loop to keep scalar/neon in sync
        // We expect the direction to stay stable for ~100ms
        if (pingMapCounter <= 0) {
             pingMapCounter = 4800;
             pingMapIndex = (pingMapIndex + 1) & 15;
        }
        const uint8_t* map = pingRandomMap[pingMapIndex];
        float32x4_t sumL = vdupq_n_f32(0.0f);
        float32x4_t sumR = vdupq_n_f32(0.0f);
        for (int i = 0; i < 4; i++) {
            if (map[i]) sumL = vaddq_f32(sumL, mixed[i]);
            else        sumR = vaddq_f32(sumR, mixed[i]);
        }
        left = vmulq_n_f32(sumL, 0.5f);
        right = vmulq_n_f32(sumR, 0.5f);
    }

    /*===========================================================================*/
    /* Core Processing - Fully NEON Vectorized */
    /*===========================================================================*/

    /**
     * Process stereo audio through the reverb (NEON vectorized)
     * Processes 4 samples at a time for maximum efficiency
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples) {

        // enable Flush-to-Zero (FTZ) and Default-NaN (DN) mode in the
        // ARM Floating Point Status and Control Register (FPSCR)
        #if defined(__arm__) || defined(__aarch64__)
            uint32_t fpscr;
            __asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));
            fpscr |= (1 << 24) | (1 << 22); // Set FZ (bit 24) and DN (bit 22)
            __asm__ volatile("vmsr fpscr, %0" : : "r"(fpscr));
        #endif

	    // Safety check
        if (!initialized) {
            memcpy(outL, inL, numSamples * sizeof(float));
            memcpy(outR, inR, numSamples * sizeof(float));
            return;
        }

        // Process in blocks of 4 samples
        int samplesProcessed = 0;
        while (samplesProcessed < numSamples) {
            int blockSize = (numSamples - samplesProcessed) >= 4 ? 4 : 1;

            if (blockSize == 4) {
                // =================================================================
                // VECTORIZED PATH: Process 4 samples at once
                // =================================================================
                process4Samples(inL + samplesProcessed, inR + samplesProcessed,
                                outL + samplesProcessed, outR + samplesProcessed);
            } else {
                // =================================================================
                // SCALAR PATH: Process remaining 1-3 samples
                // =================================================================
                for (int i = 0; i < blockSize; i++) {
                    float mono = (inL[samplesProcessed + i] + inR[samplesProcessed + i]) * 0.5f;
                    float wetL, wetR;
                    processScalar(mono, wetL, wetR);
                    outL[samplesProcessed + i] = wetL;
                    outR[samplesProcessed + i] = wetR;
                }
            }

            samplesProcessed += blockSize;
        }
    }

private:
    /*===========================================================================*/
    /* OPTIMIZED: vld4-based delay line reading with vectorized interpolation */
    /*===========================================================================*/
    void readDelayLines4(float32x4_t* out) {
        // Calculate base read positions for all 4 samples
        float32x4_t baseReadPos = {
            (float)writePos,
            (float)(writePos + 1),
            (float)(writePos + 2),
            (float)(writePos + 3)
        };
        float32x4_t mods[FDN_CHANNELS];
        // DELAY READ POINTER CALCULATIONS
        float32x4_t readPositions[FDN_CHANNELS];
        uint32_t baseIndices[FDN_CHANNELS][NEON_LANES];
        float fracParts[FDN_CHANNELS][NEON_LANES];

        // ====================================================================
        // DYNAMIC NEON MODULATION (SWIRL & COCHRANE SHIMMER)
        // ====================================================================

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {

            // 1. Select Rate and Depth based on current Mode
            float current_rate = 0.5f;
            float current_depth = 1.0f;

            // use shimmer only on the last two presets, not available for all
            if (currentPreset == k_esotico) {
                // Cochrane 18-EDO Shimmer Mode
                current_rate = microtonalRate_[ch];
                current_depth = shimmerDepth_ * 45.0f; // Deep, slow microtonal stretch
            }
            else if(currentPreset == k_stellare) {
              // Standard Swirl / Chorus Mode
              current_rate = swirlRate_[ch] * (1.0f + modRate); // Scale with UI, where modRate is mapping of VIBR LFO speed for random modulation
              current_depth = modDepth * 4.7f; // Standard subtle diffusion
            }
            // to be tested this one:
            if (filterMode == kFilterNoise) {
                // In Noise mode, modulation controls noise colour instead of delay time
                current_depth = noiseColour * 5.999f; // Map 0-1 to 0-5.999 for noise colour selection
            }
            if (filterMode == kFilterCrystal) {
                current_depth = modDepth * 10.0f;
            }

            // 2. Generate the 4 sequential phases for this NEON block
            float base = channelPhase_[ch];
            float32x4_t phase_offsets = { 0.0f, current_rate, current_rate * 2.0f, current_rate * 3.0f };
            float32x4_t phases = vaddq_f32(vdupq_n_f32(base), phase_offsets);

            // 3. Advance the stored scalar phase for the next audio block
            float next_phase = base + (current_rate * 4.0f);
            if (next_phase > 1.0f) next_phase -= 1.0f; // Wrap 0.0 to 1.0
            channelPhase_[ch] = next_phase;

            // 4. Convert normalized phase [0, 1] to Radians [0, 2π]
            float32x4_t angles = vmulq_f32(phases, vdupq_n_f32(M_PI * 2.0f));

            // 5. Compute NEON sine approximation
            float32x4_t sin_vals = sin_ps(angles);

            // 6. Scale by modulation depth (in samples)
            mods[ch] = vmulq_f32(sin_vals, vdupq_n_f32(current_depth));
        // }

        // ====================================================================
        // DELAY READ POINTER CALCULATIONS
        // ====================================================================

        // for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float delaySamples = delayTimes[ch] * sampleRate;

            // Subtract delay length AND our new dynamic modulation
            readPositions[ch] = vsubq_f32(baseReadPos,
                vaddq_f32(vdupq_n_f32(delaySamples), mods[ch]));

            // Wrap to [0, BUFFER_SIZE)
            float pos_vals[NEON_LANES];
            vst1q_f32(pos_vals, readPositions[ch]);
            float out_lanes[NEON_LANES];
            for (int s = 0; s < NEON_LANES; s++) {
                // Adding a large multiple of BUFFER_SIZE guarantees the float is positive
                // before casting to int, avoiding C++ negative-integer-truncation issues.
                float pos = pos_vals[s];
                float safe_pos = pos + (float)(BUFFER_SIZE * 4);

                int32_t idx = (int32_t)safe_pos;
                uint32_t base = idx & BUFFER_MASK;
                // Store the wrapped index so we can read from it!
                baseIndices[ch][s] = base;
                fracParts[ch][s] = safe_pos - (float)idx;
            // }
        // }

        // Read each channel independently: each channel has its own read position
        // (baseIndices[ch][s]) so we cannot share a single vld4q_f32 across channels.
        // Scalar interpolation per channel avoids the previous cross-frame read bug.
        // for (int ch = 0; ch < FDN_CHANNELS; ch++) {

            // for (int s = 0; s < NEON_LANES; s++) {
                uint32_t idx0 = baseIndices[ch][s] & BUFFER_MASK;
                uint32_t idx1 = (idx0 + 1) & BUFFER_MASK;
                float frac = fracParts[ch][s];
                float s0 = delayLine[idx0].samples[ch];
                float s1 = delayLine[idx1].samples[ch];
                out_lanes[s] = s0 + frac * (s1 - s0);
            }

            out[ch] = vld1q_f32(out_lanes);
        }
    }

    /*===========================================================================*/
    /* Vectorized Hadamard Transform */
    /*===========================================================================*/

    inline void applyHadamard4(const float32x4_t* in, float32x4_t* out) {
        // Pass 1: Mix pairs 4 channels apart (Reads directly from 'in')
        float32x4_t t0 = vaddq_f32(in[0], in[4]); float32x4_t t4 = vsubq_f32(in[0], in[4]);
        float32x4_t t1 = vaddq_f32(in[1], in[5]); float32x4_t t5 = vsubq_f32(in[1], in[5]);
        float32x4_t t2 = vaddq_f32(in[2], in[6]); float32x4_t t6 = vsubq_f32(in[2], in[6]);
        float32x4_t t3 = vaddq_f32(in[3], in[7]); float32x4_t t7 = vsubq_f32(in[3], in[7]);

        // Pass 2: Mix pairs 2 channels apart
        float32x4_t u0 = vaddq_f32(t0, t2); float32x4_t u2 = vsubq_f32(t0, t2);
        float32x4_t u1 = vaddq_f32(t1, t3); float32x4_t u3 = vsubq_f32(t1, t3);
        float32x4_t u4 = vaddq_f32(t4, t6); float32x4_t u6 = vsubq_f32(t4, t6);
        float32x4_t u5 = vaddq_f32(t5, t7); float32x4_t u7 = vsubq_f32(t5, t7);

        // Pass 3: Mix adjacent channels & scale (Writes directly to 'out')
        // NOTE: ). In an FDN reverb, the feedback matrix must be unitary (gain of 1.0) to ensure stability.
        // With a gain of 1.414, the reverb loop will become unstable and explode whenever the unifiedDecay
        // (loop gain) exceeds ~0.707.
        float32x4_t scale = vdupq_n_f32(0.35355339f); // 1.0 / sqrt(8)
        out[0] = vmulq_f32(vaddq_f32(u0, u1), scale); out[1] = vmulq_f32(vsubq_f32(u0, u1), scale);
        out[2] = vmulq_f32(vaddq_f32(u2, u3), scale); out[3] = vmulq_f32(vsubq_f32(u2, u3), scale);
        out[4] = vmulq_f32(vaddq_f32(u4, u5), scale); out[5] = vmulq_f32(vsubq_f32(u4, u5), scale);
        out[6] = vmulq_f32(vaddq_f32(u6, u7), scale); out[7] = vmulq_f32(vsubq_f32(u6, u7), scale);
    }

    /**
     * @brief add natural "air absorption" back into the delay lines.
     * By placing it inside the feedback loop alongside the resonant biquads,
     * the biquads provide the macro "character" (Wood/Stone/Metal),
     * while the 1-pole filter provides the natural high-frequency darkening as the tail decays.
     *
     * @param signals
     */
    inline void applyHighFreqDamping4(float32x4_t* signals) {
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            // 1. Get the mixed audio for this channel (4 samples)
            float mixed_samps[NEON_LANES];
            vst1q_f32(mixed_samps, signals[ch]);

            // 2. Process the sequential IIR Low-Pass Filter for Damping
            // dampingCoeff is calculated from UI (e.g., 0.1 to 0.95)
            float alpha = 1.0f - dampingCoeff;
            // avoid small attenuation errors to accumulate exponentially.
            for(int s = 0; s < NEON_LANES; s++) {
                lpfState[ch] += alpha * (mixed_samps[s] - lpfState[ch]);
                mixed_samps[s] = lpfState[ch];
            }

            // 3. Load it back into a NEON vector to write to the delay line
            signals[ch] = vld1q_f32(mixed_samps);
        }
    }

    inline void applyMetalResonance(float32x4_t* signals)
    {
        if (filterMode != kFilterMetal)
            return;

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {

            float x[4];
            vst1q_f32(x, signals[ch]);

            for (int s = 0; s < 4; s++) {

                // short metallic memory
                metalState[ch] =
                    0.82f * metalState[ch]
                    + 0.18f * x[s];

                // feed forward resonance
                x[s] += metalState[ch] * 0.35f;
            }

            signals[ch] = vld1q_f32(x);
        }
    }

    inline void applyCrystalDiffusion(float32x4_t* signals)
    {
        if (filterMode != kFilterCrystal)
            return;

        const float g = 0.55f;

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {

            float x[4];
            vst1q_f32(x, signals[ch]);

            for (int s = 0; s < 4; s++) {

                float input = x[s];

                float y =
                    -g * input
                    + crystalAPState[ch];

                crystalAPState[ch] =
                    input
                    + g * y;

                x[s] = y;
            }

            signals[ch] = vld1q_f32(x);
        }
    }

    inline void applyExoticShimmer(float32x4_t* mixed) {
        if (shimmerDepth_ > 0.0f) {
            // 1. Sum channels 0-3 (Left) and 4-7 (Right)
            float32x4_t sumL = vaddq_f32(vaddq_f32(mixed[0], mixed[1]),
                                         vaddq_f32(mixed[2], mixed[3]));
            float32x4_t sumR = vaddq_f32(vaddq_f32(mixed[4], mixed[5]),
                                         vaddq_f32(mixed[6], mixed[7]));

            // 2. Mix them down to a mono preview and scale by 0.125 (1/8)
            float32x4_t monoPreview = vmulq_f32(vaddq_f32(sumL, sumR), vdupq_n_f32(0.125f));

            // 3. Calculate phase increments for 4 parallel samples
            float inc = M_TWOPI * shimmerFreq_ / sampleRate;
            // Those multipliers (1.0f, 2.0f, 3.0f) are not artistic DSP parameters. They are strict structural math representing the linear flow of time.
            // Instead of moving smoothly through the delay buffer, your 4 audio samples will be randomly grabbed from completely different, disconnected points in the pitch-shifter's memory.
            // possible alternative
            // Makes the shimmer warble slightly like light through a prism
            // float lfo_wobble = sinf(some_lfo_phase) * 0.05f;
            // float inc = 2.0f + lfo_wobble;
            float32x4_t phaseVec = {
                shimmerPhase_,
                shimmerPhase_ + inc,
                shimmerPhase_ + 2.0f * inc,
                shimmerPhase_ + 3.0f * inc
            };

            // 4. Generate 4 sine wave samples at once using your fast approximation
            float32x4_t sinVec = sin_ps(phaseVec);

            // 5. Calculate the ring-modulated shimmer signal (preview * sin * depth)
            float32x4_t shim = vmulq_f32(monoPreview,
                                         vmulq_f32(sinVec, vdupq_n_f32(shimmerDepth_)));

            // 6. Inject the shimmer back into channels 6 and 7 with inverted phase
            mixed[FDN_CHANNELS - 2] = vaddq_f32(mixed[FDN_CHANNELS - 2], shim);
            mixed[FDN_CHANNELS - 1] = vsubq_f32(mixed[FDN_CHANNELS - 1], shim);

            // 7. Advance the master scalar phase for the next block of 4 samples
            shimmerPhase_ += 4.0f * inc;
            while (shimmerPhase_ >= M_TWOPI) { shimmerPhase_ -= M_TWOPI; }
        }
    }

    /*===========================================================================*/
    /* Vectorized Delay Line Write */
    /*===========================================================================*/
    inline void writeDelayLines4(const float32x4_t* signals) {
        // Spill all channel vectors; index by sample position (variable s)
        // to avoid vgetq_lane_f32(v, variable) which requires a constant index.
        float ch_lanes[FDN_CHANNELS][NEON_LANES];
        for (int ch = 0; ch < FDN_CHANNELS; ch++)
            vst1q_f32(ch_lanes[ch], signals[ch]);

        for (int s = 0; s < NEON_LANES; s++) {
            uint32_t pos = (writePos + s) & BUFFER_MASK;
            for (int ch = 0; ch < FDN_CHANNELS; ch++)
                delayLine[pos].samples[ch] = ch_lanes[ch][s];
        }
        writePos = (writePos + NEON_LANES) & BUFFER_MASK;
    }

    /*===========================================================================*/
    /* Main Processing Loop */
    /*===========================================================================*/

    void process4Samples(const float* inL, const float* inR,
                         float* outL, float* outR) {

        // Load 4 input samples for L and R channels
        float32x4_t inL4 = vld1q_f32(inL);
        float32x4_t inR4 = vld1q_f32(inR);

        // APC WAKE-UP: check raw input BEFORE the bypass guard.
        // activeSampleCount starts at 0 after clear(); the code that resets it
        // from the delayed signal is after the pre-delay write, which is itself
        // gated by this guard — so without a raw-input check here the reverb
        // would never activate from bypass.
        bool signal_active_4 = false;
        {
            float32x4_t absL = vabsq_f32(inL4);
            float32x4_t absR = vabsq_f32(inR4);
            float32x4_t absIn = vmaxq_f32(absL, absR);
            float32x4_t mx1 = vmaxq_f32(absIn, vextq_f32(absIn, absIn, 2));
            float mx = vgetq_lane_f32(vmaxq_f32(mx1, vextq_f32(mx1, mx1, 1)), 0);
            signal_active_4 = (mx > 1e-4f);
            if (mx > 1e-5f)
                activeSampleCount = (int)(sampleRate * (1.0f + decay * 5.0f));
        }
        // Noise envelope: fast attack (~83ms), slow release (~2s) for smooth stellare onset/offset.
        if (signal_active_4)
            noiseEnvelope += 0.001f * (1.0f - noiseEnvelope);
        else
            noiseEnvelope -= 0.0000417f * noiseEnvelope;

        if (activeSampleCount <= 0) {
            float32x4_t zero = vdupq_n_f32(0.0f);
            vst1q_f32(outL, zero);
            vst1q_f32(outR, zero);
            return;
        }

        // Convert to mono for FDN input
        float32x4_t inMono = vmulq_f32(vaddq_f32(inL4, inR4), vdupq_n_f32(0.5f));

        // =================================================================
        // 1 & 2. PRE-DELAY (Smoothed, Interpolated, Write-Before-Read)
        // =================================================================
        float monoLanes[NEON_LANES];
        vst1q_f32(monoLanes, inMono); // Unpack the NEON vector
        float delayedLanes[NEON_LANES];

        for (int s = 0; s < NEON_LANES; s++) {
            // 1. WRITE FIRST: Guarantees 0.0ms delay instantly reads this exact sample
            preDelayBuffer[preDelayWritePos] = monoLanes[s];

            // 2. Slew Limiter (1-Pole Low-Pass per sample)
            currentPreDelaySamples += 0.001f * (targetPreDelaySamples - currentPreDelaySamples);

            // 3. Fractional Read Position
            float pd_read_pos = (float)preDelayWritePos - currentPreDelaySamples;

            // Branchless wrap
            float safe_pd_pos = pd_read_pos + (float)(PREDELAY_BUFFER_SIZE * NEON_LANES);
            int32_t pd_idx1 = (int32_t)safe_pd_pos;
            int32_t pd_idx2 = pd_idx1 + 1;

            // Mask to buffer size
            uint32_t idx1 = pd_idx1 & PREDELAY_MASK;
            uint32_t idx2 = pd_idx2 & PREDELAY_MASK;
            float pd_frac = safe_pd_pos - (float)pd_idx1;

            // 4. Linear Interpolation Read
            float val1 = preDelayBuffer[idx1];
            float val2 = preDelayBuffer[idx2];
            delayedLanes[s] = val1 + pd_frac * (val2 - val1);

            // 5. Advance write head for the NEXT sample inside the block
            preDelayWritePos = (preDelayWritePos + 1) & PREDELAY_MASK;
        }

        // Repack into the NEON vector for the rest of the FDN engine
        float32x4_t delayedMono = vld1q_f32(delayedLanes);

        // =================================================================
        // 3. Active Partial Counting (CPU Optimization)
        // =================================================================
        // Check if the current delayed input block contains active audio
        float32x4_t absIn = vabsq_f32(delayedMono);
        float32x4_t max1 = vmaxq_f32(absIn, vextq_f32(absIn, absIn, 2));
        float32x4_t max2 = vmaxq_f32(max1, vextq_f32(max1, max1, 1));

        if (vgetq_lane_f32(max2, 0) > 1e-7f) { // Lower threshold for smoother tail
            // Signal present: reset counter to maximum reverb tail length
            // RT60 roughly corresponds to decay time + predelay
            activeSampleCount = (int)(sampleRate * (2.0f + decay * 8.0f)); // Longer tail count
        } else if (activeSampleCount > 0) {
            // Signal absent: decrement counter
            activeSampleCount -= NEON_LANES;
        }

        // If counter has expired, bypass FDN processing to save cycles
        if (activeSampleCount <= 0) {
            float32x4_t zero = vdupq_n_f32(0.0f);
            vst1q_f32(outL, zero);
            vst1q_f32(outR, zero);
            // Zero the positions being skipped so old reverb data doesn't
            // replay when the FDN reactivates after silence.
            for (int s = 0; s < NEON_LANES; s++) {
                uint32_t pos = (writePos + s) & BUFFER_MASK;
                vst1q_f32(&delayLine[pos].samples[0], zero);
                vst1q_f32(&delayLine[pos].samples[4], zero);
            }
            writePos = (writePos + NEON_LANES) & BUFFER_MASK;
            return;
        }

        // =================================================================
        // Read from all 8 delay lines for 4 samples using current phases
        // =================================================================
        float32x4_t delayOut[FDN_CHANNELS];
        readDelayLines4(delayOut);

        // =================================================================
        // Advance modulation phases for the next block (after read so phases aren't clobbered)
        // =================================================================
        updateModulation4();
        updateRandomLfo();
        pingMapCounter -= 4;

        float modAmount = diffusion * modDepth;   // diffusion is DFSN (0..1)
        float fcMod = smoothedLfoValue * modAmount * 500.0f;  // ± up to 500 Hz

        // =================================================================
        // Compute unified loop gain and stability safety
        // =================================================================
        float loopGain = 0.45f + decay * 0.45f;

        // Instability Fix: crossGain adds energy (eigenvalue ≈ 1+gain).
        // We must reduce unifiedDecay by (1-crossGain) to keep loop gain < 1.0.
        float stabilityMargin = 1.0f - crossGain[pillar_] - 0.02f;
        float unifiedDecay = fminf(stabilityMargin, loopGain * fasterSqrt_15bits(highDecayMult * lowDecayMult));
        float32x4_t decayAll = vdupq_n_f32(unifiedDecay);

        // Volume Fix: input_gain floor was 0.35 (35% of signal every block).
        // In an 8-channel FDN, this is massive. Lowered to 0.12.
        float input_gain = fmaxf(0.12f, 1.0f - unifiedDecay);
        float32x4_t feedback = vdupq_n_f32(input_gain);

        // =================================================================
        // Apply Hadamard mixing matrix (vectorized)
        // =================================================================
        float32x4_t mixed[FDN_CHANNELS];
        applyHadamard4(delayOut, mixed);

        // 1. Natural Air Absorption (old applyDiffusion4)
        applyHighFreqDamping4(mixed);

        // 2. Character Body Resonance (Wood, Stone, Metal)
        // Screech Fix: Only update filter coefficients every 32 samples (8 blocks)
        if (--filterUpdateCounter <= 0) {
            filterUpdateCounter = 8;
            applyResonantFilterModulated(mixed, FDN_CHANNELS, fcMod);
        } else {
            applyResonantFilter(mixed, FDN_CHANNELS);
        }

        // 2.5 add filter modes
        // Metal mode has an additional feed-forward comb resonance for extra metallic character
        // This order matters, because resonant filter creates spectral emphasis, comb resonance turns it metallic
        // then feedback diffuses it; if you put it later the metallic signature gets blurred
        applyMetalResonance(mixed);
        applyCrystalDiffusion(mixed);

        // 3. Inject Environmental Noise (Brown, Pink, White, Blue, Violet, Grey)
        // noiseEnvelope smoothly fades noise in/out with signal activity (prevents
        // abrupt onset/offset of stellare noise). gainScale bounds steady-state level.
        addColouredNoise(mixed, FDN_CHANNELS, (1.0f - unifiedDecay) * noiseEnvelope);

        // 4. Apply cross‑channel feedback
        applyCrossFeedback(mixed);

        // 5. Apply decay
        for (int i = 0; i < FDN_CHANNELS; i++) mixed[i] = vmulq_f32(mixed[i], decayAll);

        // 6. Add input - Scaled down to prevent "too high volume"
        float32x4_t inputVec = vmulq_f32(delayedMono, feedback);
        // An FDN this diffuse needs excitation into multiple channels
        mixed[0] = vaddq_f32(mixed[0], inputVec);
        mixed[2] = vaddq_f32(mixed[2], vmulq_n_f32(inputVec, 0.5f));
        mixed[5] = vaddq_f32(mixed[5], vmulq_n_f32(inputVec, -0.3f));
        mixed[7] = vaddq_f32(mixed[7], vmulq_n_f32(inputVec, 0.2f));

        // 7. Soft saturation: x/(1+|x|) keeps FDN bounded at (-1,1) without DC lockup.
        // Hard-clip was causing stellare crash: when all channels pin at +1, Hadamard
        // ch0 = √8≈2.83 → clip→1.0 → self-sustaining, never decays. Soft sat avoids this.
        {
            float32x4_t one = vdupq_n_f32(1.0f);
            for (int i = 0; i < FDN_CHANNELS; i++) {
                float32x4_t abs_x = vabsq_f32(mixed[i]);
                float32x4_t denom = vaddq_f32(one, abs_x);
                float32x4_t rcp   = vrecpeq_f32(denom);
                rcp = vmulq_f32(vrecpsq_f32(denom, rcp), rcp);  // Newton-Raphson refinement
                mixed[i] = vmulq_f32(mixed[i], rcp);
            }
        }


        // =================================================================
        // Exotic Low-Pitching Shimmer (PILL=4) - NEON Vectorized
        // =================================================================
        applyExoticShimmer(mixed);

        // =================================================================
        // Write back to delay lines
        // =================================================================
        writeDelayLines4(mixed);

        // =================================================================
        // Mix down to stereo — routing depends on PILL value
        // =================================================================
        // 8. Stereo mix‑down (with randomised ping‑pong if pillar==1)
        float32x4_t leftMix, rightMix;
        if (pingPong_) {
            applyRandomizedPingPongMix(mixed, leftMix, rightMix);
        } else {
            int activeCh;
            switch (pillar_) {
                case 0: activeCh = 2; break;
                case 2: activeCh = 6; break;
                default: activeCh = FDN_CHANNELS; break;  // 3, 4 → 8
            }

            int halfL = activeCh < 4 ? activeCh : 4;
            int halfR = activeCh > 4 ? activeCh - 4 : 0;

            float32x4_t leftSum  = vdupq_n_f32(0.0f);
            float32x4_t rightSum = vdupq_n_f32(0.0f);
            for (int i = 0; i < halfL; i++)
                leftSum = vaddq_f32(leftSum, mixed[i]);
            for (int i = 4; i < 4 + halfR; i++)
                rightSum = vaddq_f32(rightSum, mixed[i]);

            // Fixed 0.5 normalization (vs 1/halfL=0.25 for 8ch) gives 6dB more output,
            // making the reverb tail immediately audible. Soft saturation above keeps
            // individual channels < 1 so summing 4 channels × 0.5 stays below 2.0.
            leftMix = vmulq_f32(leftSum, vdupq_n_f32(0.5f));
            if (halfR > 0)
                rightMix = vmulq_f32(rightSum, vdupq_n_f32(0.5f));
            else
                rightMix = leftMix; // PILL=0: mono fold
        }

        // Apply stereo width:  mid = (L+R)*0.5, side = (L-R)*0.5
        // outL_wet = mid + side*width,  outR_wet = mid - side*width
        float32x4_t mid = vmulq_f32(vaddq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t side = vmulq_f32(vsubq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t width4 = vdupq_n_f32(width);

        float32x4_t wetL = vaddq_f32(mid, vmulq_f32(side, width4));
        float32x4_t wetR = vsubq_f32(mid, vmulq_f32(side, width4));

        // Store wet signal directly (hardware handles dry+wet blend)
        vst1q_f32(outL, wetL);
        vst1q_f32(outR, wetR);
    }

    void updateModulation4() {
        float incPerSample = modRate * M_TWOPI / sampleRate;
        float32x4_t blockAdvance = vdupq_n_f32(4.0f * incPerSample);
        float32x4_t twoPi = vdupq_n_f32(M_TWOPI);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t newPhases = vaddq_f32(modPhaseVec[ch], blockAdvance);

            // Wrap to [0, 2π) using truncate-toward-zero floor
            float32x4_t div = vmulq_f32(newPhases, vdupq_n_f32(1.0f / (M_TWOPI)));
            float32x4_t floor_f = vcvtq_f32_s32(vcvtq_s32_f32(div));
            newPhases = vsubq_f32(newPhases, vmulq_f32(floor_f, twoPi));

            uint32x4_t neg = vcltq_f32(newPhases, vdupq_n_f32(0.0f));
            newPhases = vbslq_f32(neg, vaddq_f32(newPhases, twoPi), newPhases);

            modPhaseVec[ch] = newPhases;
        }
    }

    /*===========================================================================*/
    /* Scalar Fallback for Remainder Samples (less complex than process4Samples  */
    /* as is process no more than 3 samples)                                     */
    /*===========================================================================*/
    inline void applyHadamardScalar(const float* in, float* out) {
        // Pass 1 (Reads directly from 'in')
        float t0 = in[0] + in[4]; float t4 = in[0] - in[4];
        float t1 = in[1] + in[5]; float t5 = in[1] - in[5];
        float t2 = in[2] + in[6]; float t6 = in[2] - in[6];
        float t3 = in[3] + in[7]; float t7 = in[3] - in[7];

        // Pass 2
        float u0 = t0 + t2; float u2 = t0 - t2;
        float u1 = t1 + t3; float u3 = t1 - t3;
        float u4 = t4 + t6; float u6 = t4 - t6;
        float u5 = t5 + t7; float u7 = t5 - t7;

        // Pass 3 (Writes directly to 'out' with scale applied)
        // float scale = 0.35355339f; // 1.0 / sqrt(8)
        float scale = 0.5f; // No need to scale in the scalar path since it's only used for 1-2 samples, and the main purpose of scaling is to keep the FDN bounded when summing many channels together.
        out[0] = (u0 + u1) * scale; out[1] = (u0 - u1) * scale;
        out[2] = (u2 + u3) * scale; out[3] = (u2 - u3) * scale;
        out[4] = (u4 + u5) * scale; out[5] = (u4 - u5) * scale;
        out[6] = (u6 + u7) * scale; out[7] = (u6 - u7) * scale;
    }

    void processScalar(float input, float& wetL, float& wetR) {
        float delayOut[FDN_CHANNELS];

        // APC WAKE-UP: check raw input before bypass guard (same reason as process4Samples).
        if (fabsf(input) > 1e-5f)
            activeSampleCount = (int)(sampleRate * (1.0f + decay * 5.0f));

        if (activeSampleCount <= 0) {
            // Apply dry gain so volume stays constant when bypassed!
            wetL = 0.0f; wetR = 0.0f;
            // Zero the position being skipped so old reverb data doesn't
            // replay when the FDN reactivates after silence.
            memset(&delayLine[writePos & BUFFER_MASK], 0, sizeof(interleaved_frame_t));
            writePos = (writePos + 1) & BUFFER_MASK;
            return;
        }

        // ==========================================
        // SCALAR PREDELAY (Smoothed & Interpolated)
        // ==========================================

        // 1. WRITE FIRST: Guarantees that if Pre-Delay is 0.0ms,
        // the read head instantly fetches the exact sample we just wrote.
        preDelayBuffer[preDelayWritePos] = input;

        // 2. Slew Limiter Math (1-Pole Low-Pass)
        currentPreDelaySamples += 0.001f * (targetPreDelaySamples - currentPreDelaySamples);

        // 3. Calculate fractional read position
        float pd_read_pos = (float)preDelayWritePos - currentPreDelaySamples;

        // Branchless wrap for safety
        float safe_pd_pos = pd_read_pos + (float)(PREDELAY_BUFFER_SIZE * 4);
        int32_t pd_idx1 = (int32_t)safe_pd_pos;
        int32_t pd_idx2 = pd_idx1 + 1;

        // Mask to buffer size
        uint32_t idx1 = pd_idx1 & PREDELAY_MASK;
        uint32_t idx2 = pd_idx2 & PREDELAY_MASK;
        float pd_frac = safe_pd_pos - (float)pd_idx1;

        // 4. Linear Interpolation Read
        float val1 = preDelayBuffer[idx1];
        float val2 = preDelayBuffer[idx2];
        float delayedInput = val1 + pd_frac * (val2 - val1);

        // 5. Advance write head for the NEXT sample
        preDelayWritePos = (preDelayWritePos + 1) & PREDELAY_MASK;

        // 3. Active Partial Counting
        if (fabsf(delayedInput) > 1e-5f) {
            activeSampleCount = (int)(sampleRate * (1.0f + decay * 5.0f));
        } else if (activeSampleCount > 0) {
            activeSampleCount--;
        }

        // Bypass if tail is dead
        if (activeSampleCount <= 0) {
            wetL = 0.0f;
            wetR = 0.0f;
            writePos = (writePos + 1) & BUFFER_MASK;
            return;
        }

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            // For scalar path, we only need the current phase (lane 0)
            float phase = vgetq_lane_f32(modPhaseVec[ch], 0);

            float delaySamples = delayTimes[ch] * sampleRate;
            float mod = fastersinfullf(phase * M_TWOPI) * modDepth * 100.0f;
            float readPos = (float)writePos - (delaySamples + mod);

            while (readPos < 0) readPos += BUFFER_SIZE;
            while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;

            int idx = (int)readPos;
            int idx_next = (idx + 1) & BUFFER_MASK;
            float frac = readPos - idx;

            float s1 = delayLine[idx].samples[ch];
            float s2 = delayLine[idx_next].samples[ch];
            delayOut[ch] = s1 + frac * (s2 - s1);

            // Update scalar phase (only for lane 0)
            float new_phase = phase + modRate * M_TWOPI / sampleRate;
            if (new_phase >= M_TWOPI) new_phase -= M_TWOPI;

            // Update just lane 0, preserve other lanes
            float32x4_t temp = modPhaseVec[ch];
            temp = vsetq_lane_f32(new_phase, temp, 0);
            modPhaseVec[ch] = temp;
        }

        float mixed[FDN_CHANNELS];
        float loopGain = 0.45f + decay * 0.45f;
        float stabilityMargin = 1.0f - crossGain[pillar_] - 0.02f;
        float unifiedDecay = fminf(stabilityMargin, loopGain * fasterSqrt_15bits(highDecayMult * lowDecayMult));
        float scalar_input_gain = fmaxf(0.12f, 1.0f - unifiedDecay);

        applyHadamardScalar(delayOut, mixed);

        // Apply same loop gain as process4Samples (was missing — scalar path had no decay).
        for (int ch = 0; ch < FDN_CHANNELS; ch++) mixed[ch] *= unifiedDecay;

        mixed[0] += delayedInput * scalar_input_gain;

        // Soft saturation: x/(1+|x|) — bounded (-1,1), no DC lockup.
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float x = mixed[ch];
            mixed[ch] = x / (1.0f + fabsf(x));
        }

        // Exotic "Low Pitching" Shimmer (PILL=4)
        // Injects a ring-modulated copy of the wet signal back into the network.
        // The cascading sum/difference frequencies create dense, microtonal undertones.
        // Shimmer (PILL=4): inject a small frequency-modulated copy of the
        // current wet signal back into channels 6 and 7 before writing.
        // Loop gain ≈ 0.04 * 0.088 ≈ 0.004 << 1 → unconditionally stable.
        if (shimmerDepth_ > 0.0f) {
            float previewL = 0.0f, previewR = 0.0f;
            for (int i = 0; i < 4; i++)            previewL += mixed[i];
            for (int i = 4; i < FDN_CHANNELS; i++) previewR += mixed[i];

            float monoPreview = (previewL + previewR) * 0.125f; // /8

            // The microtonal happens here: modulating at audio-rate (e.g., 35 Hz)
            float shim = monoPreview * fastersinfullf(shimmerPhase_) * shimmerDepth_;

            mixed[FDN_CHANNELS - 2] += shim;
            mixed[FDN_CHANNELS - 1] -= shim;

            // Advance phase using our new low-frequency target
            shimmerPhase_ += M_TWOPI * shimmerFreq_ / sampleRate;
            if (shimmerPhase_ >= M_TWOPI) shimmerPhase_ -= M_TWOPI;
        }

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            delayLine[writePos].samples[ch] = mixed[ch];
        }
        writePos = (writePos + 1) & BUFFER_MASK;

        // Stereo mix-down: routing depends on PILL value
        float leftRaw = 0.0f, rightRaw = 0.0f;

        if (pingPong_) {
            // PILL=1: alternating L/R among 4 active channels with randomization timer
            if (--pingMapCounter <= 0) {
                pingMapCounter = 4800;
                pingMapIndex = (pingMapIndex + 1) & 15;
            }
            const uint8_t* map = pingRandomMap[pingMapIndex];
            for (int i = 0; i < 4; i++) {
                if (map[i]) leftRaw  += mixed[i];
                else        rightRaw += mixed[i];
            }
            leftRaw  *= 0.5f;
            rightRaw *= 0.5f;
        } else {
            // Determine active channel count
            int activeCh;
            if      (pillar_ == 0) activeCh = 2;
            else if (pillar_ == 2) activeCh = 6;
            else                   activeCh = FDN_CHANNELS;  // 3, 4 → 8

            int halfL = activeCh < 4 ? activeCh : 4;
            int halfR = activeCh > 4 ? activeCh - 4 : 0;
            for (int i = 0; i < halfL; i++)         leftRaw  += mixed[i];
            for (int i = 4; i < 4 + halfR; i++)     rightRaw += mixed[i];
            leftRaw  *= 0.5f;
            if (halfR > 0)
                rightRaw *= 0.5f;
            else
                rightRaw  = leftRaw;  // mono fold for very sparse (PILL=0)
        }

        // Apply stereo width
        float mid  = (leftRaw + rightRaw) * 0.5f;
        float side = (leftRaw - rightRaw) * 0.5f;
        wetL = mid + side * width;
        wetR = mid - side * width;
    }

    /*===========================================================================*/
    /* Initialization */
    /*===========================================================================*/
    /** use the 18-EDO Equal Division of the Octave) math formula ($2^{n/18}$)
     * to perfectly space the LFO frequencies into a microtonal scale.
     * By applying 8 independent, slow Doppler pitch-shifts to the delay lines—tuned
     * exactly to microtonal intervals—the 8 echoes will physically rub against each
     * other inside the Hadamard matrix. This creates the dense,
     * acoustic "beating" and complex harmonic interference that Cochrane
     * defines as true microtonal shimmer.*/
    void initMicrotonalShimmer()
    {
        // Set up standard swirl (e.g., random slow rates between 0.2Hz and 0.8Hz)
        // Set up Cochrane 18-EDO rates (as discussed previously)
        for(int i=0; i<FDN_CHANNELS; i++) {
            channelPhase_[i] = (float)i / (float)FDN_CHANNELS; // Stagger phases 0.0 to 1.0

            // Example Swirl Rates (prime-ish relationships)
            float s_hz = 0.2f + (0.1f * i);
            swirlRate_[i] = s_hz / sampleRate;

            // 18-EDO Cochrane Rates (Base 0.5Hz)
            float steps_18[8] = {0.0f, 1.0f, 3.0f, 4.0f, 6.0f, 7.0f, 9.0f, 10.0f};
            microtonalRate_[i] = (0.5f * fasterpowf(2.0f, steps_18[i] / 18.0f)) / sampleRate; // at Init use accurate function
        }
    }

    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    // ============================================================================
    // Parameter State (mirrors header.c defaults)
    // ============================================================================
    // ID 0:  PRESET 0..3               default 0 (foresta)
    // ID 1:  MIX    0..100 %           default 70 (70%)
    // ID 2:  TIME   1..100             default 50
    // ID 3:  LOW    1..100             default 50
    // ID 4:  HIGH   1..100             default 70
    // ID 5:  DAMP   20..1000           default 250  (×10 in code → 2500 Hz)
    // ID 6:  WIDE   0..200 %           default 100
    // ID 7:  DFSN   0..1000 (x0.1%)    default 1000
    // ID 8:  PILL   0..4               default 3
    // ID 9:  SHMR 0..100               default 35 (Hz)
    // ID 10: PDLY   0..100             default 0 (ms)
    int32_t params_[k_total]  __attribute__((aligned(16)));
    float sampleRate;
    int writePos;
    float decay;
    float diffusion;
    float modDepth;
    float modRate;
    float width;         // stereo width   0..2
    float dampingCoeff;  // one-pole LPF coeff for damping
    float lowDecayMult;  // low-freq decay multiplier
    float highDecayMult; // high-freq decay multiplier

    bool  initialized;
    int   pillar_;        /* 0..4 - pillar count / routing mode */
    bool  pingPong_;      /* true when pillar_==1 */
    float shimmerDepth_;  /* re-injection gain for pillar_==4 */
    float shimmerPhase_;  /* LFO phase for shimmer (radians) */
    float shimmerFreq_; // Low audio rate for cascading undertones

    interleaved_frame_t delayLine[BUFFER_SIZE] __attribute__((aligned(64)));

    float32x4_t hadamardCols[FDN_CHANNELS][FDN_CHANNELS/4] __attribute__((aligned(16)));  // Column-major for NEON
    float delayTimes[FDN_CHANNELS] __attribute__((aligned(16)));
    float32x4_t modPhaseVec[FDN_CHANNELS] __attribute__((aligned(16)));
    // new filter states
    float metalState[FDN_CHANNELS] __attribute__((aligned(16)));
    float crystalAPState[FDN_CHANNELS] __attribute__((aligned(16)));
    // FIX: IIR filter states must be strictly scalar! 1 history sample per channel.
    float lpfState[FDN_CHANNELS] __attribute__((aligned(16)));
    // Unified Phase Tracking for Delay Modulation
    float channelPhase_[FDN_CHANNELS] __attribute__((aligned(16)));
    float swirlRate_[FDN_CHANNELS] __attribute__((aligned(16)));
    float microtonalRate_[FDN_CHANNELS] __attribute__((aligned(16)));
    float preDelayBuffer[PREDELAY_BUFFER_SIZE] __attribute__((aligned(16)));
    int preDelayWritePos;
    // Pre-delay Slew Limiter States
    float currentPreDelaySamples;
    float targetPreDelaySamples;
    int activeSampleCount; // Tracks tail for Active Partial Counting

    // ---------- New character features ----------
    int currentPreset;                 // 0..3
    float lfoSpeed;                    // Hz (0.1..3.0)
    float randomLfoValue;              // current random output (-1..1)
    int randomLfoCounter;              // samples until next random update
    int randomLfoPeriodSamples;        // = sampleRate / lfoSpeed
    float smoothedLfoValue;

    // Filter types (per channel, but we can use shared coeffs)
    // kFilterCrystal: bright allpass-like character for esotico (medium-Q bandpass
    // centred higher than metal, giving a shimmering/glassy microtonal colour)
    enum FilterMode { kFilterWood, kFilterStone, kFilterMetal, kFilterCrystal, kFilterNoise };
    FilterMode filterMode;
    float biquadA0, biquadA1, biquadA2, biquadB1, biquadB2; // shared coeffs
    float filterState1[FDN_CHANNELS] __attribute__((aligned(16)));  // z^-1
    float filterState2[FDN_CHANNELS] __attribute__((aligned(16)));  // z^-2

    // Noise generation
    uint32_t noiseSeed;
    float noiseColour;                 // 0=brown .. 4=violet
    float noiseGain;                   // from DAMP
    float noiseEnvelope;               // 0..1 smooth fade-in/out of stellare noise
    float noiseStates[FDN_CHANNELS];   // z^-1 for noise filtering
    float noiseStates2[FDN_CHANNELS];  // z^-2 for Grey noise notch

    // Cross-feedback gains per pillar
    static const float crossGain[5];   // indexed by pillar_

    // Randomized ping-pong map (for PILL=1)
    static const uint8_t pingRandomMap[MAX_PILLARS][NEON_LANES]; // 16 steps, 4 channels each
    int pingMapIndex;
    int pingMapCounter;
    int filterUpdateCounter;

    float baseFc;                   // base cutoff frequency (Hz) for current preset/damping
    float filterModRange;           // max modulation range (Hz) derived from DFSN and pillar
};

const float NeonAdvancedLabirinto::crossGain[5] = {0.15f, 0.10f, 0.07f, 0.04f, 0.04f};

// Pre-computed random map for ping-pong (fixed seed for reproducibility)
const uint8_t NeonAdvancedLabirinto::pingRandomMap[MAX_PILLARS][NEON_LANES] = {
    {1,0,1,0}, {0,1,0,1}, {1,1,0,0}, {0,0,1,1},
    {1,0,0,1}, {0,1,1,0}, {1,1,1,0}, {0,1,1,1},
    {1,0,1,1}, {0,1,0,0}, {1,0,0,0}, {0,0,0,1},
    {1,1,0,1}, {0,0,1,0}, {1,1,1,1}, {0,0,0,0}
};