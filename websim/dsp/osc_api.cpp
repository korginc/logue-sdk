/*
 * File: osc_api.cpp
 *
 * Oscillator runtime API implementation
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2018-2022 (c) Korg
 *
 */
/**
 * @file    osc_api.cpp
 * @brief   Oscillator runtime API implementation.
 *
 * @addtogroup OSC_API
 * @{
 */

#include <climits>

// #include "common.h"

#include "osc_api.h"

typedef int32_t q31_t;

// #include "wavetable_lut.h"

// #include "osc_api_init.h"

// #include "unit/unit_runtime.h"

// #include "osc_synths_common.h" //TODO: need to replace this

// #if defined(STM32F4xx)
// #define UID_BASE   0x1FFF7A10U  /*!< Unique device ID register base address - STM32F4xx*/
// #else
// #ifndef UID_BASE
// #error "UID_BASE undefined"
// #endif
// #endif

/*===========================================================================*/
/* Local definitions                                                         */
/*===========================================================================*/

#define __api_meta __attribute__((used, section(".oscapi.meta"), aligned(4)))

#define __api_var __attribute__((aligned(4))) // TODO: may want to fix the ram location to be able to use MPU

// Keep API aligned after minor updates by appending additions by revision r0, r1, r2...
#define __api_data_r0 __attribute__((used, section(".oscapi.r0"), aligned(4)))
#define __api_func_r0 __attribute__((used, section(".oscapi.r0"), optimize("Ofast")))

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
    
    const q31_t r1 = rand(mState);
    const q31_t r2 = rand(r1);
    mState = r2;
    
    
    const float r1f = (uint32_t)r1 * scale;
    const float r2f = (uint32_t)r2 * scale;
    
    //const float x = mean + var * (sqrtf(-2.f * si_logf(r1f)) * mSine.scan(r2f + 0.25f));
    const float x = mean + var * (osc_sqrtm2logf(r1f) * osc_sinf(r2f + 0.25f));
    
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
/* Variables and Constants                                                   */
/*===========================================================================*/

// __api_meta const uint32_t k_osc_api_platform = UNIT_TARGET_PLATFORM;
// __api_meta const uint32_t k_osc_api_version = UNIT_API_VERSION;

static __api_var uint32_t s_mcu_hash = 0;
static __api_var NoiseFlt s_noise_src;

/*===========================================================================*/
/* MCU Uniqueness                                                            */
/*===========================================================================*/

// static inline __attribute__((optimize("Ofast"),always_inline))
// uint32_t si_jenkins_hash32(uint32_t x) {
//   x = (x + 0x7ed55d16) + (x<<12);
//   x = (x ^ 0xc761c23c) ^ (x>>19);
//   x = (x + 0x165667b1) + (x<<5);
//   x = (x + 0xd3a2646c) ^ (x<<9);
//   x = (x + 0xfd7046c5) + (x<<3);
//   x = (x ^ 0xb55a4f09) ^ (x>>16);
//   return x;
// }

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
/* Table offsets.                                                            */
/*===========================================================================*/

static inline __attribute__((optimize("Ofast"),always_inline))
uint8_t si_wt_note_offset(const uint8_t note, const uint8_t * __restrict__ notes, const uint8_t len) {
  const uint8_t *notes_e = notes + (len - 1);
  uint8_t i=0;
  for (; notes != notes_e; ++notes, ++i) {
    if ((*notes) >=  note)
      break;
  }
  return i;
}

static inline __attribute__((optimize("Ofast"),always_inline))
float si_wt_note_offset_frac(const float note, const uint8_t *notes, const uint8_t len) {
  const uint8_t * __restrict__ n = notes;
  const uint8_t *n_e = n + (len - 1);
  uint8_t i=0;
  for (; n != n_e; ++n, ++i) {
    if ((*n) >=  note) {
      break;
    } 
  }
  
  const uint8_t prev = (i > 0) ? notes[i-1] : 0;
  return clampmaxfsel(i + (float)(note - prev) / (notes[i] - prev), len - 1);
}

/*===========================================================================*/
/* General API functions                                                     */
/*===========================================================================*/

/**
 * Get MCU hash
 */
__api_func_r0 uint32_t osc_mcu_hash(void)
{
  return s_mcu_hash;
}

/*===========================================================================*/
/* Lookup table related functions                                            */
/*===========================================================================*/

/**
 * Get band-limited sawtooth wave index for note.
 */
__api_func_r0 float osc_bl_saw_idx(float note)
{
  return si_wt_note_offset_frac(note, wt_saw_notes, k_wt_saw_notes_cnt);
}

/**
 * Get band-limited square wave index for note.
 */
__api_func_r0 float osc_bl_sqr_idx(float note)
{
  return si_wt_note_offset_frac(note, wt_sqr_notes, k_wt_sqr_notes_cnt);
}

/**
 * Get band-limited parabolic wave index for note.
 */
__api_func_r0 float osc_bl_par_idx(float note)
{
  return si_wt_note_offset_frac(note, wt_par_notes, k_wt_par_notes_cnt);
}

/*===========================================================================*/
/* Noise source                                                              */
/*===========================================================================*/

/**
 * Random integer
 *
 * @return     Value in [0, UINT_MAX].
 * @note       Generated with Park-Miller-Carta
 */
__api_func_r0 uint32_t osc_rand(void)
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
__api_func_r0 float osc_white(void)
{
  return s_noise_src.white();
}

/*===========================================================================*/
/* Initialization                                                            */
/*===========================================================================*/

/**
 * Initialize API
 */
__attribute__((used))
void osc_api_init(void)
{
  // s_mcu_hash = si_make_mcu_hash();
  s_mcu_hash = 0;

  s_noise_src.mState = s_mcu_hash;
}



/** @} */
