/**
 * @file synth.h
 * @brief stargazer drone synth supercharged
 *
 * Features:
 * - 3 distortion modes including SubOctave
 * - 3 LFOs, preset is LFO target
 * - new wavetables
 * - NEON-optimized for ARMv7
 *
 */
#pragma once

#include <cstdint>
#include <cmath>
#include <arm_neon.h>
#include "float_math.h"
#include "unit.h"

// Bring in our Session 1 components
#include "wavetables.h"
#include "oscillator.h"
#include "lfo.h"
#include "filter.h"
#include "drone_engine.h"

constexpr float Q_Limit = 0.707f;
constexpr float Mid_Note_Freq = 69.0f;
constexpr float Audio_Rate_Freq = 100000.0f;
constexpr float percent_normalizer = 0.01f;
constexpr int neon_lanes = 4;

class alignas(16) ScrutaAstri {
public:
     enum ParamIndex {
        k_paramProgram = 0, k_paramNote, k_paramOsc1Wave, k_paramOsc2Wave,
        k_paramO2Detune, k_paramO2SubOct, k_paramOsc2Mix, k_paramMastrVol,
        k_paramF1Cutoff, k_paramF1Reso, k_paramF2Cutoff, k_paramF2Reso,
        k_paramL1Wave, k_paramL1Rate, k_paramL1Depth, k_paramL2Wave,
        k_paramL2Rate, k_paramL2Depth, k_paramL3Wave, k_paramL3Rate,
        k_paramL3Depth, k_paramSampRed, k_paramBitRed, k_paramCMOSDist,
        k_paramLast // this one is just a marker - same value as header.c
    };

    enum FilterIndex {
        k_bypass,
        k_filter1,
        k_filter2,
        k_filter_both,
        k_filter_last
    };

    // Constant Borders
    static constexpr float CLEAN_MOOG_BORDER = 33.0f;
    static constexpr float MOOG_SHERMAN_BORDER = 66.0f;
    static constexpr uint32_t SAMPLE_RATE = 48000;
    static constexpr float SAMPLE_RATE_F = (float)SAMPLE_RATE;

    inline int8_t Init(const unit_runtime_desc_t * desc) {
        if (desc->samplerate != SAMPLE_RATE) return k_unit_err_samplerate;
        m_crystal_drone.init(false);
        m_metal_drone.init(true);
        m_crystal_drone.set_note(m_base_hz);
        m_metal_drone.set_note(m_base_hz);
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

        filter1.mode = mode_low; // Lowpass
        filter2.mode = mode_low; // Lowpass

        m_srr_counter = 0.0f;
        m_srr_hold_val = 0.0f;

        m_osc1_filter_target = k_filter1; // Default to Sherman Sandwich path
        m_osc2_filter_target = k_filter2;

        // Reset morphing states
        m_osc1_morph_phase = 1.0f;
        m_osc1_morph_active = false;
        m_osc1_current_wave_idx = 0; // Assuming default k_paramOsc1Wave is 0
        m_osc1_target_wave_idx = 0;
        m_osc1_prev_wave_idx = 0;

        m_osc2_morph_phase = 1.0f;
        m_osc2_morph_active = false;
        m_osc2_current_wave_idx = 0; // Assuming default k_paramOsc2Wave is 0
        m_osc2_target_wave_idx = 0;
        m_osc2_prev_wave_idx = 0;

        // Reset suboctave smoothing
        m_osc2_suboct_smooth_mult = 1.0f;

        m_crystal_drone.clear();
        m_metal_drone.clear();
    }

    inline int32_t  getParameter(uint8_t id) {
        if (id >= k_paramLast) return 0;
        return m_params[id];
    }

