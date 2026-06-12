#pragma once

/**
 * @file synth.h
 * @brief Top-level Drumlogue EffeMD synth controller.
 *
 * This file is intentionally structured as the control and routing layer:
 * - user parameter entry point (setParameter)
 * - preset loading
 * - instrument selection
 * - note handling
 * - block render orchestration (with NEON stereo store and idle skipping)
 *
 * DSP details live in:
 * - fm_perc_synth_process.h (state, note routing, per-sample process)
 * - the individual drum models (FmKickModel.cc ... TRXHiHat.cc)
 *
 * Design rules:
 * - no heap allocation
 * - deterministic real-time safe behavior
 * - ARM NEON used where it pays off; scalar model code stays host-testable
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

#include "unit.h"
#include "constants.h"
#include "fm_perc_synth_process.h"

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
        // Idle gate: the active model decayed to silence, so the whole block
        // is exactly zero. Skip all DSP.
        if (fm_perc_synth_is_idle(&synth_)) {
            memset(out, 0, frames * 2 * sizeof(float));
            return;
        }

        float* __restrict out_p = out;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        // NEON v7: generate 4 mono samples, then interleave to stereo with a
        // single vst2q (L R L R ...). Scalar tail for frames % 4.
        size_t blocks = frames >> 2;
        while (blocks--) {
            float buf[4];
            buf[0] = fm_perc_synth_process(&synth_);
            buf[1] = fm_perc_synth_process(&synth_);
            buf[2] = fm_perc_synth_process(&synth_);
            buf[3] = fm_perc_synth_process(&synth_);
            float32x4_t v = vld1q_f32(buf);
            float32x4x2_t lr = { { v, v } };
            vst2q_f32(out_p, lr);
            out_p += 8;
        }
        size_t tail = frames & 3;
        while (tail--) {
            const float sample = fm_perc_synth_process(&synth_);
            out_p[0] = sample;
            out_p[1] = sample;
            out_p += 2;
        }
#else
        const float* out_e = out_p + (frames << 1);  // Stereo output
        while (out_p < out_e) {
            const float sample = fm_perc_synth_process(&synth_);
            out_p[0] = sample;
            out_p[1] = sample;
            out_p += 2;
        }
#endif
    }

    /*===========================================================================*/
    /* MIDI Handlers                                                              */
    /*===========================================================================*/

    inline void NoteOn(uint8_t note, uint8_t velocity) {
        // MIDI: note-on with velocity 0 is a note-off.
        if (velocity == 0) {
            fm_perc_synth_note_off(&synth_, note);
            return;
        }
        fm_perc_synth_note_on(&synth_, note, velocity);
    }

    inline void NoteOff(uint8_t note) {
        fm_perc_synth_note_off(&synth_, note);
    }

    // Default gate uses the percussion root note. The active instrument
    // selector decides what gets triggered.
    inline void GateOn(uint8_t velocity) {
        NoteOn(EFFEMD_ROOT_NOTE, velocity);
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

        // Store the raw UI value, then route to the DSP layer
        synth_.params[index] = static_cast<int16_t>(value);
        fm_perc_synth_update_params(&synth_, index, value);
    }

    inline int32_t getParameterValue(uint8_t index) const {
        // The runtime expects the raw UI value back, not the derived DSP value.
        if (index >= PARAM_TOTAL) return 0;
        return synth_.params[index];
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {
        // Parameters valid for the active instrument are displayed as
        // "O:<dsp value>"; unused ones display as "x" (model returns 255).
        switch (index) {
            case K_Instrument:
                if (value >= 0 && value < INST_COUNT) return instruments_strings[value];
                break;

            case K_UseRatio:
                return value == 1 ?  "Ratio" : "ModIdx";
                break;

            case K_Euclidean_Tuning:
                if (value >= 0 && value < EUCLID_MODE_COUNT) return euclidean_mode_strings[value];
                break;

            default: {
                (void)value;
                const uint8_t instrument_id = synth_.instrument;
                const float actual_value =
                    synth_.models[instrument_id]->getParameter((fm_param_index_t)index);
                if (actual_value == 255.0f)
                    return "x"; // NOTE first page will not refresh until another page is selected!!
                snprintf(strbuf_, sizeof(strbuf_), ":%.f", actual_value);
                return strbuf_;
            }
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
            return FM_PRESET_NAMES[idx];
        }
        return nullptr;
    }

private:
    /*===========================================================================*/
    /* Private Methods                                                            */
    /*===========================================================================*/

    inline void load_preset(uint8_t idx) {
        load_fm_preset(&synth_, idx);
        current_preset_ = idx;
    }

    /*===========================================================================*/
    /* Private Member Variables                                                   */
    /*===========================================================================*/

    fm_perc_synth_t synth_;
    uint32_t sample_rate_;
    uint8_t current_preset_;
    mutable char strbuf_[16];
};
