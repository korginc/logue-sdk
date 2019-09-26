// Web Audio Blocks (WABs)
// Jari Kleimola 2019 (jari@webaudiomodules.org)
//
#include "wab_processor.h"
#include <string>
using namespace emscripten;

namespace WAB {

  bool Processor::init(uint32_t samplesPerBuffer, uint32_t sampleRate, val descriptor)
  {
    m_samplesPerBuffer = samplesPerBuffer;
    m_sampleRate = sampleRate;
    return true;
  }

  val Processor::onmessage (val verb, uint32_t iprop, uint32_t data, uint32_t byteLength)
  {
    std::string method = verb.as<std::string>();
    if (method.compare("set") == 0) setProp(iprop, (void*)data, byteLength);
    return val::undefined();
  }

  float* Processor::getBuffer(val buff, int index)
  {
    if (buff["length"].as<int>() < index) return nullptr;
    else return (float*)buff[index].as<int>();
  }

} // namespace WAB

extern WAB::Processor* createModule(int);
WAB::Processor* getInstance(int itype) { return createModule(itype); }

EMSCRIPTEN_BINDINGS(WAB_Processor) {
  class_<WAB::Processor>("Processor")
    .function("init", &WAB::Processor::init)
    .function("setParam", &WAB::Processor::setParam)
    .function("process", &WAB::Processor::process, allow_raw_pointers())
    .function("onmessage", &WAB::Processor::onmessage);
  function("getInstance", &getInstance, allow_raw_pointers());
}
