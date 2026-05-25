#pragma once

/**
 * @file synth.h
 * @brief Top-level Drumlogue FM percussion synth controller.
 *
 * This file is intentionally structured as the control and routing layer:
 * - user parameter entry point (setParameter)
 * - preset loading
 * - voice/engine selection
 * - note handling
 * - per-sample process orchestration
 *
 * DSP details live in:
 * - engine_mapping.h (parameter semantics / macro targets)
 * - kick_engine.h / snare_engine.h / metal_engine.h / perc_engine.h
 * - future optional resonant project (kept out of the active path)
 *
 * Design rules:
 * - no heap allocation
 * - fixed 4-voice SIMD layout
 * - ARM NEON preferred
 * - deterministic real-time safe behavior
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <arm_neon.h>

#include "unit.h"
#include "constants.h"
#include "prng.h"
#include "midi_handler.h"
#include "envelope_rom.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "fm_presets.h"
#include "engine_mapping.h"
#include "fm_perc_synth_process.h"

#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "hat_engine.h"

class Synth {
public:
    /*===========================================================================*/
    /* Lifecycle Methods                                                          */
    /*===========================================================================*/

    Synth(void) : sample_rate_(48000), current_preset_(0) {
        fm_perc_synth_init(&synth_);
    }

    ~Synth(void) {}

    inline int8_t Init(const unit_runtime_desc_t* desc) {
        // Check compatibility
        if (desc->samplerate != 48000)
            return k_unit_err_samplerate;

        if (desc->output_channels != 2)
            return k_unit_err_geometry;

        sample_rate_ = desc->samplerate;
        fm_perc_synth_init(&synth_);
        return k_unit_err_none;
    }

    inline void Teardown() {}
    inline void Resume() {}
    inline void Suspend() {}

    inline void Reset() {
        fm_perc_synth_init(&synth_);
        load_preset(current_preset_);
    }

    /*===========================================================================*/
    /* Audio Render                                                               */
    /*===========================================================================*/

    fast_inline void Render(float* out, size_t frames) {
        // Idle gate: if all 4 voice envelopes are in ENV_STATE_OFF, every call to
        // fm_perc_synth_process() returns exactly 0.0, so skip the entire block.
        // Horizontal AND of all 4 stage lanes (ARMv7-compatible):
        uint32x4_t off_check = vceqq_u32(synth_.envelope.stage,
                                         vdupq_n_u32(ENV_STATE_OFF));
        uint32x2_t lo_hi = vand_u32(vget_low_u32(off_check),
                                    vget_high_u32(off_check));
        lo_hi = vpmin_u32(lo_hi, lo_hi);

        if (vget_lane_u32(lo_hi, 0) == 0xFFFFFFFFu &&
            !fm_perc_synth_has_pending(&synth_)) {
            memset(out, 0, frames * 2 * sizeof(float));
            return;
        }

        float* __restrict out_p = out;
        const float* out_e = out_p + (frames << 1);  // Stereo output

        while (out_p < out_e) {
            float sample = fm_perc_synth_process(&synth_);
            out_p[0] = sample;
            out_p[1] = sample;
            out_p += 2;
        }
    }

    /*===========================================================================*/
    /* MIDI Handlers                                                              */
    /*===========================================================================*/

    inline void NoteOn(uint8_t note, uint8_t velocity) {
    fm_perc_synth_note_on(&synth_, note, velocity);
    }

    inline void NoteOff(uint8_t note) {
        fm_perc_synth_note_off(&synth_, note);
    }

    // Default gate uses a percussion root note.
    // The active instrument selector decides what gets triggered.
    inline void GateOn(uint8_t velocity) {
        NoteOn(36, velocity);   // default percussion root
    }

    inline void GateOff() {
        AllNoteOff();
    }

    inline void AllNoteOff() {
        for (int i = 0; i < 128; ++i) {
            fm_perc_synth_note_off(&synth_, static_cast<uint8_t>(i));
        }
    }

    inline void PitchBend(uint16_t bend) {
        (void)bend;  // Not implemented in basic FM percussion
        // Could be used to modulate all voices' pitch
    }

    inline void ChannelPressure(uint8_t pressure) {
        (void)pressure;  // Not implemented
    }

    inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
        (void)note;
        (void)aftertouch;
    }

    /*===========================================================================*/
    /* Parameter Interface                                                        */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        if (index >= PARAM_TOTAL) return;

        // Store parameter value
        synth_.params[index] = static_cast<int8_t>(value);

        // Update synth with new parameters
        fm_perc_synth_update_params(&synth_);
    }

    inline int32_t getParameterValue(uint8_t index) const {
        if (index >= PARAM_TOTAL) return 0;
        return synth_.params[index];
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {
        switch (index) {
            case PARAM_INSTRUMENT:
                if (value >= 0 && value < INST_COUNT) return instruments_strings[value];
                break;
            case PARAM_LFO1_SHAPE:
                if (value >= 0 && value < LFO_SHAPE_COMBO_COUNT) return lfo_shape_strings[value];
                break;
            case PARAM_LFO1_TARGET:
            case PARAM_LFO2_TARGET:
                if (value >= 0 && value < LFO_TARGET_COUNT) return lfo_target_strings[value];
                break;
            case PARAM_EUCL_TUN:
                if (value >= 0 && value < EUCLID_MODE_COUNT) return euclidean_mode_strings[value];
                break;
            default:
                break;
        }
        return nullptr;
    }

    inline const uint8_t* getParameterBmpValue(uint8_t index, int32_t value) const {
        (void)index;
        (void)value;
        return nullptr;  // Not implemented
    }

    /*===========================================================================*/
    /* Preset Management                                                          */
    /*===========================================================================*/

    inline void LoadPreset(uint8_t idx) {
        load_preset(idx);
    }

    inline uint8_t getPresetIndex() const {
        return current_preset_;
    }

    static inline const char* getPresetName(uint8_t idx) {
        if (idx < NUM_OF_PRESETS) {
            return FM_PRESETS[idx].name;
        }
        return nullptr;
    }

private:
    /*===========================================================================*/
    /* Private Methods                                                            */
    /*===========================================================================*/

    inline void load_preset(uint8_t idx) {
        load_fm_preset(idx, synth_.params);
        // Update synth with new parameters
        fm_perc_synth_update_params(&synth_);
        current_preset_ = idx;
    }

    /*===========================================================================*/
    /* Private Member Variables                                                   */
    /*===========================================================================*/

    fm_perc_synth_t synth_;
    uint32_t sample_rate_;
    uint8_t current_preset_;
};
