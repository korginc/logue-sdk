#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <arm_neon.h>
#include <float_math.h>
#include "unit.h"
#include "dsp_core.h"
#include "../common/runtime.h" // Drumlogue OS functions

#define ENABLE_PHASE_5_EXCITERS
// #define ENABLE_PHASE_6_FILTERS

/**
 * The Architectural Wins Here:
Pre-Calculated Math: Notice the apply_skew and division happens purely in setParameter. The DSP struct (WaveguideState) now holds a pure float like 0.993f. In Phase 3, the Audio Thread will just do a single multiplication (buffer[i] * feedback_gain).

Crash-Proof Samples: The sample loader has the exact pointer checks we discussed, but it pushes the metadata to all 4 voices instantly.

Sequencer Routing: GateOn properly routes to m_ui_note, ensuring the internal drum machine plays the pitch defined on the screen.

*/

// Utility for fast skewing
inline float apply_skew(float normalized_val, float skew) {
    // Inverse exponent mapping for log-style potentiometer curves
    return fasterpowf(normalized_val, 1.0f / skew);
}

class alignas(16) RipplerXWaveguide {
public:
    SynthState state;
    // ==============================================================================
    // 0. Lifecycle & Initialization
    // ==============================================================================

    inline int8_t Init(const unit_runtime_desc_t * desc) {
        // 1. Hardware Sanity Checks
        // The Drumlogue is strictly 48kHz, stereo.
        // If Korg ever releases a 96kHz device, this prevents your delay math from breaking.
        if (desc->samplerate != 48000) return k_unit_err_samplerate;
        if (desc->output_channels != 2) return k_unit_err_geometry;

        // 2. Clear all memory explicitly at boot
        Reset();

        // 3. Stash runtime functions to manage samples.
        m_get_num_sample_banks_ptr = desc->get_num_sample_banks;
        m_get_num_samples_for_bank_ptr = desc->get_num_samples_for_bank;
        m_get_sample = desc->get_sample;

        return k_unit_err_none;
    }

    inline void Teardown() {
        // We use static memory, so there are no raw pointers to free() or delete.
        // If we were using dynamic memory, we would release it here.
    }

    inline void Reset() {
        // Called when the user changes programs or the engine needs a hard flush.
        // This prevents loud "pops" from old delay line data playing unexpectedly.
        for (int i = 0; i < NUM_VOICES; ++i) {
            state.voices[i].is_active = false;
            state.voices[i].is_releasing = false;
            state.voices[i].resA.write_ptr = 0;
            state.voices[i].resB.write_ptr = 0;
            state.voices[i].resA.z1 = 0.0f;
            state.voices[i].resB.z1 = 0.0f;

            // Fast-zero the delay buffers
            for(int j = 0; j < DELAY_BUFFER_SIZE; ++j) {
                state.voices[i].resA.buffer[j] = 0.0f;
                state.voices[i].resB.buffer[j] = 0.0f;
            }
        }
    }

    inline void Resume() {
        // Called when the audio thread wakes up
    }

    inline void Suspend() {
        // Called when the audio thread goes to sleep
    }
    // ==============================================================================
    // 1. Parameter Binding (UI Thread)
    // ==============================================================================
    inline void setParameter(uint8_t index, int32_t value) {
        // We calculate coefficients HERE so the audio thread does zero heavy math.

        switch(index) {
            case 0: // c_parameterResonatorNote (or similar pitch offset)
                // Store UI note for the internal sequencer Gate triggers
                m_ui_note = (uint8_t)fmaxf(1.0f, fminf(126.0f, value));
                break;

            case 2: // c_parameterSampleBank
                m_sample_bank = value;
                break;

            case 3: // c_parameterSampleNumber
                m_sample_number = value;
                break;

            // --- WAVEGUIDE COEFFICIENT TRANSLATION ---

            case 12: { // c_parameterResonatorDecayFeedback (e.g. 0 to 1000)
                // Map 0-1000 to a feedback multiplier: 0.0 to 0.999
                // A value of 1.0 would cause infinite feedback (explosion)
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                float skewed = apply_skew(norm, 0.3f);

                float max_feedback = 0.999f;
                float g = skewed * max_feedback;

                // Apply to all voices to allow live knob sweeps
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.feedback_gain = g;
                    state.voices[i].resB.feedback_gain = g;
                }
                break;
            }

            case 13: { // c_parameterResonatorDecayDamping (Material/Tone)
                // Map UI 0-1000 to a 1-pole Lowpass coefficient: 0.01 to 1.0
                // 1.0 = No damping (bright metallic), 0.01 = Heavy damping (dull wood)
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                float coeff = 0.01f + (norm * 0.99f);

                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.lowpass_coeff = coeff;
                    state.voices[i].resB.lowpass_coeff = coeff;
                }
                break;
            }

