// Copyright 2025 tilr
// Port of FairlyChildish limiter for Reaper

// Copyright 2006, Thomas Scott Stillwell
// All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are permitted
//provided that the following conditions are met:
//
//Redistributions of source code must retain the above copyright notice, this list of conditions
//and the following disclaimer.
//
//Redistributions in binary form must reproduce the above copyright notice, this list of conditions
//and the following disclaimer in the documentation and/or other materials provided with the distribution.
//
//The name of Thomas Scott Stillwell may not be used to endorse or
//promote products derived from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY e_expffRESS OR
//IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
//BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include "float_math.h"
#include <arm_neon.h>

class Limiter
{
public:
	Limiter() {};
	~Limiter() {};

	void init(float32_t srate, float32_t _thresh = 0.0, float32_t _bias = 70.0,
        float32_t rms_win = 100.0, float32_t makeup = 0.0)
	{
		threshv = e_expff(_thresh * M_DBTOLOG);
		ratio = 20.0;
		bias = 80.0 * _bias / 100.0;
		cthresh = _thresh - bias;
		cthreshv = e_expff(cthresh * M_DBTOLOG);
		makeupv = e_expff(makeup * M_DBTOLOG);
		capsc = M_LOGTODB;
		attime = 0.0002;
		reltime = 0.3;
		atcoef = e_expff(-1.0 / (attime * srate));
		relcoef = e_expff(-1.0 / (reltime * srate));
		rmstime = rms_win / 1000000.0;
		rmscoef = vdup_n_f32(e_expff(-1.0 / (rmstime * srate)));
		runave = vdup_n_f32(0.0);
	}

	inline float32_t calculate_grv(float32_t runave)
	{
		// auto det = sqrt(fmax(0.0, runave));
        auto det = runave > 0 ? fasterSqrt(runave) : 0;
		// auto overdb = fmax(0.0, capsc * log(det/threshv));
        auto overdb = fmax(0.0, capsc * fasterlogf(det/threshv));

		if (overdb > rundb)
			rundb = overdb + atcoef * (rundb - overdb);
		else
			rundb = overdb + relcoef * (rundb - overdb);
		overdb = fmax(0.0, rundb);

		auto cratio = bias == 0.0
			? ratio
			: 1.0 + (ratio -1.0) * fasterSqrt(overdb / bias);

		auto gr = -overdb * (cratio - 1.0) / cratio;
		return e_expff(gr * M_DBTOLOG) * makeupv;
	}

	// std::tuple<float32_t, float32_t> process(float32_t spl0, float32_t spl1)
	float32x4_t process(float32x4_t split)
	{
		// auto maxspl = fmax(fabs(spl0), fabs(spl1));
		// maxspl = maxspl * maxspl;
        float32x4_t temp = vabsq_f32(split);
        float32x2_t maxspl = vmax_f32(vget_low_f32(temp), vget_high_f32(temp));
		maxspl = vmul_f32(maxspl, maxspl);

		// runave = maxspl + rmscoef * (runave - maxspl);
		runave = vmla_f32(maxspl, rmscoef, vsub_f32(runave, maxspl));
		float32_t grv0 = calculate_grv(vget_lane_f32(runave, 0));
		float32_t grv1 = calculate_grv(vget_lane_f32(runave, 1));
		float32x2_t grv = vdup_n_f32(grv0);
		grv = vset_lane_f32(grv1, grv, 1);
		return vmulq_f32(split, vcombine_f32(grv, grv));

		// auto det = sqrt(fmax(0.0, runave));
		// auto overdb = fmax(0.0, capsc * log(det/threshv));

		// if (overdb > rundb)
		// 	rundb = overdb + atcoef * (rundb - overdb);
		// else
		// 	rundb = overdb + relcoef * (rundb - overdb);
		// overdb = fmax(0.0, rundb);

		// auto cratio = bias == 0.0
		// 	? ratio
		// 	: 1.0 + (ratio -1.0) * fasterSqrt(overdb / bias);

		// auto gr = -overdb * (cratio - 1.0) / cratio;
		// auto grv = e_expff(gr * M_DBTOLOG);

		// return std::tuple<float32_t, float32_t> (
		// 	spl0 * grv * makeupv,
		// 	spl1 * grv * makeupv
		// );
	}

private:
  float32_t rundb = 0.0;
  float32x2_t runave = vdup_n_f32(0.0);
  float32_t threshv = 0.0;
  float32_t cthresh = 0.0;
  float32_t cthreshv = 0.0;
  float32_t ratio = 0;
  float32_t bias = 0.0;
  float32_t makeupv = 0.0;
  float32_t capsc = 0.0;
//   float32_t timeconstant = 1.0;
  float32_t attime = 0.0002;
  float32_t reltime = 0.3;
  float32_t atcoef = 0.0;
  float32_t relcoef = 0.0;
  float32_t rmstime = 0.0;
  float32x2_t rmscoef = vdup_n_f32(0.0);
};