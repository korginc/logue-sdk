#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <arm_neon.h>
#include <float_math.h>
#include "../common/runtime.h" // Drumlogue OS functions
#include "unit.h"
#include "dsp_core.h"

// ==============================================================================
// UNIT TEST DEBUG HOOKS
// ==============================================================================
#ifdef UNIT_TEST_DEBUG
extern float ut_exciter_out;
extern float ut_delay_read;
extern float ut_voice_out;
#endif

/**
 * The Architectural Wins Here:
 * Pre-Calculated Math: Notice the apply_skew and division happens purely in setParameter. The DSP struct (WaveguideState) now holds a pure float like 0.993f. In Phase 3, the Audio Thread will just do a single multiplication (buffer[i] * feedback_gain).
 * Crash-Proof Samples: The sample loader has the exact pointer checks we discussed, but it pushes the metadata to all 4 voices instantly.
 * Sequencer Routing: GateOn properly routes to m_ui_note, ensuring the internal drum machine plays the pitch defined on the screen.
 *
*/

// Utility for fast skewing
inline float apply_skew(float normalized_val, float skew) {
    if (skew == 1.0f) return normalized_val;
    // Inverse exponent mapping for log-style potentiometer curves
    return fasterpowf(normalized_val, 1.0f / skew);
}

#ifdef ENABLE_PHASE_7_MODELS
FastTables g_tables;
#endif

class alignas(16) RipplerXWaveguide {
public:
    // ==============================================================================
    // PARAMETER INDEX ENUM (Strictly matches header.c)
    // ==============================================================================
    enum ParamIndex {
        k_paramProgram = 0,
        k_paramNote,        // 1
        k_paramBank,        // 2
        k_paramSample,      // 3
        k_paramMlltRes,     // 4
        k_paramMlltStif,    // 5
        k_paramVlMllRes,    // 6
        k_paramVlMllStf,    // 7
        k_paramModel,       // 8
        k_paramPartls,      // 9
        k_paramDkay,        // 10
        k_paramMterl,       // 11
        k_paramTone,        // 12
        k_paramHitPos,      // 13
        k_paramRel,         // 14
        k_paramInharm,      // 15
        k_paramLowCut,      // 16
        k_paramTubRad,      // 17
        k_paramGain,        // 18
        k_paramNzMix,       // 19
        k_paramNzRes,       // 20
        k_paramNzFltr,      // 21
        k_paramNzFltFrq,    // 22
        k_paramResnc        // 23
    };

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

#ifdef ENABLE_PHASE_7_MODELS
        g_tables.generate(48000.0f); // Pre-calculate all tuning math
#endif

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

    // Called when the user changes programs or the engine needs a hard flush.
    // This prevents loud "pops" from old delay line data playing unexpectedly.
    inline void Reset() {
        memset(&state, 0, sizeof(SynthState));
        state.master_gain = 1.0f;
        state.master_drive = 1.0f;
    }

    inline void Resume() {
        // Called when the audio thread wakes up
    }

    inline void Suspend() {
        AllNoteOff();
        Reset();
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
        // CRITICAL UI FIX: Prevent OS out-of-bounds reads
        if (index >= 24) return 0;
        return m_params[index];
    }

    inline uint8_t getPresetIndex() const {
        return m_preset_idx;
    }

