#pragma once

/**
 * @file fdn_engine.h
 * @brief Feedback Delay Network (FDN) Reverb Engine
 *
 * Features:
 * - 8-channel FDN with Hadamard matrix
 * - Modulated delay lines for diffusion
 * - NEON-optimized for ARMv7
 *
 * FIXED:
 * - Added null pointer check for memalign
 * - Correct ARMv7 horizontal sum using vpadd_f32
 * - Graceful bypass on allocation failure
 */

#include <arm_neon.h>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <malloc.h>

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
        , modulation(0.2f)
        , initialized(false)
        , fdnMem(nullptr) {

        // Initialize delay times (in seconds)
        float baseDelays[FDN_CHANNELS] = {
            0.0421f, 0.0713f, 0.0987f, 0.1249f,
            0.1571f, 0.1835f, 0.2127f, 0.2413f
        };

        for (int i = 0; i < FDN_CHANNELS; i++) {
            delayTimes[i] = baseDelays[i];
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
        // FIXED: Free allocated memory
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

        // FIXED: Check if already initialized
        if (initialized && fdnMem != nullptr) {
            return true;
        }

        // FIXED: Check allocation success
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

    /*===========================================================================*/
    /* Core Processing */
    /*===========================================================================*/

    /**
     * Process one sample through FDN
     * @param input Mono input sample
     * @return Mono output sample
     *
     * NOTE: If not initialized, returns input (bypass)
     */
    float process(float input) {
        // FIXED: Bypass if not initialized
        if (!initialized || fdnMem == nullptr) {
            return input;
        }

        // Update modulation phases
        updateModulation();

        // Read delayed samples from each channel
        float delayOut[FDN_CHANNELS];
        readDelayLines(delayOut);

        // Apply Hadamard mixing matrix
        float mixed[FDN_CHANNELS];
        applyHadamard(delayOut, mixed);

        // Apply decay and add input to first channel
        for (int i = 0; i < FDN_CHANNELS; i++) {
            mixed[i] *= decay;
        }
        mixed[0] += input * (1.0f - decay);  // Input gain scales with decay

        // Write back to delay lines
        writeDelayLines(mixed);

        // Mix down to mono (average of all channels)
        float output = 0.0f;
        for (int i = 0; i < FDN_CHANNELS; i++) {
            output += mixed[i];
        }
        output /= FDN_CHANNELS;

        return output;
    }

    /**
     * Process stereo through FDN (simplified - sum to mono, then stereo out)
     */
    void processStereo(const float* inL, const float* inR,
                       float* outL, float* outR,
                       int numSamples) {
        // FIXED: Bypass if not initialized
        if (!initialized || fdnMem == nullptr) {
            memcpy(outL, inL, numSamples * sizeof(float));
            memcpy(outR, inR, numSamples * sizeof(float));
            return;
        }

        for (int i = 0; i < numSamples; i++) {
            // Convert stereo to mono for FDN input
            float monoIn = (inL[i] + inR[i]) * 0.5f;

            // Process through FDN
            float monoOut = process(monoIn);

            // Simple stereo spread (channels 0-3 to left, 4-7 to right)
            float leftOut = 0.0f, rightOut = 0.0f;
            for (int ch = 0; ch < 4; ch++) {
                leftOut += fdnState[ch];
                rightOut += fdnState[ch + 4];
            }
            leftOut *= 0.25f;
            rightOut *= 0.25f;

            // Mix with dry signal (50% wet for now)
            outL[i] = inL[i] * 0.5f + leftOut * 0.5f;
            outR[i] = inR[i] * 0.5f + rightOut * 0.5f;
        }
    }

private:
    /*===========================================================================*/
    /* Private Methods */
    /*===========================================================================*/

    /**
     * FIXED: Correct ARMv7 horizontal sum for float32x4_t
     * @param v Vector of 4 floats
     * @return Sum of all 4 elements
     */
    inline float hsum_neon(float32x4_t v) {
        // Step 1: Pairwise add low and high halves
        // vget_low_f32(v) = [a, b]
        // vget_high_f32(v) = [c, d]
        // vpadd_f32 = [a+b, c+d]
        float32x2_t sum_halves = vpadd_f32(vget_low_f32(v), vget_high_f32(v));

        // Step 2: Add the two results together
        return vget_lane_f32(sum_halves, 0) + vget_lane_f32(sum_halves, 1);
    }

    /**
     * Build Hadamard matrix (8x8)
     */
    void buildHadamard() {
        float norm = 1.0f / sqrtf(8.0f);

        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                // Walsh-Hadamard: sign = (-1)^popcount(i & j)
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

    /**
     * Update modulation phases
     */
    void updateModulation() {
        float modInc = modulation * 100.0f / sampleRate;  // Simple LFO
        for (int i = 0; i < FDN_CHANNELS; i++) {
            modPhases[i] += modInc;
            if (modPhases[i] > 1.0f) {
                modPhases[i] -= 1.0f;
            }
        }
    }

    /**
     * Read from delay lines with modulation
     */
    void readDelayLines(float* out) {
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            // Calculate base delay in samples
            float delaySamples = delayTimes[ch] * sampleRate;

            // Add modulation
            float mod = sinf(modPhases[ch] * 2.0f * M_PI) * modulation * 100.0f;
            float readPos = (float)writePos - (delaySamples + mod);

            // Wrap to buffer range
            while (readPos < 0) readPos += FDN_BUFFER_SIZE;
            while (readPos >= FDN_BUFFER_SIZE) readPos -= FDN_BUFFER_SIZE;

            // Linear interpolation
            int index1 = (int)readPos;
            int index2 = (index1 + 1) & FDN_BUFFER_MASK;
            float frac = readPos - index1;

            float s1 = fdnMem[ch * FDN_BUFFER_SIZE + index1];
            float s2 = fdnMem[ch * FDN_BUFFER_SIZE + index2];

            out[ch] = s1 + frac * (s2 - s1);
        }
    }

    /**
     * Apply Hadamard mixing matrix
     */
    void applyHadamard(const float* in, float* out) {
        for (int i = 0; i < FDN_CHANNELS; i++) {
            float sum = 0.0f;
            for (int j = 0; j < FDN_CHANNELS; j++) {
                sum += hadamard[i][j] * in[j];
            }
            out[i] = sum;
        }
    }

    /**
     * Write to delay lines
     */
    void writeDelayLines(const float* in) {
        for (int ch = 0; ch < FDN_CHANNELS; ch++) {
            fdnMem[ch * FDN_BUFFER_SIZE + writePos] = in[ch];
            fdnState[ch] = in[ch];  // Save for output mixing
        }
        writePos = (writePos + 1) & FDN_BUFFER_MASK;
    }

    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    float sampleRate;
    int writePos;
    float decay;
    float modulation;

    // FIXED: Track initialization state
    bool initialized;

    // FIXED: Buffer pointer (will be checked before use)
    float* fdnMem;

    // Delay parameters
    float delayTimes[FDN_CHANNELS];
    float modPhases[FDN_CHANNELS];

    // State
    float fdnState[FDN_CHANNELS];

    // Hadamard mixing matrix
    float hadamard[FDN_CHANNELS][FDN_CHANNELS];
};