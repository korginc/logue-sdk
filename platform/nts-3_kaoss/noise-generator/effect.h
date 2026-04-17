#pragma once
/*
 *  File: effect.h
 *
 *  Noise generator effect instance.
 *
 */
#include "processor.h"
#include "unit_genericfx.h"
#include "osc_api.h"

class NoiseGenerator : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; }

  enum
  {
    LEVEL = 0U,
    NUM_PARAMS
  };

  struct Params
  {
    float level;

    void reset()
    {
      level = 0.5f;
    }

    Params() { reset(); }
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case LEVEL:
      params_.level = param_10bit_to_f32(value);
      break;
    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    (void)index;
    (void)value;
    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    (void)allocated_buffer;
    params_.reset();
  }

  void teardown() override final { }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const Params p = params_;

    for (const float *out_end = out + frames * 2; out != out_end; in += 2, out += 2)
    {
      float val = osc_white() * p.level;
      out[0] = val;
      out[1] = val;
    }
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final
  {
    (void)id;
    (void)phase;
    (void)x;
    (void)y;
  }

private:
  Params params_;
};
