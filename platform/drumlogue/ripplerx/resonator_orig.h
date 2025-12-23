#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdbool>
#include <cstring>
#include <cmath>
#include <type_traits>
#include <arm_neon.h>

#include "unit.h"
#include "constants.h"

constexpr size_t c_numVoices = 8; /**< equivalent to polyphony */
constexpr size_t c_delayLineFrames = 10000;
constexpr size_t c_dispersionStages = 4;
/**< this is enum actually */
constexpr size_t c_parameterResonatorNote = 0;
constexpr size_t c_parameterResonatorPitchBendRange = 1;
constexpr size_t c_parameterLevelVelocitySensitivity = 2;
constexpr size_t c_parameterFilterVelocitySensitivity = 3;
constexpr size_t c_parameterSampleBank = 4;
constexpr size_t c_parameterSampleNumber = 5;
constexpr size_t c_parameterSampleStart = 6;
constexpr size_t c_parameterSampleEnd = 7;
constexpr size_t c_parameterExcitationLevel = 8;
constexpr size_t c_parameterExcitationFilterType = 9;
constexpr size_t c_parameterExcitationFilterCutoff = 10;
constexpr size_t c_parameterExcitationFilterResonance = 11;
constexpr size_t c_parameterResonatorDecayFeedback = 12;
constexpr size_t c_parameterResonatorDecayDamping = 13;
constexpr size_t c_parameterResonatorDispersion = 14;
constexpr size_t c_parameterResonatorKeyTracking = 15;
constexpr size_t c_parameterResonatorReleaseFeedback = 16;
constexpr size_t c_parameterResonatorReleaseDamping = 17;

class Resonator {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types. */
  /*===========================================================================*/
  class Voice {
  public:
    Voice()
    : m_initialized(false)
    , m_gate(false)
    , m_framesSinceNoteOn(SIZE_MAX)
    {
      memset(m_delayLine, 0, c_delayLineFrames * sizeof(float));
      m_delayLineOutPosition = 0;
    }
    ~Voice() {}

    // FOR PORTING this equivalent to voice trigger
    void noteOn(uint8_t noteNumber, uint8_t attackVelocity, uint16_t pitchBend,
      uint8_t pitchBendRange, int8_t excitationLevelVelocitySensitivity, int8_t excitationFilterVelocitySensitivity,
      const sample_wrapper_t* const sampleWrapper, uint16_t sampleStart, uint16_t sampleEnd,
      int16_t excitationLevel, Filter::Type excitationFilterType, uint16_t excitationFilterCutoff, uint16_t excitationFilterResonance,
      int16_t feedback, uint8_t damping, uint16_t dispersion, uint16_t keytrack)
    {
      if (m_gate)
      {
        // Clear out the delay line if another note was sounding.
        // (This is a bit hackish but it seems to help prevent artifacts.)
        memset(m_delayLine, 0, c_delayLineFrames * sizeof(float));
      }

      // Pitch
      m_noteNumber = noteNumber;
      m_pitchBend = pitchBend;
      m_pitchBendRange = pitchBendRange;

      // Sample
      if (sampleWrapper) {
        // Copy values we care about out of sampleWrapper before it changes.
        m_sampleChannels = sampleWrapper->channels;
        m_sampleFrames = sampleWrapper->frames;
        m_samplePointer = sampleWrapper->sample_ptr;
        m_sampleIndex = sampleWrapper->frames * m_sampleChannels * sampleStart / 1000;
        m_sampleEnd = sampleWrapper->frames * m_sampleChannels * sampleEnd / 1000;
      }

      // Excitation filter
      // Adjust excitation level by up to -6 .. +6 dB at maximum velocity sensitivity.
      m_excitationLevel = excitationLevel + 60.0 * (attackVelocity - 64) / 64.0 * excitationLevelVelocitySensitivity / 100.0;
      float cutoff = excitationFilterCutoff * 100.0;
      float resonance = excitationFilterResonance / 10.0;
      switch (excitationFilterType) {
      case Filter::Type::None:
      case Filter::Type::AP1:
        break;
      case Filter::Type::LP6:
      case Filter::Type::LP12:
        // Increase cutoff frequency geometrically 0.5 .. 2 times depending on velocity.
        cutoff *= pow(2.0, (attackVelocity - 64) / 64.0 * excitationFilterVelocitySensitivity / 100.0);
        break;
      case Filter::Type::HP6:
      case Filter::Type::HP12:
        // Decrease cutoff frequency geometrically 0.5 .. 2 times depending on velocity.
        cutoff /= pow(2.0, (attackVelocity - 64) / 64.0 * excitationFilterVelocitySensitivity / 100.0);
        break;
      case Filter::Type::BP6:
        // Increase resonance by factor of 0.25 .. 4.0 times depending on velocity.
        resonance *= pow(4.0, (attackVelocity - 64) / 64.0 * excitationFilterVelocitySensitivity / 100.0);
        break;
      }
      cutoff = fmin(cutoff, c_sampleRate / 2.0); // Nyquist limit
      m_excitationFilter.init(excitationFilterType, cutoff, resonance);

      // Parameters used by render()
      m_feedback = feedback;
      m_damping = damping;
      m_dispersion = dispersion;
      m_keytrack = keytrack;

      m_initialized = (sampleWrapper != nullptr);
      m_gate = true;
      m_framesSinceNoteOn = 0;  // Note stealing
      vst1_f32(m_dampingState, vdup_n_f32(0.0f));
    }

