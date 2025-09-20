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
		threshv = e_expff(_thresh * db2log);
		ratio = 20.0;
		bias = 80.0 * _bias / 100.0;
		cthresh = _thresh - bias;
		cthreshv = e_expff(cthresh * db2log);
		makeupv = e_expff(makeup * db2log);
		capsc = log2db;
		attime = 0.0002;
		reltime = 0.3;
		atcoef = e_expff(-1.0 / (attime * srate));
		relcoef = e_expff(-1.0 / (reltime * srate));
		rmscoef = e_expff(-1.0 / (rmstime * srate));
		rmstime = rms_win / 1000000.0;
		runave = 0.0;
	}

	// std::tuple<float32_t, float32_t> process(float32_t spl0, float32_t spl1)
	float32x2_t process(float32x2_t split)
	{
		// auto maxspl = fmax(fabs(spl0), fabs(spl1));
        float32x2_t temp = vabs_f32(split);
        temp = vmax_f32(temp, temp);
        float32_t maxspl;
        vst1_lane_f32(&maxspl, temp, 0);
		maxspl = maxspl * maxspl;

		runave = maxspl + rmscoef * (runave - maxspl);
		// auto det = sqrt(fmax(0.0, runave));
        auto det = runave > 0 ? 0 : fasterSqrt(runave);
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
		auto grv = e_expff(gr * db2log);

        return vmul_n_f32 (split, grv * makeupv);
		// return std::tuple<float32_t, float32_t> (
		// 	spl0 * grv * makeupv,
		// 	spl1 * grv * makeupv
		// );
	}

private:
  float32_t log2db = 8.6858896380650365530225783783321; // 20 / ln(10)
  float32_t db2log = 0.11512925464970228420089957273422;

  float32_t rundb = 0.0;
  float32_t runave = 0.0;
  float32_t threshv = 0.0;
  float32_t cthresh = 0.0;
  float32_t cthreshv = 0.0;
  float32_t ratio = 0;
  float32_t bias = 0.0;
  float32_t makeupv = 0.0;
  float32_t capsc = 0.0;
  float32_t timeconstant = 1.0;
  float32_t attime = 0.0002;
  float32_t reltime = 0.3;
  float32_t atcoef = 0.0;
  float32_t relcoef = 0.0;
  float32_t rmstime = 0.0;
  float32_t rmscoef = 0.0;
};