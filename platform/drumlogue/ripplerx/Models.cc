#include "Models.h"
#include "constants.h"
#include <arm_neon.h>

// NEON-optimized division by f0 for frequency to ratio conversion
// Uses reciprocal estimate for ~4x speedup over scalar division
static inline void freqs_to_ratio(std::array<float32_t, 64>& model)
{
    auto f0 = model[0];
    if (f0 == 0.0f) return;  // Safety check to prevent division by zero

    // Compute reciprocal using NEON reciprocal estimate + Newton-Raphson refinement
    float32x4_t f0_vec = vdupq_n_f32(f0);
    float32x4_t recip = vrecpeq_f32(f0_vec);  // Initial estimate
    recip = vmulq_f32(vrecpsq_f32(f0_vec, recip), recip);  // One N-R iteration for accuracy

    // Process 4 elements at a time: model[j] / f0 = model[j] * (1/f0)
    for (int j = 0; j < 64; j += 4) {
        float32x4_t model_vec = vld1q_f32(&model[j]);
        model_vec = vmulq_f32(model_vec, recip);
        vst1q_f32(&model[j], model_vec);
    }
}

// =======================================================
/* called by ::onSlider(), which do not exist on this porting.
ratio is not among the editable parameters, so these shall not be called unless
it's decided to make ratio parameter editable*/
void Models::recalcBeam(bool resA, float32_t ratio)
{
    std::array<float32_t, 64>& model = resA
        ? aModels[ModelNames::Beam]
        : bModels[ModelNames::Beam];

    int i = 0;
    for (int m = 1; m <= 8; ++m) {
        for (int n = 1; n <= 8; ++n, ++i) {
            // model[i] = sqrt(fasterpowf(m, 4.0) + fasterpowf(ratio * bFree[i], 4.0));
            model[i] = sqrtsum2acc(pwr_2_of_index[m], fasterpowf(ratio * bFree[i], 2.0));
        }
    }
    freqs_to_ratio(model);
}

void Models::recalcMembrane(bool resA, float32_t ratio)
{
    std::array<float32_t, 64>& model = resA
        ? aModels[ModelNames::Membrane]
        : bModels[ModelNames::Membrane];

    // Pre-compute ratio * n for n=1..8 using NEON (process in pairs)
    float32_t ratios[9];  // ratios[0] unused, ratios[1..8] for n=1..8
    float32x4_t ratio_vec = vdupq_n_f32(ratio);
    float32x4_t n_vec1 = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t n_vec2 = {5.0f, 6.0f, 7.0f, 8.0f};
    float32x4_t result1 = vmulq_f32(ratio_vec, n_vec1);
    float32x4_t result2 = vmulq_f32(ratio_vec, n_vec2);
    vst1q_f32(&ratios[1], result1);
    vst1q_f32(&ratios[5], result2);

    int i = 0;
    for (int m = 1; m <= 8; ++m) {
        for (int n = 1; n <= 8; ++n, ++i) {
            // model[i] = sqrt(fasterpowf(m, 2.0) + fasterpowf(ratio * n, 2.0));
            model[i] = sqrtsum2acc(m, ratios[n]);
        }
    }
    freqs_to_ratio(model);
}

void Models::recalcPlate(bool resA, float32_t ratio)
{
    std::array<float32_t, 64>& model = resA
        ? aModels[ModelNames::Plate]
        : bModels[ModelNames::Plate];

    // Pre-compute (ratio * n)^2 for n=1..8 using NEON
    float32_t ratio_sq[9];  // ratio_sq[0] unused, ratio_sq[1..8] for n=1..8
    float32x4_t ratio_vec = vdupq_n_f32(ratio);
    float32x4_t n_vec1 = {1.0f, 2.0f, 3.0f, 4.0f};
    float32x4_t n_vec2 = {5.0f, 6.0f, 7.0f, 8.0f};
    float32x4_t result1 = vmulq_f32(ratio_vec, n_vec1);
    float32x4_t result2 = vmulq_f32(ratio_vec, n_vec2);
    result1 = vmulq_f32(result1, result1);  // Square
    result2 = vmulq_f32(result2, result2);  // Square
    vst1q_f32(&ratio_sq[1], result1);
    vst1q_f32(&ratio_sq[5], result2);

    int i = 0;
    for (int m = 1; m <= 8; ++m) {
        for (int n = 1; n <= 8; ++n, ++i) {
            model[i] = pwr_2_of_index[m] + ratio_sq[n];
        }
    }
    freqs_to_ratio(model);
}
