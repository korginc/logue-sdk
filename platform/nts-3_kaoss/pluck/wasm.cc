// reference: https://emscripten.org/docs/api_reference/wasm_audio_worklets.html#wasm-audio-worklets
// example code: https://github.com/emscripten-core/emscripten/tree/main/test/webaudio

#include <emscripten/bind.h>
#include <emscripten/webaudio.h>
#include <emscripten/em_math.h>
using namespace emscripten;

#include "effect.h"

// this needs to be big enough for the stereo output, inputs, params and the worker stack
uint8_t audioThreadStack[4096];

constexpr int SAMPLE_RATE = 48000;
constexpr int WEB_AUDIO_FRAME_SIZE = 128;
std::vector<float> ram;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> interleavedIn;
std::array<float, WEB_AUDIO_FRAME_SIZE * 2> interleavedOut;

Effect processor; // dsp processor instance
extern const genericfx_unit_header_t unit_header;

static float BPM_WASM = 120.f;

void fx_set_bpm(float bpm)
{
  BPM_WASM = bpm;
  processor.setTempo(bpm);
}

uint16_t fx_get_bpm(void)
{
  return static_cast<int>(BPM_WASM * 10.f);
}

float fx_get_bpmf(void)
{
  return BPM_WASM;
}

struct AudioWorkletParameter
{
  int min;
  int max;
  // int center;
  int init;
  uint8_t type;
  std::string name;
};

enum class ParamAssign
{
  None,
  X,
  Y,
  Depth
};

enum class Curve
{
  Linear,
  Exp,
  Log,
  Toggle,
  MinClip,
  MaxClip
};

struct GenericFXParamMapping
{
  ParamAssign assign;
  Curve curve;
  bool unipolar; // bipolar if false
  int min;
  int max;
  int init;
};

float applyCurveToParameter0to1(float parameter, Curve curve, bool unipolar)
{
  int value_10bit = std::roundf(parameter * 1023.f);
  float result = parameter;

  switch (curve)
  {
  case Curve::Linear:
    if (unipolar)
    {
      result = parameter;
    }
    else
    { // linear bipolar
      float pol;
      if (value_10bit <= 500)
        pol = (value_10bit / 500.f) * 512.f;
      else if (value_10bit >= 523)
        pol = 512.f + ((value_10bit - 523.f) / 500.f) * 511.f;
      else
        pol = 512.f;
      result = (pol / 1023.f);
    }
    break;

  case Curve::Exp:
  {
    const float logmin_10bit = -1.0986122886681098f; // 1*numpy.log(1/3)
    const float logmax_10bit = 1.097743856134156f;   // 1*numpy.log((1023/128+1)/3)
    float d, logval, exp;
    if (unipolar)
    {
      d = (logmax_10bit - logmin_10bit) / 1023.f;
      logval = logmin_10bit + (d * value_10bit);
      exp = 3.f * expf(logval / 1.f) - 1.f;
      exp = exp * 128.f;
    }
    else
    { // exp bipolar
      if (value_10bit > 512)
      {
        d = (logmax_10bit - logmin_10bit) / 510.f;
        logval = logmin_10bit + (d * (value_10bit - 513.f));
        exp = 3.f * expf(logval / 1.f) - 1.f;
        exp = exp * 128.f;
        exp = 512.f + ((1023.f - 512.f) * (exp / 1023.f));
      }
      else
      {
        d = (logmin_10bit - logmax_10bit) / 512.f;
        logval = logmax_10bit + (d * value_10bit);
        exp = 3.f * expf(logval / 1.f) - 1.f;
        exp = exp * 128.f;
        exp = 512.f - (512.f * (exp / 1023.f));
      }
    }
    result = (exp / 1023.f);
  }
  break;

  case Curve::Log:
  {
    const float logmin_10bit = -1.0986122886681098f; // 1*np.log(1/3)
    const float logmax_10bit = 1.097743856134156f;   // 1*numpy.log((1023/128+1)/3)
    float d, logval;
    if (unipolar)
    {
      d = (logmax_10bit - logmin_10bit) / 1023.f;
      logval = logmax_10bit - (d * value_10bit);
      logval = 7.9921875f - (3.f * expf(logval / 1.f) - 1.f);
      logval = logval * 128.f;
    }
    else
    { // log bipolar
      if (value_10bit > 512)
      {
        d = (logmax_10bit - logmin_10bit) / 512.f;
        logval = logmax_10bit - (d * (value_10bit - 512.f));
        logval = 7.9921875f - (3.f * expf(logval / 1.f) - 1.f);
        logval = 4.f + ((7.9921875f - 4.f) * (logval / 7.9921875f));
        logval = logval * 128.f;
      }
      else
      {
        d = (logmax_10bit - logmin_10bit) / 512.f;
        logval = logmin_10bit + (d * value_10bit);
        logval = 0.f - (3.f * expf(logval / 1.f) - 1.f);
        logval = -(4.f * (logval / 7.9921875f));
        logval = logval * 128.f;
      }
    }
    result = (logval / 1023.f);
  }
  break;

  case Curve::Toggle:
    if (unipolar)
    {
      result = (value_10bit > 512) ? 1.f : 0.f;
    }
    else
    {
      result = (value_10bit > 768) ? 1.f : (value_10bit > 256) ? 0.5f
                                                               : 0.f;
    }
    break;

  case Curve::MinClip:
    if (unipolar)
    {
      result = (value_10bit > 512) ? (2.f * (value_10bit - 512)) / 1023.f : 0.f;
    }
    else
    {
      result = (value_10bit > 768) ? 0.5f + (value_10bit - 768) / 510.f : (value_10bit > 256) ? 0.5f
                                                                                              : value_10bit / 512.f;
    }
    break;

  case Curve::MaxClip:
    if (unipolar)
    {
      result = (value_10bit < 512) ? value_10bit / 512.f : 1.f;
    }
    else
    {
      result = (value_10bit > 768) ? 1.f : (value_10bit > 256) ? (value_10bit - 256) / 512.f
                                                               : 0.f;
    }
    break;

  default:
    break;
  }

  return result;
}

