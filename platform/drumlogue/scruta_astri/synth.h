#pragma once
#include <atomic>
#include <cstdint>
#include <cmath>
#include "float_math.h"
#include "unit.h"

// Bring in our Session 1 components
#include "wavetables.h"
#include "oscillator.h"
#include "lfo.h"
#include "filter.h"

class alignas(16) ScrutaAstri {
public:
    static constexpr float CLEAN_MOOG_BORDER = 33.0f;
    static constexpr float MOOG_SHE_BORDER = 66.0f;
    enum ParamIndex {
        k_paramProgram = 0, k_paramNote, k_paramOsc1Wave, k_paramOsc2Wave,
        k_paramO2Detune, k_paramO2SubOct, k_paramOsc2Mix, k_paramMastrVol,
        k_paramF1Cutoff, k_paramF1Reso, k_paramF2Cutoff, k_paramF2Reso,
        k_paramL1Wave, k_paramL1Rate, k_paramL1Depth, k_paramL2Rate,
        k_paramL2Wave, k_paramL2Depth, k_paramL3Wave, k_paramL3Rate,
        k_paramL3Depth, k_paramSampRed, k_paramBitRed, k_paramCMOSDist
    };

    inline int8_t Init(const unit_runtime_desc_t * desc) {
        if (desc->samplerate != 48000) return k_unit_err_samplerate;
        Reset();
        return k_unit_err_none;
    }

    inline void Reset() {
        // Zero out memory without destroying default struct values
        osc1.phase = 0.0f;
        osc2.phase = 0.0f;
        lfo1.phase = 0.0f;
        lfo2.phase = 0.0f;
        lfo3.phase = 0.0f;

        filter1.mode = 0; // Lowpass
        filter2.mode = 0; // Lowpass

        m_srr_counter = 0.0f;
        m_srr_hold_val = 0.0f;
    }

    inline void setParameter(uint8_t index, int32_t value) {
        m_params[index] = value;

        switch(index) {
            // -- LFO Rates: Exponential mapping from 0.01Hz to 1000Hz
            // A value of 0 = 0.01Hz (100 seconds per cycle for ADSR sweeps)
            // A value of 50 = 3.16Hz (Standard LFO)
            // A value of 100 = 1000Hz (Audio rate FM/AM)
            case k_paramL1Rate: {
                float hz = 0.01f * fasterpowf(100000.0f, (float)value / 100.0f);
                lfo1.set_rate(hz, 48000.0f);
                break;
            }
            case k_paramL2Rate: {
                float hz = 0.01f * fasterpowf(100000.0f, (float)value / 100.0f);
                lfo2.set_rate(hz, 48000.0f);
                break;
            }
            case k_paramL3Rate: {
                float hz = 0.01f * fasterpowf(100000.0f, (float)value / 100.0f);
                lfo3.set_rate(hz, 48000.0f);
                break;
            }

            // -- LFO Waves (Update UI maximum to 5 in header.c later!)
            case k_paramL1Wave: lfo1.wave_type = value % 6; break;
            case k_paramL2Wave: lfo2.wave_type = value % 6; break;
            case k_paramL3Wave: lfo3.wave_type = value % 6; break;

            // -- LFO Depths (0.0 to 1.0)
            case k_paramL1Depth: m_lfo1_depth = (float)value / 100.0f; break;
            case k_paramL2Depth: m_lfo2_depth = (float)value / 100.0f; break;
            case k_paramL3Depth: m_lfo3_depth = (float)value / 100.0f; break;

            // -- FX
            case k_paramSampRed: m_srr_rate = 1.0f - ((float)value / 105.0f); break;
            case k_paramBitRed:  m_brr_steps = fasterpowf(2.0f, 16.0f - (float)value); break;
            case k_paramCMOSDist: {
                float val = (float)value;

                // Store your mitigation multiplier for the audio thread
                m_asym_mod_depth = val / MOOG_SHE_BORDER;

                if (val < CLEAN_MOOG_BORDER) {
                    m_cmos_gain = 1.0f + (val * 0.5f);
                    filter1.drive = 0.0f;
                    filter1.sherman_asym = 0.0f;
                    filter1.lfo_res_mod = 0.0f;
                }
                else if (val < MOOG_SHE_BORDER) {
                    float moog_blend = (val - CLEAN_MOOG_BORDER) / CLEAN_MOOG_BORDER;
                    m_cmos_gain = 17.5f;
                    filter1.drive = moog_blend * 3.0f;
                    filter1.sherman_asym = 0.0f;
                    filter1.lfo_res_mod = 0.0f;
                }
                else {
                    float sherman_blend = (val - MOOG_SHE_BORDER) / (100.0f - MOOG_SHE_BORDER);
                    m_cmos_gain = 17.5f + (sherman_blend * 10.0f);
                    filter1.drive = 3.0f + (sherman_blend * 2.0f);
                    filter1.sherman_asym = sherman_blend;
                    filter1.lfo_res_mod = sherman_blend;
                }

                // Set Filter 2 base characteristics
                filter2.drive = filter1.drive;
                filter2.sherman_asym = filter1.sherman_asym;
                filter2.lfo_res_mod = filter1.lfo_res_mod;
                break;
            }

            // -- Mixer
            case k_paramOsc2Mix: m_osc2_mix = (float)value / 100.0f; break;
            case k_paramMastrVol: m_master_vol = (float)value / 100.0f; break;

            // -- Filters Base (Resonance 0-100 -> Q 0.707 to 5.0)
            case k_paramF1Cutoff: m_f1_base_hz = (float)value; break;
            case k_paramF1Reso:   m_f1_q = 0.707f + ((float)value / 25.0f); break;
            case k_paramF2Cutoff: m_f2_base_hz = (float)value; break;
            case k_paramF2Reso:   m_f2_q = 0.707f + ((float)value / 25.0f); break;
        }
    }

