/*
 * File: fx_api.cpp
 *
 * Oscillator runtime API implementation
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2018-2022 (c) Korg
 *
 */
/**
 * @file    fx_api.cpp
 * @brief   Oscillator runtime API implementation.
 *
 * @addtogroup FX_API
 * @{
 */

#include <climits>

// #include "common.h"

#include "fx_api.h"
// #include "fx_api_init.h"

// #include "unit/unit_runtime.h"

// #include "arp/tempo.h" // TODO: emulate global tempo in web audio

/*===========================================================================*/
/* Local definitions                                                         */
/*===========================================================================*/

#define __api_meta __attribute__((section(".api.meta"), aligned(4)))

#define __api_var __attribute__((aligned(4))) // TODO: may want to fix the ram location to be able to use MPU

// Keep API aligned after minor updates by appending additions by revision r0, r1, r2...
#define __api_data_r0 __attribute__((section(".api.r0"), aligned(4)))
#define __api_func_r0 __attribute__((used, section(".api.r0"), optimize("Ofast")))

#define __api_data_r1 __attribute__((section(".api.r1"), aligned(4)))
#define __api_func_r1 __attribute__((used, section(".api.r1"), optimize("Ofast")))

/*===========================================================================*/
/* Local types                                                               */
/*===========================================================================*/

typedef struct NoiseFlt {
  
  enum {
    DEFAULT_SEED = 83647U,
  };
  
  typedef struct PinkFilter {
    
    typedef struct State {
      float zff1, zff2, zff3;
      float zfb1, zfb2, zfb3;
      
      State(void) :
        zff1(0),zff2(0),zff3(0),
        zfb1(0),zfb2(0),zfb3(0)
      { }
    } State;
    
    PinkFilter(void)
    { }
    
    inline __attribute__((optimize("Ofast"),always_inline))
    float process(const float xn) {
      
      // Using cascade of 1st order direct form 1 filters
      float acc = xn;
      acc += mState.zff1 * -0.984436f;
      acc -= mState.zfb1 * -0.995728f;
      mState.zff1 = xn;
      mState.zfb1 = acc; 
      
      acc += mState.zff2 * -0.833923f;
      acc -= mState.zfb2 * -0.947906f;
      mState.zff2 = mState.zfb1;
      mState.zfb2 = acc; 
      
      acc += mState.zff3 * -0.075684f;
      acc -= mState.zfb3 * -0.535675f;
      mState.zff3 = mState.zfb2;
      mState.zfb3 = acc;
      
      acc = mState.zfb3 * 0.83516f;
      
      return acc;
    }
    
    State mState;
    
  } PinkFilter;
  
  typedef struct RedFilter {
    
    typedef struct State {
      float zff1;
      float zfb1;
      
      State(void) :
        zff1(0), zfb1(0)
      { }
    } State;
    
    RedFilter(void)
    { }
    
    inline __attribute__((optimize("Ofast"),always_inline))
    float process(const float xn) {
      
      // Using direct form 1 1st order butterworth with gain adjustment
      float acc = 0.035481f * xn;
      acc += mState.zff1 * 0.035481f;
      acc -= mState.zfb1 * -0.99869f;
      mState.zff1 = xn;
      mState.zfb1 = acc; 
      
      return acc;
    }
    
    State mState;
    
  } RedFilter;
  
  NoiseFlt(void) :
    mState(DEFAULT_SEED)
  { }
  
  
  inline __attribute__((optimize("Ofast"),always_inline))
  uint32_t rand(const uint32_t state) {
    
    //--- Park-Miller-Carta ------------------------
    
    uint32_t lo = 16807 * (state & 0xFFFF);
    const uint32_t hi = 16807 * (state >> 16);
    
    lo += (hi & 0x7FFF)<<16;
    lo += hi>>15;
    
    // Original Carta code
    // if (lo > 0x7FFFFFFF) lo -= 0x7FFFFFFF;
    // Simper's compare/branch avoidance
    lo = (lo & 0x7FFFFFFF) + (lo>>31);
    
    return lo;
  }
  
  inline __attribute__((optimize("Ofast"),always_inline))
  float white(void) {
    static const float mean = 0;
    static const float var = 0.3; // sqrtf(0.09) == 0.3
    static const float scale = 1.f / (float)UINT_MAX;
    
    const int32_t r1 = rand(mState);
    const int32_t r2 = rand(r1);
    mState = r2;
    
    
    const float r1f = (uint32_t)r1 * scale;
    const float r2f = (uint32_t)r2 * scale;
    
    //const float x = mean + var * (sqrtf(-2.f * si_logf(r1f)) * mSine.scan(r2f + 0.25f));
    const float x = mean + var * (fx_sqrtm2logf(r1f) * fx_sinf(r2f + 0.25f));
    
    return x;
  }
  
  inline __attribute__((optimize("Ofast"),always_inline))
  float pink(void) {
    return mPinkFilter.process(0.9f * white());
  }
  
  inline __attribute__((optimize("Ofast"),always_inline))
  float red(void) {
    return mRedFilter.process(0.9f * white());
  }
  
  inline __attribute__((optimize("Ofast"),always_inline))
  float blue(void) {
    const float w = 0.9f * white();
    return w - mPinkFilter.process(w);
  }
  
  inline __attribute__((optimize("Ofast"),always_inline))
  float purple(void) {
    const float w = 0.9f * white();
    return w - mRedFilter.process(w);
  }
  
  uint32_t   mState;
  
  PinkFilter mPinkFilter;
  RedFilter  mRedFilter;
  
} NoiseFlt;


