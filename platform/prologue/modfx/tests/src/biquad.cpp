/*
 * File: biquad.cpp
 *
 * Simple execution environment test using provided biquad filters.
 *
 * 
 * 
 * 2018 (c) Korg
 *
 */

#include "usermodfx.h"

#include "biquad.hpp"

static dsp::BiQuad s_bq_l, s_bq_r;
static dsp::BiQuad s_bqs_l, s_bqs_r;

enum {
  k_polelp = 0,
  k_polehp,
  k_folp,
  k_fohp,
  k_foap,
  k_foap2,
  k_solp,
  k_sohp,
  k_sobp,
  k_sobr,
  k_soap1,
  k_type_count
};

static uint8_t s_type_z, s_type;
static float s_wc_z, s_wc;
static float s_q;
static const float s_fs_recip = 1.f / 48000.f;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
  s_wc = s_wc_z = 0.49f;
  s_q = 1.4041f;
  s_type = s_type_z = k_polelp;

  s_bq_l.flush();
  s_bq_r.flush();
  s_bq_l.mCoeffs.setPoleLP(s_wc);
  s_bq_r.mCoeffs = s_bq_l.mCoeffs;

  s_bqs_l.flush();
  s_bqs_r.flush();
  s_bqs_l.mCoeffs = s_bqs_r.mCoeffs = s_bq_l.mCoeffs;

}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
  const float * mx = main_xn;
  float * __restrict my = main_yn;
  const float * my_e = my + 2*frames;

  const float *sx = sub_xn;
  float * __restrict sy = sub_yn;
  
  const uint8_t type = s_type;
  const float wc = s_wc;
  
  if (type != s_type_z
      || wc != s_wc_z) {
    
    // type changed
    switch (type) {
    case k_polelp:
      s_bq_l.mCoeffs.setPoleLP(1.f - (wc*2.f));
      break;
      
    case k_polehp:
      s_bq_l.mCoeffs.setPoleHP(wc*2.f);
      break;
      
    case k_folp:
      s_bq_l.mCoeffs.setFOLP(fx_tanpif(wc));
      break;
      
    case k_fohp:
      s_bq_l.mCoeffs.setFOHP(fx_tanpif(wc));
      break;
      
    case k_foap:
      s_bq_l.mCoeffs.setFOAP(fx_tanpif(wc));
      break;

    case k_foap2:
      s_bq_l.mCoeffs.setFOAP2(wc);
      break;

    case k_solp:
      s_bq_l.mCoeffs.setSOLP(fx_tanpif(wc), s_q);
      break;

    case k_sohp:
      s_bq_l.mCoeffs.setSOHP(fx_tanpif(wc), s_q);
      break;

    case k_sobp:
      s_bq_l.mCoeffs.setSOBP(fx_tanpif(wc), s_q);
      break;

    case k_sobr:
      s_bq_l.mCoeffs.setSOBR(fx_tanpif(wc), s_q);
      break;

    case k_soap1:
      s_bq_l.mCoeffs.setSOAP1(fx_tanpif(wc), s_q);
      break;
      
    default:
      break;
    }

    s_bq_r.mCoeffs = s_bq_l.mCoeffs;
    s_bqs_l.mCoeffs = s_bq_l.mCoeffs;
    s_bqs_r.mCoeffs = s_bq_l.mCoeffs;
    
    s_type_z = type;
    s_wc_z = wc;
  }
  
  for (; my != my_e; ) {
    *(my++) = s_bq_l.process_so(*(mx++));
    *(my++) = s_bq_r.process_so(*(mx++));
    *(sy++) = s_bqs_l.process_so(*(sx++));
    *(sy++) = s_bqs_r.process_so(*(sx++));
  }
}


void MODFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case k_user_modfx_param_time:
    s_type = si_roundf(valf * (k_type_count - 1));
    break;
  case k_user_modfx_param_depth:
    s_wc = valf * valf * 0.49f;
    break;
  default:
    break;
  }
}

