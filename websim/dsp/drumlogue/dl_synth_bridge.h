// Shared websim host bridge for drumlogue synth units (gen-2 class model,
// stereo output, NEON via SIMDe).
//
// A drumlogue synth project provides a 3-line wasm.cc:
//
//     #include "synth.h"             // the unit's DSP class header
//     #define DL_SYNTH_CLASS Synth   // the DSP class to instantiate
//     #include "dl_synth_bridge.h"   // this file
//
// drumlogue synths render interleaved stereo via Render(out, frames) and are
// driven by NoteOn/NoteOff + GateOn/GateOff. The bridge feeds the keyboard to
// both (covering pitched and gate-only drums), runs a 2-channel output worklet,
// and de-interleaves to planar stereo. Build with -msimd128 (SIMDe maps the
// unit's arm_neon.h intrinsics to wasm SIMD). See WEBSIM_EXPANSION_PLAN.md §5.
//
// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html

#ifndef DL_SYNTH_CLASS
#error "define DL_SYNTH_CLASS (the drumlogue synth DSP class) before including dl_synth_bridge.h"
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
#include <string>
#include <vector>
#include "../wav_writer.h"
#include "dl_sample_bank.h"
using namespace emscripten;

uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> interleavedOut;

DL_SYNTH_CLASS processor; // dsp processor instance
extern const unit_header_t unit_header;

static unit_runtime_desc_t s_desc;

static float BPM_WASM = 120.f;
void fx_set_bpm(float bpm) { BPM_WASM = bpm; }
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
    if (p.name[0] == '\0' && p.min == 0 && p.max == 0)
      continue;
    result.push_back({p.min, p.max, p.center, p.init, p.type, std::string(p.name)});
  }
  return result;
}

// Synth pitch comes from the note number, not a frequency; osc.html still calls
// setOscPitch on key-down, so accept and ignore it.
void setOscPitch(float f0) { (void)f0; }

void noteOn(uint8_t note, uint8_t velocity)
{
  processor.NoteOn(note, velocity);
  processor.GateOn(velocity);
}

void noteOff(uint8_t note)
{
  processor.NoteOff(note);
  processor.GateOff();
}

// Shared one-time processor setup, used by both the AudioWorklet path and the
// offline render harness (WEBSIM_RENDER).
static void websim_setup_processor()
{
  s_desc = {};
  s_desc.target = unit_header.target;
  s_desc.api = unit_header.api;
  s_desc.samplerate = SAMPLE_RATE;
  s_desc.frames_per_buffer = WEB_AUDIO_FRAME_SIZE;
  s_desc.input_channels = 0;
  s_desc.output_channels = 2;
  // Sample-bank accessors backed by a synthetic host bank so sample-playback
  // synths (e.g. SmplVox) initialise and make sound; synths that ignore them are
  // unaffected. See websim/dsp/drumlogue/dl_sample_bank.h.
  s_desc.get_num_sample_banks = &websim::websim_get_num_sample_banks;
  s_desc.get_num_samples_for_bank = &websim::websim_get_num_samples_for_bank;
  s_desc.get_sample = &websim::websim_get_sample;

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
    processor.setParameter(i, static_cast<int32_t>(params[i].data[0]));

  processor.Render(interleavedOut.data(), WEB_AUDIO_FRAME_SIZE);

  // de-interleave to planar stereo output
  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    output.data[i] = interleavedOut[2 * i];
    output.data[WEB_AUDIO_FRAME_SIZE + i] = interleavedOut[2 * i + 1];
  }
  return true;
}

void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
  if (!success)
    return;

  websim_setup_processor();

  int outputChannelCounts[1] = {2};
  EmscriptenAudioWorkletNodeCreateOptions options = {
      .numberOfInputs = 0,
      .numberOfOutputs = 1,
      .outputChannelCounts = outputChannelCounts};

  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext,
                                                                                               "logue-synth", &options, &ProcessAudio, 0);

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
      .name = "logue-synth",
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

// Offline render harness: trigger a note and render N blocks of stereo audio to
// a float WAV. Built without AudioWorklet and run under node (see `make render`).
//   argv: [out.wav] [midiNote=60] [blocks=375 (~1s @48k/128)]
int main(int argc, char **argv)
{
  const char *out = (argc > 1) ? argv[1] : "render.wav";
  const int note = (argc > 2) ? std::atoi(argv[2]) : 60;
  const int blocks = (argc > 3) ? std::atoi(argv[3]) : 375;

  websim_setup_processor();
  for (int i = 0; i < unit_header.num_params; ++i)
    processor.setParameter(i, unit_header.params[i].init);
  processor.NoteOn((uint8_t)note, 127);
  processor.GateOn(127);

  std::vector<float> samples;
  samples.reserve((size_t)blocks * WEB_AUDIO_FRAME_SIZE * 2);
  for (int b = 0; b < blocks; ++b)
  {
    processor.Render(interleavedOut.data(), WEB_AUDIO_FRAME_SIZE);
    for (int i = 0; i < WEB_AUDIO_FRAME_SIZE * 2; ++i)
      samples.push_back(interleavedOut[i]);
  }

  if (!websim::write_wav_f32(out, samples, 2, SAMPLE_RATE))
  {
    std::fprintf(stderr, "render: failed to write %s\n", out);
    return 1;
  }
  std::fprintf(stderr, "render: wrote %s (%zu frames)\n", out, samples.size() / 2);
  return 0;
}

#endif  // WEBSIM_RENDER