    // Called by unit_set_param_value(0, value) to load a new patch
    inline void LoadPreset(uint8_t idx) {
        m_preset_idx = idx;

        // Columns Map:
        // 0:Prgram | 1:Note | 2:Bank | 3:Sample | 4:MlltRes | 5:MlltStif | 6:VlMllR | 7:VlMllS
        // 8:Model  | 9:Prtls| 10:Dkay| 11:Mterl | 12:Tone   | 13:HitPos  | 14:Rel   | 15:Inharm
        // 16:LCut  | 17:TRad| 18:Gain| 19:NzMix | 20:NzRes  | 21:NzFltr  | 22:NzFrq | 23:Resnc
        static const int32_t presets[28][24] = {
            {0, 60, 0, 1, 500, 2500, 0, 0, 0, 3, 250, 10, 0, 26, 10, 3000, 10, 5, 0, 0, 300, 0, 12000, 707}, // 0: Init
            {1, 60, 0, 1, 800, 4000, 0, 0, 6, 3, 150, -5, 0, 50, 5, 1500, 10, 5, 20, 0, 300, 0, 12000, 707}, // 1: Marimba
            {2, 36, 0, 1, 100, 500, 0, 0, 5, 3, 800, -10, 0, 50, 15, 100, 10, 5, 150, 0, 300, 0, 1000, 707}, // 2: 808 Sub
            {3, 38, 0, 1, 400, 3000, 0, 0, 5, 3, 150, 15, 0, 20, 8, 5000, 150, 5, 50, 800, 500, 2, 8000, 707}, // 3: Ac Snare
            {4, 72, 0, 1, 900, 5000, 0, 0, 8, 3, 1500, 30, 0, 10, 20, 19000, 200, 5, 0, 0, 300, 0, 12000, 707}, // 4: Tubular Bell
            {5, 40, 0, 1, 300, 500, 0, 0, 3, 3, 600, -5, 0, 30, 15, 200, 10, 5, 30, 0, 300, 0, 5000, 707}, // 5: Timpani
            {6, 48, 0, 1, 600, 2000, 0, 0, 5, 3, 300, 5, 0, 10, 12, 500, 50, 5, 50, 50, 200, 0, 6000, 707}, // 6: Djambe
            {7, 36, 0, 1, 200, 800, 0, 0, 5, 3, 700, -10, 0, 50, 18, 100, 10, 5, 200, 0, 300, 0, 4000, 707}, // 7: Taiko
            {8, 65, 0, 1, 700, 4500, 0, 0, 5, 3, 80, 20, 0, 50, 3, 2000, 250, 5, 80, 950, 150, 2, 10000, 707}, // 8: March Snare
            {9, 35, 0, 1, 100, 1500, 0, 0, 4, 3, 1800, 25, 0, 25, 20, 18000, 10, 5, 60, 100, 800, 0, 8000, 707}, // 9: Tam Tam
            {10, 72, 0, 1, 600, 4500, 0, 0, 0, 3, 800, 10, 0, 80, 12, 1, 10, 5, 0, 0, 300, 0, 10000, 707}, // 10: Koto
            {11, 72, 0, 1, 500, 3000, 0, 0, 1, 3, 1200, 15, 0, 50, 18, 50, 10, 5, 0, 0, 300, 0, 10000, 707}, // 11: Vibraphone
            {12, 76, 0, 1, 800, 3500, 0, 0, 2, 3, 50, -8, 0, 50, 2, 800, 10, 5, 0, 0, 300, 0, 5000, 707}, // 12: Woodblock
            {13, 45, 0, 1, 400, 2000, 0, 0, 5, 3, 400, -2, 0, 50, 10, 300, 10, 5, 40, 20, 300, 0, 8000, 707}, // 13: Acoustic Tom
            {14, 60, 0, 1, 800, 5000, 0, 0, 4, 3, 1400, 30, 0, 10, 18, 19500, 400, 5, 20, 600, 700, 2, 14000, 707}, // 14: Cymbal
            {15, 36, 0, 1, 200, 2000, 0, 0, 4, 3, 1900, 20, 0, 50, 20, 19000, 10, 5, 40, 100, 800, 0, 6000, 707}, // 15: Gong
            {16, 72, 0, 1, 700, 4000, 0, 0, 1, 3, 200, 25, 0, 80, 5, 10, 10, 5, 10, 0, 300, 0, 10000, 707}, // 16: Kalimba
            {17, 60, 0, 1, 600, 3500, 0, 0, 4, 3, 600, 20, 0, 30, 12, 8000, 100, 5, 20, 0, 300, 0, 10000, 707}, // 17: Steel Pan
            {18, 79, 0, 1, 900, 4800, 0, 0, 2, 3, 30, 5, 0, 50, 1, 200, 10, 5, 0, 0, 300, 0, 8000, 707}, // 18: Claves
            {19, 67, 0, 1, 800, 4500, 0, 0, 4, 3, 150, 25, 0, 20, 4, 17000, 200, 5, 30, 0, 300, 0, 10000, 707}, // 19: Cowbell
            {20, 84, 0, 1, 900, 5000, 0, 0, 1, 3, 1000, 30, 0, 10, 15, 19900, 800, 5, 0, 0, 300, 0, 15000, 707}, // 20: Triangle
            {21, 36, 0, 1, 300, 1500, 0, 0, 5, 3, 200, -5, 0, 50, 6, 200, 10, 5, 100, 50, 200, 0, 3000, 707}, // 21: Kick Drum
            {22, 60, 0, 1, 500, 3000, 0, 0, 5, 3, 50, 10, 0, 50, 3, 5000, 400, 5, 50, 1000, 100, 2, 10000, 707}, // 22: Clap
            {23, 72, 0, 1, 100, 4000, 0, 0, 5, 3, 20, 15, 0, 50, 2, 1000, 800, 5, 20, 1000, 300, 2, 12000, 707}, // 23: Shaker
            {24, 72, 0, 1, 100, 500, 0, 0, 7, 3, 900, -5, 0, 10, 12, 10, 10, 5, 0, 400, 800, 0, 6000, 707}, // 24: Flute
            {25, 60, 0, 1, 100, 500, 0, 0, 8, 3, 900, -5, 0, 10, 12, 10, 10, 5, 0, 400, 800, 0, 6000, 707}, // 25: Clarinet
            {26, 36, 0, 1, 600, 2500, 0, 0, 0, 3, 600, -8, 0, 20, 10, 5, 10, 5, 60, 0, 300, 0, 5000, 707}, // 26: Pluck Bass
            {27, 76, 0, 1, 700, 3500, 0, 0, 4, 3, 1600, 25, 0, 80, 18, 12000, 100, 5, 0, 0, 300, 0, 12000, 707} // 27: Glass Bowl
        };

        if (idx >= 28) return;

        // Apply parameters, SKIPPING INDEX 0 to prevent infinite recursion stack overflow!
        for (uint8_t param_id = 0; param_id < 24; ++param_id) {
            if (param_id == k_paramProgram) continue;
            setParameter(param_id, presets[idx][param_id]);
        }
    }

