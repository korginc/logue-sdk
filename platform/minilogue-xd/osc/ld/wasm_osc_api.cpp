#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <emscripten.h>
#include "fixed_math.h"
using namespace std;


// -- note to frequency conversion table
//
#define k_midi_to_hz_size      (152)
float midi_to_hz_lut_f[k_midi_to_hz_size];

// -- sine half-wave
//
#define k_wt_sine_size_exp     (7)
#define k_wt_sine_size         (1U<<k_wt_sine_size_exp)
#define k_wt_sine_lut_size     (k_wt_sine_size+1)
float wt_sine_lut_f[k_wt_sine_lut_size];

// -- tan(pi*x)
//
#define k_tanpi_size_exp         (8)
#define k_tanpi_size             (1U<<k_tanpi_size_exp)
#define k_tanpi_lut_size         (k_tanpi_size+1)
float tanpi_lut_f[k_tanpi_lut_size];

// -- bit reduction
//
#define k_bitres_size_exp    (7)
#define k_bitres_size        (1U<<k_bitres_size_exp)
#define k_bitres_lut_size    (k_bitres_size+1)
float bitres_lut_f[k_bitres_lut_size];

// -- waves
//
#define k_waves_size_exp   (7)
#define k_waves_size       (1U<<k_waves_size_exp)
#define k_waves_u32shift   (24)
#define k_waves_lut_size   (k_waves_size+1)
#define k_waves_max_cnt			16

float * wavesA[k_waves_max_cnt];
float * wavesB[k_waves_max_cnt];
float * wavesC[k_waves_max_cnt];
float * wavesD[k_waves_max_cnt];
float * wavesE[k_waves_max_cnt];
float * wavesF[k_waves_max_cnt];
vector<float**> waves;
uint16_t nwaves[6] = { 16,16,14,13,15,16 };


EMSCRIPTEN_KEEPALIVE void initLUTs ()
{
	// -- midi freqs
	// -- how to scale? using the classic midi note number scheme, 151 is > 50kHz
	double masterTune = 440. / 32;
	for (int n=0; n<k_midi_to_hz_size; n++)
		midi_to_hz_lut_f[n] = masterTune * pow(2, ((n - 9) / 12.));

	// -- sine half-wave
	double phase = 0;
	double phinc = 1. / (k_wt_sine_lut_size-1);
	for (int n=0; n<k_wt_sine_lut_size; n++) {
		wt_sine_lut_f[n] = sin(M_PI * phase);
		phase += phinc;
	}
	
	// -- tan(pi*x)
	double x = 0;
	double dx = 1. / k_tanpi_lut_size;
	for (int n=0; n<k_tanpi_lut_size; n++) {
		tanpi_lut_f[n] = tan(M_PI * x);
		x += dx;
	}
	
	// -- waves
	waves.push_back(wavesA);
	waves.push_back(wavesB);
	waves.push_back(wavesC);
	waves.push_back(wavesD);
	waves.push_back(wavesE);
	waves.push_back(wavesF);
	for (auto& wavebank : waves) {
		for (int n=0; n<k_waves_max_cnt; n++) {
			wavebank[n] = new float[k_waves_lut_size];
			memset(wavebank[n], 0, k_waves_lut_size * sizeof(float));
		}
	}
}

// todo : free waves
EMSCRIPTEN_KEEPALIVE void freeLUTs ()
{
}

// -- dummy stubs
void qsub16(q15_t a, q15_t b) {}
void qsub(q31_t a, q31_t b) {}
q15_t sel(q15_t a, q15_t b) { return 0; }
q31_t sel(q31_t a, q31_t b) { return 0; }


extern "C" {

// -- Park-Miller RNG from wikipedia
uint32_t zwhite = 7;
float _osc_white() {
	zwhite = ((uint64_t)zwhite * 48271u) % 0x7fffffff;
	return zwhite * q31_to_f32_c;
}

}
