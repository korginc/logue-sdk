#pragma once
/*
 *  File: synth.h
 *  FM Percussion Synthesizer for drumlogue
 *
 *  4-voice FM percussion synth with LFO modulation
 *  Version 1.0
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <arm_neon.h>

#include "unit.h"
#include "fm_perc_synth.h"
#include "fm_presets.h"
#include "constants.h"

class Synth {
public:
    /*===========================================================================*/
    /* Lifecycle Methods */
    /*===========================================================================*/

    Synth(void) : sample_rate_(48000), active_voices_(0) {
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

        // Initialize synth with default preset
        fm_perc_synth_init(&synth_);
        load_preset(0);  // Load first preset

        return k_unit_err_none;
    }

    inline void Teardown() {
        // Nothing to clean up - all static allocation
    }

    inline void Reset() {
        fm_perc_synth_init(&synth_);
        active_voices_ = 0;
    }

    inline void Resume() {}
    inline void Suspend() {}

    /*===========================================================================*/
    /* Audio Render */
    /*===========================================================================*/

    fast_inline void Render(float* out, size_t frames) {
        float* __restrict out_p = out;
        const float* out_e = out_p + (frames << 1);  // Stereo output

        // Process in blocks for efficiency
        while (out_p < out_e) {
            // Generate mono sample from synth
            float sample = fm_perc_synth_process(&synth_);

            // Output to both channels (stereo)
            out_p[0] = sample;
            out_p[1] = sample;

            out_p += 2;
        }
    }

    /*===========================================================================*/
    /* MIDI Handlers */
    /*===========================================================================*/

    inline void NoteOn(uint8_t note, uint8_t velocity) {
        fm_perc_synth_note_on(&synth_, note, velocity);
        active_voices_ = vgetq_lane_u32(synth_.voice_triggered, 0) |
                        vgetq_lane_u32(synth_.voice_triggered, 1) |
                        vgetq_lane_u32(synth_.voice_triggered, 2) |
                        vgetq_lane_u32(synth_.voice_triggered, 3);
    }

    inline void NoteOff(uint8_t note) {
        fm_perc_synth_note_off(&synth_, note);
        // Update active voices count
        active_voices_ = 0;
    }

    inline void GateOn(uint8_t velocity) {
        // If no specific note, trigger default mapping (C2, D2, F#2, A2)
        NoteOn(36, velocity);  // Kick
        NoteOn(38, velocity);  // Snare
        NoteOn(42, velocity);  // Metal
        NoteOn(45, velocity);  // Perc
    }

    inline void GateOff() {
        AllNoteOff();
    }

    inline void AllNoteOff() {
        // Clear all active notes
        for (int i = 0; i < 128; i++) {
            fm_perc_synth_note_off(&synth_, i);
        }
        active_voices_ = 0;
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
        (void)aftertouch;  // Not implemented
    }

    /*===========================================================================*/
    /* Parameter Interface */
    /*===========================================================================*/

    inline void setParameter(uint8_t index, int32_t value) {
        if (index >= 24) return;

        // Store parameter value
        synth_.params[index] = (uint8_t)value;

        // Update synth with new parameters
        fm_perc_synth_update_params(&synth_);
    }

    inline int32_t getParameterValue(uint8_t index) const {
        if (index >= 24) return 0;
        return synth_.params[index];
    }

    inline const char* getParameterStrValue(uint8_t index, int32_t value) const {

        switch (index) {
            case 12: case 16:  // LFO1 Shape and LFO2 Shape
                if (value >= 0 && value <= 8) return lfo_shape_strings[value];
                break;
            case 14: case 18:  // LFO1 Dest and LFO2 Dest
                if (value >= 0 && value <= 7) return lfo_target_strings[value];
                break;
            case 21:  // Voice Alloc
                if (value >= 0 && value <= 11) return voice_alloc_strings[value];
                break;
            case 22:  // Resonant Mode
                if (value >= 0 && value <= 4) return resonant_mode_strings[value];
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
    /* Preset Management */
    /*===========================================================================*/

    inline void LoadPreset(uint8_t idx) {
        load_preset(idx);
    }

    inline uint8_t getPresetIndex() const {
        return current_preset_;
    }

    static inline const char* getPresetName(uint8_t idx) {
        if (idx < 12) {
            return FM_PRESETS[idx].name;
        }
        return nullptr;
    }

private:
    /*===========================================================================*/
    /* Private Methods */
    /*===========================================================================*/

    inline void load_preset(uint8_t idx) {
        if (idx >= 12) return;

        const fm_preset_t* p = &FM_PRESETS[idx];

        // Page 1: Probabilities
        synth_.params[0] = p->prob_kick;
        synth_.params[1] = p->prob_snare;
        synth_.params[2] = p->prob_metal;
        synth_.params[3] = p->prob_perc;

        // Page 2: Kick + Snare
        synth_.params[4] = p->kick_sweep;
        synth_.params[5] = p->kick_decay;
        synth_.params[6] = p->snare_noise;
        synth_.params[7] = p->snare_body;

        // Page 3: Metal + Perc
        synth_.params[8] = p->metal_inharm;
        synth_.params[9] = p->metal_bright;
        synth_.params[10] = p->perc_ratio;
        synth_.params[11] = p->perc_var;

        // Page 4: LFO1
        synth_.params[12] = p->lfo1_shape;
        synth_.params[13] = p->lfo1_rate;
        synth_.params[14] = p->lfo1_target;
        synth_.params[15] = (uint8_t)(p->lfo1_depth + 100);  // -100..100 to 0..200

        // Page 5: LFO2
        synth_.params[16] = p->lfo2_shape;
        synth_.params[17] = p->lfo2_rate;
        synth_.params[18] = p->lfo2_target;
        synth_.params[19] = (uint8_t)(p->lfo2_depth + 100);

        // Page 6: Envelope
        synth_.params[20] = p->env_shape;
        synth_.params[21] = 0;
        synth_.params[22] = 0;
        synth_.params[23] = 0;

        // Update synth with new parameters
        fm_perc_synth_update_params(&synth_);
        current_preset_ = idx;
    }

    /*===========================================================================*/
    /* Private Member Variables */
    /*===========================================================================*/

    fm_perc_synth_t synth_;
    uint32_t sample_rate_;
    uint32_t active_voices_;
    uint8_t current_preset_;
    std::atomic_uint_fast32_t flags_;
};