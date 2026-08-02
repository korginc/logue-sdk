// Shared websim host bridge for gen-1 (legacy) logue-SDK oscillators
// (prologue / minilogue xd / NTS-1 mkI / nutekt-digital).
//
// Unlike the gen-2 class model, gen-1 oscillators are free C functions over a
// user_osc_param_t and emit q31 samples. A gen-1 osc project provides a thin
// wasm.cc:
//
//     #include "userosc.h"
//     #define WEBSIM_LEGACY_PARAM_LIST(X)                  \
//       X("Wave A",   0,  45, k_unit_param_type_none)      \
//       ... up to 6 custom params, matching manifest.json ...
//     #include "legacy_osc_bridge.h"
//
// The bridge synthesises slider metadata from that table (gen-1 has no
// unit_header), appends the two 10-bit SHAPE / SHIFT-SHAPE knobs, drives
// OSC_INIT / OSC_CYCLE / OSC_PARAM / OSC_NOTEON / OSC_NOTEOFF and converts the
// q31 output to float for Web Audio. See WEBSIM_EXPANSION_PLAN.md §6.
//
// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html

#ifndef WEBSIM_LEGACY_PARAM_LIST
#error "define WEBSIM_LEGACY_PARAM_LIST(X) (the unit's custom params) before including legacy_osc_bridge.h"
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
using namespace emscripten;

// gen-1 oscillator entry points. The OSC_INIT/OSC_CYCLE/... macros in userosc.h
// expand to `__attribute__((used)) _hook_*`, so they can only appear in a
// definition, not a declaration or call. The unit's UCXXSRC (e.g. waves.cpp)
// defines them; here we declare and call the underlying extern "C" symbols.
extern "C" {
void _hook_init(uint32_t platform, uint32_t api);
void _hook_cycle(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames);
void _hook_on(const user_osc_param_t *const params);
void _hook_off(const user_osc_param_t *const params);
void _hook_param(uint16_t index, uint16_t value);
}

uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;
constexpr float Q31_TO_F32 = 1.0f / 2147483648.0f;

std::array<int32_t, WEB_AUDIO_FRAME_SIZE> q31Out;
std::array<float, WEB_AUDIO_FRAME_SIZE> floatOut;

// gen-1 has no unit_header param-type enum; the project param table uses these.
enum { LP_NONE = 0, LP_PERCENT, LP_ONOFF };

static uint16_t s_pitch = (60u << 8); // (note << 8) | fine; default middle C
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

static std::vector<AudioWorkletParameter> buildParameters()
{
  std::vector<AudioWorkletParameter> r;
#define X(NAME, MN, MX, TYPE) r.push_back({(MN), (MX), 0, (MN), (uint8_t)(TYPE), std::string(NAME)});
  WEBSIM_LEGACY_PARAM_LIST(X)
#undef X
  // The two always-present 10-bit knobs (k_user_osc_param_shape / shiftshape).
  r.push_back({0, 1023, 0, 0, (uint8_t)LP_NONE, std::string("SHAPE")});
  r.push_back({0, 1023, 0, 0, (uint8_t)LP_NONE, std::string("SHIFT SHAPE")});
  return r;
}

std::vector<AudioWorkletParameter> getValidParameters() { return buildParameters(); }

std::string getParameterValueString(int index, int value)
{
  auto params = buildParameters();
  if (index < 0 || index >= (int)params.size())
    return std::to_string(value);
  switch (params[index].type)
  {
  case LP_PERCENT:
    return std::to_string(value) + "%";
  case LP_ONOFF:
    return value == 0 ? "OFF" : "ON";
  default:
    return std::to_string(value);
  }
}

// gen-1 pitch is (note << 8) | fine; convert a frequency to that form
// (A4 = 440 Hz = note 69), matching the device's osc_w0f_for_note() table.
void setOscPitch(float f0)
{
  if (f0 <= 0.f)
    return;
  float note = 69.f + 12.f * std::log2(f0 / 440.f);
  if (note < 0.f)
    note = 0.f;
  if (note > 151.f)
    note = 151.f;
  uint8_t whole = (uint8_t)note;
  uint8_t fine = (uint8_t)((note - whole) * 256.f);
  s_pitch = (uint16_t)((whole << 8) | fine);
}

void noteOn(uint8_t note, uint8_t velocity)
{
  (void)velocity;
  s_pitch = (uint16_t)(note << 8);
  user_osc_param_t p = {};
  p.pitch = s_pitch;
  _hook_on(&p);
}

void noteOff(uint8_t note)
{
  (void)note;
  user_osc_param_t p = {};
  p.pitch = s_pitch;
  _hook_off(&p);
}

// Shared one-time init, used by both the AudioWorklet path and the offline
// render harness (WEBSIM_RENDER).
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

  // Push k-rate slider values. Slider order == OSC_PARAM index order:
  // [id1..id6, shape, shiftshape] == k_user_osc_param_id1..shiftshape.
  for (int i = 0; i < numParams; ++i)
    _hook_param(static_cast<uint16_t>(i), static_cast<uint16_t>(params[i].data[0]));

  user_osc_param_t p = {};
  p.shape_lfo = 0;
  p.pitch = s_pitch;
  p.cutoff = 0;
  p.resonance = 0;

  _hook_cycle(&p, q31Out.data(), WEB_AUDIO_FRAME_SIZE);

  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
    output.data[i] = q31Out[i] * Q31_TO_F32;

  return true;
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

// Offline render harness: render N blocks of the gen-1 oscillator at a fixed
// note and write a mono float WAV. Built without AudioWorklet and run under node
// (see `make render`).
//   argv: [out.wav] [midiNote=69] [blocks=375 (~1s @48k/128)]
int main(int argc, char **argv)
{
  const char *out = (argc > 1) ? argv[1] : "render.wav";
  const int note = (argc > 2) ? std::atoi(argv[2]) : 69;
  const int blocks = (argc > 3) ? std::atoi(argv[3]) : 375;

  websim_setup_processor();
  s_pitch = (uint16_t)(note << 8);

  // Drive params to their init values (slider order == OSC_PARAM index order).
  auto params = buildParameters();
  for (int i = 0; i < (int)params.size(); ++i)
    _hook_param((uint16_t)i, (uint16_t)params[i].init);

  user_osc_param_t p = {};
  p.pitch = s_pitch;
  _hook_on(&p);

  std::vector<float> samples;
  samples.reserve((size_t)blocks * WEB_AUDIO_FRAME_SIZE);
  for (int b = 0; b < blocks; ++b)
  {
    p.pitch = s_pitch;
    _hook_cycle(&p, q31Out.data(), WEB_AUDIO_FRAME_SIZE);
    for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
      samples.push_back(q31Out[i] * Q31_TO_F32);
  }

  if (!websim::write_wav_f32(out, samples, 1, SAMPLE_RATE))
  {
    std::fprintf(stderr, "render: failed to write %s\n", out);
    return 1;
  }
  std::fprintf(stderr, "render: wrote %s (%zu samples, note %d)\n", out, samples.size(), note);
  return 0;
}

#endif  // WEBSIM_RENDER