/*===========================================================================*/
/* Local variables                                                           */
/*===========================================================================*/

// __api_meta const uint32_t k_fx_api_platform = UNIT_TARGET_PLATFORM;
// __api_meta const uint32_t k_fx_api_version = UNIT_API_VERSION;

static __api_var uint32_t s_mcu_hash = 0;
static __api_var NoiseFlt s_noise_src;

/*===========================================================================*/
/* MCU Uniqueness                                                            */
/*===========================================================================*/

static inline __attribute__((optimize("Ofast"),always_inline))
uint32_t si_jenkins_hash32(uint32_t x) {
  x = (x + 0x7ed55d16) + (x<<12);
  x = (x ^ 0xc761c23c) ^ (x>>19);
  x = (x + 0x165667b1) + (x<<5);
  x = (x + 0xd3a2646c) ^ (x<<9);
  x = (x + 0xfd7046c5) + (x<<3);
  x = (x ^ 0xb55a4f09) ^ (x>>16);
  return x;
}

// #if defined(STM32F4xx)
// #define UID_BASE   0x1FFF7A10U  /*!< Unique device ID register base address - STM32F4xx*/
// #else
// #ifndef UID_BASE
// #error "UID_BASE undefined"
// #endif
// #endif

// #define MCU_UID_0 (((uint32_t *)UID_BASE)[0])
// #define MCU_UID_1 (((uint32_t *)UID_BASE)[1])
// #define MCU_UID_2 (((uint32_t *)UID_BASE)[2])

// static inline __attribute__((optimize("Ofast"),always_inline))
// uint32_t si_make_mcu_hash(void) {
//   const uint32_t x0 = si_jenkins_hash32(MCU_UID_0);
//   const uint32_t x1 = si_jenkins_hash32(MCU_UID_1);
//   const uint32_t x2 = si_jenkins_hash32(MCU_UID_2);
//   return (x0 ^ x1) ^ x2;
// }

/*===========================================================================*/
/* General API functions                                                     */
/*===========================================================================*/

/**
 * Get MCU hash
 */
__api_func_r0 uint32_t fx_mcu_hash(void)
{
  return s_mcu_hash;
}

/**
 * Get current tempo as beats per minute as integer
 */
// __api_func_r1 uint16_t fx_get_bpm(void)
// {
//   return Tempo_GetActualTempo();
// }

// /**
//  * Get current tempo as beats per minute as floating point
//  */
// __api_func_r1 float fx_get_bpmf(void)
// {
//   return Tempo_GetActualTempo() * 0.1f; // div by 10
// }

/*===========================================================================*/
/* Lookup table related functions                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Noise source                                                              */
/*===========================================================================*/

/**
 * Random integer
 *
 * @return     Value in [0, UINT_MAX].
 * @note       Generated with Park-Miller-Carta
 */
__api_func_r0 uint32_t fx_rand(void)
{
  const uint32_t r = s_noise_src.rand(s_noise_src.mState);
  s_noise_src.mState = r;
  return r;
}

/**
 * Gaussian white noise
 *
 * @return     Value in [-1.0, 1.0].
 */
__api_func_r0 float fx_white(void)
{
  return s_noise_src.white();
}

// /**
//  * Pink noise
//  *
//  * @return     Value in [-1.0, 1.0].
//  */
// __api_func_r0 float fx_pink(void)
// {
//   return s_noise_src.pink();
// }

// /**
//  * Red noise
//  *
//  * @return     Value in [-1.0, 1.0].
//  */
// __api_func_r0 float fx_red(void)
// {
//   return s_noise_src.red();
// }

// /**
//  * Blue noise
//  *
//  * @return     Value in [-1.0, 1.0].
//  */
// __api_func_r0 float fx_blue(void)
// {
//   return s_noise_src.blue();
// }

// /**
//  * Purple noise
//  *
//  * @return     Value in [-1.0, 1.0].
//  */
// __api_func_r0 float fx_purple(void)
// {
//   return s_noise_src.purple();
// }


/*===========================================================================*/
/* Initialization                                                            */
/*===========================================================================*/

/**
 * Initialize API
 */
__attribute__((used))
void fx_api_init(void)
{
  // s_mcu_hash = si_make_mcu_hash();
  s_mcu_hash = 0;

  s_noise_src.mState = s_mcu_hash;
}



/** @} */
