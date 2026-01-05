#pragma once
/*
*  File: RipplerX.h
*
*  Synth Class derived from .
*
*
*  2021-2022 (c) Korg
*
*/

#include <cstddef>
#include <cstdint>
#include <array>
#include "../common/runtime.h"
#include <arm_neon.h>
#include "constants.h"
#include "Voice.h"
#include "Limiter.h"
#include "Comb.h"
#include "Models.h"


/** This class is the equivalent of the origin pluginProcessor the
 * original RipplerX code, now it's a synth instance.
 *
 */

class RipplerX
{
    public:
    /*===========================================================================*/
    /* Public Data Structures/Types. */
    /*===========================================================================*/

    /*===========================================================================*/
    /* Lifecycle Methods. */
    /*===========================================================================*/

    RipplerX() {};
    ~RipplerX() {};
    // called at unit_init - taken from the original PluginProcessor()
    inline int8_t Init(const unit_runtime_desc_t * desc) {
        // Check compatibility of samplerate with unit, for drumlogue should be 48000
        if (desc->samplerate != c_sampleRate)
            return k_unit_err_samplerate;

        // Check compatibility of frame geometry
        if (desc->output_channels != 2)  // should be stereo output
            return k_unit_err_geometry;

        // Note: if need to allocate some memory can do it here and return k_unit_err_memory if getting allocation errors

        models = new Models(); // Allocate Models (remove smart pointers)
        for (size_t i = 0; i < polyphony; ++i) {
            voices[i] = new Voice(*models); // Allocate each Voice
        }

        // private variables (state vars set at constructor)
        // m_currentProgram = 0;    //done at LoadPreset
        m_note = 60;
        // sample management
        m_sampleBank = 0;
        m_sampleNumber = 1;
        m_sampleStart = 0;  // not editable
        m_sampleEnd = 1000;  // 100% - not editable

        // Stash runtime functions to manage samples.
        m_get_num_sample_banks_ptr = desc->get_num_sample_banks;
        m_get_num_samples_for_bank_ptr = desc->get_num_samples_for_bank;
        m_get_sample = desc->get_sample;

        // loadDefaultProgramParameters();
        LoadPreset(0);

        comb.init(getSampleRate());
        limiter.init(getSampleRate());

        Reset();	// at init Reset is mandatory to update the last model used
        // resetLastModels();
        // clearVoices();
        prepareToPlay();

        return k_unit_err_none;
    }