    void noteOff(uint8_t noteNumber, int16_t feedback, uint16_t damping)
    {
      if (noteNumber == m_noteNumber || noteNumber == 0xFF)
      {
        m_gate = false;
        m_feedback = feedback;
        m_damping = damping;
      }
    }

    void pitchBend(uint16_t bend)
    {
      if (m_gate) m_pitchBend = bend;
    }

    // FOR PORTING this is equivalent to processBlockByType
    // this is called for each voice of poliphony
    inline void render(float* __restrict outBuffer, size_t frames)
    {
      if (!m_initialized) return;

      // Pitch
      // Calculate delay line length based on current note and pitch bend.
      float noteFrequency = 440.0f * pow(
        c_semitoneFrequencyRatio,
        (float) m_noteNumber + m_pitchBendRange*((float) m_pitchBend - 0x2000)/0x4000
         - 69 // MIDI note 69 == A4 == 440.0 Hz
      );
      float delayLineLength = (float) c_sampleRate / noteFrequency;
      if (delayLineLength >= c_delayLineFrames) delayLineLength = c_delayLineFrames - 2;

      // Dispersion allpass filter chain.
      for (size_t i = 0; i < m_dispersion; i++)
      {
        float dispersionFrequency = noteFrequency * c_dispersionFilterRatios[i];
        dispersionFrequency = fmin(dispersionFrequency, c_sampleRate / 2.01f); // Nyquist with safety factor.
        m_dispersionFilters[i].init(Filter::Type::AP1, dispersionFrequency, 0.0f);
        float wT = 2.0 * M_PI * noteFrequency / c_sampleRate;
        // https://github.com/grame-cncm/faustlibraries/blob/master/misceffects.lib
        delayLineLength -= (atan(sin(wT)/(m_dispersionFilters[i].m_b0 + cos(wT))) / wT
                           - atan(sin(wT)/(1.0f / m_dispersionFilters[i].m_b0 + cos(wT))) / wT);
      }

      // Fractional delay line using allpass filter.
      // https://ccrma.stanford.edu/~jos/pasp/First_Order_Allpass_Interpolation.html
      size_t integerDelayLineLength = (size_t) delayLineLength;
      if (delayLineLength - integerDelayLineLength < 0.2f) integerDelayLineLength--;
      float fractionalDelay = delayLineLength - integerDelayLineLength; // 0.2 .. 1.2
      float fractionalDelayCoefficient = (1.0f - fractionalDelay) / (1.0f + fractionalDelay);
      m_fractionalDelayFilter.setCoefficients(fractionalDelayCoefficient, 1, 0, fractionalDelayCoefficient, 0);

      // Excitation Level (tenths of dB).
      float sampleGain = pow(2, m_excitationLevel / 30.0f);

      // Damping (convert 0 .. 100 -> 1.0 .. 0.5)
      float damping = 1.0f - (float) m_damping / 200.0f;

      // Feedback (adjust according to key tracking).
      // keytrack = 0: naive feedback, lower notes resonate longer.
      // keytrack = 100: octave up -> sqrt(feedback); octave down -> square(feedback).
      float feedback = copysign(
        pow(fabs(m_feedback / 1000.0f),
            pow(0.5, m_keytrack / 100.0 * (m_noteNumber - 60) / 12.0)),
        (float) m_feedback);


      m_delayLineInPosition = m_delayLineOutPosition + (int) delayLineLength * 2;
      if (m_delayLineInPosition >= c_delayLineFrames) m_delayLineInPosition -= c_delayLineFrames;

      float32x2_t dampingState = vld1_f32(m_dampingState);

      // For each frame in batch:
      for (size_t i = 0; i < frames; i++)
      {
        float32x2_t excitation;

        // Sample, until it runs out.
        if (m_sampleIndex < m_sampleEnd)
        {
          if (m_sampleChannels == 2) {
            // Stereo sample
            excitation = vld1_f32(&m_samplePointer[m_sampleIndex]);
          } else {
            // Mono sample
            excitation = vdup_n_f32(m_samplePointer[m_sampleIndex]);    // Load all lanes of vector to the same literal value
          }
          excitation = vmul_n_f32(excitation, sampleGain);  // TTVector multiply by scalar
          m_sampleIndex += m_sampleChannels;
        } else {
          excitation = vdup_n_f32(0.0f);
        }
        // Excitation filter
        excitation = m_excitationFilter.process(excitation);

        float32x2_t result = excitation;

        // Get output from delay line.
        float32x2_t delayOut = vld1_f32(&m_delayLine[m_delayLineOutPosition]);
        delayOut = m_fractionalDelayFilter.process(delayOut);

        // Damping filter in feedback loop.
        float32x2_t dampingDifference = vsub_f32(delayOut, dampingState);
        float32x2_t feedbackValue = vmla_n_f32(dampingState, dampingDifference, damping);
        dampingState = delayOut;

        // Dispersion filter chain in feedback loop.
        for (size_t i = 0; i < m_dispersion; i++) {
          feedbackValue = m_dispersionFilters[i].process(feedbackValue);
        }

        // Add feedback to result.
        result = vmla_n_f32(result, feedbackValue, feedback);
        // Prevent bad clipping (a bit is OK).
        result = vmax_f32(result, vdup_n_f32(-2.0f));
        result = vmin_f32(result, vdup_n_f32(2.0f));

        // Put result into delay line.
        vst1_f32(&m_delayLine[m_delayLineInPosition], result);

        // Advance delay line.
        if ((m_delayLineInPosition += 2) >= c_delayLineFrames) m_delayLineInPosition = 0;
        if ((m_delayLineOutPosition += 2) >= c_delayLineFrames) m_delayLineOutPosition = 0;

        // Add current float32x2 to output buffer.
        float32x2_t old = vld1_f32(outBuffer);
        result = vadd_f32(old, result);
        vst1_f32(outBuffer, result);
        outBuffer += 2;
      } // end for frames
      vst1_f32(m_dampingState, dampingState); // Remember damping state for next set of frames.
      m_framesSinceNoteOn += frames;  // For voice stealing.
    }

