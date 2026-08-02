// Shared websim host bridge for microKORG2 effects (modfx / delfx / revfx).
//
// An fx project provides a 3-line wasm.cc:
//
//     #include "modfx.h"             // the unit's DSP class header
//     #define MK2_FX_CLASS ModFx     // the DSP class to instantiate
//     #include "mk2_fx_bridge.h"     // this file
//
// microKORG2 effects require stereo (2-in / 2-out) geometry and an sdram_alloc
// hook, so this bridge differs from the osc bridge: it feeds the fx.html input
// (interleaved stereo) through Process(in, out, frames) and provides a host
// SDRAM allocator. See WEBSIM_EXPANSION_PLAN.md §4.7.
//
// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html

#ifndef MK2_FX_CLASS
#error "define MK2_FX_CLASS (the microKORG2 effect DSP class) before including mk2_fx_bridge.h"
#endif

// Most microKORG2 fx classes expose Process(in, out, frames); a few (e.g.
// MorphEQ) name the render method Render. A unit's wasm.cc can override this
// before including the bridge: `#define MK2_FX_RENDER Render`.
#ifndef MK2_FX_RENDER
#define MK2_FX_RENDER Process
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

// this needs to be big enough for the stereo output, inputs, params and stack
uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> interleavedIn;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> interleavedOut;

MK2_FX_CLASS processor; // dsp processor instance
extern const unit_header_t unit_header;

static unit_runtime_desc_t s_desc;

// Host SDRAM allocator stand-ins. On hardware these dispense the unit's
// dedicated SDRAM region; in the browser plain malloc/free is fine.
static uint8_t *websim_sdram_alloc(size_t size) { return reinterpret_cast<uint8_t *>(std::malloc(size)); }
static void websim_sdram_free(const uint8_t *mem) { std::free(const_cast<uint8_t *>(mem)); }
static size_t websim_sdram_avail(void) { return 32u * 1024u * 1024u; }

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

// Shared one-time processor + descriptor setup, used by both the AudioWorklet
// path and the offline render harness (WEBSIM_RENDER).
static void websim_setup_processor()
{
  s_desc = {};
  s_desc.target = unit_header.target;
  s_desc.api = UNIT_API_VERSION;
  s_desc.samplerate = SAMPLE_RATE;
  s_desc.frames_per_buffer = WEB_AUDIO_FRAME_SIZE;
  s_desc.input_channels = 2;
  s_desc.output_channels = 2;
  s_desc.hooks.runtime_context = nullptr;
  s_desc.hooks.sdram_alloc = &websim_sdram_alloc;
  s_desc.hooks.sdram_free = &websim_sdram_free;
  s_desc.hooks.sdram_avail = &websim_sdram_avail;

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
}

bool ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                  int numOutputs, AudioSampleFrame *outputs,
                  int numParams, const AudioParamFrame *params,
                  void *userData)
{
  (void)numInputs;
  (void)numOutputs;
  auto &input = inputs[0];
  auto &output = outputs[0];

  // Web Audio channels are planar; build the interleaved stereo input the unit
  // expects. A mono input source is duplicated to both channels.
  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    interleavedIn[2 * i] = input.data[i];
    interleavedIn[2 * i + 1] = (input.numberOfChannels == 1) ? input.data[i] : input.data[i + WEB_AUDIO_FRAME_SIZE];
  }

  for (int i = 0; i < numParams; ++i)
    processor.setParameter(i, static_cast<int32_t>(params[i].data[0]));

  processor.MK2_FX_RENDER(interleavedIn.data(), interleavedOut.data(), WEB_AUDIO_FRAME_SIZE);

  // de-interleave back to planar stereo output
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

  // single (mono-or-stereo) input, single stereo output
  int outputChannelCounts[1] = {2};
  EmscriptenAudioWorkletNodeCreateOptions options = {
      .numberOfInputs = 1,
      .numberOfOutputs = 1,
      .outputChannelCounts = outputChannelCounts};

  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(audioContext,
                                                                                               "logue-fx", &options, &ProcessAudio, 0);

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
      .name = "logue-fx",
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

// Offline render harness: feed a test signal through the effect for N blocks and
// write a stereo float WAV. Built without AudioWorklet and run under node (see
// `make render`); reuses the exact same DSP + SIMD shim as the browser build.
// See WEBSIM.md §B.
//   argv: [out.wav] [sig=sine|impulse] [blocks=375 (~1s)] [freq=220]
int main(int argc, char **argv)
{
  const char *out = (argc > 1) ? argv[1] : "render.wav";
  const char *sig = (argc > 2) ? argv[2] : "sine";
  const int blocks = (argc > 3) ? std::atoi(argv[3]) : 375;
  const float freq = (argc > 4) ? (float)std::atof(argv[4]) : 220.f;

  websim_setup_processor();
  for (int i = 0; i < unit_header.num_params; ++i)
    processor.setParameter(i, unit_header.params[i].init);

  const bool impulse = (std::strcmp(sig, "impulse") == 0);
  const double kPi = 3.14159265358979323846;
  const double dphase = 2.0 * kPi * (double)freq / (double)SAMPLE_RATE;

  std::vector<float> samples;
  samples.reserve((size_t)blocks * WEB_AUDIO_FRAME_SIZE * 2);
  double phase = 0.0;
  long n = 0;
  for (int b = 0; b < blocks; ++b)
  {
    for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i, ++n)
    {
      float x;
      if (impulse)
        x = (n == 0) ? 1.f : 0.f;
      else
      {
        x = 0.25f * (float)std::sin(phase);
        phase += dphase;
      }
      interleavedIn[2 * i] = x;
      interleavedIn[2 * i + 1] = x;
    }
    processor.MK2_FX_RENDER(interleavedIn.data(), interleavedOut.data(), WEB_AUDIO_FRAME_SIZE);
    for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
    {
      samples.push_back(interleavedOut[2 * i]);
      samples.push_back(interleavedOut[2 * i + 1]);
    }
  }

  if (!websim::write_wav_f32(out, samples, 2, SAMPLE_RATE))
  {
    std::fprintf(stderr, "render: failed to write %s\n", out);
    return 1;
  }
  std::fprintf(stderr, "render: wrote %s (%zu frames, %s)\n", out,
               samples.size() / 2, sig);
  return 0;
}

#endif  // WEBSIM_RENDER