    // NOTE: does this make any sense?
    inline void loadDefaultProgramParameters() {
        // Initial parameter values, editable ones are indented and matching
        // with header.c, plus others that are characterizing each "program"
            parameters[mallet_mix] = 0.0f;
        parameters[mallet_res] = 8;
        parameters[mallet_stiff] = 600;
            parameters[a_on] = 1;   // true
        parameters[a_model] = ModelNames::Squared;
        parameters[a_partials] = c_partials[3]; // in constructor of RipplerXAudioProcessor
        parameters[a_decay] = 10;   // 1%
        parameters[a_damp] = 0;
        parameters[a_tone] = 0;
        parameters[a_hit] = 0.26f;
        parameters[a_rel] = 1.0f;
        parameters[a_inharm] = 0.0001f;
            parameters[a_ratio] = 1.0f;  // (1-10)
        parameters[a_cut] = 20.0f;
        parameters[a_radius] = 0.5f;
        parameters[a_coarse] = 0.0f;
            parameters[a_fine] = 0.0f;  // -99.0 . 99.0
            parameters[b_on] = 0;   // default is off as Drumlogue has not enough user editable params - will be switched on by program
            parameters[b_model] = 0;
            parameters[b_partials] = c_partials[3];
            parameters[b_decay] = 1.0f;
            parameters[b_damp] = 0.0f;
            parameters[b_tone] = 0.0f;
            parameters[b_hit] = 0.26f;
            parameters[b_rel] = 1.0f;
            parameters[b_inharm] = 0.0001f;
            parameters[b_ratio] = 1.0f;
            parameters[b_cut] = 20.0f;
            parameters[b_radius] = 0.5f;
            parameters[b_coarse] = 0.0f;
            parameters[b_fine] = 0.0f;
        parameters[noise_mix] = 0.0f;
        parameters[noise_res] = 0.0f;
        parameters[noise_filter_mode] = 2;  // "HP"
        parameters[noise_filter_freq] = 20; // 20Hz
        parameters[noise_filter_q] =  0.707f;
            parameters[noise_att] = 1.0f;   // 1.0f, 5000.0f
            parameters[noise_dec] = 500.0f; // 1.0f, 5000.0f
            parameters[noise_sus] = 0.0f;   // 0.0f, 1.0f
            parameters[noise_rel] = 500.0f; // 1.0f, 5000.0f
            parameters[vel_mallet_mix] = 0.0f; // 0.0f, 1.0f
        parameters[vel_mallet_res] = 0.0f;     // 0.0f, 1.0f
        parameters[vel_mallet_stiff] = 0.0f;   // 0.0f, 1.0f
            parameters[vel_noise_mix] = 0.0f;  // 0.0f, 1.0f
            parameters[vel_noise_res] = 0.0f;  // 0.0f, 1.0f
            parameters[vel_a_decay] = 0.0f;    // 0.0f, 1.0f
            parameters[vel_a_hit] = 0.0f;      // 0.0f, 1.0f
            parameters[vel_a_inharm] = 0.0f;   // 0.0f, 1.0f
            parameters[vel_b_decay] = 0.0f;    // 0.0f, 1.0f
            parameters[vel_b_hit] = 0.0f;      // 0.0f, 1.0f
            parameters[vel_b_inharm] = 0.0f;   // 0.0f, 1.0f
            parameters[couple] = 0;   // "A+B", "A>B"
            parameters[ab_mix] = 0.5f;
            parameters[ab_split] = 0.01f;   // 0.0f, 1.0f
        parameters[gain] = 0;
    }

    inline void Teardown() {
        // Note: cleanup and release resources if any
        // Delete allocated voices
        for (size_t i = 0; i < polyphony; ++i) {
            delete voices[i];
        }
        delete models; // Delete Models
    }

    inline void Reset() {
        // Note: Reset synth state. I.e.: Clear filter memory, reset oscillator
        // phase etc.
        clearVoices();
        resetLastModels();
    }

    inline void Resume() {
        // Note: Synth will resume and exit suspend state. Usually means the synth
        // was selected and the render callback will be called again
    }

    inline void Suspend() {
        // Note: Synth will enter suspend state. Usually means another synth was
        // selected and thus the render callback will not be called
    }

    /*===========================================================================*/
    /* Other Public Methods. */
    /*===========================================================================*/

