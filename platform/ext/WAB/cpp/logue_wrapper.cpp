// Web Audio Blocks (WABs)
// WAB wrapper for KORG prologue / minilogue_xd user oscs/fx
// Jari Kleimola 2019 (jari@webaudiomodules.org)
//
#include "logue_wrapper.h"
using namespace emscripten;
using namespace WAB;
using namespace std;

// extern void initLUTs();
// extern void freeLUTs();
// extern vector<float**> waves;
// extern uint16_t nwaves[];
extern "C" {
  void osc_api_init(void);
}

namespace KORG {

const int k_logue_property = 100;
const int k_logue_gate			= k_logue_property;
const int k_logue_pitch			= k_logue_property + 1;
const int k_logue_cutoff		= k_logue_property + 2;
const int k_logue_resonance	= k_logue_property + 3;


// ----------------------------------------------------------------------------
// LogueProcessor : abstract superclass for LogueOscillator/Effect
// ----------------------------------------------------------------------------

bool LogueProcessor::init (uint32_t samplesPerBuffer, uint32_t sampleRate, val descriptor)
{
	Processor::init(samplesPerBuffer, sampleRate, descriptor);

        osc_api_init();
        
	// initLUTs();
	_hook_init(0,0);
	return true;
}

void LogueProcessor::exit ()
{
	// freeLUTs();
}

void LogueProcessor::setParam (uint32_t ikey, double value)
{
	_hook_param(ikey, value);
}

// internal waveforms are simulated with custom wavesets
// waveset is a zip file containing N wav files (decoded at JS side)
// just single cycle waveforms, no mip-mapping
//
void LogueProcessor::setProp (uint32_t iprop, void* data, uint32_t byteLength)
{
	// int ibank = iprop - 100;
	// if (0 <= ibank && ibank < 6) {
	// 	float** bank = waves[ibank];
	// 	float* waveset = static_cast<float*>(data);
	// 	int cnt = byteLength / 4 / k_waves_lut_size;
	// 	if (nwaves[ibank] < cnt) cnt = nwaves[ibank];
	// 	int n = 0;
	// 	for (int i=0; i<cnt; i++) {
	// 		memcpy(bank[i], &waveset[n], k_waves_lut_size * sizeof(float));
	// 		n += k_waves_lut_size;
	// 	}
	// }
}


// ----------------------------------------------------------------------------
// LogueOscillator
// ----------------------------------------------------------------------------

bool LogueOscillator::init (uint32_t samplesPerBuffer, uint32_t sampleRate, val descriptor)
{
	LogueProcessor::init(samplesPerBuffer, sampleRate, descriptor);
	memset(&m_params, 0, sizeof(m_params));
	m_active = false;
	return true;
}

// hook_params from knobs handled in superclass
// gate, pitch, cutoff and resonance properties interfaced as params for simplicity
//
void LogueOscillator::setParam (uint32_t ikey, double value)
{
	if (ikey < 8) LogueProcessor::setParam(ikey, value);
	else switch (ikey) {
		case k_logue_gate:
			m_active = int(value) != 0;
			if (m_active) _hook_on(&m_params);
			else _hook_off(&m_params);
			break;
		case k_logue_pitch:			m_params.pitch = (int16_t(value) << 8) | 0; break;
		case k_logue_cutoff:		m_params.cutoff = value; break;
		case k_logue_resonance: m_params.resonance = value; break;
	}
}

// DSP
//
void LogueOscillator::process (val inputs, val outputs, val data)
{
	// our LFO runs actually at audiorate, but here we just take the first sample
	// now unipolar
	const float* lfobuf = getBuffer(inputs, 0);
	float lfo = lfobuf[0];
	m_params.shape_lfo = lfo ? f32_to_q31(0.5 * lfo + 0.5) : 0;

	// render in user oscillator and scale the result back to 32bit floats
	float* out = getBuffer(outputs, 0);
	if (m_active) {
		_hook_cycle(&m_params, m_outbuf, m_samplesPerBuffer);
		for (int n = 0; n < m_samplesPerBuffer; n++)
			*out++ = m_outbuf[n] * q31_to_f32_c;
	}
	else memset(out, 0, m_samplesPerBuffer * sizeof(float));
}

} // namespace KORG


// entry point
EMSCRIPTEN_KEEPALIVE Processor* createModule(int itype) {
	if (itype == 0) return new KORG::LogueOscillator();
	else return nullptr; // new KORG::LogueEffect();
}

// js bindings
EMSCRIPTEN_BINDINGS(LogueProcessor) {
	class_<KORG::LogueProcessor, base<Processor>>("LogueProcessor");
	class_<KORG::LogueOscillator, base<KORG::LogueProcessor>>("LogueOscillator");
	class_<KORG::LogueEffect, base<KORG::LogueProcessor>>("LogueEffect");
}