    inline void NoteOn(uint8_t note, uint8_t velocity) {
        // Calculate Base Frequency from MIDI Note
        m_base_hz = 440.0f * fasterpowf(2.0f, ((float)note - 69.0f) / 12.0f);

        // Push immediate frequency updates to oscillators
        updateOscillators();
    }

    inline void updateOscillators() {
        // True Stargazer Pitch Mapping (1Hz to 500Hz natively)
        osc1.set_frequency(m_base_hz, 48000.0f);

        // True Stargazer Detune: Linear +/- 5Hz
        float detune_hz = ((float)m_params[k_paramO2Detune] / 100.0f) * 5.0f;
        float osc2_hz = m_base_hz + detune_hz;

        if (m_params[k_paramO2SubOct] == 1) {
            osc2_hz *= 0.5f; // Drop one octave
        }

        osc2.set_frequency(osc2_hz, 48000.0f);
    }

    inline void processBlock(float* __restrict main_out, size_t frames) {

        // Grab Wavetables from UI
        osc1.current_table = SCRUTAASTRI_WAVETABLES[m_params[k_paramOsc1Wave]];
        osc2.current_table = SCRUTAASTRI_WAVETABLES[m_params[k_paramOsc2Wave]];

        for (size_t i = 0; i < frames; ++i) {

            // 1. MODULATION BLOCK
            float l1_val = lfo1.process() * m_lfo1_depth;
            float l2_val = lfo2.process() * m_lfo2_depth;
            float l3_val = lfo3.process() * m_lfo3_depth;

            // 2. OSCILLATOR MIX (With restored LFO3 Morphing!)
            int base_wave1 = m_params[k_paramOsc1Wave];

            // LFO3 sweeps the wavetable index back and forth
            int wave1_offset = (int)(l3_val * 20.0f);
            int final_wave1 = (base_wave1 + wave1_offset) % NUM_WAVETABLES;
            if (final_wave1 < 0) final_wave1 += NUM_WAVETABLES;

            osc1.current_table = SCRUTAASTRI_WAVETABLES[final_wave1];
            osc2.current_table = SCRUTAASTRI_WAVETABLES[m_params[k_paramOsc2Wave]];

            float sig1 = osc1.process() * 0.5f;
            float sig2 = osc2.process() * m_osc2_mix;
            float mixed_sig = sig1 + sig2;

            // 3. FILTER 1 (Pass l3_val for the Sherman resonance modulation)
            float f1_mod_hz = m_f1_base_hz * fasterpowf(2.0f, l1_val * 3.0f);
            filter1.set_coeffs(f1_mod_hz, m_f1_q, 48000.0f);
            mixed_sig = filter1.process(mixed_sig, l3_val);

            // 4. THE CRUSH SANDWICH
            m_srr_counter += m_srr_rate;
            if (m_srr_counter >= 1.0f) {
                m_srr_hold_val = mixed_sig;
                m_srr_counter -= 1.0f;
            }
            mixed_sig = m_srr_hold_val;

            if (m_brr_steps < 65536.0f) {
                mixed_sig = roundf(mixed_sig * m_brr_steps) / m_brr_steps;
            }

            // 5. FILTER 2 (YOUR DYNAMIC ASYMMETRY CONCEPT!)
            float f2_mod_hz = m_f2_base_hz * fasterpowf(2.0f, l2_val * 3.0f);
            filter2.set_coeffs(f2_mod_hz, m_f2_q, 48000.0f);

            // Inject the raw wavetable shape (sig1) into Filter 2's asymmetry,
            // mitigated by your MO_SHE_BORDER math!
            float dynamic_asym = filter1.sherman_asym + (sig1 * m_asym_mod_depth);

            // Keep it mathematically safe so the filter doesn't explode
            filter2.sherman_asym = fmaxf(0.0f, fminf(2.0f, dynamic_asym));

            mixed_sig = filter2.process(mixed_sig, l3_val);

            // 6. LFO 3 VCA & CMOS DISTORTION
            // LFO3 acts as a Tremolo (1.0 DC offset + LFO)
            mixed_sig *= (1.0f + l3_val);

            mixed_sig *= m_cmos_gain;
            float master_out = (mixed_sig / (1.0f + fabsf(mixed_sig))) * m_master_vol;

            main_out[i * 2]     = master_out; // Left
            main_out[i * 2 + 1] = master_out; // Right
        }
    }

private:
    int32_t m_params[24] = {0};

    // Core Engine Instances
    WavetableOsc osc1;
    WavetableOsc osc2;
    MorphingFilter filter1;
    MorphingFilter filter2;
    FastLFO lfo1;
    FastLFO lfo2;
    FastLFO lfo3;

    // Derived State Variables
    float m_base_hz = 65.4f; // Default to Note 36 (C2)
    float m_lfo1_depth = 0.0f;
    float m_lfo2_depth = 0.0f;
    float m_lfo3_depth = 0.0f;

    float m_osc2_mix = 0.5f;
    float m_master_vol = 0.5f;
    float m_asym_mod_depth = 0.0f;

    float m_f1_base_hz = 10000.0f;
    float m_f1_q = 0.707f;
    float m_f2_base_hz = 10000.0f;
    float m_f2_q = 0.707f;

    // FX State
    float m_srr_counter = 0.0f;
    float m_srr_rate = 1.0f;
    float m_srr_hold_val = 0.0f;
    float m_brr_steps = 65536.0f;
    float m_cmos_gain = 1.0f;
};