    // this should be equivalent to processBlockByType in PluginProcessor_orig.cpp
    inline void Render(float * __restrict outBuffer, size_t frames)
    {
        bool a_on = (bool)getParameterValue(Parameters::a_on);
        bool b_on = (bool)getParameterValue(Parameters::b_on);
        float32_t mallet_mix = (float32_t)getParameterValue(Parameters::mallet_mix);
        float32_t mallet_res = (float32_t)getParameterValue(Parameters::mallet_res);
        float32_t vel_mallet_mix = (float32_t)getParameterValue(Parameters::vel_mallet_mix);
        float32_t vel_mallet_res = (float32_t)getParameterValue(Parameters::vel_mallet_res);
        float32_t noise_mix = getParameterValue(Parameters::noise_mix);
        float32_t noise_res = getParameterValue(Parameters::noise_res);
        float32_t vel_noise_mix = getParameterValue(Parameters::vel_noise_mix);
        float32_t vel_noise_res = getParameterValue(Parameters::vel_noise_res);
        // auto noise_mix_range = getParameterValue(Parameters::noise_mix);
        // noise_mix = noise_mix_range.convertTo0to1(noise_mix);  //not needed (?)
        // float32_t noise_res = getParameterValue(Parameters::noise_res);
        // auto noise_res_range = getParameterValue(Parameters::noise_res); // Paramater value already set in percentage
        // noise_res = noise_res_range.convertTo0to1(noise_res);
        bool serial = (bool)getParameterValue(Parameters::couple);
        float32_t ab_mix = (float32_t)getParameterValue(Parameters::ab_mix);
        float32_t gain = (float32_t)getParameterValue(Parameters::gain);
        bool couple = (bool)getParameterValue(Parameters::couple);
        // gain = pow(10.0, gain / 20.0);   //NOTE: it's precomputed at parameter change

        // For each frame in batch:
        // NOTE frame is sample for numSamples in PluginProcessor_orig.cpp.
        // Here is different as a single frame can have more than one sample (stereo)
        // So we are loading 2 stereo samples / 4 mono samples at time
        for (size_t frame = 0; frame < frames; frame+=2) {
            // Set all lanes to same value
            float32x4_t dirOut  = vdupq_n_f32(0.0f);  // direct output per sample (sum of all voices)
            float32x4_t resAOut = vdupq_n_f32(0.0f);  // resonator A output
            float32x4_t resBOut = vdupq_n_f32(0.0f);  // resonator B output
            // Action and Mixing stage
            float32x4_t audioIn;  //NOTE: same as excitation in resonator_orig.h

            // step 1: get samples
            // Sample, until it runs out. (from resonator_orig.h). Set at NoteOn
            if (m_sampleIndex < m_sampleEnd)
            {
                if (m_sampleChannels == 2) {
                    // 2 Stereo samples
                    audioIn = vld1q_f32(&m_samplePointer[m_sampleIndex]);
                } else {
                    // 2 Mono samples duplicated to stereo
                    audioIn = vcombine_f32(vdup_n_f32(m_samplePointer[m_sampleIndex]), vdup_n_f32(m_samplePointer[m_sampleIndex + 1]));
                }
                audioIn = vmulq_n_f32(audioIn, gain);    // TODO: new sampleGain as resonator_orig.h?
                m_sampleIndex += m_sampleChannels * 2;  //only usage of m_sampleIndex
            } else {
                audioIn = vdupq_n_f32(0.0f);
            }

            // NOTE: we cannot process all the voices in parallel as we need to sum their outputs,
            // and each voice has different settings and state.
            // SO we are processing 4 samples at time (as much as wo stereo output) but for each voice sequentially
            // insisting to same samples.
            // Refactor using neon intrinsics from processBlockByType in PluginProcessor_orig.cpp
            for (size_t i = 0; i < c_numVoices; ++i)
            {
                float32x4_t resOut = vdupq_n_f32(0.0f);
                Voice & voice = *voices[i];
                float32_t msample = voice.m_initialized ? voice.mallet.process() : 0.0f;    // TODO move out of frame loop
                // step 2.1: handle mallet
                // dirOut += msample * fmax(0.0, fmin(1.0, mallet_mix + vel_mallet_mix * voice.vel));
                // resOut += msample * fmax(0.0, fmin(1.0, mallet_res + vel_mallet_res * voice.vel));
                if (msample) {
                    dirOut = vaddq_f32(dirOut, vmaxq_f32(vdupq_n_f32(0.0), vminq_f32(vdupq_n_f32(1.0), vdupq_n_f32(mallet_mix + vel_mallet_mix * voice.vel))));
                    resOut = vaddq_f32(resOut, vmaxq_f32(vdupq_n_f32(0.0), vminq_f32(vdupq_n_f32(1.0), vdupq_n_f32(mallet_res + vel_mallet_res * voice.vel))));
                }
                // step 2.2: add sample input
                // if (audioIn && voice.isPressed) // NoteOn => voice.trigger
                //     resOut += audioIn;
                resOut = vaddq_f32(resOut, voice.isPressed ? audioIn : vdupq_n_f32(0.0f));

                // step 2.3: handle noise
                // dirOut += nsample * (double)noise_mix_range.convertFrom0to1(fmax(0.f, fmin(1.f, noise_mix + vel_noise_mix * (float)voice.vel)));
                // resOut += nsample * (double)noise_res_range.convertFrom0to1(fmax(0.f, fmin(1.f, noise_res + vel_noise_res * (float)voice.vel)));
                float32_t nsample = voice.noise.process();
                if (nsample) {
                    dirOut = vmulq_n_f32(dirOut, nsample * fmax(0.0, fmin(1.0, noise_mix + vel_noise_mix * voice.vel)) * 1000.0f);  //I think that 1000 is the equivalent of noise_mix_range.convertFrom0to1, as here we have a range of 0-1000 (see header.cpp, Noise Mix)
                    resOut = vmulq_n_f32(resOut, nsample * fmax(0.0, fmin(1.0, noise_res + vel_noise_res * voice.vel)) * 1000.0f);  //I think that 1000 is the equivalent of noise_res_range.convertFrom0to1, as here we have a range of 0-1000 (see header.cpp, Noise Resonance)
                }

                // // step 2.4: handle resonator A
                float32x4_t out_from_a = vdupq_n_f32(0.0f); // output from voice A into B in case of resonator serial coupling
                if (a_on) {
                    float32x4_t out = voice.resA.process(resOut); // this resOut contains the input from sample (so must be here in frame loop), mallet, noise
                    if (voice.resA.cut > c_res_cutoff) out = voice.resA.filter.df1(out);
                    resAOut = vaddq_f32(out, resAOut);
                    out_from_a = out;
                }

                if (b_on) {
                    float32x4_t out = voice.resB.process(a_on && couple ? out_from_a : resOut);
                    if (voice.resB.cut > c_res_cutoff)
                        out = voice.resB.filter.df1(out);
                    resBOut = vaddq_f32(out, resBOut);
                }
                voice.m_framesSinceNoteOn += frames; // TODO: check this - Voice stealing - TODO: check according to for loop change
            }   // end for polyphony

            // step 3: mix resonator outputs
            float32x4_t resOut;
            if (a_on && b_on)
                // resAOut * (1-ab_mix) + resBOut * ab_mix;
                resOut = serial ? resBOut : vaddq_f32(vmulq_n_f32(resBOut, ab_mix), vmulq_n_f32(resAOut, 1 - ab_mix));
            else
                resOut = vaddq_f32(resAOut, resBOut); // one of them is turned off, just sum the two

            // step 4: total output processing
            // dirOut + resOut * gain
            float32x4_t totalOut = vmlaq_n_f32(dirOut, resOut, gain);
            // auto [spl0, spl1] =  comb.process(totalOut);
            float32x4_t split = comb.process(totalOut);  // process comb filter, returns stereo float32x2_t
            // auto [left, right] = limiter.process(split);
            float32x4_t channels = limiter.process(split);

            // Add current float32x2 to output buffer.
            float32x4_t old = vld1q_f32(outBuffer);  // load existing buffer from pointer
            channels = vaddq_f32(old, channels);
            // each voice contributes to single sample, in stereo
            vst1q_f32(outBuffer, channels);  // replace existing buffer with new value
            outBuffer += 4; // move pointer by two positions for each sample, as we process two values at once, one per channel
        }   // end for frames
    }   // end Render()

