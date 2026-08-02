// Shared websim host bridge for microKORG2 oscillators (gen-2 / logue SDK 2.0).
//
// A microKORG2 osc project provides a 3-line wasm.cc:
//
//     #include "waves.h"            // the unit's DSP class header
//     #define MK2_OSC_CLASS Waves   // the DSP class to instantiate
//     #include "mk2_osc_bridge.h"   // this file
//
// It binds directly to the DSP class' Init(desc)/Process(out,frames)/
// setParameter/getParameterValue methods and emulates the runtime osc context
// the unit reads its pitch from. v1 is single-voice (voiceLimit = 1, pitch[0]
// from the keyboard, outputStride = 1) — see WEBSIM_EXPANSION_PLAN.md.
//
// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html

#ifndef MK2_OSC_CLASS
#error "define MK2_OSC_CLASS (the microKORG2 oscillator DSP class) before including mk2_osc_bridge.h"
#endif

#include <emscripten/bind.h>
#include <emscripten/em_math.h>
#ifndef WEBSIM_RENDER
#include <emscripten/webaudio.h>
#endif
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../wav_writer.h"
using namespace emscripten;

// this needs to be big enough for the output, params and the worker stack
uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;

// Number of voices to render. v1 default is single-voice; a unit opts into small
// polyphony by defining MK2_OSC_VOICES before including this bridge — vox uses 4.
// Poly drives one microKORG2 "x4" voice group: per-voice pitch from a round-robin
// allocator, output interleaved [v0..v3] per sample (outputStride = 4), downmixed
// to mono over the currently-held voices. 4 is the natural value (a single x4
// group with GetBufferOffset == bufferOffset); see WEBSIM_FOLLOWUP_PLAN.md §D.
#ifndef MK2_OSC_VOICES
#define MK2_OSC_VOICES 1
#endif
#if MK2_OSC_VOICES != 1 && MK2_OSC_VOICES != 4
#error "MK2_OSC_VOICES must be 1 (mono) or 4 (one x4 voice group)"
#endif

// The microKORG2 oscillator processes blocks of at most kMk2BufferSize samples
// per voice — its internal mOscBuffer is sized kMk2HalfVoices*kMk2BufferSize. The
// Web Audio quantum is 128, larger than kMk2BufferSize (64), so the x4 (poly)
// path would overrun mOscBuffer (4*128 > 4*64) and corrupt adjacent state. Render
// in <= kMk2BufferSize-sample sub-blocks for poly; mono fits 128 in one call so
// keep it unchanged. See WEBSIM_FOLLOWUP_PLAN.md §D.
#if MK2_OSC_VOICES > 1
constexpr int MK2_OSC_BLOCK = kMk2BufferSize;
#else
constexpr int MK2_OSC_BLOCK = WEB_AUDIO_FRAME_SIZE;
#endif
std::array<float, MK2_OSC_BLOCK * MK2_OSC_VOICES> interleavedOut;
std::array<float, MK2_OSC_BLOCK> monoOut;  // downmixed sub-block output

MK2_OSC_CLASS processor; // dsp processor instance
extern const unit_header_t unit_header;

// Runtime context emulation. The microKORG2 oscillator reads its pitch (and, on
// hardware, mod data) from this structure each block. We keep it global so the
// copy the unit caches in Init() stays valid for the unit's lifetime.
static unit_runtime_osc_context_t s_osc_ctx;
static unit_runtime_desc_t s_desc;
static float s_mod_plus[kMk2MaxVoices];
static float s_mod_plus_minus[kMk2MaxVoices];

static float BPM_WASM = 120.f;

void fx_set_bpm(float bpm)
{
  BPM_WASM = bpm; // microKORG2 oscillators don't consume tempo; kept for UI parity
}

uint16_t fx_get_bpm(void) { return static_cast<uint16_t>(BPM_WASM * 10.f); }
float fx_get_bpmf(void) { return BPM_WASM; }

struct AudioWorkletParameter
{
  int min;
  int max;
  int center;
  int init;
  uint8_t type;
  std::string name;
};

