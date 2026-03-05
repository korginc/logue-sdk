#include "float_math.h"
#include "Voice.h"
// #include "Models.h"
#include "constants.h"

/* called by Voice::trigger*/
float32_t Voice::note2freq(int _note)
{
    // MIDI note 69 == A4 == 440.0 Hz - see resonator_orig.h for reference
    return c_midi_a4_freq * fasterpowf(2.0f, (_note - c_midi_a4_note) / c_semitones_per_octave);
}

// Set freq note, prepares mallet, noise generator, resonators, to be used at actual trigger
// Called by onNote(MIDIMsg msg)
// Triggers mallet and noise generator
void Voice::trigger(/**uint64_t timestamp,*/ float32_t srate, int _note,
		float32_t _vel, /**MalletType malletType, */float32_t malletFreq/** TODO:,
		float32_t malletKTrack, bool skip_fade*/)
{
	resA.clear();
	resB.clear();
	note = _note;
	/** TODO: code froze at 1.5.0-2 version
	srate = _srate;
	malletType = _malletType;
	malletFreq = _malletFreq;

	newVel = _vel;
	newNote = _note;
	newFreq = note2freq(newNote);
	malletKtrack = _mallet_ktrack;
	*/
	isRelease = false;
	isPressed = true;
	vel = _vel;
	freq = note2freq(note);
	mallet.trigger(srate, malletFreq);
	noise.attack(_vel);
	/** TODO:
	pressed_ts = timestamp;

	// fade out active voice before re-triggering
	if (skip_fadeout) {
		triggerStart(false);
	}
	else if (((resA.on && resA.active) || (resB.on && resB.active))) {
		isFading = true;
		fadeTotalSamples = (int)(c_repeatNoteFadeMs * 0.001 * srate);
		fadeSamples = fadeTotalSamples;
		updateResonators();
	}
	else {
		triggerStart(true);
	}
}

float32_t Voice::fadeOut()
{
	fadeSamples--;
	if (fadeSamples <= 0) {
		isFading = false;
		triggerStart(true);
	}
	return isFading ? (double)fadeSamples / (double)fadeTotalSamples : 1.0;
}

void Voice::triggerStart(bool reset)
{
	if (reset) {
		resA.clear();
		resB.clear();
	}
	note = newNote;
	vel = newVel;
	freq = newFreq;

	mallet.trigger(malletType, srate, malletFreq, note, malletKtrack);
	noise.attack(vel);
	*/
	if (resA.isOn()) resA.activate();
	if (resB.isOn()) resB.activate();
	updateResonators();
}

void Voice::release(/**uint64_t timestamp*/)
{
	isRelease = true;
	isPressed = false;
	//TODO: release_ts = timestamp;
	noise.release();
	updateResonators();
}

void Voice::clear()
{
    mallet.clear();
    noise.clear();
    resA.clear();
    resB.clear();
}
// onSlider
void Voice::setCoupling(bool _couple, float32_t _split) {
    couple = _couple;
    split = _split;
}