    // Read parameter from user (6 pages listed in header.c)
    // I suppose that's the same as onSlider() of the original PluginProcessor
    inline void setParameter(uint8_t index, int32_t value) {
        bool noiseChanged = false;
        bool pitchChanged = false;
        bool resonatorChangedA = false;
        bool resonatorChangedB = false;
        bool couplingChanged = false;
        if (index < c_parameterTotal)
        {
            switch(index) {
                case c_parameterProgramName:
                    // load whole set of parameters according to pre-calculated program model
                    if (value < last_program)
                        setCurrentProgram(value);
                    noiseChanged = true;
                    pitchChanged = true;
                    resonatorChangedA = true;
                    resonatorChangedB = true;
                    couplingChanged = true;
                    break;
                case c_parameterGain:
                    parameters[gain] = fasterpowf(10.0, value / 20.0);  // it's faster than do calculation in Render, which should be "hard real time"
                    break;
                case c_parameterSampleBank:
                if ((size_t)value < c_sampleBankElements)
                        m_sampleBank = value;
                    break;
                case c_parameterSampleNumber:
                    m_sampleNumber = value;
                    break;
                case c_parameterMalletResonance:
                    parameters[mallet_res] = value;
                    break;
                case c_parameterMalletStifness:
                    parameters[mallet_stiff] = value;
                    break;
                case c_parameterVelocityMalletResonance:
                    parameters[vel_mallet_res] = value;
                    break;
                case c_parameterVelocityMalletStifness:
                    parameters[vel_mallet_stiff] = value;
                    break;
                case c_parameterModel:
                    if ((size_t)value < c_modelElements){
                        parameters[a_model] = value;
                        resonatorChangedA = true;}
                        break;
                case c_parameterPartials:   // from original OnSlider()
                    if ((size_t)value < c_partialElements){
                        // only A partials can be changed by user. B partials by program/model only
                        parameters[a_partials] = c_partials[value];
                        resonatorChangedA = true;}
                    break;
                case c_parameterDecay:
                    parameters[a_decay] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterMaterial:
                    parameters[a_damp] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterTone:
                    parameters[a_tone] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterHitPosition:
                    parameters[a_hit] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterRelease:
                    parameters[a_rel] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterInharmonic:
                    parameters[a_inharm] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterFilterCutoff:
                    parameters[a_cut] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterTubeRadius:
                    parameters[a_radius] = value;
                    resonatorChangedA = true;
                    break;
                case c_parameterCoarsePitch:
                    parameters[a_coarse] = value;
                    pitchChanged = true;
                    break;
                case c_parameterNoiseMix:
                    parameters[noise_mix] = value;
                    noiseChanged = true;
                    break;
                    case c_parameterNoiseResonance:
                    parameters[noise_res] = value;
                    noiseChanged = true;
                    break;
                case c_parameterNoiseFilterMode:
                    if ((size_t)value < c_noiseFilterModeElements){
                        parameters[noise_filter_mode] = value;
                        noiseChanged = true;}
                        break;
                    case c_parameterNoiseFilterFreq:
                        parameters[noise_filter_freq] = value;
                        noiseChanged = true;
                    break;
                case c_parameterNoiseFilterQ:
                    parameters[noise_filter_q] = value;
                    noiseChanged = true;
                    break;
                default:
                    break;
            }
        }
        prepareToPlay(noiseChanged, pitchChanged, resonatorChangedA, resonatorChangedB, couplingChanged);
        Reset(); // to reset voice states and models after parameter change
    }