std::string getParameterValueString(int index, int value)
{
  const unit_param_t &p = unit_header.params[index];

  std::string suffix;

  switch (p.type)
  {
  case k_unit_param_type_none:
    break;
  case k_unit_param_type_percent:
    suffix = "%";
    break;
  case k_unit_param_type_db:
    suffix = " dB";
    break;
  case k_unit_param_type_cents:
    suffix = " cents";
    break;
  case k_unit_param_type_semi:
    suffix = " semitones";
    break;
  case k_unit_param_type_oct:
    suffix = " octaves";
    break;
  case k_unit_param_type_hertz:
    suffix = " Hz";
    break;
  case k_unit_param_type_khertz:
    suffix = " kHz";
    break;
  case k_unit_param_type_bpm:
    suffix = " bpm";
    break;
  case k_unit_param_type_msec:
    suffix = " ms";
    break;
  case k_unit_param_type_sec:
    suffix = " s";
    break;
  case k_unit_param_type_enum:
    break;
  case k_unit_param_type_strings:
  {
    const char *s = processor.getParameterStrValue(index, value);
    return s ? std::string(s) : std::to_string(value);
  }
  case k_unit_param_type_drywet:
    suffix = "%";
    break;
  case k_unit_param_type_pan:
  case k_unit_param_type_spread:
    if (value < 0)
      suffix = "L";
    else if (value > 0)
      suffix = "R";
    else if (value == p.center)
      return "CNTR";
    break;
  case k_unit_param_type_onoff:
    return value == 0 ? "OFF" : "ON";
  case k_unit_param_type_midi_note:
  default:
    return "unimplemented";
  };

  std::string numerical;
  if (p.frac_mode == k_unit_param_frac_mode_fixed)
    numerical = std::to_string(value / static_cast<double>(1 << p.frac));
  else
    numerical = std::to_string(value / std::pow(10.0, p.frac));
  numerical.erase(numerical.find_last_not_of('0') + 1);
  if (!numerical.empty() && numerical.back() == '.')
    numerical.pop_back();

  return numerical + suffix;
}

std::vector<AudioWorkletParameter> getValidParameters()
{
  std::vector<AudioWorkletParameter> result;
  for (int i = 0; i < unit_header.num_params; ++i)
  {
    const unit_param_t &p = unit_header.params[i];
    // Skip trailing empty placeholder params (name == "", zero range).
    if (p.name[0] == '\0' && p.min == 0 && p.max == 0)
      continue;
    result.push_back({p.min, p.max, p.center, p.init, p.type, std::string(p.name)});
  }
  return result;
}

#if MK2_OSC_VOICES > 1
// --- small polyphony: round-robin voice allocator -------------------------
static struct VoiceSlot { bool active; uint8_t note; uint32_t age; } s_voices[MK2_OSC_VOICES];
static uint32_t s_voice_age = 0;

static int websim_alloc_voice()
{
  int oldest = 0;
  uint32_t oldest_age = 0xFFFFFFFFu;
  for (int v = 0; v < MK2_OSC_VOICES; ++v)
  {
    if (!s_voices[v].active)
      return v;  // prefer a free voice
    if (s_voices[v].age < oldest_age) { oldest_age = s_voices[v].age; oldest = v; }
  }
  return oldest;  // else steal the oldest
}

void noteOn(uint8_t note, uint8_t velocity)
{
  const int v = websim_alloc_voice();
  s_voices[v] = { true, note, ++s_voice_age };
  s_osc_ctx.pitch[v] = static_cast<float>(note);
  processor.voiceEvent(k_voice_event_allocation, v, note, velocity);
}

void noteOff(uint8_t note)
{
  for (int v = 0; v < MK2_OSC_VOICES; ++v)
    if (s_voices[v].active && s_voices[v].note == note)
    {
      s_voices[v].active = false;
      processor.voiceEvent(k_voice_event_release, v, note, 0);
    }
}

// In poly mode pitch comes from the (exact, integer) note number; the continuous
// frequency from osc.html's keyboard is redundant, so ignore it.
void setOscPitch(float f0) { (void)f0; }

#else
// --- single-voice (default) ------------------------------------------------
// Set oscillator pitch from a frequency by converting to the fractional MIDI
// note number the microKORG2 osc context expects (A4 = 440 Hz = note 69).
void setOscPitch(float f0)
{
  if (f0 <= 0.f)
    return;
  float note = 69.f + 12.f * std::log2(f0 / 440.f);
  if (note < 0.f)
    note = 0.f;
  s_osc_ctx.pitch[0] = note;
}

// microKORG2 oscillators are gated by the runtime, not the unit; pitch alone
// drives the continuously-running osc (same behaviour as the NTS-1 mkII sim).
void noteOn(uint8_t note, uint8_t velocity)
{
  (void)velocity;
  s_osc_ctx.pitch[0] = static_cast<float>(note);
}

void noteOff(uint8_t note) { (void)note; }
#endif  // MK2_OSC_VOICES > 1