void Voice::setPitch(float32_t a_coarse, float32_t b_coarse, float32_t a_fine, float32_t b_fine)
{
    aPitchFactor = fasterpowf(2.0f, (a_coarse + a_fine / c_cents_per_semitone) / c_semitones_per_octave);
    bPitchFactor = fasterpowf(2.0f, (b_coarse + b_fine / c_cents_per_semitone) / c_semitones_per_octave);
}
/**< TODO:
void Voice::setRatio(float32_t _a_ratio, float32_t _b_ratio)
{
	a_ratio = _a_ratio;
	b_ratio = _b_ratio;
}
*/
void Voice::applyPitch(std::array<float32_t, 64>& model, float32_t factor)
{
    // for (float32_t& ratio : model)
    //     ratio *= factor;

    float32x4_t a0, a1, a2, a3;
    for (size_t i = 0; i < 64; i += 16) {
        // load from model
        a0 = vld1q_f32(&model[i]);
        a1 = vld1q_f32(&model[i] + 4);
        a2 = vld1q_f32(&model[i] + 8);
        a3 = vld1q_f32(&model[i] + 12);
        a0 = vmulq_n_f32(a0, factor);
        a1 = vmulq_n_f32(a1, factor);
        a2 = vmulq_n_f32(a2, factor);
        a3 = vmulq_n_f32(a3, factor);
        // Store the results into model
        vst1q_f32(&model[i], a0);
        vst1q_f32(&model[i] + 4, a1);
        vst1q_f32(&model[i] + 8, a2);
        vst1q_f32(&model[i] + 12, a3);
    }
// TODO: Voice::processOscillators(bool isA)

/** NOTE for semplification purposes this function has been moved
 * completely inside calcFrequencyShifts()
 *  NOTE 2 : now also calcFrequencyShifts() has been absorbed in order to make
 * less calls (which yeld an overhead) and to avoid tuple creation
 */
// float32x4_t inline Voice::freqShift(float32_t fa, float32_t fb) const
// {
    // original code is:
    //     auto avg = (fa + fb) / 2.0;
    //     auto k = split + cos(avg) / 5.0; // add some pseudo random offset to coupling so that frequency couples are not in perfect sync
    //     auto w = avg + sqrt(pow((fa - fb) / 2.0, 2.0) + pow(k / 2.5, 2.0));
    //     return fabs(fmax(fa, fb) - w);
    //
    //  --- Let's do some refactoring ---
    // First of all, le'ts consider K = split, and let's have the approximation of the square root
    // to introduce the pseudo random offset instead of cos, since this function will be called 64+64 times
    // we want to make this as fast as possible
    // Some  refactoring of the return value is possible as:
    //     fmax(fa, fb) - w => fmax(fa, fb) - (avg + sqrt()) => fmax(fa, fb) - ((fa + fb) / 2.0 + sqrt())
    //     case a is greater => fa - fa/2 - fb/2 - sqrt() = fa/2 - fb/2 - sqrt()
    //     case b is greater => fb - fa/2 - fb/2 - sqrt() = fb/2 - fa/2 - sqrt()
    // both cases are covered by: abs(fa - fb)*0.5 - sqrt(...)

    // now let's expand again the square root:
    //   abs(fa - fb)*0.5 - sqrt( pow((fa - fb) / 2.0, 2.0) + pow(k / 2.5, 2.0) )
    //   abs(fa - fb)*0.5 - sqrt( ((fa - fb)*0.5)^2 + (k*0.4)^2 )
    //
    // let's consider dx = abs(fa - fb)*0.5 and dy = (k*0.4)
    //   dx - sqrt( dx^2 + dy^2 )
    // so fabs(fmax(fa, fb) - w) will be
    //   fabs( dx - sqrt( dx^2 + dy^2 ) )
    //
    // It's self evident that sqr(a^2+b^2) > a (in Real numbers realm), and let's consider only the positive solutions of the sqrt
    // so the absolute value is this:
    //   sqrt( dx^2 + dy^2 ) - dx
    //
    // now let's face the square root itself: it's an the euclidean distance
    // i.e. [sqr((y2-y1)^2 + (x2-x1)^2)] where dy=abs(y2-y1)
    // that can be approximated by (dy > dx) ? (0.4 * dx + 0.96 * dy) : (0.96 * dx + 0.4 * dy)
    // let's formulate like this:
    //     0.4*dx + 0.56*dx if (dx>dy) + 0.4*dy + 0.56*dy if (dy>dx)
    //     0.4*(dx+dy) + 0.56*(dx*(dx>dy) + dy*(dy>dx))
    //
    // So the overrall return value becomes:
    //     0.4*(dx+dy) + 0.56*(dx*(dx>dy) + dy*(dy>dx)) - dx
    //     0.4*dy - 0.6*dx + 0.56*max(dx,dy)
    // where dx=abs(fa - fb)*0.5 and dy=(k*0.4)
    //
    // let's go back to the caller => calcFrequencyShifts
}

/**
* When resonators are coupled in serial a frequency split is applied
* using the formula f +-= (fa + fb) / 2 + sqrt(((fa - fb) / 2)**2 + k**2) where k is the coupling strength
* Called by Voice::updateResonators()
*
* NOTE : semplification of freqShift() lead to put the equivalent code
* directly inside calcFrequencyShifts()
*
* NOTE 2 : MOVED COMPLETELY TO updateResonators !!!!
*/
//std::tuple<std::array<double, 64>, std::array<double,64>> Voice::calcFrequencyShifts(
//	std::array<double, 64>& aModel,
//	std::array<double, 64>& bModel
//) {
//	std::array<double, 64> aShifts = aModel;
//	std::array<double, 64> bShifts = bModel;
//
//	double fa, fb, shift;
//	for (int i = 0; i < 64; ++i) {
//		fa = aModel[i];
//		for (int j = 0; j < 64; ++j) {
//			fb = bModel[j];
//			if (fabs(fa - fb) <= 4.0) {
//				shift = freqShift(fa * freq, fb * freq) / freq;
//				aShifts[i] += fa > fb ? shift : -shift;
//				bShifts[i] += fa > fb ? -shift : shift;
//			}
//		}
//	}
//
//	return std::tuple<std::array<double,64>, std::array<double,64>> (aShifts, bShifts);
//}

