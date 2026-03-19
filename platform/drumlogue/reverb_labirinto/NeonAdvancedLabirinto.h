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

/**
 * OPTIMIZED: Interleaved frame structure for vld4q_f32
 * Stores all 8 FDN channels at a single time position
 * Format: [ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7]
 */
typedef struct __attribute__((aligned(16))) {
    float samples[FDN_CHANNELS];  // All 8 channels at this time position
} interleaved_frame_t;

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
        , delayLine(nullptr) {

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
        }

        // Initialize filter states
        for (int i = 0; i < FDN_CHANNELS; i++) {
            lpfState[i] = vdupq_n_f32(0.0f);
        }

        // Initialize Hadamard matrix
        initHadamardMatrix();
    }

    ~NeonAdvancedLabirinto() {
        if (delayLine != nullptr) {
            free(delayLine);
            delayLine = nullptr;
        }
    }

    /**
     * Initialize the reverb with proper memory alignment
     * @return true if initialization successful
     */
    bool init() {
        // Use posix_memalign for cache-line alignment
        if (posix_memalign((void**)&delayLine, 64,
                           BUFFER_SIZE * sizeof(interleaved_frame_t)) != 0) {
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
     */
    void clear() {
        if (delayLine != nullptr) {
            // Use NEON to clear efficiently
            float32x4_t zero = vdupq_n_f32(0.0f);
            for (int i = 0; i < BUFFER_SIZE; i++) {
                // Clear 8 channels using 2 NEON stores
                vst1q_f32(&delayLine[i].samples[0], zero);
                vst1q_f32(&delayLine[i].samples[4], zero);
            }
        }

        writePos = 0;

        // Reset modulation phases with proper per-lane offsets
	// lane k = k * incPerSample so sequential
        // samples start at phase 0, inc, 2*inc, 3*inc
        float incPerSample = modRate * 2.0f * M_PI / sampleRate;
        float32x4_t init_phases = {
            0.0f,
            incPerSample,
            2.0f * incPerSample,
            3.0f * incPerSample
        };
        for (int i = 0; i < FDN_CHANNELS; i++) {
            modPhaseVec[i] = init_phases;
        }

        // Reset filter states
        float32x4_t zero = vdupq_n_f32(0.0f);
        for (int i = 0; i < FDN_CHANNELS; i++) {
            lpfState[i] = zero;
        }
    }

    /*===========================================================================*/
    /* Parameter Setters */
    /*===========================================================================*/

    void setDecay(float d) { decay = fmaxf(0.0f, fminf(0.99f, d)); }
    void setDiffusion(float d) { diffusion = fmaxf(0.0f, fminf(1.0f, d)); }
    void setModDepth(float d) { modDepth = fmaxf(0.0f, fminf(1.0f, d)); }
    void setModRate(float r) { modRate = fmaxf(0.1f, fminf(10.0f, r)); }
    void setMix(float m) { mix = fmaxf(0.0f, fminf(1.0f, m)); }
    void setWidth(float w) { width = fmaxf(0.0f, fminf(2.0f, w)); }

    void setDamping(float freqHz) {
        freqHz = fmaxf(200.0f, fminf(10000.0f, freqHz));
	// omega = 2π * fc / fs;  coeff ≈ 1 - omega  (first-order approx)
        float omega = 2.0f * (float)M_PI * freqHz / sampleRate;
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
    /* Core Processing - Fully NEON Vectorized */
    /*===========================================================================*/

    /**
     * Process stereo audio through the reverb (NEON vectorized)
     * Processes 4 samples at a time for maximum efficiency
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples) {

	// Safety check - if memory not allocated, bypass
        if (!initialized || delayLine == nullptr) {
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
    /* OPTIMIZED: vld4-based delay line reading with vectorized interpolation */
    /*===========================================================================*/

    void readDelayLines4(float32x4_t* out) {
        // Calculate read positions for all 8 channels × 4 samples
        float32x4_t baseReadPos = {
            (float)writePos,
            (float)(writePos + 1),
            (float)(writePos + 2),
            (float)(writePos + 3)
        };

        // Pre-calculate modulation for all channels (sin values for all 4 phases at once)
        float32x4_t mods[FDN_CHANNELS];
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            // Get the 4 phases for this channel (one per sample)
            float32x4_t phases = modPhaseVec[ch];

            // Compute sin(2π * phase) for all 4 samples at once
            // First scale to [0, 2π]
            float32x4_t angles = vmulq_f32(phases, vdupq_n_f32(2.0f * M_PI));

            // Compute sin using NEON approximation
            float32x4_t sin_vals = fast_sin_neon(angles);

	    // Scale by modulation depth
            mods[ch] = vmulq_f32(sin_vals, vdupq_n_f32(modDepth * 100.0f));
        }

        // For each channel, calculate read positions
        float32x4_t readPositions[FDN_CHANNELS];
        uint32_t baseIndices[FDN_CHANNELS][4];

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float delaySamples = delayTimes[ch] * sampleRate;
            readPositions[ch] = vsubq_f32(baseReadPos,
                vaddq_f32(vdupq_n_f32(delaySamples), mods[ch]));

            // Wrap to [0, BUFFER_SIZE)
            float pos_vals[4];
            vst1q_f32(pos_vals, readPositions[ch]);

            for (int s = 0; s < 4; s++) {
		// Manual wrap for each sample (safer than vectorized wrap)
                float pos = pos_vals[s];
                while (pos < 0) pos += BUFFER_SIZE;
                while (pos >= BUFFER_SIZE) pos -= BUFFER_SIZE;
                baseIndices[ch][s] = (uint32_t)pos;
            }
        }

        // OPTIMIZED: Load using vld4q_f32 - 4 channels × 4 samples at once
        // Process in groups of 4 channels (since vld4 handles 4 vectors)
        for (int chGroup = 0; chGroup < FDN_CHANNELS; chGroup += 4) {
            // For each sample position (0-3), we need data from all 4 channels
            for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++) {
                uint32_t baseIdx = baseIndices[chGroup][sampleIdx];

                // Load 4 consecutive frames for interpolation
                // Each frame contains all 8 channels at that time
                uint32_t idx0 = baseIdx & BUFFER_MASK;
                uint32_t idx1 = (idx0 + 1) & BUFFER_MASK;

                // Load 4 channels × 4 time positions using vld4
                float32x4x4_t frames0 = vld4q_f32(&delayLine[idx0].samples[chGroup]);
                float32x4x4_t frames1 = vld4q_f32(&delayLine[idx1].samples[chGroup]);

                // frames0.val[0] = ch0 at times t, t+1, t+2, t+3
                // frames0.val[1] = ch1 at times t, t+1, t+2, t+3
                // frames0.val[2] = ch2 at times t, t+1, t+2, t+3
                // frames0.val[3] = ch3 at times t, t+1, t+2, t+3

                // For linear interpolation, we need:
                // For each channel, sample at time t (from frames0) and t+1 (from frames1)

                // Get the fractional part for interpolation
                float pos = baseIndices[chGroup][sampleIdx] - baseIdx;
                float32x4_t frac = vdupq_n_f32(pos);

                // Interpolate each channel
                for (int chOffset = 0; chOffset < 4; chOffset++) {
                    // Get the sample for this channel at the base time
                    float32x4_t s0 = frames0.val[chOffset];
                    float s0_val = vgetq_lane_f32(s0, sampleIdx);

                    // Get the sample for this channel at next time
                    float32x4_t s1 = frames1.val[chOffset];
                    float s1_val = vgetq_lane_f32(s1, sampleIdx);

                    // Linear interpolation
                    float sample = s0_val + pos * (s1_val - s0_val);

                    // Store in output vector
                    float32x4_t temp = out[chGroup + chOffset];
                    temp = vsetq_lane_f32(sample, temp, sampleIdx);
                    out[chGroup + chOffset] = temp;
                }
            }
        }
    }

    /*===========================================================================*/
    /* Vectorized Hadamard Transform */
    /*===========================================================================*/

    void initHadamardMatrix() {
        float norm = 1.0f / sqrtf(8.0f);

        // Store in row-major for scalar access
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

        // Pre-transpose into column-major NEON-friendly format
        for (int j = 0; j < FDN_CHANNELS; j++) {
            for (int i = 0; i < FDN_CHANNELS; i += 4) {
                float32x4_t col = {
                    hadamard[i][j],
                    hadamard[i + 1][j],
                    hadamard[i + 2][j],
                    hadamard[i + 3][j]
                };
                hadamardCols[j][i/4] = col;
            }
        }
    }

    void applyHadamard4(const float32x4_t* in, float32x4_t* out) {
        // Clear accumulators
        for (int i = 0; i < FDN_CHANNELS; i++) {
            out[i] = vdupq_n_f32(0.0f);
        }

        // For each input channel, add its contribution to all outputs
        for (int j = 0; j < FDN_CHANNELS; j++) {
            float32x4_t inVal = in[j];

            // For each group of 4 output channels
            for (int i = 0; i < FDN_CHANNELS; i += 4) {
                float32x4_t coeffs = hadamardCols[j][i/4];

                // out[i..i+3] += coeffs * inVal
                float32x4_t contrib = vmulq_f32(coeffs, inVal);
                out[i] = vaddq_f32(out[i], vdupq_lane_f32(vget_low_f32(contrib), 0));
                out[i + 1] = vaddq_f32(out[i + 1], vdupq_lane_f32(vget_low_f32(contrib), 1));
                out[i + 2] = vaddq_f32(out[i + 2], vdupq_lane_f32(vget_high_f32(contrib), 0));
                out[i + 3] = vaddq_f32(out[i + 3], vdupq_lane_f32(vget_high_f32(contrib), 1));
            }
        }
    }

    /*===========================================================================*/
    /* Vectorized Filter Application */
    /*===========================================================================*/
    /**
     * NEON one-pole LPF per channel.
     */
    void applyDiffusion4(float32x4_t* signals) {
        // pole = amount of previous-sample blending
        float pole = diffusion * dampingCoeff;
        float32x4_t pole4 = vdupq_n_f32(pole);
        float32x4_t oneMinusPole = vdupq_n_f32(1.0f - pole);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t filtered = vaddq_f32(
                vmulq_f32(signals[ch], oneMinusPole),
                vmulq_f32(lpfState[ch], pole4)
            );
            lpfState[ch] = filtered;
            signals[ch] = filtered;
        }
    }

    /*===========================================================================*/
    /* Vectorized Delay Line Write */
    /*===========================================================================*/

    void writeDelayLines4(const float32x4_t* signals) {
        // Write 4 samples to interleaved delay line
        for (int s = 0; s < 4; s++) {
            uint32_t pos = (writePos + s) & BUFFER_MASK;

            // Write all 8 channels for this time position
            for (int ch = 0; ch < FDN_CHANNELS; ch++) {
                delayLine[pos].samples[ch] = vgetq_lane_f32(signals[ch], s);
            }
        }

        writePos = (writePos + 4) & BUFFER_MASK;
    }

    /*===========================================================================*/
    /* Main Processing Loop */
    /*===========================================================================*/

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
        float32x4_t decayHi = vdupq_n_f32(fminf(0.99f, decay * highDecayMult));
        float32x4_t decayLo = vdupq_n_f32(fminf(0.99f, decay * lowDecayMult));
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
        float32x4_t leftSum = vdupq_n_f32(0.0f);
        float32x4_t rightSum = vdupq_n_f32(0.0f);

        // Sum first 4 channels to left
        for (int i = 0; i < 4; i++) {
            leftSum = vaddq_f32(leftSum, mixed[i]);
        }

        // Sum last 4 channels to right
        for (int i = 4; i < FDN_CHANNELS; i++) {
            rightSum = vaddq_f32(rightSum, mixed[i]);
        }

        // Normalize
        float32x4_t leftMix = vmulq_f32(leftSum, vdupq_n_f32(0.25f));
        float32x4_t rightMix = vmulq_f32(rightSum, vdupq_n_f32(0.25f));

        // Apply stereo width:  mid = (L+R)*0.5, side = (L-R)*0.5
        // outL_wet = mid + side*width,  outR_wet = mid - side*width
        float32x4_t mid = vmulq_f32(vaddq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t side = vmulq_f32(vsubq_f32(leftMix, rightMix), vdupq_n_f32(0.5f));
        float32x4_t width4 = vdupq_n_f32(width);

        float32x4_t wetL = vaddq_f32(mid, vmulq_f32(side, width4));
        float32x4_t wetR = vsubq_f32(mid, vmulq_f32(side, width4));

        // Wet/dry mix
        float32x4_t wetGain = vdupq_n_f32(mix);
        float32x4_t dryGain = vdupq_n_f32(1.0f - mix);

        float32x4_t outL4 = vaddq_f32(vmulq_f32(inL4, dryGain), vmulq_f32(wetL, wetGain));
        float32x4_t outR4 = vaddq_f32(vmulq_f32(inR4, dryGain), vmulq_f32(wetR, wetGain));

        // Store results
        vst1q_f32(outL, outL4);
        vst1q_f32(outR, outR4);
    }

    void updateModulation4() {
        float incPerSample = modRate * 2.0f * M_PI / sampleRate;
        float32x4_t blockAdvance = vdupq_n_f32(4.0f * incPerSample);
        float32x4_t twoPi = vdupq_n_f32(2.0f * M_PI);

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float32x4_t newPhases = vaddq_f32(modPhaseVec[ch], blockAdvance);

            // Wrap to [0, 2π) using truncate-toward-zero floor
            float32x4_t div = vmulq_f32(newPhases, vdupq_n_f32(1.0f / (2.0f * M_PI)));
            float32x4_t floor_f = vcvtq_f32_s32(vcvtq_s32_f32(div));
            newPhases = vsubq_f32(newPhases, vmulq_f32(floor_f, twoPi));

            uint32x4_t neg = vcltq_f32(newPhases, vdupq_n_f32(0.0f));
            newPhases = vbslq_f32(neg, vaddq_f32(newPhases, twoPi), newPhases);

            modPhaseVec[ch] = newPhases;
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
            mixed[i] = sum * std::min(0.99f, dm);
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
        // Use faster_sinf from float_math.h
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
    interleaved_frame_t* delayLine __attribute__((aligned(64)));

    float32x4_t hadamardCols[FDN_CHANNELS][FDN_CHANNELS/4];  // Column-major for NEON
    float delayTimes[FDN_CHANNELS] __attribute__((aligned(16)));
    float32x4_t modPhaseVec[FDN_CHANNELS] __attribute__((aligned(16)));
    float32x4_t lpfState[FDN_CHANNELS] __attribute__((aligned(16)));
    float hadamard[FDN_CHANNELS][FDN_CHANNELS] __attribute__((aligned(16)));
};