// Downmix one rendered sub-block (n samples) of the voice buffer to mono. In poly
// mode, average the currently held voices (consistent level, no clipping); in mono
// mode it's a straight copy.
static inline void websim_mix_to_mono(int n)
{
#if MK2_OSC_VOICES > 1
  int active = 0;
  for (int v = 0; v < MK2_OSC_VOICES; ++v) active += s_voices[v].active ? 1 : 0;
  const float g = active ? 1.0f / static_cast<float>(active) : 0.f;
  for (int i = 0; i < n; ++i)
  {
    float acc = 0.f;
    for (int v = 0; v < MK2_OSC_VOICES; ++v)
      if (s_voices[v].active) acc += interleavedOut[i * MK2_OSC_VOICES + v];
    monoOut[i] = acc * g;
  }
#else
  for (int i = 0; i < n; ++i)
    monoOut[i] = interleavedOut[i];
#endif
}

// Shared one-time processor + runtime-context setup, used by both the
// AudioWorklet path and the offline render harness (WEBSIM_RENDER).
static void websim_setup_processor()
{
  s_osc_ctx = {};
  for (int v = 0; v < kMk2MaxVoices; ++v) s_osc_ctx.pitch[v] = 60.f; // middle C until played
#if MK2_OSC_VOICES > 1
  for (int v = 0; v < MK2_OSC_VOICES; ++v) s_voices[v] = { false, 0, 0 };
  s_voice_age = 0;
  s_osc_ctx.voiceLimit = MK2_OSC_VOICES;
  s_osc_ctx.outputStride = 4;  // the x4 group writes 4 interleaved lanes per sample
#else
  s_osc_ctx.voiceLimit = 1;
  s_osc_ctx.outputStride = 1;
#endif
  s_osc_ctx.voiceOffset = 0;
  s_osc_ctx.bufferOffset = 0;
  s_osc_ctx.modDataSize = kMk2MaxVoices;
  s_osc_ctx.unitModDataPlus = s_mod_plus;
  s_osc_ctx.unitModDataPlusMinus = s_mod_plus_minus;

  s_desc = {};
  s_desc.target = unit_header.target;
  s_desc.api = UNIT_API_VERSION;
  s_desc.samplerate = SAMPLE_RATE;
  s_desc.frames_per_buffer = WEB_AUDIO_FRAME_SIZE;
  s_desc.input_channels = 0;
  s_desc.output_channels = 1;
  s_desc.hooks.runtime_context = &s_osc_ctx;

  processor.Init(&s_desc);
  processor.Reset();
}

#ifndef WEBSIM_RENDER
EMSCRIPTEN_BINDINGS(my_module)
{
  value_object<AudioWorkletParameter>("AudioWorkletParameter")
      .field("min", &AudioWorkletParameter::min)
      .field("max", &AudioWorkletParameter::max)
      .field("center", &AudioWorkletParameter::center)
      .field("init", &AudioWorkletParameter::init)
      .field("type", &AudioWorkletParameter::type)
      .field("name", &AudioWorkletParameter::name);

  register_vector<AudioWorkletParameter>("ParameterList");

  function("getValidParameters", &getValidParameters);
  function("getParameterValueString", &getParameterValueString);
  function("fx_set_bpm", &fx_set_bpm);
  function("setOscPitch", &setOscPitch);
  function("noteOn", &noteOn);
  function("noteOff", &noteOff);
}

bool ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                  int numOutputs, AudioSampleFrame *outputs,
                  int numParams, const AudioParamFrame *params,
                  void *userData)
{
  (void)numInputs;
  (void)inputs;
  (void)numOutputs;
  auto &output = outputs[0];

  for (int i = 0; i < numParams; ++i)
  {
    // K-rate parameter: use the first sample for the frame.
    processor.setParameter(i, static_cast<int32_t>(params[i].data[0]));
  }

  for (int off = 0; off < WEB_AUDIO_FRAME_SIZE; off += MK2_OSC_BLOCK)
  {
    const int n = (WEB_AUDIO_FRAME_SIZE - off < MK2_OSC_BLOCK) ? (WEB_AUDIO_FRAME_SIZE - off) : MK2_OSC_BLOCK;
#ifdef MK2_OSC_PROCESS_HAS_IN
    // Some templates (e.g. dummy-osc) keep the (in, out, frames) signature.
    processor.Process(nullptr, interleavedOut.data(), n);
#else
    processor.Process(interleavedOut.data(), n);
#endif
    websim_mix_to_mono(n);
    for (int i = 0; i < n; ++i)
      output.data[off + i] = monoOut[i];
  }

  return true; // Keep the graph output going
}

