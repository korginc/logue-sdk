#ifndef WFLCG_INCLUDE_GUARD
#define WFLCG_INCLUDE_GUARD

#include <cstdint>

#define WFLCG_VERSION 0x010001
#define WFLCG_VERSION_STRING "1.0.1"
#define WFLCG_COPYRIGHT_STRING "WFLCG v" WFLCG_VERSION_STRING " (C)2019 Juha Nieminen"

//============================================================================
// WFLCG
//============================================================================
class WFLCG
{
 public:
    WFLCG(std::uint32_t seed = 0);
    WFLCG(std::uint32_t seed1, std::uint32_t seed2);

    using result_type = std::uint32_t;

    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT32_MAX; }
    result_type operator()();

    float getFloat();
    double getDouble();
    double getDouble2();

    static constexpr unsigned kBufferSize = 16;

    const std::uint32_t* buffer() const { return mSeeds; }
    float bufferElementAsFloat(unsigned) const;
    double bufferElementAsDouble(unsigned) const;
    double bufferElementAsDouble2(unsigned) const;
    void refillBuffer();


 private:
    std::uint32_t mSeeds[kBufferSize];
    unsigned mIndex = 0;
};

//============================================================================
// WFLCG_f
//============================================================================
class WFLCG_f: public WFLCG
{
 public:
    using WFLCG::WFLCG;

    using result_type = float;
    static constexpr result_type min() { return 1.0f; }
    static constexpr result_type max() { return 2.0f; }
    result_type operator()();
};

//============================================================================
// WFLCG_d
//============================================================================
class WFLCG_d: public WFLCG
{
 public:
    using WFLCG::WFLCG;

    using result_type = double;
    static constexpr result_type min() { return 1.0; }
    static constexpr result_type max() { return 2.0; }
    result_type operator()();
};

//============================================================================
// WFLCG_d2
//============================================================================
class WFLCG_d2: public WFLCG
{
 public:
    using WFLCG::WFLCG;

    using result_type = double;
    static constexpr result_type min() { return 1.0; }
    static constexpr result_type max() { return 2.0; }
    result_type operator()();
};


//============================================================================
// Implementations
//============================================================================
inline WFLCG::WFLCG(std::uint32_t seed)
{
    seed = seed * UINT32_C(2364742333) + UINT32_C(14567);
    for(unsigned i = 0; i < kBufferSize; ++i)
    {
        seed = seed * UINT32_C(2364742333) + UINT32_C(14567);
        mSeeds[i] = seed;
    }
}

inline WFLCG::WFLCG(std::uint32_t seed1, std::uint32_t seed2)
{
    seed1 = seed1 * UINT32_C(2364742333) + UINT32_C(14567);
    seed2 = seed2 * UINT32_C(4112992229) + UINT32_C(12345);
    for(unsigned i = 0; i < kBufferSize; i += 2)
    {
        seed1 = seed1 * UINT32_C(2364742333) + UINT32_C(14567);
        seed2 = seed2 * UINT32_C(4112992229) + UINT32_C(12345);
        mSeeds[i] = seed1;
        mSeeds[i+1] = seed2;
    }
}

inline void WFLCG::refillBuffer()
{
    static const std::uint32_t multipliers[kBufferSize] =
    {
        UINT32_C(3363461597), UINT32_C(3169304909), UINT32_C(2169304933), UINT32_C(2958304901),
        UINT32_C(2738319061), UINT32_C(2738319613), UINT32_C(3238311437), UINT32_C(1238311381),
        UINT32_C(1964742293), UINT32_C(1964743093), UINT32_C(2364742333), UINT32_C(2312912477),
        UINT32_C(2312913061), UINT32_C(1312912501), UINT32_C(2812992317), UINT32_C(4112992229)
    };
    static const std::uint32_t increments[kBufferSize] =
    {
        UINT32_C(8346591), UINT32_C(18134761), UINT32_C(12345), UINT32_C(234567),
        UINT32_C(14567), UINT32_C(12345), UINT32_C(123123), UINT32_C(11223345),
        UINT32_C(123131), UINT32_C(83851), UINT32_C(14567), UINT32_C(134567),
        UINT32_C(34567), UINT32_C(32145), UINT32_C(123093), UINT32_C(12345)
    };

    for(unsigned i = 0; i < kBufferSize; ++i)
        mSeeds[i] = mSeeds[i] * multipliers[i] + increments[i];

    mIndex = 0;
}

inline WFLCG::result_type WFLCG::operator()()
{
    if(mIndex == kBufferSize) refillBuffer();
    const std::uint32_t result = mSeeds[mIndex] ^ (mSeeds[mIndex] >> 24);
    ++mIndex;
    return result;
}

inline float WFLCG::getFloat()
{
    if(mIndex == kBufferSize) refillBuffer();
    union { float fValue; std::uint32_t uValue; } conv;
    conv.uValue = UINT32_C(0x3F800000) | (mSeeds[mIndex++] >> 9);
    return conv.fValue;
}

inline double WFLCG::getDouble()
{
    union { double dValue; std::uint64_t uValue; } conv;
    conv.uValue = (UINT64_C(0x3FF0000000000000) |
                   ((static_cast<std::uint64_t>(operator()())) << 20));
    return conv.dValue;
}

inline double WFLCG::getDouble2()
{
    if(mIndex == kBufferSize) refillBuffer();
    union { double dValue; std::uint64_t uValue; } conv;
    std::uint32_t value1 = mSeeds[mIndex], value2 = mSeeds[mIndex+1];
    conv.uValue = (UINT64_C(0x3FF0000000000000) |
                   (((static_cast<std::uint64_t>(value1)) << 20) ^
                    ((static_cast<std::uint64_t>(value2)) >> 4)));
    mIndex += 2;
    return conv.dValue;
}

inline float WFLCG::bufferElementAsFloat(unsigned index) const
{
    union { float fValue; std::uint32_t uValue; } conv;
    conv.uValue = UINT32_C(0x3F800000) | (mSeeds[index] >> 9);
    return conv.fValue;
}

inline double WFLCG::bufferElementAsDouble(unsigned index) const
{
    union { double dValue; std::uint64_t uValue; } conv;
    std::uint32_t value = mSeeds[index];
    value ^= value >> 24;
    conv.uValue = (UINT64_C(0x3FF0000000000000) |
                   ((static_cast<std::uint64_t>(value)) << 20));
    return conv.dValue;
}

inline double WFLCG::bufferElementAsDouble2(unsigned index) const
{
    union { double dValue; std::uint64_t uValue; } conv;
    std::uint32_t value1 = mSeeds[index], value2 = mSeeds[index+1];
    conv.uValue = (UINT64_C(0x3FF0000000000000) |
                   (((static_cast<std::uint64_t>(value1)) << 20) ^
                    ((static_cast<std::uint64_t>(value2)) >> 4)));
    return conv.dValue;
}

inline WFLCG_f::result_type WFLCG_f::operator()()
{
    return getFloat();
}

inline WFLCG_d::result_type WFLCG_d::operator()()
{
    return getDouble();
}

inline WFLCG_d::result_type WFLCG_d2::operator()()
{
    return getDouble2();
}
#endif