    static inline const char * getPresetName(uint8_t idx) {
        static const char* const preset_names[] = {
            "Init",         "Marimba",      "808 Sub",      "Ac Snare",
            "TubularBell",  "Timpani",      "Djambe",       "Taiko",
            "March Snare",  "Tam Tam",      "Koto",         "Vibraphone",
            "Woodblock",    "AcousticTom",  "Cymbal",       "Gong",
            "Kalimba",      "Steel Pan",    "Claves",       "Cowbell",
            "Triangle",     "Kick Drum",    "Clap",         "Shaker",
            "Flute",        "Clarinet",     "Pluck Bass",   "Glass Bowl"
        };
        if (idx < 28) return preset_names[idx];
        return "Unknown";
    }

    // ==============================================================================
    // 2. Parameter Binding (UI Thread)
    // ==============================================================================
    inline void setParameter(uint8_t index, int32_t value) {
        // CRITICAL UI FIX: Prevent OS out-of-bounds writes
        if (index >= 24) return;
        m_params[index] = value;

        switch(index) {
            case k_paramProgram:
                LoadPreset((uint8_t)value);
                break;

            case k_paramNote:
                m_ui_note = (uint8_t)fmaxf(1.0f, fminf(126.0f, value));
                break;

            case k_paramBank:
                m_sample_bank = value;
                break;

            case k_paramSample:
                m_sample_number = value;
                break;
            case k_paramMlltStif: {
                float norm = fmaxf(0.01f, fminf(1.0f, (float)value / 5000.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].exciter.mallet_stiffness = norm;
                }
                break;
            }
            case k_paramModel: {
                m_model_a = value % 9;
#ifdef ENABLE_PHASE_7_MODELS
                for (int i = 0; i < NUM_VOICES; ++i) {
                    if (m_model_a == 7 || m_model_a == 8) {
                        state.voices[i].resA.phase_mult = -1.0f;
                        state.voices[i].resB.phase_mult = -1.0f;
                    } else {
                        state.voices[i].resA.phase_mult = 1.0f;
                        state.voices[i].resB.phase_mult = 1.0f;
                    }
                }
#endif
                break;
            }

            case k_paramPartls: {
                m_active_partials = value;
                break;
            }

            case k_paramDkay: {
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 2000.0f));
                // 0.85 = instant dead thud. 0.999 = rings for 5 seconds.
                float g = 0.85f + (norm * 0.149f);
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.feedback_gain = g;
                    state.voices[i].resB.feedback_gain = g;
                }
                break;
            }

            case k_paramMterl: {
                float norm = (fmaxf(-10.0f, fminf(30.0f, (float)value)) + 10.0f) / 40.0f;
                float coeff = 0.01f + (norm * 0.99f);
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.lowpass_coeff = coeff;
                    state.voices[i].resB.lowpass_coeff = coeff;
                }
                break;
            }

            case k_paramHitPos: {
                state.mix_ab = fmaxf(0.0f, fminf(1.0f, (float)value / 100.0f));
                break;
            }

            case k_paramRel: {
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 20.0f));
                // Much slower release fade to prevent clicking when GateOff is received
                float rel_rate = 0.00005f + ((1.0f - norm) * 0.01f);
                for (int i = 0; i < NUM_VOICES; ++i) {
#ifdef ENABLE_PHASE_5_EXCITERS
                    state.voices[i].exciter.noise_env.release_rate = rel_rate;
                    state.voices[i].exciter.master_env.release_rate = rel_rate;
#endif
                }
                break;
            }

            case k_paramInharm: {
                float norm = (float)value / 20000.0f;
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].resA.ap_coeff = norm;
                    state.voices[i].resB.ap_coeff = norm;
                }
                break;
            }
            case k_paramLowCut: {
#ifdef ENABLE_PHASE_6_FILTERS
                m_master_cutoff = (float)value;
                float res_val = fmaxf(0.707f, (float)m_params[k_paramResnc] / 1000.0f);
                state.master_filter.set_coeffs(m_master_cutoff, res_val, 48000.0f);
#endif
                break;
            }
            case k_paramGain: {
                float norm = fmaxf(0.0f, (float)value / 100.0f);
                state.master_drive = 1.0f + (norm * 20.0f);
                break;
            }
            case k_paramNzMix: {
                // Updated for the new 0-100 header.c range
#ifdef ENABLE_PHASE_5_EXCITERS
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 100.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].exciter.noise_decay_coeff = norm;
                }
