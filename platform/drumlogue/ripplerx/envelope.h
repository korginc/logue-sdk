#pragma once

enum EnvState { ENV_IDLE = 0, ENV_ATTACK, ENV_DECAY, ENV_RELEASE };

/**
 * Fast, exponentially-curved AR/ADSR Envelope.
 */
struct FastEnvelope {
    EnvState state = ENV_IDLE;
    float value = 0.0f;

    // Rates (Calculated in UI thread: e.g., rate = 1.0 - exp(-1.0 / (time_ms * srate)))
    float attack_rate = 0.01f;
    float decay_rate = 0.001f;
    float release_rate = 0.005f;

    // Levels
    float sustain_level = 0.0f; // 0.0 makes it an AR envelope (Percussive)

    inline void trigger() {
        state = ENV_ATTACK;
        value = 0.0f; // Reset phase
    }

    inline void release() {
        if (state != ENV_IDLE) {
            state = ENV_RELEASE;
        }
    }

    inline float process() {
        switch (state) {
            case ENV_IDLE:
                break;
            case ENV_ATTACK:
                value += (1.0f - value) * attack_rate;
                if (value >= 0.99f) {
                    value = 1.0f;
                    state = ENV_DECAY;
                }
                break;
            case ENV_DECAY:
                value += (sustain_level - value) * decay_rate;
                break;
            case ENV_RELEASE:
                value += (0.0f - value) * release_rate;
                if (value <= 0.001f) {
                    value = 0.0f;
                    state = ENV_IDLE;
                }
                break;
        }
        return value;
    }
};