// Shared websim host bridge for gen-1 (legacy) logue-SDK effects
// (prologue / minilogue xd / NTS-1 mkI / nutekt-digital): modfx / delfx / revfx.
//
// Like the gen-1 osc, these are free C functions, but they process float audio
// (interleaved stereo, "2 samples per frame"). modfx has a separate in/out and a
// sub timbre; delfx/revfx process a single buffer in place. A gen-1 fx project
// provides a thin wasm.cc selecting the type:
//
//     #include "usermodfx.h"           // or userdelfx.h / userrevfx.h
//     #define WEBSIM_LEGACY_FX_MODFX   // or _DELFX / _REVFX
//     #include "legacy_fx_bridge.h"
//
// The bridge exposes the two always-present 10-bit fx knobs (TIME, DEPTH) as
// sliders, drives OSC_*-style _hook_init/_hook_param, feeds the fx.html input
// through _hook_process, and de-interleaves to planar stereo. See
// WEBSIM_FOLLOWUP_PLAN.md §C.1.
//
// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html

#if !defined(WEBSIM_LEGACY_FX_MODFX) && !defined(WEBSIM_LEGACY_FX_DELFX) && !defined(WEBSIM_LEGACY_FX_REVFX)
#error "define WEBSIM_LEGACY_FX_MODFX | WEBSIM_LEGACY_FX_DELFX | WEBSIM_LEGACY_FX_REVFX before including legacy_fx_bridge.h"
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

// gen-1 fx entry points (the *FX_INIT/PROCESS/PARAM macros expand to
// `__attribute__((used)) _hook_*`, so the unit's source defines them; here we
// declare and call the underlying extern "C" symbols).
extern "C" {
void _hook_init(uint32_t platform, uint32_t api);
void _hook_param(uint8_t index, int32_t value);
#if defined(WEBSIM_LEGACY_FX_MODFX)
void _hook_process(const float *main_xn, float *main_yn,
                   const float *sub_xn, float *sub_yn, uint32_t frames);
#else
void _hook_process(float *xn, uint32_t frames);
#endif
}

uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;

// Interleaved stereo work buffers (2 samples per frame).
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> bufIn;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> bufOut;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> bufSub;  // modfx sub timbre (silent)

// The two always-present 10-bit fx knobs (TIME == param 0, DEPTH == param 1).
enum { FX_PARAM_TIME = 0, FX_PARAM_DEPTH = 1 };

// gen-1 fx query tempo via fx_get_bpm()/fx_get_bpmf() (inline wrappers in
// fx_api.h around the firmware externs _fx_get_bpm/_fx_get_bpmf). Provide those
// ROM stand-ins here; don't redefine the public inlines (fx_api.h owns them).
static float BPM_WASM = 120.f;
void fx_set_bpm(float bpm) { BPM_WASM = bpm; }
extern "C" uint16_t _fx_get_bpm(void) { return static_cast<uint16_t>(BPM_WASM * 10.f); }
extern "C" float _fx_get_bpmf(void) { return BPM_WASM; }

struct AudioWorkletParameter
{
  int min;
  int max;
  int center;
  int init;
  uint8_t type;
  std::string name;
};

static std::vector<AudioWorkletParameter> buildParameters()
{
  std::vector<AudioWorkletParameter> r;
  r.push_back({0, 1023, 0, 512, 0, std::string("Time")});
  r.push_back({0, 1023, 0, 512, 0, std::string("Depth")});
  return r;
}

std::vector<AudioWorkletParameter> getValidParameters() { return buildParameters(); }

std::string getParameterValueString(int index, int value)
{
  (void)index;
  return std::to_string(value);
}

// Run one block: build interleaved input from a planar (mono/stereo) source
// already laid into bufIn's left/right, then call the type-specific hook.
static inline void websim_fx_block()
{
#if defined(WEBSIM_LEGACY_FX_MODFX)
  std::memset(bufSub.data(), 0, bufSub.size() * sizeof(float));
  _hook_process(bufIn.data(), bufOut.data(), bufSub.data(), bufSub.data(), WEB_AUDIO_FRAME_SIZE);
#else
  std::memcpy(bufOut.data(), bufIn.data(), bufIn.size() * sizeof(float));
  _hook_process(bufOut.data(), WEB_AUDIO_FRAME_SIZE);  // in place
#endif
}

static void websim_setup_processor() { _hook_init(0, 0); }

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

  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    const float l = input.data[i];
    const float r = (input.numberOfChannels == 1) ? l : input.data[i + WEB_AUDIO_FRAME_SIZE];
    bufIn[2 * i] = l;
    bufIn[2 * i + 1] = r;
  }

  for (int i = 0; i < numParams; ++i)
    _hook_param(static_cast<uint8_t>(i), static_cast<int32_t>(params[i].data[0]));

  websim_fx_block();

  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    output.data[i] = bufOut[2 * i];
    output.data[WEB_AUDIO_FRAME_SIZE + i] = bufOut[2 * i + 1];
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

// Offline render harness: feed a test signal through the gen-1 fx for N blocks
// and write a stereo float WAV. Built without AudioWorklet, run under node.
//   argv: [out.wav] [sig=sine|impulse] [blocks=375] [freq=220]
int main(int argc, char **argv)
{
  const char *out = (argc > 1) ? argv[1] : "render.wav";
  const char *sig = (argc > 2) ? argv[2] : "sine";
  const int blocks = (argc > 3) ? std::atoi(argv[3]) : 375;
  const float freq = (argc > 4) ? (float)std::atof(argv[4]) : 220.f;

  websim_setup_processor();
  auto params = buildParameters();
  for (int i = 0; i < (int)params.size(); ++i)
    _hook_param((uint8_t)i, params[i].init);

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
      bufIn[2 * i] = x;
      bufIn[2 * i + 1] = x;
    }
    websim_fx_block();
    for (int i = 0; i < WEB_AUDIO_FRAME_SIZE * 2; ++i)
      samples.push_back(bufOut[i]);
  }

  if (!websim::write_wav_f32(out, samples, 2, SAMPLE_RATE))
  {
    std::fprintf(stderr, "render: failed to write %s\n", out);
    return 1;
  }
  std::fprintf(stderr, "render: wrote %s (%zu frames, %s)\n", out, samples.size() / 2, sig);
  return 0;
}

#endif  // WEBSIM_RENDER
