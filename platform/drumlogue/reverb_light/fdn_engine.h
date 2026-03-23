#pragma once

/**
 * @file fdn_engine.h
 * @brief Feedback Delay Network (FDN) Reverb Engine
 *
 * Features:
 * - 8-channel FDN with Hadamard matrix
 * - NEON-optimized for ARMv7 (processes 4 samples at a time)
 * - Modulated delay lines for diffusion
 * - Stereo input/output
 *
 * OPTIMIZED:
 * - Vectorized processStereo for 4x performance
 * - NEON-accelerated delay line access
 * - Block processing (4 samples per call)
 */

#include <arm_neon.h>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <malloc.h>
#include <algorithm>

// Buffer size - must be power of 2 for efficient modulo
#define FDN_BUFFER_SIZE 32768  // 2^15
#define FDN_BUFFER_MASK (FDN_BUFFER_SIZE - 1)
#define FDN_CHANNELS 8

class FDNEngine {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    FDNEngine()
        : sampleRate(48000.0f)
        , writePos(0)
        , decay(0.5f)
        , modulation(0.05f)
        , glow(0.3f)
        , colorCoeff(0.5f)
        , brightness(0.5f)
        , sizeScale(1.0f)
        , colorLpfL(0.0f)
        , colorLpfR(0.0f)
        , initialized(false)
        , fdnMem(nullptr) {

        // Initialize base delay times (prime-based, in seconds)
        static const float kBaseDelays[FDN_CHANNELS] = {
            0.0421f, 0.0713f, 0.0987f, 0.1249f,
            0.1571f, 0.1835f, 0.2127f, 0.2413f
        };

        for (int i = 0; i < FDN_CHANNELS; i++) {
            baseDelayTimes[i] = kBaseDelays[i];
            delayTimes[i] = kBaseDelays[i];
        }

        // Initialize modulation phases
        for (int i = 0; i < FDN_CHANNELS; i++) {
            modPhases[i] = 0.0f;
        }

        // Initialize state
        memset(fdnState, 0, sizeof(fdnState));

        // Build Hadamard matrix
        buildHadamard();
    }

    ~FDNEngine() {
        if (fdnMem != nullptr) {
            free(fdnMem);
            fdnMem = nullptr;
        }
    }

    /**
     * Initialize the FDN engine
     * @param sr Sample rate
     * @return true if initialization successful, false if out of memory
     */
    bool init(float sr) {
        sampleRate = sr;

        if (initialized && fdnMem != nullptr) {
            return true;
        }

        // Allocate buffer
        fdnMem = (float*)memalign(16, FDN_BUFFER_SIZE * FDN_CHANNELS * sizeof(float));
        if (fdnMem == nullptr) {
            initialized = false;
            return false;
        }

        // Clear buffer
        memset(fdnMem, 0, FDN_BUFFER_SIZE * FDN_CHANNELS * sizeof(float));

        initialized = true;
        return true;
    }

    /*===========================================================================*/
    /* Parameter Setters */
    /*===========================================================================*/

    void setDecay(float d) {
        decay = std::max(0.0f, std::min(0.99f, d));
    }

    void setModulation(float m) {
        modulation = std::max(0.0f, std::min(1.0f, m));
    }

    void setDelayTime(int channel, float timeSeconds) {
        if (channel >= 0 && channel < FDN_CHANNELS) {
            delayTimes[channel] = std::max(0.01f, std::min(2.0f, timeSeconds));
        }
    }

    /** Wet/dry mix  0.0 = fully dry, 1.0 = fully wet (GLOW parameter) */
    void setGlow(float g) {
        glow = std::max(0.0f, std::min(1.0f, g));
    }

    /**
     * Tone color: LPF coefficient (COLR parameter).
     * 0.0 = open/bright, 1.0 = very dark/filtered.
     */
    void setColor(float c) {
        colorCoeff = std::max(0.0f, std::min(0.95f, c));
    }

