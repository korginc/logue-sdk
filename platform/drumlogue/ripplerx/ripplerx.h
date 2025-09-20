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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include "runtime.h"
#include <arm_neon.h>
#include "constants.h"
#include "Voice.h"
#include "Resonator.h"
#include "Models.h"

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
    // taken from the original PluginProcessor()
    inline int8_t Init(const unit_runtime_desc_t * desc) {
        // Check compatibility of samplerate with unit, for drumlogue should be 48000
        if (desc->samplerate != c_sampleRate)
            return k_unit_err_samplerate;

        // Check compatibility of frame geometry
        if (desc->output_channels != 2)  // should be stereo output
            return k_unit_err_geometry;

        // Note: if need to allocate some memory can do it here and return k_unit_err_memory if getting allocation errors


        models = std::make_unique<Models>();
        // create intruments
        for (size_t i = 0; i < polyphony; ++i) {
            voices.push_back(std::make_unique<Voice>(*models));
            voices[i].comb.init(c_sampleRate);
            voices[i].limiter.init(c_sampleRate);
        }

        // private variables (state vars set at constructor)
        m_currentProgram = 0;
        m_note = 60;
        // sample management
        m_sampleBank = 0;
        m_sampleNumber = 1;
        m_sampleStart = 0;
        m_sampleEnd = 1000;  // 100%

        // Stash runtime functions to manage samples.
        m_get_num_sample_banks_ptr = desc->get_num_sample_banks;
        m_get_num_samples_for_bank_ptr = desc->get_num_samples_for_bank;
        m_get_sample = desc->get_sample;

        // Initial parameter values, editable one are indented and matching
        // with header.c, plus others that are characterizing each "program"
            parameters[mallet_mix] = 0.0f;
        parameters[mallet_res] = 8;
        parameters[mallet_stiff] = 600;
            parameters[a_on] = 1;   // true
        parameters[a_model] = 3;    // TODO - "Squared"
        parameters[a_partials] = c_partials[3]; // TODO check this value
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
            parameters[b_on] = 0;   // false, off as Drumlogue cannot edit them
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


        resetLastModels();
        // clearVoices();
        // onSlider();

        return k_unit_err_none;
    }

    inline void Teardown() {
        // Note: cleanup and release resources if any
    }

    inline void Reset() {
        // Note: Reset synth state. I.e.: Clear filter memory, reset oscillator
        // phase etc.
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

    inline void Render(float * __restrict outBuffer, size_t frames) {

        auto a_on = (bool)getParameterValue(a_on);
        auto b_on = (bool)getParameterValue(b_on);
        auto mallet_mix = (float32_t)getParameterValue(mallet_mix);
        auto mallet_res = (float32_t)getParameterValue(mallet_res);
        auto vel_mallet_mix = (float32_t)getParameterValue(vel_mallet_mix);
        auto vel_mallet_res = (float32_t)getParameterValue(vel_mallet_res);
        auto noise_mix = getParameterValue(noise_mix);
        // auto noise_mix_range = getParameterValue(noise_mix);
        // noise_mix = noise_mix_range.convertTo0to1(noise_mix);  //not needed (?)
        auto noise_res = getParameterValue(noise_res);
        // auto noise_res_range = getParameterValue(noise_res); // Paramater value already set in percentage
        // noise_res = noise_res_range.convertTo0to1(noise_res);
        auto vel_noise_mix = getParameterValue(vel_noise_mix);
        auto vel_noise_res = getParameterValue(vel_noise_res);
        auto serial = (bool)getParameterValue(couple);
        auto ab_mix = (float32_t)getParameterValue(ab_mix);
        auto gain = (float32_t)getParameterValue(gain);
        auto couple = (bool)getParameterValue(couple);
        gain = pow(10.0, gain / 20.0);

        // TODO NoteOn?

        // For each frame in batch:
        for (size_t i = 0; i < frames; i++) {
            // Set all lanes to same value
            float32x2_t dirOut  = vdup_n_f32(0.0f);  // direct output per sample (sum of all voices)
            float32x2_t resAOut = vdup_n_f32(0.0f);  // resonator A output
            float32x2_t resBOut = vdup_n_f32(0.0f);  // resonator B output
            // Action and Mixing stage
            float32x2_t audioIn;  //FOR PORTING as excitation

            // Sample, until it runs out. (from original resonator)
            if (m_sampleIndex < m_sampleEnd)
            {
                if (m_sampleChannels == 2) {
                    // Stereo sample
                    audioIn = vld1_f32(&m_samplePointer[m_sampleIndex]);
                } else {
                    // Mono sample
                    audioIn = vdup_n_f32(m_samplePointer[m_sampleIndex]);
                }
                audioIn = vmul_n_f32(audioIn, sampleGain);
                m_sampleIndex += m_sampleChannels;  //only usage of m_sampleIndex
            } else {
                audioIn = vdup_n_f32(0.0f);
            }

            for (int i = 0; i < polyphony; ++i)   //FOR PORTING in resonator orig. this is done at Render, which is the caller
            {
                Voice& voice = *voices[i];
                float32x2_t resOut = vdup_n_f32(0.0f);

                auto msample = voice.mallet.process(); // process mallet
                if (msample) {
                    // dirOut += msample * fmin(1.0, mallet_mix + vel_mallet_mix * voice.vel);
                    dirOut = vfma_f32(dirOut, msample, vmin_f32(1.0, vfma_f32(mallet_mix, vel_mallet_mix, voice.vel)));
                    // resOut += msample * fmin(1.0, mallet_res + vel_mallet_res * voice.vel);
                    resOut = vfma_f32(resOut, msample, vmin_f32(1.0, vfma_f32(mallet_res, vel_mallet_mix, voice.vel)));
                }

                if (audioIn && voice.isPressed)  //TODO isPressed
                resOut += audioIn;

                float32x2_t nsample = voice.noise.process(); // process noise
                if (nsample) {
                    // dirOut += nsample * (float32_t)noise_mix_range.convertFrom0to1(fmin(1.f, noise_mix + vel_noise_mix * (float)voice.vel));
                    dirOut = vfma_f32(nsample, (float32_t)noise_mix_range.convertFrom0to1(vmin_f32(1.f, vfma_f32(noise_mix, vel_noise_mix,
                                                                                                              (float)voice.vel))), dirOut);
                    // resOut += nsample * (float32_t)noise_res_range.convertFrom0to1(fmin(1.f, noise_res + vel_noise_res * (float)voice.vel));
                    resOut = vfma_f32(nsample, (float32_t)noise_res_range.convertFrom0to1(vmin_f32(1.f, vfma_f32(noise_res, vel_noise_res,
                                                                                                              (float)voice.vel))), resOut);
                }

                auto out_from_a = 0.0; // output from voice A into B in case of resonator serial coupling
                if (a_on) {
                    auto out = voice.resA.process(resOut);  //TODO add Neon assembler APIs into process?
                    if (voice.resA.cut > 20.0001) out = voice.resA.filter.df1(out);
                    resAOut = vadd_f32(out, resAOut);
                    out_from_a = out;
                }

                if (b_on) {
                    auto out = voice.resB.process(a_on && couple ? out_from_a : resOut);
                    if (voice.resB.cut > 20.0001)
                    out = voice.resB.filter.df1(out);
                    resBOut = vadd_f32(out, resBOut);
                }
            }   // end for polyphony

            // two floats as one per channel
            float32x2_t resOut;
            if (a_on && b_on)
                resOut = serial ? resBOut : vadd_f32(vmul_n_f32(resBOut, ab_mix), vmul_n_f32(resAOut, 1 - ab_mix)); // resAOut * (1-ab_mix) + resBOut * ab_mix;
            else
                resOut = vadd_f32(resAOut, resBOut); // one of them is turned off, just sum the two

            // TODO if GAIN will be not an user editable parameter this operation can be skipped?
            auto totalOut = vmla_n_f32(dirOut, resOut, gain); // dirOut + resOut * gain
            float32x2_t split;
            // auto [spl0, spl1] =  voice.comb.process(totalOut);
            split =  voice.comb.process(totalOut);  // TODO correct this
            // auto [left, right] = voice.limiter.process(split);
            float32x2_t channels = voice.limiter.process(split);

            // Add current float32x2 to output buffer.
            float32x2_t old = vld1_f32(outBuffer);  // load existing buffer from pointer
            channels = vadd_f32(old, channels);
            // each voice contributes to single sample, in stereo
            vst1_f32(outBuffer, channels);  // replace existing buffer with new value
            outBuffer += 2; // move pointer by two positions for each sample, as we process two values at once, one per channel

        }   // end for frames
        m_framesSinceNoteOn += frames;  // For voice stealing.
    }

    // Read parameter from user (6 pages listed in header.c)
    // I suppose that's the same as onSlider() of the original PluginProcessor
    inline void setParameter(uint8_t index, int32_t value) {
        if (index < c_parameterTotal)
        {
            switch(index) {
                case c_parameterProgramName:
                    // load whole set of parameters according to pre-calculated program model
                    if (value < last_program)
                        setCurrentProgram(value);
                    break;
                case c_parameterGain:
                    parameters[gain] = value;
                    break;
                case c_parameterSampleBank:
                if (value < c_sampleBankElements)
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
                    if (value < c_modelElements)
                        parameters[a_model] = value;
                    break;
                case c_parameterPartials:
                    if (value < c_partialElements)
                        parameters[a_partials] = c_partials(a_partials, value);
                    break;
                case c_parameterDecay:
                    parameters[a_decay] = value;
                    break;
                case c_parameterMaterial:
                    parameters[a_damp] = value;
                    break;
                case c_parameterTone:
                    parameters[a_tone] = value;
                    break;
                case c_parameterHitPosition:
                    parameters[a_hit] = value;
                    break;
                case c_parameterRelease:
                    parameters[a_rel] = value;
                    break;
                case c_parameterInharmonic:
                    parameters[a_inharm] = value;
                    break;
                case c_parameterFilterCutoff:
                    parameters[a_cut] = value;
                    break;
                case c_parameterTubeRadius:
                    parameters[a_radius] = value;
                    break;
                case c_parameterCoarsePitch:
                    parameters[a_coarse] = value;
                    break;
                case c_parameterNoiseMix:
                    parameters[noise_mix] = value;
                    break;
                case c_parameterNoiseResonance:
                    parameters[noise_res] = value;
                    break;
                case c_parameterNoiseFilterMode:
                    if (value < c_noiseFilterModeElements)
                        parameters[noise_filter_mode] = value;
                    break;
                case c_parameterNoiseFilterFreq:
                    parameters[noise_filter_freq] = value;
                    break;
                case c_parameterNoiseFilterQ:
                    parameters[noise_filter_q] = value;
                    break;
                default:
                    break;
            }
        }
    }

    // TODO as OnSlider ? Or put everything above in setParameter?

    // for (int i = 0; i < polyphony; i++) {
    //     Voice& voice = *voices[i];
    //     voice.noise.init(srate, noise_filter_mode, noise_filter_freq, noise_filter_q, noise_att, noise_dec, noise_sus, noise_rel);
    //     voice.setPitch(a_coarse, b_coarse, a_fine, b_fine);
    //     voice.resA.setParams(srate, a_on, a_model, a_partials, a_decay, a_damp, a_tone, a_hit, a_rel, a_inharm, a_cut, a_radius, vel_a_decay, vel_a_hit, vel_a_inharm);
    //     voice.resB.setParams(srate, b_on, b_model, b_partials, b_decay, b_damp, b_tone, b_hit, b_rel, b_inharm, b_cut, b_radius, vel_b_decay, vel_b_hit, vel_b_inharm);
    //     voice.setCoupling(couple, split);
    //     voice.updateResonators();
    // }

    inline int32_t getParameterValue(uint8_t index) const {
        if (index < c_parameterTotal)
        {
            switch(index) {
                case c_parameterProgramName = 0:
                    return m_currentProgram;
                    break;
                case c_parameterGain = 1:
                    return parameters[gain];
                    break;
                case c_parameterSampleBank = 2:
                    return m_sampleBank;
                    break;
                case c_parameterSampleNumber = 3:
                    return m_sampleNumber;
                    break;
                case c_parameterMalletResonance = 4:
                    return parameters[mallet_res];
                    break;
                case c_parameterMalletStifness = 5:
                    return parameters[mallet_stiff];
                    break;
                case c_parameterVelocityMalletResonance = 6:
                    return parameters[vel_mallet_res];
                    break;
                case c_parameterVelocityMalletStifness = 7:
                    return parameters[vel_mallet_stiff];
                    break;
                case c_parameterModel = 8:
                    return parameters[a_model];
                    break;
                case c_parameterPartials = 9:
                    return parameters[a_partials];
                    break;
                case c_parameterDecay = 10:
                    return parameters[a_decay];
                    break;
                case c_parameterMaterial = 11:
                    return parameters[a_damp];
                    break;
                case c_parameterTone = 12:
                    return parameters[a_tone];
                    break;
                case c_parameterHitPosition = 13:
                    return parameters[a_hit];
                    break;
                case c_parameterRelease = 14:
                    return parameters[a_rel];
                    break;
                case c_parameterInharmonic = 15:
                    return parameters[a_inharm];
                    break;
                case c_parameterFilterCutoff = 16:
                    return parameters[a_cut];
                    break;
                case c_parameterTubeRadius = 17:
                    return parameters[a_radius];
                    break;
                case c_parameterCoarsePitch = 18:
                    return parameters[a_coarse];
                    break;
                case c_parameterNoiseMix = 19:
                    return parameters[noise_mix];
                    break;
                case c_parameterNoiseResonance = 20:
                    return parameters[noise_res];
                    break;
                case c_parameterNoiseFilterMode = 21:
                    return parameters[noise_filter_mode];
                    break;
                case c_parameterNoiseFilterFreq = 22:
                    return parameters[noise_filter_freq];
                    break;
                case c_parameterNoiseFilterQ = 23:
                    return parameters[noise_filter_q];
                    break;
                default:
                    break;
            }
        }
        return 0;
    }

    inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
        (void)value;
        switch (index) {
        // Note: String memory must be accessible even after function returned.
        //       It can be assumed that caller will have copied or used the string
        //       before the next call to getParameterStrValue
        case c_parameterSampleBank:
            if (value < 0 || value >= c_sampleBankElements)
            return nullptr;
            return c_sampleBankName[value];
        case c_parameterProgramName:
            if (value < 0 || value >= last_program)
            return nullptr;
            return c_programName[value];
        case c_parameterModel:
            if (value < 0 || value >= c_modelElements)
            return nullptr;
            return c_modelName[value];
        case c_parameterPartials:
            if (value < 0 || value >= c_partialElements)
            return nullptr;
            return c_partialsName[value];
        case c_parameterNoiseFilterMode:
            if (value < 0 || value >= c_noiseFilterModeElements)
            return nullptr;
            return c_noiseFilterModeName[value];
        default:
            break;
        }
        return nullptr;
    }

    inline const uint8_t * getParameterBmpValue(uint8_t index,
        int32_t value) const {
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

    inline void NoteOn(uint8_t note, uint8_t velocity) {
        auto srate = getSampleRate();
        Voice& voice = *voices[nvoice];
        nvoice = (nvoice + 1) % polyphony;

        // Sample
        if (sampleWrapper) {
            // Copy values we care about out of sampleWrapper before it changes.
            m_sampleChannels = sampleWrapper->channels;
            m_sampleFrames = sampleWrapper->frames;
            m_samplePointer = sampleWrapper->sample_ptr;
            m_sampleIndex = sampleWrapper->frames * m_sampleChannels * sampleStart / 1000;
            m_sampleEnd = sampleWrapper->frames * m_sampleChannels * sampleEnd / 1000;
        }

        // used to calculate the malletFreq for the trigger
        auto mallet_stiff = (float32_t)getParameterValue(mallet_stiff);
        auto vel_mallet_stiff = (float32_t)getParameterValue(vel_mallet_stiff);
        auto malletFreq = fmin(5000.0, exp(log(mallet_stiff) + velocity / 127.0 * vel_mallet_stiff * (log(5000.0) - log(100.0))));

        voice.trigger(srate, note, velocity / 127.0, malletFreq);
    }

    inline void NoteOff(uint8_t note) {
        m_gate = false;
        for (int i = 0; i < polyphony; ++i) {
            Voice& voice = *voices[i];
            if (voice.note == note || note == 0xFF) {
                voice.release();
            }
        }
    }
    // play chromatically only via MIDI
    inline void GateOn(uint8_t velocity) {
        NoteOn(m_note, velocity);   //TODO m_note - see Note (Gate mode) in header.c
    }

    inline void GateOff() {
        NoteOff(m_note);
    }

    inline void AllNoteOff() {
        NoteOff(0xFF);
    }

    inline void PitchBend(uint16_t bend)
    {
        if (m_gate) parameters[a_fine] = bend;  // TODO convert to -99 - 99
    }

    inline void ChannelPressure(uint8_t pressure) { (void)pressure; }

    inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
        (void)note;
        (void)aftertouch;
    }

    inline void LoadPreset(uint8_t idx) { (void)idx; }

    inline uint8_t getPresetIndex() const { return 0; }

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
        for (int i = 0; i < polyphony; ++i) {
            voices[i].clear();
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
        return nullptr;
    }

    private:

    /*===========================================================================*/
    /* Private Methods. */
    /*===========================================================================*/
    inline const sample_wrapper_t* GetSample(size_t bank, size_t number) {
        if (bank >= m_get_num_sample_banks_ptr()) return nullptr;
        if (number >= m_get_num_samples_for_bank_ptr(bank)) return nullptr;
        return m_get_sample(bank, number);
    }

    inline const size_t getSampleRate() {
        return c_sampleRate;
    }

    inline const size_t getFramesSinceNoteOn() {
        if (!m_initialized) return SIZE_MAX;
        return m_framesSinceNoteOn;
    }

    size_t nextVoiceNumber()
    {
        size_t longestFramesSinceNoteOn = 0;
        size_t bestVoice = 0;

        for (size_t i = 1; i < c_numVoices; i++)
        {
            size_t thisVoiceFramesSinceNoteOn = voices[i].getFramesSinceNoteOn();
            if (thisVoiceFramesSinceNoteOn > longestFramesSinceNoteOn)
            {
                longestFramesSinceNoteOn = thisVoiceFramesSinceNoteOn;
                bestVoice = i;
            }
        }

        return bestVoice;
    }

    /** this can be called via enum with program name! */
    inline const void setCurrentProgram (int index)
    {
        m_currentProgram = index;

        parameters = programs[index];

        // TODO?
        clearVoices();
        resetLastModels();
    }

    /*===========================================================================*/
    /* Private Member Variables. */
    /*===========================================================================*/
    std::vector<std::unique_ptr<Voice>> voices;
    std::unique_ptr<Models> models;
    Comb comb{};
    Limiter limiter{};
      // equivalent to m_voice
    int     nvoice = 0; // next voice to use

    // Parameters, both editable and not
    // no need to define each of them, the enum will detect them exactly
    // float32_t parameters[last_param];
    std::array<float32_t, last_param> parameters;  // store all the params, either editable or not and access them via enum

    // state variables - prefer static default instead of initialization as there's just one possible value
    bool     m_initialized = false;
    bool     m_gate = false;
    size_t   m_framesSinceNoteOn = SIZE_MAX; // Voice stealing
    uint8_t  m_currentProgram = 0;
    uint8_t  m_note = 60;

    // sample management
    uint8_t  m_sampleBank = 0;  // parameter editable
    uint8_t  m_sampleNumber = 1; // parameter editable
    // see Init
    uint8_t  m_sampleChannels; // From sample_wrapper
    size_t   m_sampleFrames; // From sample_wrapper
    const float * m_samplePointer; // From sample_wrapper
    uint16_t m_sampleStart = 0;
    size_t   m_sampleIndex; // Counts in float*, stride == channels
    size_t   m_sampleEnd = 1000; // 100%. Counts in float*, stride == channels
    // Functions from unit runtime
    unit_runtime_get_num_sample_banks_ptr m_get_num_sample_banks_ptr;
    unit_runtime_get_num_samples_for_bank_ptr m_get_num_samples_for_bank_ptr;
    unit_runtime_get_sample_ptr m_get_sample;

    /*===========================================================================*/
    /* Constants. */
    /*===========================================================================*/
    static const char* const c_sampleBankName[c_sampleBankElements];
    static const char* const c_modelName[c_modelElements];
    static const char* const c_partialsName[c_partialElements];
    static const char* const c_noiseFilterModeName[c_noiseFilterModeElements];
    static const char* const c_programName[last_program];
}; // end class RipplerX
