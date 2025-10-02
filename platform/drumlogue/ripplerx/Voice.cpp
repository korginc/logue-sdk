#include "float_math.h"
#include "Voice.h"
#include "Models.h"

/* called by Voice::trigger*/
float32_t Voice::note2freq(int _note)
{
    return 440 * fasterpowf(2.0, (_note - 69) / 12.0);
}

// Set freq note, prepares mallet, noise generator, resonators, to be used at actual trigger
// Called by onNote(MIDIMsg msg)
void Voice::trigger(float32_t srate, int _note, float32_t _vel, float32_t malletFreq)
{
    resA.clear();
    resB.clear();
    note = _note;
    isRelease = false;
    isPressed = true;
    vel = _vel;
    freq = note2freq(note);
    mallet.trigger(kImpulse, srate, malletFreq);    // TODO merge version of Ripplerx from 1.5.0 onward
    noise.attack(_vel);
    if (resA.on) resA.activate();
    if (resB.on) resB.activate();
    updateResonators();
}

void Voice::release()
{
    isRelease = true;
    isPressed = false;
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
// TODO onSlider?
void Voice::setCoupling(bool _couple, float32_t _split) {
    couple = _couple;
    split = _split;
}

void Voice::setPitch(float32_t a_coarse, float32_t b_coarse, float32_t a_fine, float32_t b_fine)
{
    aPitchFactor = fasterpowf(2.0, (a_coarse + a_fine / 100.0) / 12.0);
    bPitchFactor = fasterpowf(2.0, (b_coarse + b_fine / 100.0) / 12.0);
}

void Voice::applyPitch(std::array<float32_t, 64>& model, float32_t factor)
{
    for (float32_t& ratio : model)
        ratio *= factor;
}

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
    //     case a => fa - fa/2 - fb/2 - sqrt() = fa/2 - fb/2 - sqrt()
    //     case b => fb - fa/2 - fb/2 - sqrt() = fb/2 - fa/2 - sqrt()
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
    // It's self evident that sqr(a^2+b^2) > a (in Real numbers realm), so the absolute value is this:
    //   sqrt( dx^2 + dy^2 ) - dx
    //
    // now let's face the square root: it's an the euclidean distance
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
// }

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
// std::tuple<std::array<float32_t, 64>, std::array<float32_t, 64>> Voice::calcFrequencyShifts()
// {
//     // load the model into local copy
//     std::array<float32_t, 64> aModel = models.aModels[resA.nmodel];
//     std::array<float32_t, 64> bModel = models.bModels[resB.nmodel];
//     // create the output with default value
//     std::array<float32_t, 64> aShifts = aModel;
//     std::array<float32_t, 64> bShifts = bModel;


//     for(size_t i = 0; i < 16; i += 4)
//     {
//         if (aPitchFactor != 1.0) applyPitch(aModel, aPitchFactor);
//         if (bPitchFactor != 1.0) applyPitch(bModel, bPitchFactor);
//     }

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
// void Voice::updateResonators()
// {
    // // std::array<float32_t, 64> aModel;
    // // std::array<float32_t, 64> bModel;

    // // if coupling mode is serial apply frequency splitting
    // // if (couple && resA.on && resB.on) {
    //     auto [aShifts, bShifts] = calcFrequencyShifts();    // this function will load the models and apply pitch. So let's put the coupling mode condtiion there
    //     // aModel = aShifts;
    //     // bModel = bShifts;
    // // }
    // // else {
    // //     aModel = aModels[resA.nmodel];
    // //     bModel = bModels[resB.nmodel];
    // //     if (aPitchFactor != 1.0) applyPitch(aModel, aPitchFactor);
    // //     if (bPitchFactor != 1.0) applyPitch(bModel, bPitchFactor);
    // // }

    // // if (resA.on) resA.update(freq, vel, isRelease, aModel);
    // // if (resB.on) resB.update(freq, vel, isRelease, bModel);
    // if (resA.on) resA.update(freq, vel, isRelease, aShifts);
    // if (resB.on) resB.update(freq, vel, isRelease, bShifts);
// }

/* called by Voice::trigger and Voice::release, plus ::onSlider()
 * see above original code, completly commented out in favour
 * of a new call which unrolls the calculate frquency shift call
 * check comments and refactoring steps on above commented code
 */
void Voice::updateResonators()
{

    // load the model into local copy
    std::array<float32_t, 64> aModel = models.aModels[resA.nmodel];
    std::array<float32_t, 64> bModel = models.bModels[resB.nmodel];
    // create the output with default value
    std::array<float32_t, 64> aShifts = aModel;
    std::array<float32_t, 64> bShifts = bModel;


    for(size_t i = 0; i < 16; i += 4)
    {
        if (aPitchFactor != 1.0) applyPitch(aModel, aPitchFactor);
        if (bPitchFactor != 1.0) applyPitch(bModel, bPitchFactor);
    }

    // if coupling mode is serial apply frequency splitting (from updateResonators())
    if (couple && resA.on && resB.on) {
        float32_t fa, fb, dx, dy;
        auto k = split * 0.4 / freq;
        dy = k * 0.4 / freq;
        int k_count = 0;
        float32_t x_count = 0;
        float32_t dx_max = 0;
        int dy_max = 0;
        for (int i = 0; i < 64; ++i) {
            fa = aModel[i];
            for (int j = 0; j < 64; ++j) {
                fb = bModel[j];
                dx = (fa - fb) * 0.5;
                if (fabs(dx) <= 2.0) {    // equal to fabs(fa - fb) <= 4.0)
                    x_count += dx;
                    if (dx > 0) {       // equal to fa > fb
                        k_count++;      // positive dy
                        if (dx > dy)
                            dx_max += dx;
                        else
                            dy_max++;
                    }
                    else {
                        k_count--;      // negative dy
                        if (dx < -dy)
                            dx_max += dx;  // dx is negative
                        else
                            dy_max--;
                        }
                }   // end fabs
            }   // end for j
            // equivalent ot square root of sum with split
            auto freqShiftOpt = 0.4 * (dy * k_count) - 0.6 * x_count + 0.56 * dx_max + 0.56 * (dy * dy_max);
            aShifts[i] += freqShiftOpt;
            bShifts[i] -= freqShiftOpt;
        }   // end for i
    }   // end res ON

    if (resA.on) resA.update(freq, vel, isRelease, aShifts);
    if (resB.on) resB.update(freq, vel, isRelease, bShifts);
}