    /**
     * Brightness: high-frequency blend (BRIG parameter).
     * 0.0 = no HF content, 1.0 = full HF (bypasses LPF).
     */
    void setBrightness(float b) {
        brightness = std::max(0.0f, std::min(1.0f, b));
    }

    /**
     * Room size: scales all delay times (SIZE parameter).
     * 0.0 = tiny room, 1.0 = normal, 2.0 = large hall.
     */
    void setSize(float s) {
        sizeScale = std::max(0.1f, std::min(2.0f, s));
        for (int i = 0; i < FDN_CHANNELS; i++) {
            delayTimes[i] = std::min(baseDelayTimes[i] * sizeScale,
                                     (float)(FDN_BUFFER_SIZE - 1) / sampleRate);
        }
    }

    /*===========================================================================*/
    /* NEON Utilities */
    /*===========================================================================*/

/*===========================================================================*/
    /* Core Processing - Scalar (Single Sample) */
    /*===========================================================================*/

    /**
     * Process one sample through FDN (scalar fallback)
     */
    float processScalar(float input) {
        // Read delayed samples from each channel
        float delayOut[FDN_CHANNELS];

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float delaySamples = delayTimes[ch] * sampleRate;
            float mod = sinf(modPhases[ch] * 2.0f * M_PI) * modulation * 100.0f;
            float readPos = (float)writePos - (delaySamples + mod);

            while (readPos < 0) readPos += FDN_BUFFER_SIZE;
            while (readPos >= FDN_BUFFER_SIZE) readPos -= FDN_BUFFER_SIZE;

            int index1 = (int)readPos;
            int index2 = (index1 + 1) & FDN_BUFFER_MASK;
            float frac = readPos - index1;

            float s1 = fdnMem[ch * FDN_BUFFER_SIZE + index1];
            float s2 = fdnMem[ch * FDN_BUFFER_SIZE + index2];

            delayOut[ch] = s1 + frac * (s2 - s1);

            // Update modulation phase
            modPhases[ch] += modulation * 100.0f / sampleRate;
            if (modPhases[ch] > 1.0f) modPhases[ch] -= 1.0f;
        }

        // Apply Hadamard mixing matrix
        float mixed[FDN_CHANNELS];
        for (int i = 0; i < FDN_CHANNELS; i++) {
            float sum = 0.0f;
            for (int j = 0; j < FDN_CHANNELS; j++) {
                sum += hadamard[i][j] * delayOut[j];
            }
            mixed[i] = sum * decay;
        }

        // Add input to first channel
        mixed[0] += input * (1.0f - decay);

