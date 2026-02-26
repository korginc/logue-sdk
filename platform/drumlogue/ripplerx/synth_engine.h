#pragma once

#include "unit.h"
#include "dsp_core.h"
#include "../common/runtime.h" // Drumlogue OS functions

/**
 * The Architectural Wins Here:
Pre-Calculated Math: Notice the apply_skew and division happens purely in setParameter. The DSP struct (WaveguideState) now holds a pure float like 0.993f. In Phase 3, the Audio Thread will just do a single multiplication (buffer[i] * feedback_gain).

Crash-Proof Samples: The sample loader has the exact pointer checks we discussed, but it pushes the metadata to all 4 voices instantly.

Sequencer Routing: GateOn properly routes to m_ui_note, ensuring the internal drum machine plays the pitch defined on the screen.

*/

// Utility for fast skewing
inline float apply_skew(float normalized_val, float skew) {
    // Inverse exponent mapping for log-style potentiometer curves
    return powf(normalized_val, 1.0f / skew);
}

class alignas(16) RipplerXWaveguide {
public:
    SynthState state;

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

            case 4: // c_parameterSampleBank
                m_sample_bank = value;
                loadConfigureSample();
                break;

            case 5: // c_parameterSampleNumber
                m_sample_number = value;
                loadConfigureSample();
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

            default:
                break;
        }
    }

    // ==============================================================================
    // 2. Safe Sample Loading (OS Interrogation)
    // ==============================================================================
    inline void loadConfigureSample() {
        if (m_sample_bank >= m_get_num_sample_banks_ptr()) {
            clearSampleState(); return;
        }

        size_t actualIndex = (m_sample_number > 0) ? (size_t)(m_sample_number - 1) : 0;
        if (actualIndex >= m_get_num_samples_for_bank_ptr(m_sample_bank)) {
            clearSampleState(); return;
        }

        const sample_wrapper_t* wrapper = m_get_sample(m_sample_bank, actualIndex);
        if (wrapper && wrapper->sample_ptr) {
            for (int i = 0; i < NUM_VOICES; ++i) {
                state.voices[i].exciter.sample_ptr = wrapper->sample_ptr;
                state.voices[i].exciter.sample_frames = wrapper->frames;
                state.voices[i].exciter.channels = wrapper->channels;
            }
        } else {
            clearSampleState();
        }
    }

    // ==============================================================================
    // 3. Sequencer and MIDI Routing
    // ==============================================================================
    inline void NoteOn(uint8_t note, uint8_t velocity) {
        state.next_voice_idx = (state.next_voice_idx + 1) % NUM_VOICES;
        VoiceState& v = state.voices[state.next_voice_idx];

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
    }

    inline void GateOn(uint8_t velocity) {
        // Route internal Drumlogue sequencer to the UI Note parameter
        NoteOn(m_ui_note, velocity);
    }

private:
    uint8_t m_ui_note = 60;
    uint8_t m_sample_bank = 0;
    uint8_t m_sample_number = 0;

    inline void clearSampleState() {
        for (int i = 0; i < NUM_VOICES; ++i) {
            state.voices[i].exciter.sample_ptr = nullptr;
            state.voices[i].exciter.sample_frames = 0;
        }
    }
};