    inline void setParameter(uint8_t index, int32_t value) {
        m_params[index] = value;

        switch(index) {
            case k_paramProgram: {
                // Re-sync LFOs on Program change to act as a Drone Reset
                lfo1.phase = 0.0f;
                lfo2.phase = 0.0f;
                lfo3.phase = 0.0f;

                // Reset multipliers/offsets
                m_lfo1_mod_val = 0.0f;
                m_lfo2_mod_val = 0.0f;
                m_lfo3_mod_val = 0.0f;

                // preset will target different modulation
                int32_t mod_target = m_params[k_paramProgram] % k_paramLast;
                // set here the targets addressed by ring_modulation and let the
                // other to be processed runtime in processBlock() as they use directly
                // the lfo values, that are not available here.
                switch (mod_target) {
                    case k_paramL1Wave: filter1.mode =
                        (filter_mode)((m_params[k_paramL1Wave] + (int)(m_lfo1_mod_val * 10.0f))
                                      % mode_last);
                        break;
                    case k_paramL2Wave: filter2.mode =
                        (filter_mode)((m_params[k_paramL2Wave]  + (int)(m_lfo2_mod_val * 10.0f))
                                      % mode_last);
                        break;

                    case k_paramL1Depth:
                        m_lfo1_mod_val = m_lfo1_depth;
                        break;

                    case k_paramL2Depth:
                        m_lfo2_mod_val = m_lfo2_depth;
                        break;

                    case k_paramL3Depth:
                        m_lfo3_mod_val = m_lfo3_depth;
                        break;
                    case k_paramO2SubOct:
                        m_lfo1_mod_val = m_lfo1_depth * 0.1f;
                        m_lfo2_mod_val = m_lfo2_depth;
                        m_lfo3_mod_val = m_lfo3_depth * 0.3f;
                        break;
                    case k_paramL3Wave:
                        m_lfo1_mod_val = m_lfo1_depth * 0.2f;
                        m_lfo2_mod_val = m_lfo2_depth * 0.3f;
                        m_lfo3_mod_val = m_lfo3_depth;
                        break;
                    case k_paramBitRed:
                        m_lfo1_mod_val = m_lfo1_depth;
                        m_lfo2_mod_val = m_lfo2_depth;
                        m_lfo3_mod_val = m_lfo3_depth;
                        break;
                    default:
                        break;
                }
                // Decode ranges:
                // 0-23: Normal
                // 24-47: Osc 1 Reversed
                // 48-71: Osc 2 Reversed
                // 72-95: Both Reversed
                int preset = value % 96;
                m_osc1_dir = (preset >= k_paramLast && preset < 2*k_paramLast) || (preset >= 3*k_paramLast) ? -1.0f : 1.0f;
                m_osc2_dir = (preset >= 2*k_paramLast) ? -1.0f : 1.0f;

                // Force frequency target updates
                updateOscillators();

                // Entering drone preset: re-init FDN state and sync all drone controls
                // from current knob positions so the FDN starts fresh on every entry.
                if (value >= 95) {
                    m_crystal_drone.init(false);
                    m_metal_drone.init(true);
                    m_crystal_drone.set_note(m_base_hz);
                    m_metal_drone.set_note(m_base_hz);
                    m_crystal_drone.set_feedback(m_params[k_paramO2Detune]);
                    m_metal_drone.set_feedback(m_params[k_paramO2Detune]);
                    m_crystal_drone.set_drive(m_params[k_paramO2SubOct]);
                    m_metal_drone.set_drive(m_params[k_paramO2SubOct]);
                    m_crystal_drone.set_noise(m_params[k_paramOsc2Mix]);
                    m_metal_drone.set_noise(m_params[k_paramOsc2Mix]);
                }
                // Initialize current wave indices to match the program's default
                // This is important for morphing logic to start from a known state.
                m_osc1_current_wave_idx = m_params[k_paramOsc1Wave];
                m_osc1_target_wave_idx = m_params[k_paramOsc1Wave];
                m_osc1_morph_phase = 1.0f; // Morph is not active initially
                m_osc2_current_wave_idx = m_params[k_paramOsc2Wave];
                m_osc2_target_wave_idx = m_params[k_paramOsc2Wave];
                m_osc2_morph_phase = 1.0f; // Morph is not active initially
                break;
            }
            case k_paramOsc1Wave: {
                if (value != m_osc1_target_wave_idx) {
                    m_osc1_prev_wave_idx = m_osc1_current_wave_idx;
                    m_osc1_target_wave_idx = value;
                    m_osc1_morph_phase = 0.0f;
                    m_osc1_morph_active = true;
                    m_osc1_morph_speed = 1.0f / (SAMPLE_RATE_F * 0.05f); // 50ms morph duration
                }
                break;
            }
            case k_paramOsc2Wave: {
                if (value != m_osc2_target_wave_idx) {
                    m_osc2_prev_wave_idx = m_osc2_current_wave_idx;
                    m_osc2_target_wave_idx = value;
                    m_osc2_morph_phase = 0.0f;
                    m_osc2_morph_active = true;
                    m_osc2_morph_speed = 1.0f / (SAMPLE_RATE_F * 0.05f); // 50ms morph duration
                }
                break;
            }
            case k_paramNote:
                m_base_hz = 440.0f * fasterpow2f(((float)value - Mid_Note_Freq) * 0.083333f); // same as 1/12
                updateOscillators();
                m_crystal_drone.set_note(m_base_hz);
                m_metal_drone.set_note(m_base_hz);
                break;
            case k_paramO2Detune:
                updateOscillators();
                // Drone mode: O2Detune → FDN feedback gain (chaos level)
                m_crystal_drone.set_feedback(value);
                m_metal_drone.set_feedback(value);
                break;
            case k_paramO2SubOct:
                updateOscillators();
                // Drone mode: O2SubOct → FDN saturation drive (harmonic density)
                m_crystal_drone.set_drive(value);
                m_metal_drone.set_drive(value);
                break;
            // -- LFO Rates: Exponential mapping from 0.01Hz to 1000Hz
            // A value of 0 = 0.01Hz (100 seconds per cycle for ADSR sweeps)
            // A value of 50 = 3.16Hz (Standard LFO)
            // A value of 100 = 1000Hz (Audio rate FM/AM)
            case k_paramL1Rate: {
                float hz = 0.01f * fasterpowf(Audio_Rate_Freq, (float)value * percent_normalizer);
                lfo1.set_rate(hz, SAMPLE_RATE_F);
                break;
            }
            case k_paramL2Rate: {
                float hz = 0.01f * fasterpowf(Audio_Rate_Freq, (float)value * percent_normalizer);
                lfo2.set_rate(hz, SAMPLE_RATE_F);
                break;
            }
            case k_paramL3Rate: {
                float hz = 0.01f * fasterpowf(Audio_Rate_Freq, (float)value * percent_normalizer);
                lfo3.set_rate(hz, SAMPLE_RATE_F);
                break;
            }

            // -- LFO Waves (Updated UI maximum to 8 in header.c)
            case k_paramL1Wave: lfo1.wave_type = value % LFO_WAVE_COUNT; break;
            case k_paramL2Wave: lfo2.wave_type = value % LFO_WAVE_COUNT; break;
            case k_paramL3Wave: lfo3.wave_type = value % LFO_WAVE_COUNT; break;

            // -- LFO Depths (0.0 to 1.0)
            case k_paramL1Depth: m_lfo1_depth = (float)value * percent_normalizer; break;
            case k_paramL2Depth: m_lfo2_depth = (float)value * percent_normalizer; break;
            case k_paramL3Depth: m_lfo3_depth = (float)value * percent_normalizer; break;

            // -- FX
            case k_paramSampRed: m_srr_rate = 1.0f - ((float)value * 0.00952381f); break; // same as 1/105
            case k_paramBitRed:  m_brr_steps = fasterpow2f(16.0f - (float)value); break;

            // FIX: The Sherman Destruction Boost
            case k_paramCMOSDist: {
                if (value <= CLEAN_MOOG_BORDER) {
                    float t = (float)value / (float)CLEAN_MOOG_BORDER;
                    m_cmos_gain = 1.0f + t;  // 1.0→2.0
                    // Gentle drive even in the "clean" zone so 1-32% isn't a dead zone
                    m_cmos_filter_drive = t * 0.5f;  // 0.0→0.5
                    m_sherman_asym_base = 0.0f;
                    m_sherman_makeup = 1.0f;
                } else if (value <= MOOG_SHERMAN_BORDER) {
                    m_cmos_gain = 2.0f;
                    m_cmos_filter_drive = ((float)(value - CLEAN_MOOG_BORDER) / (float)(MOOG_SHERMAN_BORDER - CLEAN_MOOG_BORDER)) * 2.0f;
                    m_sherman_asym_base = 0.0f;
                    m_sherman_makeup = 1.0f;
                } else {
                    m_cmos_gain = 2.0f;
                    m_cmos_filter_drive = 2.0f;
                    // Update base with the parameter calculation
                    m_sherman_asym_base = ((float)(value - MOOG_SHERMAN_BORDER) / (percent_normalizer - MOOG_SHERMAN_BORDER)) * 2.0f;
                    m_sherman_makeup = 1.0f + m_sherman_asym_base * 0.5f;
                }
                break;
            }

            // -- Mixer
            case k_paramOsc2Mix:
                m_osc2_mix = (float)value * percent_normalizer;
                // Drone mode: O2Mix → FDN noise injection level
                m_crystal_drone.set_noise(value);
                m_metal_drone.set_noise(value);
                break;
            // FIX: 300% Volume Headroom!
            case k_paramMastrVol: m_master_vol_base = ((float)value * percent_normalizer) * 3.0f; break;
            // FIX: Cutoff 10x Trick
            case k_paramF1Cutoff: m_f1_base_hz = (float)value * 10.0f; break;
            case k_paramF2Cutoff: m_f2_base_hz = (float)value * 10.0f; break;
            // -- Filters Base (Resonance 0-100 -> Q 0.707 to 5.0)
            case k_paramF1Reso:
                // We still set true resonance normally on knob turn (calculated once per block)
                m_f1_q = Q_Limit + ((float)value * 0.04f);
                filter1.set_coeffs(m_f1_base_hz, m_f1_q, Audio_Rate_Freq);
                // BUT we also track a drive base for the LFO to modulate later
                m_f1_drive_base = (value * percent_normalizer) * 5.0f;
                break;
            case k_paramF2Reso:
                // We still set true resonance normally on knob turn (calculated once per block)
                m_f2_q =Q_Limit + ((float)value / 25.0f);
                filter2.set_coeffs(m_f2_base_hz, m_f2_q, Audio_Rate_Freq);
                // BUT we also track a drive base for the LFO to modulate later
                m_f2_drive_base = (value * percent_normalizer) * 5.0f;
                break;
        }
    }

