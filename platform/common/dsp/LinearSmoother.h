/**
 * @file    LinearSmoother.h
 * @brief   Linear Smoother for parameters. Guaranteed to converge 
 *
 * @addtogroup dsp DSP
 * @{
 *
 */

#include "attributes.h"
#include <arm_neon.h>
#include "utils/common_float_math.h"
#include "utils/common_fixed_math.h"
#include "utils/common_int_math.h"

namespace dsp
{

class LinearSmoother
{
public:
    LinearSmoother():
    mInitialValue(0),
    mTargetValue(0),
    mSmoothedValue(0),
    mPhase(0),
    mInterval(0x01FFFFFF) // 1 / 64
    {
    }

    LinearSmoother(int32_t period):
    mInitialValue(0),
    mTargetValue(0),
    mSmoothedValue(0),
    mPhase(0),
    mInterval(period)
    {
    }

    void Flush()
    {
        mSmoothedValue = mInitialValue = mTargetValue;
        mPhase = 0;
    }

    float Process()
    {
        const float frac = q31_to_f32(mPhase);
        mSmoothedValue = linintf(frac, mInitialValue, mTargetValue);

        // up-cast to prevent wrapping
        uint32_t next = static_cast<uint32_t>(mPhase) + static_cast<uint32_t>(mInterval);
        mPhase = static_cast<int32_t>((next > mSmoothingMax) ? mSmoothingMax : next);

        return mSmoothedValue;
    }

    void SetTarget(float value)
    {
        if(value != mTargetValue)
        {
            mPhase = 0;
            mInitialValue = mSmoothedValue;
            mTargetValue = value;
        }
    }

    // normalized 0~1
    void SetInterval(float interval)
    {
        mInterval = interval * 0x7FFFFFFF;
        mPhase = (interval == 1.f) ? 0x7FFFFFFF : mPhase;
    }

    // if value is already known
    void SetInterval(int32_t interval)
    {
        mInterval = interval;
        mPhase = (interval == 0x7FFFFFFF) ? 0x7FFFFFFF : mPhase;
    }

    // calculates interval multiplier based on number of
    // periods until the smoothed value will converge with the target
    void SetIntervalPeriods(int periods)
    {
        mInterval = (1.f / periods) * 0x7FFFFFFF;
    }

    float GetTarget()
    {
        return mTargetValue;
    }

    float GetSmoothedValue()
    {
        return mSmoothedValue;
    }

    float GetInitialValue()
    {
        return mInitialValue;
    }

    int32_t GetPhase()
    {
        return mPhase;
    }

    int32_t GetInterval()
    {
        return mInterval;
    }

private:
    float mInitialValue;
    float mTargetValue;
    float mSmoothedValue;
    int32_t mPhase;
    int32_t mInterval;
    const uint32_t mSmoothingMax = 0x7FFFFFFF;
};

}
/** @} */