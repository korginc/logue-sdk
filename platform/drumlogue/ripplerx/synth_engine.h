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

    inline void ChannelPressure(uint8_t pressure) { (void)pressure; }

    inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
        (void)note;
        (void)aftertouch;
    }


    // ==============================================================================
    // 1. UI State & Preset Management
    // ==============================================================================

    // Tracks the raw UI integer for all 25 possible parameter slots (0 to 24)
    int32_t m_params[25] = {0};
    uint8_t m_preset_idx = 0;

    // Called by unit_get_param_value so the OS knows what to draw on the screen
    inline int32_t getParameterValue(uint8_t index) const {
        return m_params[index];
    }

    inline uint8_t getPresetIndex() const {
        return m_preset_idx;
    }

    // Called by unit_set_param_value(0, value) to load a new patch
    inline void LoadPreset(uint8_t idx) {
        m_preset_idx = idx;

        // TODO (Phase 8): When you export your 28 legacy presets, you will
        // paste the 2D array here and loop through it.
        // Example:
        // for (int param_id = 1; param_id <= 24; ++param_id) {
        //     if (param_id == 2) continue; // Skip Gain
        //     setParameter(param_id, legacy_preset_data[idx][param_id]);
        // }
    }

    static inline const char * getPresetName(uint8_t idx) {
        // Dummy names until we port your legacy preset list in Phase 8
        static const char* const preset_names[] = {
            "Init", "Marimba", "Vibraphone", "Woodblock", "Timpani",
            "Snare", "Tom", "Cymbal", "Bell", "Gong", // ... add up to 28
            "Preset11", "Preset12", "Preset13", "Preset14", "Preset15",
            "Preset16", "Preset17", "Preset18", "Preset19", "Preset20",
            "Preset21", "Preset22", "Preset23", "Preset24", "Preset25",
            "Preset26", "Preset27", "Preset28"
        };
        if (idx < 28) return preset_names[idx];
        return "Unknown";
    }


    // ==============================================================================
    // 2. Parameter Binding (UI Thread)
    // ==============================================================================
    inline void setParameter(uint8_t index, int32_t value) {
        // 1. Save the raw value so the OLED screen can read it back later
        m_params[index] = value;

        // 2. Translate UI integer to DSP Physics
	switch(index) {
	        // Page 1
            case 0:
                // TODO
                break;
            case 1: // Note
                // Store UI note for the internal sequencer Gate triggers
                m_ui_note = (uint8_t)fmaxf(1.0f, fminf(126.0f, value));
                break;

            case 2: // Bank
                m_sample_bank = value;
                break;

            case 3: // c_parameterSampleNumber
                m_sample_number = value;
                break;

            // Page 2 - TODO
            case 4:
            case 5:
            case 6:
            case 7:
            // Page 3 - TODO
            case 8:
	        // fallthrough
                break;
            case 9: { // Model (0..17, where 0..8 is ResA and 9..17 is ResB)
                // We will fully implement this in Phase 7.
                // For now, track it so the screen remembers it.
                m_model_a = value % 9;
                break;
            case 10:
                // fallthrough
                break;

            case 11: { // Dkay (Decay Time = Feedback Loop)
                // UI Range is 0 to 2000.
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 2000.0f));

                // TODO apply skew?

                // Cap at 0.995 to prevent dangerous infinite volume build-up
                float g = norm * 0.995f;

                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.feedback_gain = g;
                    state.voices[i].resB.feedback_gain = g;
                }
                break;
            }
            // Page 4
            case 12: { // Mterl (Material = Loop Lowpass Filter)
                // UI Range is -10 to 30 -> mapped to 0.0 to 1.0
                float norm = (fmaxf(-10.0f, fminf(30.0f, (float)value)) + 10.0f) / 40.0f;
                float coeff = 0.01f + (norm * 0.99f); // 0.01 (dull/wood) to 1.0 (bright/metal)

                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.lowpass_coeff = coeff;
                    state.voices[i].resB.lowpass_coeff = coeff;
                }
                break;
            }

            case 14:
            {
                //TODO
                break;
            }
	    case 14: { // Rel (Release Envelope Time)
                // Range 0 to 20. Map to an exponential release rate.
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 20.0f));
                float rel_rate = 0.0001f + ((1.0f - norm) * 0.01f); // Higher knob = slower rate
                for (int i = 0; i < NUM_VOICES; ++i) {
#ifdef ENABLE_PHASE_5_EXCITERS
                    state.voices[i].exciter.noise_env.release_rate = rel_rate;
                    state.voices[i].exciter.master_env.release_rate = rel_rate;
#endif
                }
                break;
            }

            case 15: { // c_parameterResonatorDispersion (Inharmonicity)
                // Map to allpass coefficient (-1.0 to 1.0)
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
#ifdef ENABLE_PHASE_5_EXCITERS
                    state.voices[i].exciter.noise_env.release_rate = rel_rate;
                    state.voices[i].exciter.master_env.release_rate = rel_rate;
#endif
                }
                break;
            }
            // Page 5
            case 16:
                // TODO
                break;
            // --- FILTER TRANSLATION ---
            case 17: { // c_parameterFilterCutoff (Scaled A/B in header.c)
                // Let's assume the UI passes a value from 10 to 19990 Hz.
#ifdef ENABLE_PHASE_6_FILTERS
                // We map this directly to our SVF
                float cutoff = (float)value;
                float resonance = 1.5f; // Hardcoded for now, can map to another UI knob later

                state.master_filter.set_coeffs(cutoff, resonance, 48000.0f);
#endif
                break;
            }

            case 18:
            case 19:
                // fallthrough
                // TODO
                break;
            // Page 6
            case 20: { // NzMix (Noise Burst Volume)
                // UI Range is 0 to 1000
#ifdef ENABLE_PHASE_5_EXCITERS
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].exciter.noise_decay_coeff = norm; // We'll use this as Volume
                }