    inline void NoteOn(uint8_t note, uint8_t velocity) {
        // Calculate Base Frequency from MIDI Note
        m_base_hz = 440.0f * fasterpow2f(((float)note - Mid_Note_Freq) * 0.083333f);    // same as 1/12
        updateOscillators();
        m_crystal_drone.set_note(m_base_hz);
        m_metal_drone.set_note(m_base_hz);
    }

    inline void updateOscillators() {
        // Apply direction to Target Osc 1
        m_osc1_target_hz = m_base_hz * m_osc1_dir;

        // Calculate target for Osc 2 (sub-oct multiplier is smoothed at runtime)
        float detune_hz = (((float)m_params[k_paramO2Detune] / 50.0f) - 1.0f) * 5.0f;
        m_osc2_target_hz = (m_base_hz + detune_hz) * m_osc2_dir;
        m_osc2_suboct_target_mult = sub_oct_ratio_for_value(m_params[k_paramO2SubOct]);

        // Immediately set oscillator frequencies so sound starts on the first sample.
        // Zero-crossing updates in the audio loop handle live pitch changes without clicks.
        osc1.set_frequency(m_osc1_target_hz, SAMPLE_RATE_F);
        // m_osc2_suboct_smooth_mult will be smoothed in processBlock
        osc2.set_frequency(m_osc2_target_hz * m_osc2_suboct_smooth_mult, SAMPLE_RATE_F);
    }

    // The Drumlogue sends this when the sequencer stops!
    inline void AllNoteOff() {}

    // Hard stop
    inline void Suspend() {}

    /**
     * Fast NEON hyperbolic tangent approximation.
     * Uses the Padé approximant: x * (27 + x^2) / (27 + 9 * x^2)
     * Clamped to [-3.0, 3.0] to guarantee stability.
     * NOTE: clamping perfectly emulates an analog voltage rail
     */
    fast_inline float32x4_t fast_tanh_neon(float32x4_t x) {
        // 1. Clamp to [-3.0, 3.0] to prevent polynomial divergence
        float32x4_t clamped = vmaxq_f32(vminq_f32(x, vdupq_n_f32(3.0f)), vdupq_n_f32(-3.0f));

        float32x4_t x2 = vmulq_f32(clamped, clamped);

        // 2. Numerator = clamped * (27.0f + x2)
        float32x4_t num = vmulq_f32(clamped, vaddq_f32(vdupq_n_f32(27.0f), x2));

        // 3. Denominator = 27.0f + 9.0f * x2
        float32x4_t den = vmlaq_f32(vdupq_n_f32(27.0f), x2, vdupq_n_f32(9.0f));

        // 4. Fast division: num * (1.0f / den)
        // We use 2 Newton-Raphson iterations for high audio fidelity
        float32x4_t recip = vrecpeq_f32(den);
        recip = vmulq_f32(vrecpsq_f32(den, recip), recip); // NR Step 1
        recip = vmulq_f32(vrecpsq_f32(den, recip), recip); // NR Step 2

        return vmulq_f32(num, recip);
    }