std::vector<GenericFXParamMapping> getDefaultMapping()
{
  std::vector<GenericFXParamMapping> result;
  for (int i = 0; i < unit_header.common.num_params; ++i)
  {
    const auto &m = unit_header.default_mappings[i];
    result.push_back({static_cast<ParamAssign>(m.assign),
                      static_cast<Curve>(m.curve),
                      m.curve_polarity == k_genericfx_curve_unipolar,
                      m.min, m.max, m.value});
  }
  return result;
}

std::string getParameterValueString(int index, int value)
{
  const unit_param_t &p = unit_header.common.params[index];

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
    return processor.getParameterStrValue(index, value);
    break;
  case k_unit_param_type_drywet:
    suffix = "%";
    break;
  case k_unit_param_type_pan:
  case k_unit_param_type_spread:

    if (value < 0)
    {
      suffix = "L";
    }
    else if (value > 0)
    {
      suffix = "R";
    }
    else if (value == p.center)
    {
      return "CNTR";
    }
    break;

  case k_unit_param_type_onoff:
    if (value == 0)
    {
      return "OFF";
    }
    else
    {
      return "ON";
    }
    break;
  case k_unit_param_type_midi_note:
    // todo
  default:
    return "unimplemented";
    break;
  };

  std::string numerical;
  if (p.frac_mode == k_unit_param_frac_mode_fixed)
  {
    numerical = std::to_string(value / static_cast<double>(1 << p.frac));
  }
  else
  {
    numerical = std::to_string(value / std::pow(10.0, p.frac));
  }
  numerical.erase(numerical.find_last_not_of('0') + 1);
  if (!numerical.empty() && numerical.back() == '.')
  {
    numerical.pop_back();
  }

  return numerical + suffix;
}

std::vector<AudioWorkletParameter> getValidParameters()
{
  std::vector<AudioWorkletParameter> result;
  for (int i = 0; i < unit_header.common.num_params; ++i)
  {
    const unit_param_t &p = unit_header.common.params[i];
    result.push_back({p.min,
                      p.max,
                      // p.center,
                      p.init,
                      p.type,
                      std::string(p.name)});
  }
  return result;
}

enum class TouchEvent
{
  Began,
  Moved,
  Ended,
}; // stationary and canceled are ambiguous so ignore them

void touchEvent(TouchEvent phase, float x, float y)
{
  processor.touchEvent(0, static_cast<uint8_t>(phase), static_cast<uint32_t>(x * 1023), static_cast<uint32_t>((1.f - y) * 1023));
}