	// This must be done before Reset(), as the the latter will update the last model used
	// but at the Init()
    inline void prepareToPlay(bool noiseChanged = true, bool pitchChanged = true,
        bool resonatorChangedA = true, bool resonatorChangedB = true,
        bool couplingChanged = true) {
        // Originally from OnSlider() of the original PluginProcessor
        // NOTE: Even if a_ratio and b_ratio are not editable in this version of mine
        // different model may have different default ratio.
        if (last_a_model != parameters[a_model])
        {
            if (parameters[a_model] == ModelNames::Beam) models->recalcBeam(true, parameters[a_ratio]);
            else if (parameters[a_model] == ModelNames::Membrane) models->recalcMembrane(true, parameters[a_ratio]);
            else if (parameters[a_model] == ModelNames::Plate) models->recalcPlate(true, parameters[a_ratio]);

        }
        if (last_b_model != parameters[b_model])
        {
            if (parameters[b_model] == ModelNames::Beam) models->recalcBeam(false, parameters[b_ratio]);
            else if (parameters[b_model] == ModelNames::Membrane) models->recalcMembrane(false, parameters[b_ratio]);
            else if (parameters[b_model] == ModelNames::Plate) models->recalcPlate(false, parameters[b_ratio]);

        }
        auto srate = getSampleRate();
        for (size_t i = 0; i < c_numVoices; ++i) {
            Voice& voice = *voices[i];
            if (noiseChanged) {
                voice.noise.init(srate,
                    parameters[noise_filter_mode],
                    parameters[noise_filter_freq],
                    parameters[noise_filter_q],
                    parameters[noise_att],
                    parameters[noise_dec],
                    parameters[noise_sus],
                    parameters[noise_rel],
                    parameters[vel_noise_freq],
                    parameters[vel_noise_q]
                );
            }
            // in this moment only a_coarse can be editable, b_coarse and a_fine, b_fine are not
            // editable in Drumlogue due to lack of parameters. Keep model default.
            if (pitchChanged) {
                voice.setPitch(parameters[a_coarse], parameters[b_coarse], parameters[a_fine], parameters[b_fine]);
            }
            if (resonatorChangedA) {
                voice.resA.setParams(srate, parameters[a_on], parameters[a_model], parameters[a_partials], parameters[a_decay], parameters[a_damp], parameters[a_tone], parameters[a_hit], parameters[a_rel], parameters[a_inharm], parameters[a_cut], parameters[a_radius], parameters[vel_a_decay], parameters[vel_a_hit], parameters[vel_a_inharm]);
            }
            if (resonatorChangedB) {
                voice.resB.setParams(srate, parameters[b_on], parameters[b_model], parameters[b_partials], parameters[b_decay], parameters[b_damp], parameters[b_tone], parameters[b_hit], parameters[b_rel], parameters[b_inharm], parameters[b_cut], parameters[b_radius], parameters[vel_b_decay], parameters[vel_b_hit], parameters[vel_b_inharm]);
            }
            if (couplingChanged) {
                voice.setCoupling(parameters[couple], parameters[ab_split]);
            }
            // not enough parameters to change resonator B, keep model default
            if (resonatorChangedA || resonatorChangedB)
                voice.updateResonators();
        }
    }