    inline void ring_modulation(float& l1_val, float& l2_val, float& l3_val) {
        // 1. Get the raw bipolar output of all active LFOs
        float l1_raw = lfo1.process();
        float l2_raw = lfo2.process();
        float l3_raw = lfo3.process();

        // mod_val arrives as a bipolar signal (-1.0f to 1.0f)
        // We take the absolute value so it acts as a 0.0 to 1.0 crossfader
        float ring1_mod_amount = fabsf(m_lfo1_mod_val);
        float ring2_mod_amount = fabsf(m_lfo2_mod_val);
        float ring3_mod_amount = fabsf(m_lfo3_mod_val);

        // 2. Multiply them together (This is the magic!)
        float l1_multiplied = l1_raw * l2_raw;
        float l2_multiplied = l3_raw * l2_raw;
        float l3_multiplied = l1_raw * l3_raw;

        // 3. Smoothly crossfade from Normal LFO 1 to the Multiplied LFO
        // If ring_mod_amount is 0.0, it's just normal LFO 1.
        // If ring_mod_amount is 1.0, it is pure chaotic Ring Modulation.
        l1_val = (l1_raw * (1.0f - ring1_mod_amount)) + (l1_multiplied * ring1_mod_amount);
        l2_val = (l2_raw * (1.0f - ring2_mod_amount)) + (l2_multiplied * ring2_mod_amount);
        l3_val = (l3_raw * (1.0f - ring3_mod_amount)) + (l3_multiplied * ring3_mod_amount);
    }

    inline float lfo_rate_from_param(float param_value) {
        return 0.01f * fasterpowf(Audio_Rate_Freq, param_value * percent_normalizer);
    }

    inline float sub_oct_ratio_for_value(int32_t sub_oct_value) {
        switch (sub_oct_value) {
            case 1: return 0.5f;
            case 2: return 0.25f;
            case 3: return 2.0f;
            case 4: return fasterpow2f(1.8f); // +1.8 octaves (intentionally non-musical)
            default: return 1.0f;
        }
    }

