// Web Audio Blocks (WABs)
// WAB wrapper for KORG logue-sdk user oscs/fx
// Jari Kleimola 2019 (jari@webaudiomodules.org)
//
#include "wab_processor.h"
#include "userosc.h"

namespace KORG {

  class LogueProcessor : public WAB::Processor
  {
  public:
    bool init(uint32_t m_samplesPerBuffer, uint32_t sampleRate, val descriptor) override;
    void exit() override;
    void setParam(uint32_t iparam, double value) override;

  protected:
    void setProp (uint32_t iprop, void* data, uint32_t byteLength) override;
    int32_t m_outbuf[128];
  };

  class LogueOscillator : public LogueProcessor
  {
  public:
    bool init(uint32_t m_samplesPerBuffer, uint32_t sampleRate, val descriptor) override;
    void setParam(uint32_t iparam, double value) override;
    void process (val inputs, val outputs, val data) override;

  protected:
    user_osc_param_t m_params;
    bool m_active;
  };

  class LogueEffect : public LogueProcessor
  {
  };

} // namespace KORG