    inline size_t getFramesSinceNoteOn() const {
      if (!m_initialized) return SIZE_MAX;
      return m_framesSinceNoteOn;
    }

  private:
    bool m_initialized;
    bool m_gate;
    uint8_t m_noteNumber;
    uint8_t m_attackVelocity;
    uint16_t m_pitchBend;
    uint8_t m_pitchBendRange;
    size_t m_framesSinceNoteOn; // Voice stealing

    uint8_t m_sampleChannels; // From sample_wrapper
    size_t m_sampleFrames; // From sample_wrapper
    const float * m_samplePointer; // From sample_wrapper

    size_t m_sampleIndex; // Counts in float*, stride == channels
    size_t m_sampleEnd; // Counts in float*, stride == channels

    float m_excitationLevel;
    BiQuadFilter m_excitationFilter;

    uint8_t m_damping;
    int16_t m_feedback;
    uint8_t m_dispersion;
    int16_t m_keytrack;

    float m_dampingState[2]; // z^-1 in resonator feedback
    BiQuadFilter m_dispersionFilters[c_dispersionStages];

    size_t m_delayLineInPosition;
    size_t m_delayLineOutPosition;
    // float delayLineLength;
    BiQuadFilter m_fractionalDelayFilter;
    float m_delayLine[c_delayLineFrames]; // Stereo.

  };

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/
// FOR PORTING: this is more or less equivalent to class RipplerXAudioProcessor
  Resonator(void) {}
  ~Resonator(void) {}

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    // Check compatibility of samplerate with unit, for drumlogue should be 48000
    if (desc->samplerate != c_sampleRate)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    if (desc->output_channels != 2)  // should be stereo output
      return k_unit_err_geometry;