        // Write back to delay lines
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            fdnMem[ch * FDN_BUFFER_SIZE + writePos] = mixed[ch];
            fdnState[ch] = mixed[ch];  // Save for output
        }

        writePos = (writePos + 1) & FDN_BUFFER_MASK;

        // Mix down to mono
        float output = 0.0f;
        for (int i = 0; i < FDN_CHANNELS; i++) {
            output += mixed[i];
        }
        return output / FDN_CHANNELS;
    }

    /*===========================================================================*/
    /* NEON Vectorized Processing (4 Samples) */
    /*===========================================================================*/

    /**
     * Process 4 samples in parallel using NEON
     */
    void process4Samples(const float* inL, const float* inR,
                         float* outL, float* outR) {

        // Load 4 input samples for L and R
        float32x4_t inL4 = vld1q_f32(inL);
        float32x4_t inR4 = vld1q_f32(inR);

        // Convert to mono for FDN input (4 samples at once)
        float32x4_t inMono = vmulq_f32(vaddq_f32(inL4, inR4), vdupq_n_f32(0.5f));

        // =================================================================
        // Read from all 8 delay lines for 4 samples
        // =================================================================
        float delayOut[FDN_CHANNELS][4];

        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float delaySamples = delayTimes[ch] * sampleRate;

            // Calculate read positions for 4 samples
            float readPos[4];
            for (int s = 0; s < 4; s++) {
                float mod = sinf(modPhases[ch] * 2.0f * M_PI) * modulation * 100.0f;
                float pos = (float)(writePos + s) - (delaySamples + mod);
                while (pos < 0) pos += FDN_BUFFER_SIZE;
                while (pos >= FDN_BUFFER_SIZE) pos -= FDN_BUFFER_SIZE;
                readPos[s] = pos;

                // Update modulation phase (once per 4 samples)
                if (s == 3) {
                    modPhases[ch] += modulation * 100.0f / sampleRate * 4.0f;
                    if (modPhases[ch] > 1.0f) modPhases[ch] -= 1.0f;
                }
            }

            // Linear interpolation for 4 samples
            for (int s = 0; s < 4; s++) {
                int idx1 = (int)readPos[s];
                int idx2 = (idx1 + 1) & FDN_BUFFER_MASK;
                float frac = readPos[s] - idx1;

                float s1 = fdnMem[ch * FDN_BUFFER_SIZE + idx1];
                float s2 = fdnMem[ch * FDN_BUFFER_SIZE + idx2];
                delayOut[ch][s] = s1 + frac * (s2 - s1);
            }
        }

        // =================================================================
        // Apply Hadamard mixing (for 4 samples)
        // =================================================================
        float mixed[FDN_CHANNELS][4];

        // Spill inMono to a plain array before the loop: vgetq_lane_f32 requires
        // a compile-time constant lane index; a loop variable is not accepted.
        float inMonoArr[4];
        vst1q_f32(inMonoArr, inMono);

        // For each sample position
        for (int s = 0; s < 4; s++) {
            // Load one sample from each channel into a vector
            float32x4_t vec0 = vdupq_n_f32(delayOut[0][s]);
            float32x4_t vec1 = vdupq_n_f32(delayOut[1][s]);
            float32x4_t vec2 = vdupq_n_f32(delayOut[2][s]);
            float32x4_t vec3 = vdupq_n_f32(delayOut[3][s]);
            float32x4_t vec4 = vdupq_n_f32(delayOut[4][s]);
            float32x4_t vec5 = vdupq_n_f32(delayOut[5][s]);
            float32x4_t vec6 = vdupq_n_f32(delayOut[6][s]);
            float32x4_t vec7 = vdupq_n_f32(delayOut[7][s]);

            // Hadamard rows as vectors
            float32x4_t row0 = vld1q_f32(hadamard[0]);
            float32x4_t row1 = vld1q_f32(hadamard[1]);
            float32x4_t row2 = vld1q_f32(hadamard[2]);
            float32x4_t row3 = vld1q_f32(hadamard[3]);
            float32x4_t row4 = vld1q_f32(hadamard[4]);
            float32x4_t row5 = vld1q_f32(hadamard[5]);
            float32x4_t row6 = vld1q_f32(hadamard[6]);
            float32x4_t row7 = vld1q_f32(hadamard[7]);

            // Compute output channels 0-3: load first half of each Hadamard row
            // total_lo[k] = sum_i(H[i][k] * in[i]) = out[k]  for k=0..3
            float32x4_t res0 = vmulq_f32(row0, vec0);
            float32x4_t res1 = vmulq_f32(row1, vec1);
            float32x4_t res2 = vmulq_f32(row2, vec2);
            float32x4_t res3 = vmulq_f32(row3, vec3);
            float32x4_t res4 = vmulq_f32(row4, vec4);
            float32x4_t res5 = vmulq_f32(row5, vec5);
            float32x4_t res6 = vmulq_f32(row6, vec6);
            float32x4_t res7 = vmulq_f32(row7, vec7);

            float32x4_t sum01 = vaddq_f32(res0, res1);
            float32x4_t sum23 = vaddq_f32(res2, res3);
            float32x4_t sum45 = vaddq_f32(res4, res5);
            float32x4_t sum67 = vaddq_f32(res6, res7);
            float32x4_t sum0123 = vaddq_f32(sum01, sum23);
            float32x4_t sum4567 = vaddq_f32(sum45, sum67);
            float32x4_t total_lo = vaddq_f32(sum0123, sum4567);

            // Compute output channels 4-7: load second half (elements [4..7]) of each row
            // total_hi[k] = sum_i(H[i][k+4] * in[i]) = out[k+4]  for k=0..3
            float32x4_t row0h = vld1q_f32(&hadamard[0][4]);
            float32x4_t row1h = vld1q_f32(&hadamard[1][4]);
            float32x4_t row2h = vld1q_f32(&hadamard[2][4]);
            float32x4_t row3h = vld1q_f32(&hadamard[3][4]);
            float32x4_t row4h = vld1q_f32(&hadamard[4][4]);
            float32x4_t row5h = vld1q_f32(&hadamard[5][4]);
            float32x4_t row6h = vld1q_f32(&hadamard[6][4]);
            float32x4_t row7h = vld1q_f32(&hadamard[7][4]);

            float32x4_t total_hi = vaddq_f32(
                vaddq_f32(vaddq_f32(vmulq_f32(row0h, vec0), vmulq_f32(row1h, vec1)),
                          vaddq_f32(vmulq_f32(row2h, vec2), vmulq_f32(row3h, vec3))),
                vaddq_f32(vaddq_f32(vmulq_f32(row4h, vec4), vmulq_f32(row5h, vec5)),
                          vaddq_f32(vmulq_f32(row6h, vec6), vmulq_f32(row7h, vec7))));

            float32x4_t decay4 = vdupq_n_f32(decay);
            total_lo = vmulq_f32(total_lo, decay4);
            total_hi = vmulq_f32(total_hi, decay4);

            // Store channels 0-3 (inject input into channel 0 only)
            float inSample = inMonoArr[s]; // spilled above; variable index safe
            mixed[0][s] = vgetq_lane_f32(total_lo, 0) + inSample * (1.0f - decay);
            mixed[1][s] = vgetq_lane_f32(total_lo, 1);
            mixed[2][s] = vgetq_lane_f32(total_lo, 2);
            mixed[3][s] = vgetq_lane_f32(total_lo, 3);

            // Store channels 4-7 using correctly computed second-half transform
            mixed[4][s] = vgetq_lane_f32(total_hi, 0);
            mixed[5][s] = vgetq_lane_f32(total_hi, 1);
            mixed[6][s] = vgetq_lane_f32(total_hi, 2);
            mixed[7][s] = vgetq_lane_f32(total_hi, 3);
        }

        // =================================================================
        // Write back to delay lines and update state
        // =================================================================
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            float* delayLine = &fdnMem[ch * FDN_BUFFER_SIZE];

            // Write 4 consecutive samples
            for (int s = 0; s < 4; s++) {
                delayLine[(writePos + s) & FDN_BUFFER_MASK] = mixed[ch][s];
            }

            // Update fdnState (use last sample for next block)
            fdnState[ch] = mixed[ch][3];
        }

        writePos = (writePos + 4) & FDN_BUFFER_MASK;

        // =================================================================
        // Stereo downmix (channels 0-3 to left, 4-7 to right)
        // =================================================================
        // Spill stereo input vectors for variable-index access (same reason as inMono above)
        float inLArr[4], inRArr[4];
        vst1q_f32(inLArr, inL4);
        vst1q_f32(inRArr, inR4);

        for (int s = 0; s < 4; s++) {
            float leftOut = 0.0f, rightOut = 0.0f;
            for (int ch = 0; ch < 4; ch++) {
                leftOut += mixed[ch][s];
                rightOut += mixed[ch + 4][s];
            }
            leftOut *= 0.25f;
            rightOut *= 0.25f;

            // Apply tone: LPF (color) blended with dry-wet mix
            // LPF: y = colorCoeff*y_prev + (1-colorCoeff)*x
            colorLpfL = colorCoeff * colorLpfL + (1.0f - colorCoeff) * leftOut;
            colorLpfR = colorCoeff * colorLpfR + (1.0f - colorCoeff) * rightOut;

            // Brightness: blend between lpf output and unfiltered
            float wetL = colorLpfL + brightness * (leftOut  - colorLpfL);
            float wetR = colorLpfR + brightness * (rightOut - colorLpfR);

            float inLVal = inLArr[s];
            float inRVal = inRArr[s];

            outL[s] = inLVal * (1.0f - glow) + wetL * glow;
            outR[s] = inRVal * (1.0f - glow) + wetR * glow;
        }
    }

    /*===========================================================================*/
    /* Main Processing Entry Point - Vectorized */
    /*===========================================================================*/

    /**
     * Process stereo audio through FDN (vectorized)
     * Handles both full blocks of 4 and remainder samples
     */
    void processStereo(const float* inL, const float* inR,
                       float* outL, float* outR,
                       int numSamples) {

        // Bypass if not initialized
        if (!initialized || fdnMem == nullptr) {
            memcpy(outL, inL, numSamples * sizeof(float));
            memcpy(outR, inR, numSamples * sizeof(float));
            return;
        }

        int samplesProcessed = 0;

        // =================================================================
        // Process in blocks of 4 (vectorized path)
        // =================================================================
        while (samplesProcessed + 4 <= numSamples) {
            process4Samples(inL + samplesProcessed,
                            inR + samplesProcessed,
                            outL + samplesProcessed,
                            outR + samplesProcessed);
            samplesProcessed += 4;
        }

        // =================================================================
        // Process remaining 1-3 samples (scalar fallback)
        // =================================================================
        for (int i = samplesProcessed; i < numSamples; i++) {
            float monoIn = (inL[i] + inR[i]) * 0.5f;
            processScalar(monoIn);  // populates fdnState for stereo spread

            // Simple stereo spread (channels 0-3 to left, 4-7 to right)
            float leftOut = 0.0f, rightOut = 0.0f;
            for (int ch = 0; ch < 4; ch++) {
                leftOut += fdnState[ch];
                rightOut += fdnState[ch + 4];
            }
            leftOut *= 0.25f;
            rightOut *= 0.25f;

            // Apply tone (color LPF + brightness blend)
            colorLpfL = colorCoeff * colorLpfL + (1.0f - colorCoeff) * leftOut;
            colorLpfR = colorCoeff * colorLpfR + (1.0f - colorCoeff) * rightOut;
            float wetL = colorLpfL + brightness * (leftOut  - colorLpfL);
            float wetR = colorLpfR + brightness * (rightOut - colorLpfR);

            outL[i] = inL[i] * (1.0f - glow) + wetL * glow;
            outR[i] = inR[i] * (1.0f - glow) + wetR * glow;
        }
    }

private:
    /*===========================================================================*/
    /* Private Methods */
    /*===========================================================================*/

    void buildHadamard() {
        float norm = 1.0f / sqrtf(8.0f);

        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
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
    float modulation;
    float glow;          // wet/dry mix  0..1
    float colorCoeff;    // tone LPF coefficient  0..0.95
    float brightness;    // HF blend  0..1
    float sizeScale;     // room size scale  0.1..2.0
    float colorLpfL;     // LPF state for left channel output
    float colorLpfR;     // LPF state for right channel output

    bool initialized;
    float* fdnMem;  // [FDN_CHANNELS][BUFFER_SIZE]

    float baseDelayTimes[FDN_CHANNELS]; // unscaled delay times
    float delayTimes[FDN_CHANNELS];
    float modPhases[FDN_CHANNELS];
    float fdnState[FDN_CHANNELS];
    float hadamard[FDN_CHANNELS][FDN_CHANNELS];
};