    inline void processBlock(float* __restrict main_out, size_t frames) {

        int buf_idx = 0;
        float neon_buffer[neon_lanes];
        uint8_t out_idx_buffer[neon_lanes];
        float lfo_presence = 0.0f;
        float lfo1_p = 0.0f, lfo2_p = 0.0f, lfo3_p = 0.0f; // Persistent values for drone modulation
        const float dc_bias = 0.005f; // adjust to taste (0.001-0.01 works)

        // Source selection: preset 95 = crystal drone, preset 96 = metal drone
        const int32_t prog_preset = m_params[k_paramProgram];
        const bool use_drone = (prog_preset == 95 || prog_preset == 96);
        const bool use_metal_drone = (prog_preset == 96);

        for (size_t i = 0; i < frames; ++i) {

            // 1. CORE MODULATION SIGNALS
            float l1_val;
            float l2_val;
            float l3_val;
            ring_modulation(l1_val, l2_val, l3_val);
            // 2 FREQUENCY UPDATES
            // Store previous phase states
            float pre_phase1 = osc1.phase;
            float pre_phase2 = osc2.phase;

            // Update frequency every sample if pitch mod is active OR we are slewing the suboctave.
            // This eliminates stepping sounds during sub-octave transitions.
            bool is_suboct_slewing = fabsf(m_osc2_suboct_smooth_mult - m_osc2_suboct_target_mult) > 0.0001f;
            if (m_pitch_mod_multiplier != 1.0f || is_suboct_slewing) {
                osc1.set_frequency(m_osc1_target_hz * m_pitch_mod_multiplier, SAMPLE_RATE_F);
                osc2.set_frequency(m_osc2_target_hz * m_osc2_suboct_smooth_mult * m_pitch_mod_multiplier, SAMPLE_RATE_F);
            }

            if (is_suboct_slewing) {
                m_osc2_suboct_smooth_mult += (m_osc2_suboct_target_mult - m_osc2_suboct_smooth_mult) * m_osc2_suboct_slew;
            } else {
                m_osc2_suboct_smooth_mult = m_osc2_suboct_target_mult;
            }

            // 3 ACTIVE PARTIAL COUNTING (APC) BLOCK
            // Evaluate complex modulation targets and LFO-driven morphing only every 4 samples to save CPU cycles
            if (++m_apc_counter >= APC_FACTOR) {
                m_apc_counter = 0;

                // Reset multipliers/offsets
                m_pitch_mod_multiplier = 1.0f;
                m_f1_mod_multiplier = 1.0f;
                m_f2_mod_multiplier = 1.0f;
                m_f1_lfo_mult = 1.0f; // Reset: no filter LFO unless preset targets it
                m_f2_lfo_mult = 1.0f;
                m_cmos_mod_multiplier = 1.0f;
                m_srr_mod_offset = 0.0f;
                m_mix2_mod_offset = 0.0f;
                m_osc2_fm_mult = 1.0f;
                // reset assignment - to avoid remembering in case of preset change
                m_osc1_filter_target = k_filter_both;
                m_osc2_filter_target = k_filter_both;
                // NOTE: m_osc2_target_hz is NOT reset here — it is set once by
                // setParameter(k_paramOsc2Pitch) and must persist across APC cycles.
                m_volume_mod_multiplier = 0.0f; // Reset to 0, will be added to base
                m_drv1_mod_multiplier = 0.0f;
                m_drv2_mod_multiplier = 0.0f;
                m_mix1_mod_offset = 0.0f;

                int32_t mod_target = m_params[k_paramProgram] % k_paramLast;
                float lfo1_presence = (l1_val * m_lfo1_depth);
                float lfo2_presence = (l2_val * m_lfo2_depth);
                float lfo3_presence = (l3_val * m_lfo3_depth);

                // Fix: Update the persistent lfo_presence variable
                // We use a weighted sum to give more "morphing power" to LFO1 and LFO2.
                lfo_presence = lfo1_presence * 0.5f + lfo2_presence * 0.4f + lfo3_presence * 0.1f;

                lfo1_p = lfo1_presence;
                lfo2_p = (l2_val * lfo2_p); // Use local persistent storage
                lfo3_p = (l3_val * lfo3_p)
                switch (mod_target) {
                    case k_paramProgram:
                        // Macro target: subtle movement across multiple destinations.
                        m_pitch_mod_multiplier = fasterpow2f((l1_val + l2_val) * 0.25f);
                        m_mix2_mod_offset = l3_val * 0.2f;
                        m_volume_mod_multiplier = lfo_presence * 0.1f; // Add to base volume
                        break;
                    case k_paramNote:
                        // Modulate global pitch by +/- 12 semitones using LFO 1
                        m_pitch_mod_multiplier = fasterpow2f(l1_val);
                        break;
                    case k_paramF1Cutoff:
                        // Mapping: Osc 1 -> Filter 1, Osc 2 -> Bypass
                        m_osc1_filter_target = (fabsf(lfo1_presence) > fabsf(lfo2_presence)) : k_filter1, k_filter2;
                        m_osc2_filter_target = k_bypass;
                        m_f1_lfo_mult = fasterpow2f(l1_val * 3.0f);
                        m_f1_mod_multiplier = fasterpow2f(l2_val * 4.0f);
                        break;
                    case k_paramF2Cutoff:
                        // Mapping: Osc 1 -> Bypass, Osc 2 -> Filter 2
                        m_osc1_filter_target = k_bypass;
                        m_osc2_filter_target = (fabsf(lfo1_presence) > fabsf(lfo2_presence)) : k_filter1, k_filter2;
                        m_f2_lfo_mult = fasterpow2f(l2_val * 3.0f);
                        m_f2_mod_multiplier = fasterpow2f(l1_val * 4.0f);
                        break;
                    case k_paramCMOSDist: {
                        float a1 = fabsf(lfo1_presence);
                        float a2 = fabsf(lfo2_presence);
                        float a3 = fabsf(lfo3_presence);

                        if (a1 >= a2 && a1 >= a3) {
                            // LFO 1 Predominant: Drive saturation on Filter 1
                            m_drv1_mod_multiplier = lfo1_presence;
                        } else if (a2 >= a1 && a2 >= a3) {
                            // LFO 2 Predominant: Drive saturation on Filter 2
                            m_drv2_mod_multiplier = lfo2_presence;
                        } else {
                            // LFO 3 Predominant: Saturation drive on both filters
                            m_drv1_mod_multiplier = lfo3_presence;
                            m_drv2_mod_multiplier = lfo3_presence;
                        }
                    } break;
                    case k_paramSampRed: {
                        m_srr_mod_offset = lfo_presence * 0.5f;
                    } break;
                    case k_paramOsc2Mix:
                        m_mix2_mod_offset = l2_val; // Crossfade modulation
                        break;
                    case k_paramO2Detune:
                        // FM modulation: compute multiplier from LFO; applied at zero-crossing.
                        // Do NOT accumulate into m_osc2_target_hz — it would grow to Inf.
                        m_osc2_fm_mult = fasterpow2f(lfo_presence * 2.0f);
                        break;
                    case k_paramL1Wave: filter1.mode = (filter_mode)(m_params[k_paramL1Wave] % mode_last);
                        break;
                    case k_paramL2Wave: filter2.mode = (filter_mode)(m_params[k_paramL2Wave] % mode_last);
                        break;
                    case k_paramOsc1Wave: {
                            int base_wave1 = m_params[k_paramOsc1Wave];
                            // LFO1 sweeps the wavetable index back and forth
                            int wave1_offset = (int)(l1_val * 20.0f);
                            int final_wave1 = ((base_wave1 + wave1_offset) % NUM_WAVETABLES + NUM_WAVETABLES) % NUM_WAVETABLES;

                            if (final_wave1 != m_osc1_target_wave_idx) {
                                m_osc1_prev_wave_idx = m_osc1_current_wave_idx;
                                m_osc1_target_wave_idx = final_wave1;
                                m_osc1_morph_phase = 0.0f;
                                m_osc1_morph_active = true;
                                m_osc1_morph_speed = 1.0f / (SAMPLE_RATE_F * 0.05f); // 50ms morph
                            }
                        }
                        break;
                    case k_paramOsc2Wave: {
                            int base_wave2 = m_params[k_paramOsc2Wave];
                            // LFO2 sweeps the wavetable index back and forth
                            int wave1_offset = (int)(l2_val * 20.0f);
                            int final_wave2 = ((base_wave2 + wave1_offset) % NUM_WAVETABLES + NUM_WAVETABLES) % NUM_WAVETABLES;

                            if (final_wave2 != m_osc2_target_wave_idx) {
                                m_osc2_prev_wave_idx = m_osc2_current_wave_idx;
                                m_osc2_target_wave_idx = final_wave2;
                                m_osc2_morph_phase = 0.0f;
                                m_osc2_morph_active = true;
                                m_osc2_morph_speed = 1.0f / (SAMPLE_RATE_F * 0.05f); // 50ms morph
                            }
                        }
                        break;
                    // -----------------------------------------------------
                    // 1. MASTER VOLUME (Tremolo / AM)
                    // -----------------------------------------------------
                    case k_paramMastrVol:
                        m_volume_mod_multiplier = lfo_presence;
                        break;

                    case k_paramL1Rate: {
                        float rate_mix = lfo_presence;
                        float mod_param = fmaxf(0.0f, fminf(100.0f, (float)m_params[k_paramL1Rate] + (rate_mix * 25.0f)));
                        lfo1.set_rate(lfo_rate_from_param(mod_param), SAMPLE_RATE_F);
                        break;
                    }
                    case k_paramL2Rate: {
                        float rate_mix = lfo_presence;
                        float mod_param = fmaxf(0.0f, fminf(100.0f, (float)m_params[k_paramL2Rate] + (rate_mix * 25.0f)));
                        lfo2.set_rate(lfo_rate_from_param(mod_param), SAMPLE_RATE_F);
                        break;
                    }
                    case k_paramL3Rate: {
                        float rate_mix = lfo_presence;
                        float mod_param = fmaxf(0.0f, fminf(100.0f, (float)m_params[k_paramL3Rate] + (rate_mix * 25.0f)));
                        lfo3.set_rate(lfo_rate_from_param(mod_param), SAMPLE_RATE_F);
                        break;
                    }
                    case k_paramL1Depth:
                        m_lfo1_mod_val = fmaxf(-1.0f, fminf(1.0f, m_lfo1_depth + (l1_val * 0.5f)));
                        break;
                    case k_paramL2Depth:
                        m_lfo2_mod_val = fmaxf(-1.0f, fminf(1.0f, m_lfo2_depth + (l2_val * 0.5f)));
                        break;
                    case k_paramL3Depth:
                        m_lfo3_mod_val = fmaxf(-1.0f, fminf(1.0f, m_lfo3_depth + (l3_val * 0.5f)));
                        break;
                    case k_paramL3Wave:
                        // Continuously scan LFO3 waveform for evolving motion.
                        lfo3.wave_type = ((int)m_params[k_paramL3Wave] + (int)(l3_val * 3.0f) + LFO_WAVE_COUNT) % LFO_WAVE_COUNT;
                        break;
                    case k_paramBitRed:
                        // Dynamic bit depth variation around user value.
                        m_brr_steps = 1.0f + fasterpowf(2.0f, (float)m_params[k_paramBitRed] * 0.08f + (lfo_presence * 2.0f));
                        if (m_brr_steps > 65536.0f) m_brr_steps = 65536.0f;
                        break;

                    // -----------------------------------------------------
                    // 3. THE "FAKE RESONANCE" (CPU-Safe Filter Drive)
                    // -----------------------------------------------------
                    case k_paramF1Reso:
                        m_f1_q += lfo1_presence * 0.1;
                        break;
                        case k_paramF2Reso:
                        m_f1_q += lfo2_presence * 0.1;
                        break;
                        case k_paramO2SubOct:
                        m_drv1_mod_multiplier = lfo1_presence;
                        m_drv2_mod_multiplier = lfo2_presence;
                        m_mix1_mod_offset += lfo_presence;
                        break;
                    default:
                        break;
                }
            }

            // 4. OSCILLATOR / DRONE MIX + WAVETABLE MORPHING
            float f1_in = 0.0f;
            float f2_in = 0.0f;
            float bypass_sig = 0.0f;
            float sig1_raw = 0.0f; // Tracks Osc 1 for Sherman asymmetry logic

            if (use_drone) {
                float drone_sig = use_metal_drone ?
                    m_metal_drone.process(lfo1_p, lfo2_p, lfo3_p) :
                    m_crystal_drone.process(lfo1_p, lfo2_p, lfo3_p);
                f1_in = drone_sig; // Drones always run through the full chain
            } else {
                // Oscillator 1 Morphing (Crossfading output values between old and new tables)
                float o1_val;
                if (m_osc1_morph_active) {
                    float p = osc1.phase;
                    osc1.current_table = SCRUTAASTRI_WAVETABLES[m_osc1_prev_wave_idx];
                    float s1 = osc1.process();
                    osc1.phase = p; // process the target table at the exact same sample point
                    osc1.current_table = SCRUTAASTRI_WAVETABLES[m_osc1_target_wave_idx];
                    float s2 = osc1.process();

                    o1_val = s1 + (s2 - s1) * m_osc1_morph_phase;
                    m_osc1_morph_phase += m_osc1_morph_speed;
                    if (m_osc1_morph_phase >= 1.0f) {
                        m_osc1_morph_active = false;
                        m_osc1_current_wave_idx = m_osc1_target_wave_idx;
                    }
                } else {
                    osc1.current_table = SCRUTAASTRI_WAVETABLES[m_osc1_current_wave_idx];
                    o1_val = osc1.process();
                }
                float out_osc1 = fmaxf(0.0f, fminf(1.0f, o1_val * 0.5f + m_mix1_mod_offset));
                sig1_raw = out_osc1;

                // Oscillator 2 Morphing
                float o2_val;
                if (m_osc2_morph_active) {
                    float p = osc2.phase;
                    osc2.current_table = SCRUTAASTRI_WAVETABLES[m_osc2_prev_wave_idx];
                    float s1 = osc2.process();
                    osc2.phase = p;
                    osc2.current_table = SCRUTAASTRI_WAVETABLES[m_osc2_target_wave_idx];
                    float s2 = osc2.process();

                    o2_val = s1 + (s2 - s1) * m_osc2_morph_phase;
                    m_osc2_morph_phase += m_osc2_morph_speed;
                    if (m_osc2_morph_phase >= 1.0f) {
                        m_osc2_morph_active = false;
                        m_osc2_current_wave_idx = m_osc2_target_wave_idx;
                    }
                } else {
                    osc2.current_table = SCRUTAASTRI_WAVETABLES[m_osc2_current_wave_idx];
                    o2_val = osc2.process();
                }

                float dynamic_mix = fmaxf(0.0f, fminf(1.0f, m_osc2_mix + m_mix2_mod_offset));
                float out_osc2 = o2_val * dynamic_mix;

                // Mapping logic
                f1_in += out_osc1 *
                            ((m_osc1_filter_target == k_filter1) || (m_osc1_filter_target == k_filter_both)) +
                         out_osc2 *
                            ((m_osc2_filter_target == k_filter1) || (m_osc2_filter_target == k_filter_both));
                f2_in += out_osc2 *
                            ((m_osc1_filter_target == k_filter2) || (m_osc1_filter_target == k_filter_both)) +
                         out_osc2 *
                            ((m_osc2_filter_target == k_filter2) || (m_osc2_filter_target == k_filter_both));
                bypass_sig += out_osc1 * (m_osc1_filter_target == k_bypass) +
                              out_osc2 * (m_osc2_filter_target == k_bypass);

                // Bidirectional phase wrap detection
                bool osc1_wrapped = (m_osc1_dir > 0.0f) ? (osc1.phase < pre_phase1) : (osc1.phase > pre_phase1);
                if (osc1_wrapped) {
                    osc1.set_frequency(m_osc1_target_hz, SAMPLE_RATE_F);
                }

                bool osc2_wrapped = (m_osc2_dir > 0.0f) ? (osc2.phase < pre_phase2) : (osc2.phase > pre_phase2);
                if (osc2_wrapped) {
                    osc2.set_frequency(m_osc2_target_hz * m_osc2_suboct_smooth_mult * m_osc2_fm_mult, SAMPLE_RATE_F);
                }
            }

            // 5. FILTER 1
            // m_f1_lfo_mult is only set when the preset targets F1 cutoff;
            // otherwise it holds 1.0f so the base cutoff is unaffected.
            float f1_mod_hz = m_f1_base_hz * m_f1_lfo_mult;
            f1_mod_hz *= m_f1_mod_multiplier;

            filter1.set_coeffs(f1_mod_hz, m_f1_q, SAMPLE_RATE_F);

            // Calculate dynamic asymmetry using tracked Osc 1 value
            float dynamic_asym = m_sherman_asym_base + (sig1_raw * m_asym_mod_depth * 2.0f);

            // Inject the clamped modulated value into the filter state
            filter1.sherman_asym = fmaxf(0.0f, fminf(4.0f, dynamic_asym));

            // Instead of calling sinf() and sqrtf() to modulate Q,
            // we modulate the Sherman Drive. This pushes the filter into
            // aggressive screaming/squelching territory safely!
            // Combine CMOS drive (from k_paramCMOSDist) with resonance drive and LFO drive.
            // m_cmos_filter_drive is the only source that survives across APC cycles.
            filter1.drive = fmaxf(0.0f, fminf(5.0f, m_cmos_filter_drive + m_f1_drive_base + (m_drv1_mod_multiplier * 5.0f)));
            float f1_out = filter1.process(f1_in, l3_val);

            // 6. THE CRUSH SANDWICH
            // Apply APC offset to SRR rate, clamping to avoid reversed counters
            float dynamic_srr = fmaxf(0.001f, fminf(1.0f, m_srr_rate + m_srr_mod_offset));
            m_srr_counter += dynamic_srr;

            if (m_srr_counter >= 1.0f) {
                m_srr_hold_val = f1_out;
                m_srr_counter -= 1.0f;
            }
            float crushed_sig = m_srr_hold_val;

            if (m_brr_steps < 65536.0f) {
                crushed_sig = roundf(crushed_sig * m_brr_steps) / m_brr_steps;
            }

            // 7. FILTER 2 - Polivoks
            float total_f2_in = f2_in + crushed_sig;

            // m_f2_lfo_mult is only set when the preset targets F2 cutoff
            float f2_mod_hz = m_f2_base_hz * m_f2_lfo_mult;
            f2_mod_hz *= m_f2_mod_multiplier; // Apply APC multiplier

            filter2.set_coeffs(f2_mod_hz, m_f2_q, SAMPLE_RATE_F);
            filter2.drive = fmaxf(0.0f, fminf(5.0f, m_cmos_filter_drive + m_f2_drive_base + (m_drv2_mod_multiplier * 5.0f)));
            float f2_out = filter2.process(total_f2_in);
            f2_out *= m_sherman_makeup;

            // Final sum of filtered signals and any bypassed oscillator paths
            float mixed_sig = f2_out + bypass_sig;

            // =================================================================
            // 8. MASTER VCA & MICRO-BUFFER
            // =================================================================
            float dynamic_cmos = m_cmos_gain * m_cmos_mod_multiplier;
            float pre_dist = mixed_sig * dynamic_cmos + dc_bias;

            // Accumulate into the NEON buffer instead of calculating scalar tanh
            neon_buffer[buf_idx] = pre_dist;
            out_idx_buffer[buf_idx] = i; // Save the exact frame index
            buf_idx++;

            // When buffer hits 4 samples, blast it through NEON FPU
            if (buf_idx == neon_lanes) {
                float32x4_t v_in = vld1q_f32(neon_buffer);

                // NEON Double Tanh: distorted = fast_tanh( fast_tanh( distorted * 1.2f ) ) * 0.9f;
                float32x4_t v_stage1 = fast_tanh_neon(vmulq_n_f32(v_in, 1.2f));
                float32x4_t v_stage2 = fast_tanh_neon(v_stage1);
                float32x4_t v_out = vmulq_n_f32(v_stage2, 0.9f);

                // Remove DC bias
                v_out = vsubq_f32(v_out, vdupq_n_f32(dc_bias * 0.9f));

                // Clamp between 0.0 (silence) and 1.0 (max volume)
                m_master_vol = fmaxf(0.0f, fminf(1.0f, m_master_vol_base + m_volume_mod_multiplier));
                // Apply master volume
                v_out = vmulq_n_f32(v_out, m_master_vol);

                // Extract back to memory
                float out_arr[neon_lanes];
                vst1q_f32(out_arr, v_out);

                // Write to the audio output interleaving
                for(int b = 0; b < neon_lanes; b++) {
                    size_t out_i = out_idx_buffer[b];
                    main_out[out_i * 2]     = out_arr[b];
                    main_out[out_i * 2 + 1] = out_arr[b];
                }

                buf_idx = 0; // Reset micro-buffer
            }
        }

        // =================================================================
        // 9. SCALAR TAIL (For frames not perfectly divisible by 4)
        // =================================================================
        for(uint8_t b = 0; b < buf_idx; b++) {
            float distorted = neon_buffer[b];

            // Use your existing scalar fast_tanh for the remainders
            distorted = fast_tanh( fast_tanh( distorted * 1.2f ) ) * 0.9f;
            distorted -= dc_bias * 0.9f;

            float master_out = distorted * m_master_vol;
            size_t out_i = out_idx_buffer[b];

            main_out[out_i * 2]     = master_out;
            main_out[out_i * 2 + 1] = master_out;
        }
    }

private:
    int32_t m_params[k_paramLast] = {0};