    // Note: if need to allocate some memory can do it here and return k_unit_err_memory if getting allocation errors

    // Stash runtime functions to manage samples.
    m_get_num_sample_banks_ptr = desc->get_num_sample_banks;
    m_get_num_samples_for_bank_ptr = desc->get_num_samples_for_bank;
    m_get_sample = desc->get_sample;

    // Initial parameter values, matching header.c
    // page 1
    m_note = 60;
    m_resonatorPitchBendRange = 2;
    m_excitationLevelVelocitySensitivity = 0;
    m_excitationFilterVelocitySensitivity = 0;
    // page 2
    m_sampleBank = 0;
    m_sampleNumber = 1;
    m_sampleStart = 0;
    m_sampleEnd = 1000;  // 100%
    // page 3
    m_excitationLevel = 0;
    m_excitationFilterType = Filter::Type::None;
    m_excitationFilterCutoff = 500;
    m_excitationFilterResonance = 10;
    // page 4
    m_resonatorDecayFeedback = 990;
    m_resonatorDecayDamping = 20;
    m_resonatorDispersion = 0;
    m_resonatorKeyTracking = 100;
    // page 5
    m_resonatorReleaseFeedback = 950;
    m_resonatorReleaseDamping = 80;

    // Initial other values
    m_pitchBend = 0x2000;  // Centre

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

  fast_inline void Render(float * __restrict out, size_t frames) {

    memset(out, 0, frames * 2 * sizeof(float));

    for (size_t i = 0; i < c_numVoices; i++)
    {
      m_voice[i].render(out, frames);
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    (void)value;
    switch (index) {
      case c_parameterResonatorNote:
        m_note = value;
        break;
      case c_parameterResonatorPitchBendRange:
        m_resonatorPitchBendRange = value;
        break;
      case c_parameterLevelVelocitySensitivity:
        m_excitationLevelVelocitySensitivity = value;
        break;
      case c_parameterFilterVelocitySensitivity:
        m_excitationFilterVelocitySensitivity = value;
        break;
      case c_parameterSampleBank:
        m_sampleBank = value;
        break;
      case c_parameterSampleNumber:
        m_sampleNumber = value;
        break;
      case c_parameterSampleStart:
        m_sampleStart = value;
        break;
      case c_parameterSampleEnd:
        m_sampleEnd = value;
        break;
      case c_parameterExcitationLevel:
        m_excitationLevel = value;
        break;
      case c_parameterExcitationFilterType:
        m_excitationFilterType = static_cast<Filter::Type>(value);
        break;
      case c_parameterExcitationFilterCutoff:
        m_excitationFilterCutoff = value;
        break;
      case c_parameterExcitationFilterResonance:
        m_excitationFilterResonance = value;
        break;
      case c_parameterResonatorDecayFeedback:
        m_resonatorDecayFeedback = value;
        break;
      case c_parameterResonatorDecayDamping:
        m_resonatorDecayDamping = value;
        break;
      case c_parameterResonatorDispersion:
        m_resonatorDispersion = value;
        break;
      case c_parameterResonatorKeyTracking:
        m_resonatorKeyTracking = value;
        break;
      case c_parameterResonatorReleaseFeedback:
        m_resonatorReleaseFeedback = value;
        break;
      case c_parameterResonatorReleaseDamping:
        m_resonatorReleaseDamping = value;
        break;
      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
      case c_parameterResonatorNote:
        return m_note;
      case c_parameterResonatorPitchBendRange:
        return m_resonatorPitchBendRange;
      case c_parameterLevelVelocitySensitivity:
        return m_excitationLevelVelocitySensitivity;
      case c_parameterFilterVelocitySensitivity:
        return m_excitationFilterVelocitySensitivity;
      case c_parameterSampleBank:
        return m_sampleBank;
      case c_parameterSampleNumber:
        return m_sampleNumber;
      case c_parameterSampleStart:
        return m_sampleStart;
      case c_parameterSampleEnd:
        return m_sampleEnd;
      case c_parameterExcitationLevel:
        return m_excitationLevel;
      case c_parameterExcitationFilterType:
        return static_cast<std::underlying_type_t<Filter::Type>>(m_excitationFilterType);
      case c_parameterExcitationFilterCutoff:
        return m_excitationFilterCutoff;
      case c_parameterExcitationFilterResonance:
        return m_excitationFilterResonance;
      case c_parameterResonatorDecayFeedback:
        return m_resonatorDecayFeedback;
      case c_parameterResonatorDecayDamping:
        return m_resonatorDecayDamping;
      case c_parameterResonatorDispersion:
        return m_resonatorDispersion;
      case c_parameterResonatorKeyTracking:
        return m_resonatorKeyTracking;
      case c_parameterResonatorReleaseFeedback:
        return m_resonatorReleaseFeedback;
      case c_parameterResonatorReleaseDamping:
        return m_resonatorReleaseDamping;
      default:
        break;
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
        if (value < 0 || value >= (int32_t) (sizeof c_sampleBankName / sizeof (const char*)))
          return nullptr;
        return c_sampleBankName[value];
      case c_parameterExcitationFilterType:
        if (value < 0 || value >= (int32_t) (sizeof c_excitationFilterTypeName / sizeof (const char*)))
          return nullptr;
        return c_excitationFilterTypeName[value];
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
      default:
        break;
    }
    return nullptr;
  }

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    size_t voice = nextVoiceNumber();
    m_voice[voice].noteOn(note, velocity,
      m_pitchBend, m_resonatorPitchBendRange, m_excitationLevelVelocitySensitivity, m_excitationFilterVelocitySensitivity,
      GetSample(m_sampleBank, m_sampleNumber-1), m_sampleStart, m_sampleEnd,
      m_excitationLevel, m_excitationFilterType, m_excitationFilterCutoff, m_excitationFilterResonance,
      m_resonatorDecayFeedback, m_resonatorDecayDamping, m_resonatorDispersion, m_resonatorKeyTracking);
  }