#endif
                break;
            }

            case k_paramNzRes: {
#ifdef ENABLE_PHASE_5_EXCITERS
                // Leaving this at the old 0-1000 scale
                float norm = fmaxf(0.0f, fminf(1.0f, (float)value / 1000.0f));
                for (int i = 0; i < NUM_VOICES; ++i) {
                    state.voices[i].exciter.noise_env.attack_rate = 0.9f - (norm * 0.8f);
                    // Slower decay so the noise actually injects energy into the tube
                    state.voices[i].exciter.noise_env.decay_rate = 0.0001f + ((1.0f - norm) * 0.005f);
                }
#endif
                break;
            }
            case k_paramResnc: {
#ifdef ENABLE_PHASE_6_FILTERS
                // UI passes 707 to 4000. Divide by 1000 to get a Q factor of 0.707 to 4.0
                float res_val = fmaxf(0.707f, (float)value / 1000.0f);
                state.master_filter.set_coeffs(m_master_cutoff, res_val, 48000.0f);
#endif
                break;
            }

            default:
                break;
        }
    }

    inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
        static const char* const bank_names[] = { "CH", "OH", "RS", "CP", "MISC", "USER", "EXP" };
        static const char* const model_names[] = {
            "String", "Beam", "Square", "Membrn", "Plate",
            "Drumhd", "Marimb", "OpnTub", "ClsTub"
        };
        static const char* const partial_names[] = {"4", "8", "16", "32", "64"};
        static const char* const nz_filter_names[] = {"LP", "BP", "HP"};

        if (index == k_paramProgram) {
            return getPresetName((uint8_t)value);
        } else if (index == k_paramBank) {
            if (value >= 0 && value < 7) return bank_names[value];
        } else if (index == k_paramModel) {
            if (value >= 0 && value < 9) return model_names[value];
            if (value >= 9 && value < 18) return model_names[value - 9];
        } else if (index == k_paramPartls) {
            if (value >= 0 && value < 5) return partial_names[value];
        } else if (index == k_paramNzFltr) {
            if (value >= 0 && value < 3) return nz_filter_names[value];
        }

        // Unconditional failsafe to prevent OS screen crashes
        return "---";
    }

    // ==============================================================================
    // 4. Sequencer and MIDI Routing
    // ==============================================================================
    inline void NoteOn(uint8_t note, uint8_t velocity) {
        state.next_voice_idx = (state.next_voice_idx + 1) % NUM_VOICES;
        VoiceState& v = state.voices[state.next_voice_idx];

        // CRITICAL FIX 2: Ensure the voice actually turns on!
        v.is_active = true;
        v.is_releasing = false;

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
#ifdef ENABLE_PHASE_7_MODELS
        // 1. O(1) Array Lookup for absolute baseline pitch
        float base_delay = g_tables.note_to_delay_length[note & 127];

        // 2. Structural Routing based on Model
        if (m_model_a == 3 || m_model_a == 5) {
            // Membrane / Drumhead Logic:
            // A drum skin vibrates in chaotic 2D modes. We fake this by forcing
            // Resonator B to track an irrational offset (e.g., 0.68) of Resonator A.
            v.resA.delay_length = base_delay;
            v.resB.delay_length = base_delay * 0.68f;
        } else {
            // Standard matched resonators (Strings, Tubes, Bars)
            v.resA.delay_length = base_delay;
            v.resB.delay_length = base_delay;
        }
#else
        // Legacy fallback calculation
        // 1. Convert MIDI Note to Frequency (Hz)
        float freq = 440.0f * fasterpowf(2.0f, ((float)note - 69.0f) / 12.0f);
        // 2. Convert Frequency to Delay Line Length (Samples)
        // Drumlogue sample rate is strictly 48000.0f
        float delay_len = 48000.0f / freq;
        // 3. Assign to resonators (Apply fine-tuning offsets here later)
        v.resA.delay_length = delay_len;
        v.resB.delay_length = delay_len;
#endif

        // Reset phase
        v.exciter.current_frame = 0;
        v.resA.ap_x1 = 0.0f;
        v.resA.ap_y1 = 0.0f;
        v.resB.ap_x1 = 0.0f;
        v.resB.ap_y1 = 0.0f;

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


    // ==============================================================================
    // 5. The Core Physics (Executed per-voice, per-sample)
    // ==============================================================================

    // Processes a single sample through the Waveguide
    inline float process_waveguide(WaveguideState& wg, float exciter_input) {
        // 1. Calculate the read pointer position for exact pitch
        float read_pos = (float)wg.write_ptr - wg.delay_length;
        float read_idx = (float)wg.write_ptr - wg.delay_length;

        // CRITICAL DSP FIX: Handle massive sub-bass wrap-arounds
        while (read_idx < 0.0f) {
            read_idx += (float)DELAY_BUFFER_SIZE;
        }

        // Explicitly mask BOTH indices to guarantee we never read out-of-bounds memory
        uint32_t idx_A = ((uint32_t)read_idx) & DELAY_MASK;
        uint32_t idx_B = (idx_A + 1) & DELAY_MASK;
        float frac = read_idx - (float)((uint32_t)read_idx);

        // Blend the two samples based on the fraction
        float delay_out = (wg.buffer[idx_A] * (1.0f - frac)) + (wg.buffer[idx_B] * frac);

        // 3. The Loss Filter (1-pole Lowpass) - Simulates Material (Wood/Metal)
        // wg.lowpass_coeff was pre-calculated in setParameter()
        wg.z1 = (delay_out * wg.lowpass_coeff) + (wg.z1 * (1.0f - wg.lowpass_coeff));
        float filtered_out = wg.z1;


        // --- NEW: Dispersion (Allpass Filter) ---
        // This slightly delays high frequencies, stretching the harmonics out of tune
        // to create metallic, stiff, or bell-like tones.
        float ap_out = (wg.ap_coeff * filtered_out) + wg.ap_x1 - (wg.ap_coeff * wg.ap_y1);
        wg.ap_x1 = filtered_out;
        wg.ap_y1 = ap_out;

        // 4. Feedback & Exciter Addition
        // wg.feedback_gain is our "Decay" time
        // --- PHASE 7: The Topology Multiplier (Use ap_out instead of filtered_out) ---
#ifdef ENABLE_PHASE_7_MODELS
        float new_val = exciter_input + (ap_out * wg.feedback_gain * wg.phase_mult);
#else
        float new_val = exciter_input + (ap_out * wg.feedback_gain);
#endif

        // 5. Write back to the delay line and advance the pointer
        wg.buffer[wg.write_ptr] = new_val;
        wg.write_ptr = (wg.write_ptr + 1) & DELAY_MASK;

        return delay_out;
    }

    // Processes the Exciter (Generates the initial "strike" or sample burst)
    inline float process_exciter(ExciterState& ex) {
        float out = 0.0f;

        if (ex.sample_ptr && ex.current_frame < ex.sample_frames) {
            size_t raw_idx = ex.current_frame * ex.channels;
            out = ex.sample_ptr[raw_idx];
        }

#ifdef ENABLE_PHASE_5_EXCITERS
        float noise_env_val = ex.noise_env.process();
        if (noise_env_val > 0.001f) {
            float raw_noise = ex.noise_gen.process();
            out += raw_noise * noise_env_val * ex.noise_decay_coeff;
        }
#endif

        // 3. The Modal Mallet Strike
        float mallet_impulse = (ex.current_frame == 0) ? 1.0f : 0.0f;
        ex.mallet_lp = (mallet_impulse * ex.mallet_stiffness) + (ex.mallet_lp * (1.0f - ex.mallet_stiffness));

        // Massive energy multiplier to physically ring the tube
        out += ex.mallet_lp * 15.0f;

        // CRITICAL FIX: Increment time AT THE VERY END so Frame 0 actually triggers
        ex.current_frame++;

        return out;
    }

    // ==============================================================================
    // 6. The Master Audio Loop (Called by Drumlogue OS)
    // ==============================================================================
    inline void processBlock(float* __restrict main_out, size_t frames) {

        // CRITICAL FIX 1: Clear the hardware buffer!
        // If we don't do this, the += accumulation can cause NaN silence.
        for (size_t i = 0; i < frames * 2; ++i) {
            main_out[i] = 0.0f;
        }

        // Sum active voices into the master buffer
        for (int voice_idx = 0; voice_idx < NUM_VOICES; ++voice_idx) {
            VoiceState& voice = state.voices[voice_idx];

            if (!voice.is_active) continue;

            // Process the block for this voice
            for (size_t i = 0; i < frames; ++i) {

                // 1. Generate the Exciter burst
                float exciter_sig = process_exciter(voice.exciter);

                // 2. Feed the Exciter into Resonator A (Always active)
                float outA = process_waveguide(voice.resA, exciter_sig);
                float outB = 0.0f;

                // 3. Active Partial Counting: Only process ResB if complexity is high enough
                if (m_active_partials >= 2) {
                    outB = process_waveguide(voice.resB, exciter_sig);
                }

                // 4. Mix the resonators together
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

// ==============================================================================
// [UT1/UT2/UT3] TRACK ACTIVE SIGNAL FOR UNIT TEST
// ==============================================================================
#ifdef UNIT_TEST_DEBUG
                if (voice_idx == state.next_voice_idx) {
                    ut_exciter_out = exciter_sig;
                    ut_delay_read = outA;
                    ut_voice_out = voice_out;
                }
#endif
            }
        }

        // 4. Master Effects & Overdrive
        for (size_t i = 0; i < frames; ++i) {
            float mix_l = main_out[i * 2];
            float mix_r = main_out[i * 2 + 1];

#ifdef ENABLE_PHASE_6_FILTERS
            mix_l = state.master_filter.process(mix_l);
            mix_r = mix_l;
#endif

            // --- THE NEW DRUM OVERDRIVE ---
            // 1. Multiply by the UI Gain parameter (e.g., 1.0x to 10.0x)
            mix_l *= state.master_drive;
            mix_r *= state.master_drive;

            // 2. Soft-Clipping (Distortion Curve)
            // Equation: x / (1.0 + abs(x)) curves the peaks mathematically
            float clipped_l = mix_l / (1.0f + fabsf(mix_l));
            float clipped_r = mix_r / (1.0f + fabsf(mix_r));

            // TODO - new distortion

            // 5. The Hardware Safety Net (Brickwall Limiter)
            main_out[i * 2]     = fmaxf(-0.99f, fminf(0.99f, clipped_l));
            main_out[i * 2 + 1] = fmaxf(-0.99f, fminf(0.99f, clipped_r));
        }
    }

    inline void GateOn(uint8_t velocity) {
        // Route internal Drumlogue sequencer to the UI Note parameter
        NoteOn(m_ui_note, velocity);
    }

private:
    float m_master_cutoff = 10000.0f; // Default open filter

    // Functions from unit runtime
    unit_runtime_get_num_sample_banks_ptr m_get_num_sample_banks_ptr;
    unit_runtime_get_num_samples_for_bank_ptr m_get_num_samples_for_bank_ptr;
    unit_runtime_get_sample_ptr m_get_sample;

    uint8_t m_ui_note = 60;
    uint8_t m_sample_bank = 0;
    uint8_t m_sample_number = 0;
    uint8_t m_model_a = 0;

    uint8_t m_active_partials = 4; // Default to maximum complexity
};