// bind unit parameters
EMSCRIPTEN_BINDINGS(my_module)
{
  value_object<AudioWorkletParameter>("AudioWorkletParameter")
      .field("min", &AudioWorkletParameter::min)
      .field("max", &AudioWorkletParameter::max)
      // .field("center", &AudioWorkletParameter::center)
      .field("init", &AudioWorkletParameter::init)
      .field("type", &AudioWorkletParameter::type)
      .field("name", &AudioWorkletParameter::name);

  register_vector<AudioWorkletParameter>("ParameterList");

  function("getValidParameters", &getValidParameters);

  function("getParameterValueString", &getParameterValueString);

  function("fx_set_bpm", &fx_set_bpm);

  // parameter mappings
  enum_<ParamAssign>("ParamAssign")
      .value("None", ParamAssign::None)
      .value("X", ParamAssign::X)
      .value("Y", ParamAssign::Y)
      .value("Depth", ParamAssign::Depth);

  enum_<Curve>("Curve")
      .value("Linear", Curve::Linear)
      .value("Exp", Curve::Exp)
      .value("Log", Curve::Log)
      .value("Toggle", Curve::Toggle)
      .value("MinClip", Curve::MinClip)
      .value("MaxClip", Curve::MaxClip);

  // Bind struct
  value_object<GenericFXParamMapping>("GenericFXParamMapping")
      .field("assign", &GenericFXParamMapping::assign)
      .field("curve", &GenericFXParamMapping::curve)
      .field("unipolar", &GenericFXParamMapping::unipolar)
      .field("min", &GenericFXParamMapping::min)
      .field("max", &GenericFXParamMapping::max)
      .field("init", &GenericFXParamMapping::init);

  register_vector<GenericFXParamMapping>("GenericFXParamMappingList");
  function("getDefaultMapping", &getDefaultMapping);
  function("applyCurveToParameter0to1", &applyCurveToParameter0to1);

  function("touchEvent", &touchEvent);

  enum_<TouchEvent>("TouchEvent")
      .value("Began", TouchEvent::Began)
      .value("Moved", TouchEvent::Moved)
      .value("Ended", TouchEvent::Ended);
}

bool ProcessAudio(int numInputs, const AudioSampleFrame *inputs,
                  int numOutputs, AudioSampleFrame *outputs,
                  int numParams, const AudioParamFrame *params,
                  void *userData)
{
  assert(numInputs == 1);
  assert(numOutputs == 1);
  // assert(inputs->numberOfChannels == 1); // this is not true when plug in a stereo output node (e.g MediaElementPlayer) into this node
  assert(outputs->numberOfChannels == 2);
  assert(outputs->samplesPerChannel == WEB_AUDIO_FRAME_SIZE);
  auto &input = inputs[0];
  auto &output = outputs[0];

  // interleave input buffer (mono -> stereo)
  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    interleavedIn[2 * i] = input.data[i];
    interleavedIn[2 * i + 1] = (inputs->numberOfChannels == 1) ? input.data[i] : input.data[i + WEB_AUDIO_FRAME_SIZE];
  }

  for (int i = 0; i < numParams; ++i)
  {
    // K-rate parameter: use the first sample for the frame
    float value = params[i].data[0];
    processor.setParameter(i, value);
  }

  // emscripten_log(EM_LOG_CONSOLE, "bpm=%d", fx_get_bpm());
  processor.process(interleavedIn.data(), interleavedOut.data(), WEB_AUDIO_FRAME_SIZE);

  // de-interleave output buffer
  for (int i = 0; i < WEB_AUDIO_FRAME_SIZE; ++i)
  {
    output.data[i] = interleavedOut[2 * i];
    output.data[WEB_AUDIO_FRAME_SIZE + i] = interleavedOut[2 * i + 1];
  }
  return true; // Keep the graph output going
}

void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void *userData)
{
  if (!success)
    return; // Check browser console in a debug build for detailed errors

  ram.resize(processor.getBufferSize());
  processor.init(ram.data());

  // single mono input, single stereo output
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
    return; // Check browser console in a debug build for detailed errors

  auto valid_parameters = getValidParameters();

  WebAudioParamDescriptor params[valid_parameters.size()];
  for (int i = 0; i < valid_parameters.size(); ++i)
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

  int sample_rate = emscripten_audio_context_sample_rate(context);
  int frame_size = emscripten_audio_context_quantum_size(context);
  printf("Sample rate: %d\n", sample_rate);
  printf("Frame size: %d\n", frame_size);

  emscripten_start_wasm_audio_worklet_thread_async(context, audioThreadStack, sizeof(audioThreadStack),
                                                   &AudioThreadInitialized, 0);

  emscripten_exit_with_live_runtime();
}
