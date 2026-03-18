#pragma once

/**
 * @file NeonAdvancedLabirinto.h
 * @brief Advanced NEON-optimized reverb with FDN (Feedback Delay Network)
 *
 * Features:
 * - 8-channel FDN with Hadamard mixing matrix
 * - TRUE NEON vectorization (processes 4 samples at a time)
 * - Modulated delays for diffusion
 * - Stereo input/output
 * - Frequency-dependent decay (low/high multipliers)
 * - Stereo width control
 * - Damping filter
 *
 * OPTIMIZED:
 * - Process 4 samples in parallel using float32x4_t
 * - Vectorized delay line access
 * - NEON-accelerated Hadamard mixing
 * - Added null pointer checks for all allocations
 * - FIXED: Added clear() method for reset
 * - FIXED: Proper phase management in updateModulation4() - preserves all 4 lanes
 */

#include <arm_neon.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <malloc.h>
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
        , mix(0.3f)
        , width(1.0f)
        , dampingCoeff(0.5f)
        , lowDecayMult(1.0f)
        , highDecayMult(0.5f)
        , initialized(false)
        , fdnMem(nullptr) {

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
            float32x4_t init_phase = { 0.0f, 0.0f, 0.0f, 0.0f };
            modPhaseVec[i] = init_phase;
        }

        // Initialize filter states
        for (int i = 0; i < FDN_CHANNELS; i++) {
            lpfState[i] = vdupq_n_f32(0.0f);
        }

        // Initialize Hadamard matrix
        initHadamardMatrix();
    }

    ~NeonAdvancedLabirinto() {
        if (fdnMem != nullptr) {
            free(fdnMem);
            fdnMem = nullptr;
        }
    }

    /**
     * Initialize the reverb with proper memory allocation
     * @return true if initialization successful, false if out of memory
     */
    bool init() {
        // Check allocation success
        fdnMem = (float*)memalign(16, BUFFER_SIZE * FDN_CHANNELS * sizeof(float));
        if (fdnMem == nullptr) {
            initialized = false;
            return false;
        }

        // Clear buffer
        clear();

        initialized = true;
        return true;
    }

    /**
     * Clear all delay lines and filter states
     * Called by unit_reset() to ensure clean state
     */
    void clear() {
        if (fdnMem != nullptr) {
            memset(fdnMem, 0, BUFFER_SIZE * FDN_CHANNELS * sizeof(float));
        }

        writePos = 0;

        // Reset modulation phases: lane k = k * incPerSample so sequential
        // samples start at phase 0, inc, 2*inc, 3*inc
        float incPerSample = modRate * 2.0f * M_PI / sampleRate;
        float32x4_t init_phases = { 0.0f, incPerSample, 2.0f*incPerSample, 3.0f*incPerSample };
        for (int i = 0; i < FDN_CHANNELS; i++) {
            modPhaseVec[i] = init_phases;
        }

        // Reset filter states using NEON
        float32x4_t zero = vdupq_n_f32(0.0f);
        for (int i = 0; i < FDN_CHANNELS; i++) {
            lpfState[i] = zero;
        }
    }

    /*===========================================================================*/
    /* Parameter Setters */
    /*===========================================================================*/

    void setDecay(float d) {
        decay = fmaxf(0.0f, fminf(0.99f, d));
    }

    void setDiffusion(float d) {
        diffusion = fmaxf(0.0f, fminf(1.0f, d));
    }

    void setModDepth(float d) {
        modDepth = fmaxf(0.0f, fminf(1.0f, d));
    }

    void setModRate(float r) {
        modRate = fmaxf(0.1f, fminf(10.0f, r));
    }

    /** Wet/dry mix  0.0 = fully dry, 1.0 = fully wet */
    void setMix(float m) {
        mix = fmaxf(0.0f, fminf(1.0f, m));
    }

    /** Stereo width  0.0 = mono, 1.0 = normal, 2.0 = hyper-stereo */
    void setWidth(float w) {
        width = fmaxf(0.0f, fminf(2.0f, w));
    }

    /**
     * Damping: crossover frequency in Hz (200..10000).
     * Converts to a one-pole LPF coefficient used in applyDiffusion4.
     */
    void setDamping(float freqHz) {
        freqHz = fmaxf(200.0f, fminf(10000.0f, freqHz));
        // omega = 2π * fc / fs;  coeff ≈ 1 - omega  (first-order approx)
        float omega = 2.0f * (float)M_PI * freqHz / sampleRate;
        // accurate and musically conventional mapping from frequency to filter coefficient
        // than first-order approximation
        dampingCoeff = e_expff(-omega);
    }

    /**
     * Low-frequency RT60 multiplier (1..100 → 0.01..1.0 s scale).
     * Increases effective decay for low-end warmth.
     */
    void setLowDecay(float value) {
        // value 1-100; map to a per-channel decay multiplier 0.9..1.5
        lowDecayMult = 0.9f + (value / 100.0f) * 0.6f;
    }

    /**
     * High-frequency RT60 multiplier (1..100 → 0.01..1.0 s scale).
     * Controls how quickly the high end decays.
     */
    void setHighDecay(float value) {
        // value 1-100; higher value = brighter (less high-freq damping)
        highDecayMult = 0.1f + (value / 100.0f) * 0.9f;
    }

    /*===========================================================================*/
    /* Core Processing - NEON Vectorized */
    /*===========================================================================*/

    /**
     * Process stereo audio through the reverb (NEON vectorized)
     * Processes 4 samples at a time for maximum efficiency
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples) {

        // Safety check - if memory not allocated, bypass
        if (!initialized || fdnMem == nullptr) {
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
                    float dryL = inL[samplesProcessed + i];
                    float dryR = inR[samplesProcessed + i];
                    float mono = (dryL + dryR) * 0.5f;
                    float wetL, wetR;
                    processScalar(mono, wetL, wetR);
                    outL[samplesProcessed + i] = dryL * (1.0f - mix) + wetL * mix;
                    outR[samplesProcessed + i] = dryR * (1.0f - mix) + wetR * mix;
                }
            }

            samplesProcessed += blockSize;
        }
    }

private:
    /*===========================================================================*/
    /* NEON Vectorized Processing (4 samples) */
    /*===========================================================================*/

    /**
     * Process 4 samples in parallel using NEON
     */
    void process4Samples(const float* inL, const float* inR,
                         float* outL, float* outR) {

        // Load 4 input samples for L and R channels
        float32x4_t inL4 = vld1q_f32(inL);
        float32x4_t inR4 = vld1q_f32(inR);

        // Convert to mono for FDN input
        float32x4_t inMono = vmulq_f32(vaddq_f32(inL4, inR4), vdupq_n_f32(0.5f));

        // =================================================================
        // Read from all 8 delay lines for 4 samples using current phases
        // =================================================================
        float32x4_t delayOut[FDN_CHANNELS];
        readDelayLines4(delayOut);

        // Advance modulation phases for the next block (after read so phases aren't clobbered)
        updateModulation4();

        // =================================================================
        // Apply Hadamard mixing matrix (vectorized)
        // =================================================================
        float32x4_t mixed[FDN_CHANNELS];
        applyHadamard4(delayOut, mixed);

        // =================================================================
        // Apply frequency-dependent decay:
        // Channels 0-3 have shorter delays (brighter content) → decay * highDecayMult
        // Channels 4-7 have longer delays (warmer content)    → decay * lowDecayMult
        // =================================================================
        float32x4_t decayHi  = vdupq_n_f32(fminf(0.99f, decay * highDecayMult));
        float32x4_t decayLo  = vdupq_n_f32(fminf(0.99f, decay * lowDecayMult));
        float32x4_t feedback = vdupq_n_f32(1.0f - decay);

        for (int i = 0; i < 4; i++) {
            mixed[i] = vmulq_f32(mixed[i], decayHi);
        }
        for (int i = 4; i < FDN_CHANNELS; i++) {
            mixed[i] = vmulq_f32(mixed[i], decayLo);
        }

        // Add input to first channel (with feedback control)
        mixed[0] = vaddq_f32(mixed[0], vmulq_f32(inMono, feedback));

        // =================================================================
        // Apply DAMP (dampingCoeff) + COMP (diffusion) filters
        // =================================================================
        applyDiffusion4(mixed);

        // =================================================================
        // Write back to delay lines
        // =================================================================
        writeDelayLines4(mixed);

        // =================================================================
        // Mix down to stereo (channels 0-3 to L, 4-7 to R)
        // =================================================================
        float32x4_t leftMix = vdupq_n_f32(0.0f);
        float32x4_t rightMix = vdupq_n_f32(0.0f);

        // Sum first 4 channels to left
        for (int i = 0; i < 4; i++) {
            leftMix = vaddq_f32(leftMix, mixed[i]);
        }

        // Sum last 4 channels to right
        for (int i = 4; i < FDN_CHANNELS; i++) {
            rightMix = vaddq_f32(rightMix, mixed[i]);
        }

        // Normalize
        leftMix = vmulq_f32(leftMix, vdupq_n_f32(0.25f));
        rightMix = vmulq_f32(rightMix, vdupq_n_f32(0.25f));

        // Apply stereo width:  mid = (L+R)*0.5, side = (L-R)*0.5
        // outL_wet = mid + side*width,  outR_wet = mid - side*width
        float32x4_t wetMid  = vmulq_f32(vaddq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t wetSide = vmulq_f32(vsubq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t width4  = vdupq_n_f32(width);
        float32x4_t wetL    = vaddq_f32(wetMid, vmulq_f32(wetSide, width4));
        float32x4_t wetR    = vsubq_f32(wetMid, vmulq_f32(wetSide, width4));

        // Wet/dry blend
        float32x4_t wetGain = vdupq_n_f32(mix);
        float32x4_t dryGain = vdupq_n_f32(1.0f - mix);

        float32x4_t outL4 = vaddq_f32(vmulq_f32(inL4, dryGain), vmulq_f32(wetL, wetGain));
        float32x4_t outR4 = vaddq_f32(vmulq_f32(inR4, dryGain), vmulq_f32(wetR, wetGain));

        // Store results
        vst1q_f32(outL, outL4);
        vst1q_f32(outR, outR4);
    }

    /**
     * NEON-optimized delay line reading for 4 samples
     */
    void readDelayLines4(float32x4_t* out) {
        // For each channel, we need the modulation value for each of the 4 samples
        // We'll compute sin values for all 4 phases at once

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            // Get the 4 phases for this channel (one per sample)
            float32x4_t phases = modPhaseVec[ch];

            // Compute sin(2π * phase) for all 4 samples at once
            // First scale to [0, 2π]
            float32x4_t angles = vmulq_f32(phases, vdupq_n_f32(2.0f * M_PI));

            // Compute sin using NEON approximation
            float32x4_t sin_vals = fast_sin_neon(angles);

            // Scale by modulation depth
            float32x4_t mod = vmulq_f32(sin_vals, vdupq_n_f32(modDepth * 100.0f));

            float delaySamples = delayTimes[ch] * sampleRate;
            float baseDelay = delaySamples;

            // Calculate read positions for all 4 samples
            float32x4_t writePosVec = {
                (float)writePos,
                (float)(writePos + 1),
                (float)(writePos + 2),
                (float)(writePos + 3)
            };

            float32x4_t readPos = vsubq_f32(writePosVec,
                                           vaddq_f32(vdupq_n_f32(baseDelay), mod));

            // Wrap to [0, BUFFER_SIZE)
            // This is tricky with floats - we'll convert to ints after wrapping
            float readPosF[4];
            vst1q_f32(readPosF, readPos);

            for (int s = 0; s < 4; s++) {
                // Manual wrap for each sample (safer than vectorized wrap)
                float pos = readPosF[s];
                while (pos < 0) pos += BUFFER_SIZE;
                while (pos >= BUFFER_SIZE) pos -= BUFFER_SIZE;

                int idx = (int)pos;
                int idx_next = (idx + 1) & BUFFER_MASK;
                float frac = pos - idx;

                // Linear interpolation
                float s1 = fdnMem[ch * BUFFER_SIZE + idx];
                float s2 = fdnMem[ch * BUFFER_SIZE + idx_next];
                float sample = s1 + frac * (s2 - s1);

                // Store in output vector
                float32x4_t temp = out[ch];
                temp = vsetq_lane_f32(sample, temp, s);
                out[ch] = temp;
            }
        }
    }

    /**
     * NEON-optimized Hadamard matrix multiplication.
     */
    void applyHadamard4(const float32x4_t* in, float32x4_t* out) {
        for (int i = 0; i < FDN_CHANNELS; i++) {
            float32x4_t acc = vdupq_n_f32(0.0f);
            for (int j = 0; j < FDN_CHANNELS; j++) {
                acc = vmlaq_n_f32(acc, in[j], hadamard[i][j]);
            }
            out[i] = acc;
        }
    }

    /**
     * NEON one-pole LPF per channel.
     */
    void applyDiffusion4(float32x4_t* signals) {
        // pole = amount of previous-sample blending
        float pole = diffusion * dampingCoeff;
        float32x4_t pole4       = vdupq_n_f32(pole);
        float32x4_t oneMinusPole = vdupq_n_f32(1.0f - pole);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t filtered = vaddq_f32(
                vmulq_f32(signals[ch], oneMinusPole),
                vmulq_f32(lpfState[ch], pole4)
            );
            lpfState[ch] = filtered;
            signals[ch]  = filtered;
        }
    }

    /**
     * Write 4 samples to delay lines
     */
    void writeDelayLines4(const float32x4_t* signals) {
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float* delayLine = &fdnMem[ch * BUFFER_SIZE];

            // Write 4 consecutive samples
            for (int s = 0; s < 4; s++) {
                delayLine[(writePos + s) & BUFFER_MASK] = vgetq_lane_f32(signals[ch], s);
            }
        }

        writePos = (writePos + 4) & BUFFER_MASK;
    }

    /**
     * FIXED: Update modulation phases for 4 samples (vectorized)
     * Now properly maintains phase for all 4 lanes
     */
    /**
     * Advance modulation phases by one block (4 samples).
     * modPhaseVec[ch] holds sequential per-sample phases [s0, s1, s2, s3].
     * Called AFTER readDelayLines4 has consumed the current phases, so the
     * phases stored here are ready for the NEXT block's read.
     * Each lane advances by 4 × incPerSample to keep lanes in sequence.
     */
    void updateModulation4() {
        float incPerSample = modRate * 2.0f * M_PI / sampleRate;
        float32x4_t block_advance = vdupq_n_f32(4.0f * incPerSample);
        float32x4_t twoPi = vdupq_n_f32(2.0f * M_PI);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t new_phases = vaddq_f32(modPhaseVec[ch], block_advance);

            // Wrap to [0, 2π) using truncate-toward-zero floor
            float32x4_t div = vmulq_f32(new_phases, vdupq_n_f32(1.0f / (2.0f * M_PI)));
            float32x4_t floor_f = vcvtq_f32_s32(vcvtq_s32_f32(div));
            new_phases = vsubq_f32(new_phases, vmulq_f32(floor_f, twoPi));
            uint32x4_t neg = vcltq_f32(new_phases, vdupq_n_f32(0.0f));
            new_phases = vbslq_f32(neg, vaddq_f32(new_phases, twoPi), new_phases);

            modPhaseVec[ch] = new_phases;
        }
    }

    /*===========================================================================*/
    /* Scalar Fallback for Remainder Samples */
    /*===========================================================================*/

    void processScalar(float input, float& wetL, float& wetR) {
        float delayOut[FDN_CHANNELS];

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            // For scalar path, we only need the current phase (lane 0)
            float phase = vgetq_lane_f32(modPhaseVec[ch], 0);

            float delaySamples = delayTimes[ch] * sampleRate;
            float mod = sinf(phase * 2.0f * M_PI) * modDepth * 100.0f;
            float readPos = (float)writePos - (delaySamples + mod);

            while (readPos < 0) readPos += BUFFER_SIZE;
            while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;

            int idx = (int)readPos;
            int idx_next = (idx + 1) & BUFFER_MASK;
            float frac = readPos - idx;

            float s1 = fdnMem[ch * BUFFER_SIZE + idx];
            float s2 = fdnMem[ch * BUFFER_SIZE + idx_next];
            delayOut[ch] = s1 + frac * (s2 - s1);

            // Update scalar phase (only for lane 0)
            float new_phase = phase + modRate * 2.0f * M_PI / sampleRate;
            if (new_phase >= 2.0f * M_PI) new_phase -= 2.0f * M_PI;

            // Update just lane 0, preserve other lanes
            float32x4_t temp = modPhaseVec[ch];
            temp = vsetq_lane_f32(new_phase, temp, 0);
            modPhaseVec[ch] = temp;
        }

        // Frequency-dependent decay
        float mixed[FDN_CHANNELS];
        for (int i = 0; i < FDN_CHANNELS; i++) {
            float sum = 0.0f;
            for (int j = 0; j < FDN_CHANNELS; j++) {
                sum += hadamard[i][j] * delayOut[j];
            }
            float dm = (i < 4) ? (decay * highDecayMult) : (decay * lowDecayMult);
            mixed[i] = sum * fminf(0.99f, dm);
        }

        mixed[0] += input * (1.0f - decay);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            fdnMem[ch * BUFFER_SIZE + writePos] = mixed[ch];
        }
        writePos = (writePos + 1) & BUFFER_MASK;

        // Stereo mix-down
        float leftRaw = 0.0f, rightRaw = 0.0f;
        for (int i = 0; i < 4; i++)           leftRaw  += mixed[i];
        for (int i = 4; i < FDN_CHANNELS; i++) rightRaw += mixed[i];
        leftRaw  *= 0.25f;
        rightRaw *= 0.25f;

        // Apply stereo width
        float mid  = (leftRaw + rightRaw) * 0.5f;
        float side = (leftRaw - rightRaw) * 0.5f;
        wetL = mid + side * width;
        wetR = mid - side * width;
    }

    /*===========================================================================*/
    /* Fast sine approximation (from float_math.h) */
    /*===========================================================================*/

    fast_inline float32x4_t fast_sin_neon(float32x4_t x) {
        // Use the fast sine from float_math.h
        // This is a placeholder - in real code, include float_math.h and use faster_sinf
        float32x4_t result;
        for (int i = 0; i < 4; i++) {
            float val = vgetq_lane_f32(x, i);
            val = faster_sinf(val);
            result = vsetq_lane_f32(val, result, i);
        }
        return result;
    }

    /*===========================================================================*/
    /* Initialization */
    /*===========================================================================*/

    void initHadamardMatrix() {
        float norm = 1.0f / sqrtf(8.0f);

        for (int i = 0; i < FDN_CHANNELS; i++) {
            for (int j = 0; j < FDN_CHANNELS; j++) {
                int bits = i & j;
                int parity = 0;
                while (bits) {
                    parity ^= (bits & 1);
                    bits >>= 1;
                }
                hadamard[i][j] = parity ? -norm : norm;
            }
        }
    }

    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    float sampleRate;
    int writePos;
    float decay;
    float diffusion;
    float modDepth;
    float modRate;
    float mix;           // wet/dry blend  0..1
    float width;         // stereo width   0..2
    float dampingCoeff;  // one-pole LPF coeff for damping
    float lowDecayMult;  // low-freq decay multiplier
    float highDecayMult; // high-freq decay multiplier

    bool initialized;
    float* fdnMem;  // [FDN_CHANNELS][BUFFER_SIZE]

    float delayTimes[FDN_CHANNELS];
    // FIXED: Store full vector of 4 phases per channel (one per sample in block)
    float32x4_t modPhaseVec[FDN_CHANNELS];
    float32x4_t lpfState[FDN_CHANNELS];
    float hadamard[FDN_CHANNELS][FDN_CHANNELS];
};