  inline void NoteOff(uint8_t note) {
    for (size_t i = 0; i < c_numVoices; i++)
      m_voice[i].noteOff(note, m_resonatorReleaseFeedback, m_resonatorReleaseDamping);
  }

  inline void GateOn(uint8_t velocity) {
    NoteOn(m_note, velocity);
  }

  inline void GateOff() {
    NoteOff(m_note);
  }

  inline void AllNoteOff() {
    NoteOff(0xFF);
  }

  inline void PitchBend(uint16_t bend) {
    m_pitchBend = bend;
    for (size_t i = 0; i < c_numVoices; i++)
      m_voice[i].pitchBend(bend);
  }

  inline void ChannelPressure(uint8_t pressure) { (void)pressure; }

  inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note;
    (void)aftertouch;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }

  inline uint8_t getPresetIndex() const { return 0; }

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
  /* Private Member Variables. */
  /*===========================================================================*/

  // std::atomic_uint_fast32_t flags_;

  // Functions from unit runtime
  unit_runtime_get_num_sample_banks_ptr m_get_num_sample_banks_ptr;
  unit_runtime_get_num_samples_for_bank_ptr m_get_num_samples_for_bank_ptr;
  unit_runtime_get_sample_ptr m_get_sample;

  // Parameters
  uint8_t m_sampleBank;
  uint8_t m_sampleNumber;
  uint8_t m_excitationLevelVelocitySensitivity;
  uint8_t m_excitationFilterVelocitySensitivity;
  uint16_t m_sampleStart;
  uint16_t m_sampleEnd;
  int16_t m_excitationLevel;
  Filter::Type m_excitationFilterType;
  int16_t m_excitationFilterCutoff;
  int16_t m_excitationFilterResonance;
  uint8_t m_note;
  int8_t m_resonatorPitchBendRange;
  int16_t m_resonatorDecayFeedback;
  uint16_t m_resonatorDecayDamping;
  uint8_t m_resonatorDispersion;
  uint16_t m_resonatorKeyTracking;
  int16_t m_resonatorReleaseFeedback;
  uint16_t m_resonatorReleaseDamping;

  // Common to all voices
  uint16_t m_pitchBend;

  // Voices
  Voice m_voice[c_numVoices];

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/
  size_t nextVoiceNumber();

  inline const sample_wrapper_t* GetSample(size_t bank, size_t number) const {
    if (bank >= m_get_num_sample_banks_ptr()) return nullptr;
    if (number >= m_get_num_samples_for_bank_ptr(bank)) return nullptr;
    return m_get_sample(bank, number);
  }

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
  static const char* const c_sampleBankName[7];
  static const char* const c_excitationFilterTypeName[6];
  static const float c_dispersionFilterRatios[c_dispersionStages];
};
