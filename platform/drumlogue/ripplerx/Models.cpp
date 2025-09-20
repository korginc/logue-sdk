#include "Models.h"
#include "constants.h"

// no arm neon 32bit optimization for division exist
static inline void freqs_to_ratio(std::array<float32_t, 64>& model)
{
    auto f0 = model[0];
    for (int j = 0; j < 64; ++j) {
        model[j] = model[j] / f0; // freqs to ratio
    }
}

// =======================================================
/* TODO called by ::onSlider(), which do not exist on this porting.
ration is not among the editable parameters, so these shall not be called unless
it's decided to make ratio parameter editable*/
void Models::recalcBeam(bool resA, float32_t ratio)
{
    std::array<float32_t, 64>& model = resA
        ? aModels[ModelNames::Beam]
        : bModels[ModelNames::Beam];

    int i = 0;
    for (int m = 1; m <= 8; ++m) {
        for (int n = 1; n <= 8; ++n) {
            // model[i] = sqrt(fasterpowf(m, 4.0) + fasterpowf(ratio * bFree[i], 4.0));
            model[i] = sqrtsum2acc(pwr_2_of_index[m], fasterpowf(ratio * bFree[i], 2.0));
            i += 1;
        }
    }
    freqs_to_ratio(model);
}

void Models::recalcMembrane(bool resA, float32_t ratio)
{
    std::array<float32_t, 64>& model = resA
        ? aModels[ModelNames::Membrane]
        : bModels[ModelNames::Membrane];

    for (int n = 1; n <= 8; ++n) {
        new_ratio[n] = ratio * n;
    }
    int i = 0;
    for (int m = 1; m <= 8; ++m) {
        for (int n = 1; n <= 8; ++n) {
            // model[i] = sqrt(fasterpowf(m, 2.0) + fasterpowf(ratio * n, 2.0));
            model[i] = sqrtsum2acc(m, new_ratio[n]);
            i += 1;
        }
    }
    freqs_to_ratio(model);
}

void Models::recalcPlate(bool resA, float32_t ratio)
{
    std::array<float32_t, 64>& model = resA
        ? aModels[ModelNames::Plate]
        : bModels[ModelNames::Plate];

    for (int n = 1; n <= 8; ++n) {
        new_ratio[n] = fasterpow2f(ratio * n);
    }
    int i = 0;
    for (int m = 1; m <= 8; ++m) {
        for (int n = 1; n <= 8; ++n) {
            model[i] = pwr_2_of_index[m] + new_ratio[n];
            i += 1;
        }
    }
    freqs_to_ratio(model);
}