    inline int32_t getParameterValue(uint8_t index) const {
        if (index < c_parameterTotal)
        {
            switch(index) {
                case c_parameterProgramName:
                    return m_currentProgram;
                    break;
                case c_parameterGain:
                    return parameters[gain];
                    break;
                case c_parameterSampleBank:
                    return m_sampleBank;
                    break;
                case c_parameterSampleNumber:
                    return m_sampleNumber;
                    break;
                case c_parameterMalletResonance:
                    return parameters[mallet_res];
                    break;
                case c_parameterMalletStifness:
                    return parameters[mallet_stiff];
                    break;
                case c_parameterVelocityMalletResonance:
                    return parameters[vel_mallet_res];
                    break;
                case c_parameterVelocityMalletStifness:
                    return parameters[vel_mallet_stiff];
                    break;
                case c_parameterModel:
                    return parameters[a_model];
                    break;
                case c_parameterPartials:
                    return parameters[a_partials];
                    break;
                case c_parameterDecay:
                    return parameters[a_decay];
                    break;
                case c_parameterMaterial:
                    return parameters[a_damp];
                    break;
                case c_parameterTone:
                    return parameters[a_tone];
                    break;
                case c_parameterHitPosition:
                    return parameters[a_hit];
                    break;
                case c_parameterRelease:
                    return parameters[a_rel];
                    break;
                case c_parameterInharmonic:
                    return parameters[a_inharm];
                    break;
                case c_parameterFilterCutoff:
                    return parameters[a_cut];
                    break;
                case c_parameterTubeRadius:
                    return parameters[a_radius];
                    break;
                case c_parameterCoarsePitch:
                    return parameters[a_coarse];
                    break;
                case c_parameterNoiseMix:
                    return parameters[noise_mix];
                    break;
                case c_parameterNoiseResonance:
                    return parameters[noise_res];
                    break;
                case c_parameterNoiseFilterMode:
                    return parameters[noise_filter_mode];
                    break;
                case c_parameterNoiseFilterFreq:
                    return parameters[noise_filter_freq];
                    break;
                case c_parameterNoiseFilterQ:
                    return parameters[noise_filter_q];
                    break;
                default:
                    break;
            }
        }
        return 0;
    }

    inline const char *getParameterStrValue(uint8_t index, int32_t value) const {
        (void)value;
        switch (index) {
        // Note: String memory must be accessible even after function returned.
        //       It can be assumed that caller will have copied or used the string
        //       before the next call to getParameterStrValue
        case c_parameterSampleBank:
            if (value >= 0 && (size_t)value < c_sampleBankElements)
            return c_sampleBankName[value];
        case c_parameterProgramName:
            if (value >= 0 && value < last_program)
            return c_programName[value];
        case c_parameterModel:
            if (value >= 0 && (size_t)value < c_modelElements)
            return c_modelName[value];
        case c_parameterPartials:
            if (value >= 0 && (size_t)value < c_partialElements)
            return c_partialsName[value];
        case c_parameterNoiseFilterMode:
            if (value >= 0 && (size_t)value < c_noiseFilterModeElements)
            return c_noiseFilterModeName[value];
        default:
            break;
        }
        return nullptr;
    }

    inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const {
        (void)value;
        switch (index) {
            // Note: Bitmap memory must be accessible even after function returned.
            //       It can be assumed that caller will have copied or used the bitmap
            //       before the next call to getParameterBmpValue
            // Note: Not yet implemented upstream
            default:
            break;
        }
        return nullptr;
    }

    // onNote in PluginProcessor_orig.cpp
    inline void NoteOn(uint8_t note, uint8_t velocity) {
        auto srate = getSampleRate();

        nvoice = nextVoiceNumber();  // this is from resonator_orig.h
        Voice & voice = *voices[nvoice];
        // nvoice = (nvoice + 1) % polyphony;   // this is from PluginProcessor_orig.cpp

        // TODO: check voice gate and reset what?
        m_note = note;  // for GateOn/GateOff
        voice.note = note;

        // Sample - from resonator_orig.h
        const sample_wrapper_t* sampleWrapper = GetSample(m_sampleBank, m_sampleNumber - 1);

        if (sampleWrapper) {
            // Copy values we care about out of sampleWrapper before it changes.
            m_sampleChannels = sampleWrapper->channels;
            m_sampleFrames = sampleWrapper->frames;
            m_samplePointer = sampleWrapper->sample_ptr;    // from common/sample_wrapper.h
            m_sampleIndex = sampleWrapper->frames * m_sampleChannels * m_sampleStart / 1000;
            m_sampleEnd = sampleWrapper->frames * m_sampleChannels * m_sampleEnd / 1000;
        }

        // from resonator_orig.h
        voice.m_initialized = (sampleWrapper != nullptr);
        voice.m_gate = true;
        voice.m_framesSinceNoteOn = 0;  // Note stealing

        // from ripplerX.h
        // used to calculate the malletFreq for the trigger
        auto mallet_stiff = (float32_t)getParameterValue(Parameters::mallet_stiff);
        auto vel_mallet_stiff = (float32_t)getParameterValue(Parameters::vel_mallet_stiff);
        // auto malletFreq = fmin(5000.0, exp(log(mallet_stiff) + velocity / 127.0 * vel_mallet_stiff * (log(5000.0) - log(100.0))));
        auto malletFreq = fmin(5000.0, e_expf(fasterlogf(mallet_stiff) + velocity * vel_mallet_stiff * c_malletStiffnessCorrectionFactor));
        // equivalent to noteOn in resonator_orig.h
        voice.trigger(srate, note, velocity / 127.0, malletFreq);
    }

    // offNote in PluginProcessor_orig.cpp
    inline void NoteOff(uint8_t note) {
        for (auto& voice : voices)
        {
            voice->m_gate = false;
            if (voice->note == note || note == 0xFF) {
                voice->release();
            }
        }
        Reset();
    }
    // Gate should be set by the step sequencer or MIDI input
    // play chromatically only via MIDI
    inline void GateOn(uint8_t velocity) {
        NoteOn(m_note, velocity);
        // TODO: can this be used for multitimbrality? Create an array of x steps/notes/intruments, just one voice?
    }

    inline void GateOff() {
        NoteOff(m_note);
    }

    inline void AllNoteOff() {
        NoteOff(0xFF);
    }
    // merge from resonator_orig.h and PluginProcessor_orig.cpp
    inline void PitchBend(uint16_t bend)
    {
        // if (m_gate)
        // gate is now per voice; bend is for resonator A only
        // according to render in resonator_orig.h, bend is between 0 and 0x4000, with 0x2000 as center (no bend)
        // RipplerX uses -99 to 99 for fine pitch, so we convert
        parameters[a_fine] = (bend - 0x2000) * 100 / 0x2000;
        prepareToPlay(false, true, false, false, false);
    }