//     // if coupling mode is serial apply frequency splitting (from updateResonators())
//     if (couple && resA.on && resB.on) {
//         float32_t fa, fb, dx, dy;
//         auto k = split * 0.4 / freq;
//         dy = k * 0.4 / freq;
//         int k_count = 0;
//         float32_t x_count = 0;
//         float32_t dx_max = 0;
//         int dy_max = 0;
//         for (int i = 0; i < 64; ++i) {
//             fa = aModel[i];
//             for (int j = 0; j < 64; ++j) {
//                 fb = bModel[j];
//                 // original code:
//                 // if (fabs(fa - fb) <= 4.0) {
//                 //     shift = freqShift(fa * freq, fb * freq) / freq;
//                 //     aShifts[i] += fa > fb ? shift : -shift;
//                 //     bShifts[i] -= fa > fb ? shift : -shift;
//                 // }
//                 //
//                 // the freqShift function outcome shall be (see above):
//                 //      0.4*dy - 0.6*dx + 0.56*max(dx,dy)
//                 // where dx=abs(fa - fb)*0.5 and dy=(k*0.4)
//                 // So if we substitute fa with fa*freq and fb with fb*freq
//                 // we will have dx=abs(fa - fb)*0.5*freq and dy=k*0.4
//                 // and shift is this result divided by freq
//                 // so dx=abs(fa - fb)*0.5 and dy=k*0.4/freq
//                 //
//                 // Let's a have a look to ashift and bshift: shift will be added or
//                 // subtracted in case fa>fb or fa<fb.
//                 // This means we can always add and either not change sign or change sign in those cases.
//                 // case fa>fb) no sign change:
//                 //         dy is already positive;
//                 //         dx is an absolute value, so it's already positive: dx=abs(fa - fb)*0.5 will be just (fa - fb)*0.5
//                 // case fa<fb) sign change:
//                 //         dy will be negative and dx must change the sign
//                 //         dx is an absolute value, to have a change of the sign, the content must be negative
//                 //   dx=abs(fa - fb)*0.5 will be just (fa - fb)*0.5, as we are in case 2 and fb is greater than fa
//                 // so dx is already fitting the case without any check! dy must have a check.
//                 //   aShifts[i] += 0.4*dy - 0.6*dx + 0.56*max(dx,dy)
//                 //   bShifts[i] -= 0.4*dy - 0.6*dx + 0.56*max(dx,dy)
//                 // where dx=(fa - fb)*0.5 and dy=k*0.4/freq * -1(fa<fb)
//                 //
//                 // The for loop will perform a summatory, where dy is constant in value and changes in sign,
//                 // like this: dy*How_may_times_fa_gt_fb - dy*How_may_times_fb_gt_fa
//                 // which is : dy*(How_may_times_fa_gt_fb-How_may_times_fb_gt_fa)
//                 // so we can just increase a counter if fa is greater than fb and decrease otherwise.
//                 // Similar is the max(dx,dy), but dx is not constant so actual sum is mandatory

//                 // Let's try to compute:
//                 dx = (fa - fb) * 0.5;
//                 if (fabs(dx) <= 2.0) {    // equal to fabs(fa - fb) <= 4.0)
//                     x_count += dx;
//                     if (dx > 0) {       // equal to fa > fb
//                         k_count++;      // positive dy
//                         if (dx > dy)
//                             dx_max += dx;
//                         else
//                             dy_max++;
//                     }
//                     else {
//                         k_count--;      // negative dy
//                         if (dx < -dy)
//                             dx_max += dx;  // dx is negative
//                         else
//                             dy_max--;
//                         }
//                 }   // end fabs
//             }   // end for j
//             auto freqShiftOpt = 0.4 * (dy * k_count) - 0.6 * x_count + 0.56 * dx_max + 0.56 * (dy * dy_max);
//             aShifts[i] += freqShiftOpt;
//             bShifts[i] -= freqShiftOpt;
//         }   // end for i
//     }   // end res ON
//     return std::tuple<std::array<float32_t, 64>, std::array<float32_t, 64>> (aShifts, bShifts);
// }


/* called by Voice::trigger and Voice::release, plus ::onSlider()
 *  NOTE 2 : in order to create seamless fusion of the above refactory
 * this function has been commented out and sustituted with new version below
 */
