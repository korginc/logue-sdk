#include <iostream>
#include <iomanip>
#include <cmath>

// Include real Drumlogue OS types before the engine
#include "unit.h"

// Include the engine
#include "synth.h"

int main() {
    std::cout << "--- STARTING SCRUTAASTRI DRONE TEST ---\n";

    ScrutaAstri synth;
    unit_runtime_desc_t desc = {};
    desc.samplerate = 48000;
    desc.output_channels = 2;
    synth.Init(&desc);

    // Set parameters
    synth.setParameter(ScrutaAstri::k_paramOsc1Wave, 0);
    synth.setParameter(ScrutaAstri::k_paramOsc2Wave, 1);
    synth.setParameter(ScrutaAstri::k_paramOsc2Mix, 50);

    // Test LFO 3 VCA Tremolo with EXP_DECAY (wave_type = LFO_EXP_DECAY = 5)
    synth.setParameter(ScrutaAstri::k_paramL3Wave, 5); // LFO_EXP_DECAY (was 6 — off by one, % 6 wrapped to 0)
    synth.setParameter(ScrutaAstri::k_paramL3Rate, 1); // Minimum rate (0 is below param range min of 1)
    synth.setParameter(ScrutaAstri::k_paramL3Depth, 100);

    // Crank the distortion
    synth.setParameter(ScrutaAstri::k_paramCMOSDist, 100);
    synth.setParameter(ScrutaAstri::k_paramMastrVol, 50);

    std::cout << "Triggering Drone at Note 36...\n";
    synth.NoteOn(36, 127);

    float frame_buffer[2] = {0.0f};

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Frame | Master Out L | Master Out R\n";
    std::cout << "-----------------------------------\n";

    // Run 20 frames of audio
    for (int i = 0; i < 20; ++i) {
        synth.processBlock(frame_buffer, 1);

        std::cout << std::setw(5) << i << " | "
                  << std::setw(12) << frame_buffer[0] << " | "
                  << std::setw(12) << frame_buffer[1] << "\n";
    }

    // Sanity checks
    std::cout << "\n--- SANITY CHECKS ---\n";

    // Check for NaN/Inf in output
    bool nan_detected = false;
    float check_buf[2] = {0.0f};
    for (int i = 0; i < 4800; ++i) {
        synth.processBlock(check_buf, 1);
        if (std::isnan(check_buf[0]) || std::isinf(check_buf[0])) {
            std::cout << "FAIL: NaN/Inf detected at frame " << i + 20 << "\n";
            nan_detected = true;
            break;
        }
    }
    if (!nan_detected) std::cout << "PASS: No NaN/Inf in 4820 frames\n";

    // Check output is bounded [-1, 1] (soft clipper should enforce this)
    synth.setParameter(ScrutaAstri::k_paramCMOSDist, 100);
    synth.setParameter(ScrutaAstri::k_paramMastrVol, 100);
    bool clip_detected = false;
    for (int i = 0; i < 4800; ++i) {
        synth.processBlock(check_buf, 1);
        if (std::fabs(check_buf[0]) > 1.0f) {
            std::cout << "FAIL: Output exceeds [-1,1] at frame " << i << ": " << check_buf[0] << "\n";
            clip_detected = true;
            break;
        }
    }
    if (!clip_detected) std::cout << "PASS: Output stays within [-1, 1]\n";

    // Check that high-freq filter modulation doesn't explode (stability fix test)
    std::cout << "\n--- FILTER STABILITY TEST (high LFO depth) ---\n";
    ScrutaAstri synth2;
    synth2.Init(&desc);
    synth2.setParameter(ScrutaAstri::k_paramF1Cutoff, 15000);
    synth2.setParameter(ScrutaAstri::k_paramL1Wave, 2);   // LFO_SAW_UP
    synth2.setParameter(ScrutaAstri::k_paramL1Rate, 50);  // mid-rate
    synth2.setParameter(ScrutaAstri::k_paramL1Depth, 100);
    synth2.NoteOn(60, 127);
    bool filter_exploded = false;
    for (int i = 0; i < 4800; ++i) {
        synth2.processBlock(check_buf, 1);
        if (std::isnan(check_buf[0]) || std::isinf(check_buf[0]) || std::fabs(check_buf[0]) > 100.0f) {
            std::cout << "FAIL: Filter exploded at frame " << i << ": " << check_buf[0] << "\n";
            filter_exploded = true;
            break;
        }
    }
    if (!filter_exploded) std::cout << "PASS: Filter stable with full LFO depth\n";

    std::cout << "\n--- TEST COMPLETE ---\n";
    return 0;
}
