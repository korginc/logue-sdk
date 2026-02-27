#include <cstdint>

/**
 * Fast PRNG (Pseudo-Random Number Generator) for Audio.
 * Uses Xorshift algorithm. Extremely lightweight for ARM Cortex.
 */
struct FastNoise {
    uint32_t seed = 2463534242UL; // Non-zero initial seed

    inline float process() {
        // Xorshift32 algorithm
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;

        // Convert uint32_t to a float between -1.0f and +1.0f
        // 2147483648.0f is 2^31
        return ((float)(int32_t)seed / 2147483648.0f);
    }
};

