#pragma once
#include "fm_operator.h"
#include "float_math.h"
#include "constants.h
/** virtual class from each instrument is derived */
class DrumModel {
public:
    virtual ~DrumModel() {}
    virtual void Init() = 0;
    virtual void Trigger() = 0;
    virtual float Process() = 0;
    virtual void RenderControls() = 0;

    virtual void  loadPreset(uint8_t idx) = 0;
    virtual uint8_t getPresetIndex() = 0;
    virtual void  setParameter(fm_param_index_t param_index, float value) = 0;
    virtual float getParameter(fm_param_index_t param_index) = 0;
};