    // Core Engine Instances
    WavetableOsc osc1;
    WavetableOsc osc2;
    MorphingFilter filter1;
    PolivoksFilter filter2;
    FastLFO lfo1;
    FastLFO lfo2;
    FastLFO lfo3;
    DroneEngine m_crystal_drone;
    DroneEngine m_metal_drone;


    // Hardware Gate Trackers
    // float m_drone_target = 0.0f;
    // float m_drone_amp = 0.0f;

    float m_sherman_makeup = 1.0f;

    // Derived State Variables
    float m_base_hz = 65.4f; // Default to Note 36 (C2)
    float m_lfo1_depth = 0.0f;
    float m_lfo2_depth = 0.0f;
    float m_lfo3_depth = 0.0f;

    float m_osc2_mix = 0.5f;
    float m_master_vol = 0.5f;
    float m_master_vol_base = 0.5f;
    float m_lfo1_rate_base = 1.0f;
    float m_f1_drive_base = 0.0f;
    float m_f2_drive_base = 0.0f;
    float m_asym_mod_depth = 0.0f;

    float m_f1_base_hz = 10000.0f;
    float m_f1_q = Q_Limit;
    float m_f2_base_hz = 10000.0f;
    float m_f2_q = Q_Limit;

    // Filter Asymmetry Base State
    float m_sherman_asym_base = 0.0f;