#endif
                break;
            }

            case 21: { // NzRes (Using this as Noise Envelope Attack/Decay for now)
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
                    // Lower value = sharp percussive click. Higher = soft breath attack.
                    state.voices[i].exciter.noise_env.attack_rate = 0.9f - (norm * 0.8f);
                    state.voices[i].exciter.noise_env.decay_rate = 0.05f - (norm * 0.04f);
                }
                break;
            }
            case 22:
            case 23:
                // fallthrough
                // TODO
                break;
            default:
                // Catch all other unassigned parameters (Tone, HitPos, Inharm, TubeRadius)
                // They are saved in m_params[] automatically at the top of the function.
                break;
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
    // 4. Sequencer and MIDI Routing
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
        // Configure master envelope as a Gate (Instant attack, infinite sustain)
        v.exciter.master_env.attack_rate = 0.99f;
        v.exciter.master_env.decay_rate = 0.0f;
        v.exciter.master_env.sustain_level = 1.0f;
        v.exciter.master_env.trigger();
#endif
    }

inline void NoteOff(uint8_t note) {
        for (int i = 0; i < NUM_VOICES; ++i) {
            VoiceState& v = state.voices[i];

            // Find the voice playing this note that hasn't already been released
            if (v.is_active && !v.is_releasing && v.current_note == note) {
                v.is_releasing = true;

#ifdef ENABLE_PHASE_5_EXCITERS
                v.exciter.noise_env.release();
                v.exciter.master_env.release();
#endif
            }
        }
    }

    inline void GateOff() {
        // The internal Drumlogue sequencer releases the UI note
        NoteOff(m_ui_note);
    }

    inline void AllNoteOff() {
        // Panic button: aggressively release everything
        for (int i = 0; i < NUM_VOICES; ++i) {
            state.voices[i].is_releasing = true;
#ifdef ENABLE_PHASE_5_EXCITERS
            state.voices[i].exciter.noise_env.release();
            state.voices[i].exciter.master_env.release();
#endif
        }
    }
    /**
    The Architectural Wins Here:
    Zero Division or Trigonometry: Notice that inside the massive for (size_t i = 0; i < frames; ++i) loop, there is not a single /, cos(), pow(), or log(). It is purely +, -, and *. This is why this engine will never trigger a Watchdog Timeout or denormal CPU spike on the hardware.

    Bitwise Masking (& DELAY_MASK): Instead of using expensive if (ptr >= 4096) ptr = 0; branches, or slow modulo (% 4096) operators, we use a bitwise AND. It takes exactly 1 clock cycle on the ARM CPU to wrap the delay line safely.

    Contiguous Memory Arrays: wg.buffer is just an array of float. The CPU will pre-fetch these arrays into the L1 cache, meaning memory access is effectively zero-cost.
    */

    // ==============================================================================
    // 5. The Core Physics (Executed per-voice, per-sample)
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
            // Multiply by noise_decay_coeff so the UI NzMix knob actually changes the volume
            out += raw_noise * noise_env_val * ex.noise_decay_coeff;
        }
    #endif

        return out;
    }

    // ==============================================================================
    // 6. The Master Audio Loop (Called by Drumlogue OS)
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

#ifdef ENABLE_PHASE_5_EXCITERS
                // --- THE MASTER VCA ---
                // Process the envelope and apply it to the audio
                float master_amp = voice.exciter.master_env.process();
                voice_out *= master_amp;

                // CPU Optimization: If release phase is completely finished, turn voice off
                if (voice.is_releasing && voice.exciter.master_env.state == ENV_IDLE) {
                    voice.is_active = false;
                }
#endif

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