//void Voice::updateResonators()
//{
//	std::array<double,64> aModel = models.aModels[resA.nmodel];
//	std::array<double,64> bModel = models.bModels[resB.nmodel];
//	std::array<double, 64> aGain = models.getGains((ModalModels)resA.nmodel);
//	std::array<double, 64> bGain = models.getGains((ModalModels)resB.nmodel);
//
//	if (resA.nmodel == ModalModels::Djembe) {
//		aModel = models.calcDjembe(freq, a_ratio);
//	}
//	if (resB.nmodel == ModalModels::Djembe) {
//		bModel = models.calcDjembe(freq, b_ratio);
//	}
//
//	if (aPitchFactor != 1.0) applyPitch(aModel, aPitchFactor);
//	if (bPitchFactor != 1.0) applyPitch(bModel, bPitchFactor);
//
//	// if coupling mode is serial apply frequency splitting
//	if (couple && resA.on && resB.on) {
//		auto [aShifts, bShifts] = calcFrequencyShifts(aModel, bModel);
//		aModel = aShifts;
//		bModel = bShifts;
//	}
//
//	if (resA.on) resA.update(freq, vel, isRelease, pitchBend, aModel, aGain);
//	if (resB.on) resB.update(freq, vel, isRelease, pitchBend, bModel, bGain);
//}
/* called by Voice::trigger and Voice::release, plus ::onSlider()
 * see above original code, completly commented out in favour
 * of a new call which unrolls the calculate frequency shift call
 * check comments and refactoring steps on above commented code
 */
