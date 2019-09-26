// Web Audio Blocks (WABs)
// Jari Kleimola 2019 (jari@webaudiomodules.org)
//
#include <emscripten.h>
#include <emscripten/bind.h>
#include <cstdint>
using namespace emscripten;

namespace WAB {

  class Processor
  {
  public:
    virtual ~Processor() {}
    virtual bool init(uint32_t m_samplesPerBuffer, uint32_t sampleRate, val descriptor);
    virtual void exit() {}
    virtual void setParam(uint32_t iparam, double value) {}
    virtual void process(val inputs, val outputs, val data) = 0;
    virtual val onmessage (val verb, uint32_t iprop, uint32_t data, uint32_t byteLength);

  protected:
    virtual void setProp (uint32_t iprop, void* data, uint32_t byteLength);
    float* getBuffer(val buff, int index);
    uint32_t m_sampleRate;
    uint32_t m_samplesPerBuffer;
  };

}