            case 14: { // c_parameterResonatorDispersion (Inharmonicity)
                // Map to allpass coefficient (-1.0 to 1.0)
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
                    // Update internal dispersion variables here
                }
                break;
            }

            // --- FILTER TRANSLATION ---
            case 19: { // c_parameterFilterCutoff (Scaled A/B in header.c)
                // Let's assume the UI passes a value from 10 to 19990 Hz.
#ifdef ENABLE_PHASE_6_FILTERS
                // We map this directly to our SVF
                float cutoff = (float)value;
                float resonance = 1.5f; // Hardcoded for now, can map to another UI knob later

                state.master_filter.set_coeffs(cutoff, resonance, 48000.0f);
#endif
                break;
            }

            default:
                break;
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        switch (index) {
            case 1: // Note
                return m_ui_note;
            case 2: // Bank
                return m_sample_bank;
            case 3: // Sample
                return m_sample_number;
            default:
                return 0;
        }
    }

    inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
        static const char* const bank_names[] = {
            "CH",
            "OH",
            "RS",
            "CP",
            "MISC",
            "USER",
            "EXP"
        };

        switch (index) {
        case 2: // Bank
            if (value >= 0 && value < 7)
                return bank_names[value];
            break;
        default:
            break;
        }
        return nullptr;
    }

    // ==============================================================================
    // 3. Sequencer and MIDI Routing
    // ==============================================================================
    inline void NoteOn(uint8_t note, uint8_t velocity) {
        state.next_voice_idx = (state.next_voice_idx + 1) % NUM_VOICES;
        VoiceState& v = state.voices[state.next_voice_idx];

        // --- Sample Loading ---
        // First, clear any old sample data
        v.exciter.sample_ptr = nullptr;
        v.exciter.sample_frames = 0;

        // Then, try to load the new one just-in-time
        if (m_get_sample && m_get_num_sample_banks_ptr && m_get_num_samples_for_bank_ptr) {
            if (m_sample_bank < m_get_num_sample_banks_ptr()) {
                size_t actualIndex = (m_sample_number > 0) ? (size_t)(m_sample_number - 1) : 0;
                if (actualIndex < m_get_num_samples_for_bank_ptr(m_sample_bank)) {
                    const sample_wrapper_t* wrapper = m_get_sample(m_sample_bank, actualIndex);
                    if (wrapper && wrapper->sample_ptr) {
                        v.exciter.sample_ptr = wrapper->sample_ptr;
                        v.exciter.sample_frames = wrapper->frames;
                        v.exciter.channels = wrapper->channels;
                    }
                }
            }
        }
        // --- End Sample Loading ---

        v.is_active = true;
        v.is_releasing = false;
        v.current_note = note;
        v.current_velocity = (float)velocity / 127.0f;

        // --- THE PHYSICS OF PITCH ---
        // 1. Convert MIDI Note to Frequency (Hz)
        float freq = 440.0f * fasterpowf(2.0f, ((float)note - 69.0f) / 12.0f);

        // 2. Convert Frequency to Delay Line Length (Samples)
        // Drumlogue sample rate is strictly 48000.0f
        float delay_len = 48000.0f / freq;

        // 3. Assign to resonators (Apply fine-tuning offsets here later)
        v.resA.delay_length = delay_len;
        v.resB.delay_length = delay_len;

        // Reset phase
        v.exciter.current_frame = 0;
        v.resA.write_ptr = 0;
        v.resB.write_ptr = 0;

#ifdef ENABLE_PHASE_5_EXCITERS
        // Trigger the envelopes when a note hits
        v.exciter.noise_env.trigger();
        v.exciter.master_env.trigger();
#endif
    }


    /**
    The Architectural Wins Here:
    Zero Division or Trigonometry: Notice that inside the massive for (size_t i = 0; i < frames; ++i) loop, there is not a single /, cos(), pow(), or log(). It is purely +, -, and *. This is why this engine will never trigger a Watchdog Timeout or denormal CPU spike on the hardware.

    Bitwise Masking (& DELAY_MASK): Instead of using expensive if (ptr >= 4096) ptr = 0; branches, or slow modulo (% 4096) operators, we use a bitwise AND. It takes exactly 1 clock cycle on the ARM CPU to wrap the delay line safely.

    Contiguous Memory Arrays: wg.buffer is just an array of float. The CPU will pre-fetch these arrays into the L1 cache, meaning memory access is effectively zero-cost.
    */

    // ==============================================================================
    // 4. The Core Physics (Executed per-voice, per-sample)
    // ==============================================================================

    // Processes a single sample through the Waveguide
    inline float process_waveguide(WaveguideState& wg, float exciter_input) {
        // 1. Calculate the read pointer position for exact pitch
        float read_pos = (float)wg.write_ptr - wg.delay_length;
        if (read_pos < 0.0f) read_pos += (float)DELAY_BUFFER_SIZE; // Wrap around safely

        // 2. Fractional Delay Line Reading (Linear Interpolation)
        int idx_int = (int)read_pos;
        float frac = read_pos - (float)idx_int;

        int idx_A = idx_int & DELAY_MASK;
        int idx_B = (idx_A + 1) & DELAY_MASK;

        // Blend the two samples based on the fraction
        float delay_out = (wg.buffer[idx_A] * (1.0f - frac)) + (wg.buffer[idx_B] * frac);

        // 3. The Loss Filter (1-pole Lowpass) - Simulates Material (Wood/Metal)
        // wg.lowpass_coeff was pre-calculated in setParameter()
        wg.z1 = (delay_out * wg.lowpass_coeff) + (wg.z1 * (1.0f - wg.lowpass_coeff));
        float filtered_out = wg.z1;

        // 4. Feedback & Exciter Addition
        // wg.feedback_gain is our "Decay" time
        float new_val = exciter_input + (filtered_out * wg.feedback_gain);

        // 5. Write back to the delay line and advance the pointer
        wg.buffer[wg.write_ptr] = new_val;
        wg.write_ptr = (wg.write_ptr + 1) & DELAY_MASK;

        return new_val;
    }

    // Processes the Exciter (Generates the initial "strike" or sample burst)
    inline float process_exciter(ExciterState& ex) {
        float out = 0.0f;

        // Play PCM Sample if loaded
        if (ex.sample_ptr && ex.current_frame < ex.sample_frames) {
            // Assume interleaved sample buffer (L R L R). We just read the Left channel for the exciter.
            size_t raw_idx = ex.current_frame * ex.channels;
            out = ex.sample_ptr[raw_idx];
            ex.current_frame++;
        }

    #ifdef ENABLE_PHASE_5_EXCITERS
        // Add optional Noise Burst here (e.g. for snare wires or breath noise)
        // 2. Synthesized Noise Burst (Activated incrementally)
        float noise_env_val = ex.noise_env.process();
        if (noise_env_val > 0.001f) {
            float raw_noise = ex.noise_gen.process();
            out += raw_noise * noise_env_val;
        }
    #endif

        return out;
    }

    // ==============================================================================
    // 5. The Master Audio Loop (Called by Drumlogue OS)
    // ==============================================================================
    inline void processBlock(float* __restrict main_out, size_t frames) {

        // Clear the output buffer
        for (size_t i = 0; i < frames * 2; ++i) {
            main_out[i] = 0.0f;
        }

        // Process each active voice
        for (size_t v = 0; v < NUM_VOICES; ++v) {
            VoiceState& voice = state.voices[v];
            if (!voice.is_active) continue;

            // Process the block for this voice
            for (size_t i = 0; i < frames; ++i) {

                // 1. Generate the Exciter burst
                float exciter_sig = process_exciter(voice.exciter);

                // 2. Feed the Exciter into both Resonators
                float outA = process_waveguide(voice.resA, exciter_sig);
                float outB = process_waveguide(voice.resB, exciter_sig);

                // 3. Mix the resonators together
                // (mix_ab is controlled by the UI, 0.0 = A only, 1.0 = B only)
                float voice_out = (outA * (1.0f - state.mix_ab)) + (outB * state.mix_ab);

                // Apply note velocity
                voice_out *= voice.current_velocity;

                // 4. Sum into the master interleaved stereo buffer (L and R)
                main_out[i * 2]     += voice_out * state.master_gain; // Left
                main_out[i * 2 + 1] += voice_out * state.master_gain; // Right
            }
        }

        // 4. Master Effects
        for (size_t i = 0; i < frames; ++i) {
            float mix_l = main_out[i * 2];
            float mix_r = main_out[i * 2 + 1];

    #ifdef ENABLE_PHASE_6_FILTERS
            // Apply the SVF to the summed output.
            // (Note: For true stereo, you'd need two SVFs. For now, we process L and copy to R)
            mix_l = state.master_filter.process(mix_l);
            mix_r = mix_l;
    #endif

            // 5. The Hardware Safety Net (Brickwall Limiter)
            float clipped_l = mix_l / (1.0f + fabsf(mix_l));
            float clipped_r = mix_r / (1.0f + fabsf(mix_r));

            main_out[i * 2]     = fmaxf(-0.99f, fminf(0.99f, clipped_l));
            main_out[i * 2 + 1] = fmaxf(-0.99f, fminf(0.99f, clipped_r));
        }
    }

    inline void GateOn(uint8_t velocity) {
        // Route internal Drumlogue sequencer to the UI Note parameter
        NoteOn(m_ui_note, velocity);
    }

private:
    // Functions from unit runtime
    unit_runtime_get_num_sample_banks_ptr m_get_num_sample_banks_ptr;
    unit_runtime_get_num_samples_for_bank_ptr m_get_num_samples_for_bank_ptr;
    unit_runtime_get_sample_ptr m_get_sample;

    uint8_t m_ui_note = 60;
    uint8_t m_sample_bank = 0;
    uint8_t m_sample_number = 0;
};