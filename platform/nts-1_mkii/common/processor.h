#pragma once
#include <stdint.h>

// base class to present a hardware-agonistic interface for logue-sdk osc/fx/genericfx

class Processor
{
public:
    // returns the sample rate, which is 48k for all logue-sdk products
    static constexpr float getSampleRate() { return 48000.f; }

    // the size of dynamic RAM allocated for this effect
    // the allocated buffer address will be passed via init function
    // subclass can override this function to claim more (or less) RAM
    virtual uint32_t getBufferSize() const { return 0x8000U; } // default 128 KB

    virtual void init(float *allocated_buffer) = 0;

    virtual void process(const float *__restrict in, float *__restrict out, uint32_t frames) = 0;

    // note: the deconstructor will not be called for the static instance
    // so make sure to free any resources in this function
    virtual void teardown() {};

    // maintains the mapping from index (enum) to the actual parameter
    virtual void setParameter(uint8_t id, int32_t value) = 0;

    // for string type parameters
    virtual const char *getParameterStrValue(uint8_t id, int32_t value) const
    {
        return nullptr;
    }

    // reset effect state, excluding exposed parameter values
    virtual void reset() {}

    // effect will resume and exit suspend state
    // usually means the synth was selected and the render callback will be called again
    virtual void resume() {}

    // effect will enter suspend state
    // Usually means another effect was selected and thus the render callback will not be called
    virtual void suspend() {}

    virtual void noteOn(uint8_t note, uint8_t velo)
    {
        (void)note;
        (void)velo;
    }

    virtual void noteOff(uint8_t note)
    {
        (void)note;
    }

    virtual void allNoteOff() {}

    // for NTS-3 genericfx
    virtual void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y)
    {
        (void)id;
        (void)phase;
        (void)x;
        (void)y;
    }

    virtual void pitchBend(uint8_t bend)
    {
        (void)bend;
    }

    virtual void channelPressure(uint8_t press)
    {
        (void)press;
    }

    virtual void aftertouch(uint8_t note, uint8_t press)
    {
        (void)note;
        (void)press;
    }

    virtual void setTempo(float tempo)
    {
        (void)tempo;
    }

    virtual void tempo4ppqnTick(uint32_t counter)
    {
        (void)counter;
    }
};