void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
  if (!success)
    return;

  websim_setup_processor();

  int outputChannelCounts[1] = {1};
  EmscriptenAudioWorkletNodeCreateOptions options = {
      .numberOfInputs = 0,
      .numberOfOutputs = 1,
      .outputChannelCounts = outputChannelCounts};

  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext,
                                                                                               "logue-osc", &options, &ProcessAudio, 0);

  EM_ASM({ setupWebAudioAndUI(emscriptenGetAudioObject($0), emscriptenGetAudioObject($1)); }, audioContext, wasmAudioWorklet);
}

void AudioThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
  if (!success)
    return;

  auto valid_parameters = getValidParameters();

  WebAudioParamDescriptor params[valid_parameters.size()];
  for (int i = 0; i < (int)valid_parameters.size(); ++i)
  {
    params[i].automationRate = WEBAUDIO_PARAM_K_RATE;
    params[i].defaultValue = valid_parameters[i].init;
    params[i].minValue = valid_parameters[i].min;
    params[i].maxValue = valid_parameters[i].max;
  }

  WebAudioWorkletProcessorCreateOptions opts = {
      .name = "logue-osc",
      .numAudioParams = static_cast<int>(valid_parameters.size()),
      .audioParamDescriptors = params};

  emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &AudioWorkletProcessorCreated, 0);
}

int main()
{
  EmscriptenWebAudioCreateAttributes attrs = {
      .latencyHint = "interactive",
      .sampleRate = SAMPLE_RATE};

  EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(&attrs);

  printf("Sample rate: %d\n", emscripten_audio_context_sample_rate(context));
  printf("Frame size: %d\n", emscripten_audio_context_quantum_size(context));

  emscripten_start_wasm_audio_worklet_thread_async(context, audioThreadStack, sizeof(audioThreadStack),
                                                   &AudioThreadInitialized, 0);

  emscripten_exit_with_live_runtime();
}

#else  // WEBSIM_RENDER

// Offline render harness: render N blocks of the oscillator and write a mono
// float WAV. Built without AudioWorklet and run under node (see `make render`);
// reuses the exact same DSP + SIMD shim as the browser build, so it validates
// audio correctness headlessly. See WEBSIM.md §B.
//   argv: [out.wav] [note(s)=60] [blocks=375 (~1s @48k/128)]
//   In poly builds (MK2_OSC_VOICES>1) the note arg may be a comma-separated
//   chord, e.g. "57,60,64" — one voice per note. See WEBSIM_FOLLOWUP_PLAN.md §D.
int main(int argc, char **argv)
{
  const char *out = (argc > 1) ? argv[1] : "render.wav";
  const char *notes = (argc > 2) ? argv[2] : "60";
  const int blocks = (argc > 3) ? std::atoi(argv[3]) : 375;

  websim_setup_processor();

#if MK2_OSC_VOICES > 1
  // Trigger one voice per comma-separated note (chord).
  char buf[128];
  std::strncpy(buf, notes, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  for (char *tok = std::strtok(buf, ","); tok; tok = std::strtok(nullptr, ","))
    noteOn(static_cast<uint8_t>(std::atoi(tok)), 100);
#else
  s_osc_ctx.pitch[0] = (float)std::atof(notes);
#endif

  // Drive params to their init values (k-rate, as the worklet would).
  for (int i = 0; i < unit_header.num_params; ++i)
    processor.setParameter(i, unit_header.params[i].init);

  std::vector<float> samples;
  samples.reserve((size_t)blocks * WEB_AUDIO_FRAME_SIZE);
  for (int b = 0; b < blocks; ++b)
    for (int off = 0; off < WEB_AUDIO_FRAME_SIZE; off += MK2_OSC_BLOCK)
    {
      const int n = (WEB_AUDIO_FRAME_SIZE - off < MK2_OSC_BLOCK) ? (WEB_AUDIO_FRAME_SIZE - off) : MK2_OSC_BLOCK;
#ifdef MK2_OSC_PROCESS_HAS_IN
      processor.Process(nullptr, interleavedOut.data(), n);
#else
      processor.Process(interleavedOut.data(), n);
#endif
      websim_mix_to_mono(n);
      for (int i = 0; i < n; ++i)
        samples.push_back(monoOut[i]);
    }

  if (!websim::write_wav_f32(out, samples, 1, SAMPLE_RATE))
  {
    std::fprintf(stderr, "render: failed to write %s\n", out);
    return 1;
  }
  std::fprintf(stderr, "render: wrote %s (%zu samples, notes %s)\n", out, samples.size(), notes);
  return 0;
}

#endif  // WEBSIM_RENDER