    // FX State
    float m_srr_counter = 0.0f;
    float m_srr_rate = 1.0f;
    float m_srr_hold_val = 0.0f;
    float m_brr_steps = 65536.0f;
    float m_cmos_gain = 1.0f;

    // Zero-Crossing Target Trackers
    float m_osc1_target_hz = 65.4f;
    float m_osc2_target_hz = 65.4f;

    // Active Partial Counting (APC) Variables
    uint8_t m_apc_counter = 0;
    static constexpr uint8_t APC_FACTOR = 4; // Calculate mod targets every 4th sample

    // Held Modulation State
    float m_f1_mod_multiplier = 1.0f;
    float m_f2_mod_multiplier = 1.0f;
    float m_f1_lfo_mult = 1.0f;    // APC-gated: only set when preset targets F1 cutoff
    float m_f2_lfo_mult = 1.0f;    // APC-gated: only set when preset targets F2 cutoff
    float m_cmos_filter_drive = 0.0f; // Drive set by k_paramCMOSDist; persists across APC cycles
    float m_cmos_mod_multiplier = 1.0f;
    float m_srr_mod_offset = 0.0f;
    float m_mix1_mod_offset = 0.0f;
    float m_mix2_mod_offset = 0.0f;
    float m_pitch_mod_multiplier = 1.0f;
    float m_osc2_fm_mult = 1.0f;        // FM via LFO for k_paramO2Detune preset
    float m_osc2_suboct_target_mult = 1.0f;
    float m_osc2_suboct_smooth_mult = 1.0f;
    float m_osc2_suboct_slew = 0.0025f;
    float m_volume_mod_multiplier = 1.0f;
    float m_drv1_mod_multiplier = 1.0f;
    float m_drv2_mod_multiplier = 1.0f;
    float m_lfo1_mod_val = 0.0f;
    float m_lfo2_mod_val = 0.0f;
    float m_lfo3_mod_val = 0.0f;

    // Wave Morphing
    float m_osc1_morph_phase = 1.0f; // 0.0 to 1.0, 1.0 means morph complete
    float m_osc1_morph_speed = 0.0f; // Increment per sample
    bool m_osc1_morph_active = false;
    uint16_t m_osc1_current_wave_idx = 0; // The wave index currently being played (or morphed from)
    uint16_t m_osc1_target_wave_idx = 0; // The wave index we are morphing to (from knob or LFO)
    uint16_t m_osc1_prev_wave_idx = 0; // Index of the waveform before the change

    float m_osc2_morph_phase = 1.0f;
    float m_osc2_morph_speed = 0.0f;
    bool m_osc2_morph_active = false;
    uint16_t m_osc2_current_wave_idx = 0;
    uint16_t m_osc2_target_wave_idx = 0;
    uint16_t m_osc2_prev_wave_idx = 0;

    // Filter Mapping Variables
    uint8_t m_osc1_filter_target = k_filter1; // 0: F1 Path, 1: F2 Path, 2: Bypass
    uint8_t m_osc2_filter_target = k_filter2;

    // Playback Direction Multipliers
    float m_osc1_dir = 1.0f;
    float m_osc2_dir = 1.0f;
};