void Voice::updateResonators()
{
    std::array<float32_t, 64> aModel = models.getAModels()[resA.getModel()];
    std::array<float32_t, 64> bModel = models.getBModels()[resB.getModel()];

    if (aPitchFactor != 1.0f) applyPitch(aModel, aPitchFactor);
    if (bPitchFactor != 1.0f) applyPitch(bModel, bPitchFactor);

    // Initialize shifts AFTER pitch modifications
    std::array<float32_t, 64> aShifts = aModel;
    std::array<float32_t, 64> bShifts = bModel;

    if (couple && resA.isOn() && resB.isOn()) {
        // Precompute constants once
        const float32_t k = split * c_coupling_split_factor / freq;
        const float32_t dy = k * c_coupling_split_factor / freq;
        const float32_t threshold = c_coupling_threshold;

        // Constants for frequency shift formula
        const float32_t coeff_dy_04 = c_freq_shift_coeff_dy * dy;
        const float32_t coeff_dx_06 = c_freq_shift_coeff_dx;
        const float32_t coeff_max_56 = c_freq_shift_coeff_max;
        const float32_t coeff_dy_56 = c_freq_shift_coeff_max * dy;

        // NEON constant vectors (preload once)
        float32x4_t v_threshold = vdupq_n_f32(threshold);
        float32x4_t v_dy = vdupq_n_f32(dy);
        float32x4_t v_half = vdupq_n_f32(0.5f);
        float32x4_t v_coeff_dy_04 = vdupq_n_f32(coeff_dy_04);
        float32x4_t v_coeff_dx_06 = vdupq_n_f32(coeff_dx_06);
        float32x4_t v_coeff_max_56 = vdupq_n_f32(coeff_max_56);
        float32x4_t v_coeff_dy_56 = vdupq_n_f32(coeff_dy_56);
        float32x4_t v_zero = vdupq_n_f32(0.0f);

        // Process outer loop in tiles of 4
        constexpr int OUTER_TILE = 4;

        for (int i_tile = 0; i_tile < 64; i_tile += OUTER_TILE) {
            // Load 4 fa values for this tile
            float32x4_t fa_vec = vld1q_f32(&aModel[i_tile]);

            // Accumulators for this tile (one accumulator per fa)
            float32x4_t k_count_tile = vdupq_n_f32(0.0f);    // Sum of +1/-1 for each fa
            float32x4_t x_count_tile = vdupq_n_f32(0.0f);    // Sum of dx values
            float32x4_t dx_max_tile = vdupq_n_f32(0.0f);     // Sum of |dx| when |dx| > |dy|
            float32x4_t dy_max_tile = vdupq_n_f32(0.0f);     // Count when |dy| >= |dx|

            // Inner loop: process fb values in groups of 4
            for (int j_tile = 0; j_tile < 64; j_tile += OUTER_TILE) {
                float32x4_t fb_vec = vld1q_f32(&bModel[j_tile]);

                // Process each of 4 fb values against all 4 fa values - unrolled with constant indices
                for (int fb_idx = 0; fb_idx < OUTER_TILE; ++fb_idx) {
                    // Broadcast single fb value to all lanes using constant index
                    float32_t fb_scalar;
                    switch (fb_idx) {
                        case 0: fb_scalar = vgetq_lane_f32(fb_vec, 0); break;
                        case 1: fb_scalar = vgetq_lane_f32(fb_vec, 1); break;
                        case 2: fb_scalar = vgetq_lane_f32(fb_vec, 2); break;
                        case 3: fb_scalar = vgetq_lane_f32(fb_vec, 3); break;
                        default: fb_scalar = 0.0f;
                    }
                    float32x4_t fb_dup = vdupq_n_f32(fb_scalar);

                    // dx = (fa - fb) * 0.5
                    float32x4_t dx_vec = vmulq_f32(vsubq_f32(fa_vec, fb_dup), v_half);

                    // abs(dx) <= threshold check
                    float32x4_t abs_dx = vabsq_f32(dx_vec);
                    uint32x4_t valid_mask = vcleq_f32(abs_dx, v_threshold);

                    // Conditional operations only on valid elements
                    // ========================================

                    // 1. Accumulate x_count: sum all dx where valid
                    float32x4_t dx_masked = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(dx_vec), valid_mask));
                    x_count_tile = vaddq_f32(x_count_tile, dx_masked);

                    // 2. k_count: +1 if dx > 0, -1 if dx < 0 (only where valid)
                    uint32x4_t dx_positive = vcgtq_f32(dx_vec, v_zero);
                    uint32x4_t dx_negative = vcltq_f32(dx_vec, v_zero);

                    uint32x4_t active_positive = vandq_u32(valid_mask, dx_positive);
                    uint32x4_t active_negative = vandq_u32(valid_mask, dx_negative);

                    float32x4_t k_inc = vreinterpretq_f32_u32(active_positive);
                    float32x4_t k_dec = vreinterpretq_f32_u32(active_negative);

                    k_count_tile = vaddq_f32(k_count_tile, k_inc);
                    k_count_tile = vsubq_f32(k_count_tile, k_dec);

                    // 3. dx_max accumulation: sum |dx| where |dx| > |dy|
                    uint32x4_t abs_dx_gt_dy = vcgtq_f32(abs_dx, v_dy);
                    uint32x4_t active_dx_max = vandq_u32(valid_mask, abs_dx_gt_dy);

                    float32x4_t dx_contrib = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(dx_vec), active_dx_max));
                    dx_max_tile = vaddq_f32(dx_max_tile, dx_contrib);

                    // 4. dy_max accumulation: count where |dy| >= |dx|
                    uint32x4_t abs_dy_gte_dx = vcgeq_f32(v_dy, abs_dx);
                    uint32x4_t active_dy_max = vandq_u32(valid_mask, abs_dy_gte_dx);

                    float32x4_t dy_contrib = vreinterpretq_f32_u32(active_dy_max);
                    dy_max_tile = vaddq_f32(dy_max_tile, dy_contrib);
                }
            }

            // Final computation for this tile:
            // freqShiftOpt = 0.4*dy*k_count - 0.6*x_count + 0.56*dx_max + 0.56*dy*dy_max
            float32x4_t term1 = vmulq_f32(v_coeff_dy_04, k_count_tile);
            float32x4_t term2 = vmulq_f32(v_coeff_dx_06, x_count_tile);
            float32x4_t term3 = vmulq_f32(v_coeff_max_56, dx_max_tile);
            float32x4_t term4 = vmulq_f32(v_coeff_dy_56, dy_max_tile);

            float32x4_t freqShiftOpt_vec = vsubq_f32(vaddq_f32(term1, term3), term2);
            freqShiftOpt_vec = vaddq_f32(freqShiftOpt_vec, term4);

            // Apply frequency shifts to aShifts and bShifts
            float32x4_t aShifts_vec = vld1q_f32(&aShifts[i_tile]);
            float32x4_t bShifts_vec = vld1q_f32(&bShifts[i_tile]);

            vst1q_f32(&aShifts[i_tile], vaddq_f32(aShifts_vec, freqShiftOpt_vec));
            vst1q_f32(&bShifts[i_tile], vsubq_f32(bShifts_vec, freqShiftOpt_vec));
        }
    }

    if (resA.isOn()) resA.update(freq, vel, isRelease, aShifts);
    if (resB.isOn()) resB.update(freq, vel, isRelease, bShifts);
}