    inline void ChannelPressure(uint8_t pressure) { (void)pressure; }

    inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
        (void)note;
        (void)aftertouch;
    }

    inline void LoadPreset(uint8_t idx) {
        (void)idx;
        setCurrentProgram(idx); // TODO: review this
    }

    inline uint8_t getPresetIndex() const { return m_currentProgram; }

    // Reset last models and last partials so they don't trigger changes onSlider()
    inline void resetLastModels()
    {
        last_a_model = parameters[a_model];
        last_a_partials = parameters[a_partials];
        last_b_model = parameters[b_model];
        last_b_partials = parameters[b_partials];
    }

    inline void clearVoices()
    {
        for (auto& voice : voices)
        {
            voice->clear();
        }
    }

    /*===========================================================================*/
    /* Static Members. */
    /*===========================================================================*/

    static inline const char * getPresetName(uint8_t idx) {
        (void)idx;
        // Note: String memory must be accessible even after function returned.
        //       It can be assumed that caller will have copied or used the string
        //       before the next call to getPresetName
        return c_programName[idx];
    }

    private:

    /*===========================================================================*/
    /* Private Methods. */
    /*===========================================================================*/
    // from resonator_orig.h
    inline const sample_wrapper_t* GetSample(size_t bank, size_t number) {
        if (bank >= m_get_num_sample_banks_ptr()) return nullptr;
        if (number >= m_get_num_samples_for_bank_ptr(bank)) return nullptr;
        return m_get_sample(bank, number);
    }

    inline const size_t getSampleRate() {
        return c_sampleRate;
    }

    size_t nextVoiceNumber();

    /** this can be called via enum with program name!
    * store from programs to parameters the whole set of values
    */
    inline void setCurrentProgram(int index)
    {
        m_currentProgram = index;
        // programs are in constants.h
        parameters = programs[index];
    }

    /*===========================================================================*/
    // Use raw pointers instead of smart pointers
    Voice* voices[polyphony];
    Models* models; // Raw pointer for Models
    Comb    comb{};
    Limiter limiter{};
    // equivalent to m_voice
    int     nvoice = 0; // next voice to use

    /**
    * Parameters, both editable and not
    * no need to define each of them, the enum will detect them exactly
    * Static array: association between values and it's eaning is done via
    * enum values stored by setCurrentProgram().
    * parameter list can be found in constants.h
    */
    std::array<float32_t, last_param> parameters;
    // state variables - prefer static default instead of initialization as there's just one possible value
    uint8_t   m_currentProgram = 0;
    uint8_t   m_note = 60;
    float32_t scale = 1.0f; // TODO: make this editable?
    uint8_t   last_a_model = -1;
    uint8_t   last_b_model = -1;
    uint8_t   last_a_partials = -1;
    uint8_t   last_b_partials = -1;

    // sample management
    uint8_t   m_sampleBank = 0;  // parameter editable
    uint8_t   m_sampleNumber = 1; // parameter editable
    // see Init for default - adding initial values to avoid potential issues
    const sample_wrapper_t * sampleWrapper = nullptr; // set at NoteOn
    uint8_t   m_sampleChannels = 0; // Actual value set by sample_wrapper. 1 Mono, 2 Stereo
    size_t    m_sampleFrames = 0; // Actual value set by sample_wrapper.
    const float32_t * m_samplePointer; // From sample_wrapper
    uint16_t  m_sampleStart = 0; // NOTE: this is not editable, see c_parameterSampleStart in resonator_orig.h
    size_t    m_sampleIndex = 0; // Counts in float*, stride == channels
    size_t    m_sampleEnd = 1000; // 100%. Counts in float*, stride == channels
    // Functions from unit runtime
    unit_runtime_get_num_sample_banks_ptr       m_get_num_sample_banks_ptr = nullptr;
    unit_runtime_get_num_samples_for_bank_ptr   m_get_num_samples_for_bank_ptr = nullptr;
    unit_runtime_get_sample_ptr                 m_get_sample = nullptr;
}; // end class RipplerX

/*===========================================================================*/