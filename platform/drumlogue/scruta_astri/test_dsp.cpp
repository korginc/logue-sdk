#include <iostream>
#include <iomanip>

// Mock Drumlogue OS descriptors
#define UNIT_API_VERSION 0
#define UNIT_TARGET_PLATFORM 0
#define k_unit_module_synth 0
typedef struct { int samplerate; int output_channels; void* get_num_sample_banks; void* get_num_samples_for_bank; void* get_sample; } unit_runtime_desc_t;

// Include the engine
#include "synth.h"

int main() {
    std::cout << "--- STARTING SCRUTAASTRI DRONE TEST ---\n";

    ScrutaAstri synth;
    unit_runtime_desc_t desc = { 48000, 2, nullptr, nullptr, nullptr };
    synth.Init(&desc);

    // Set parameters
    synth.setParameter(ScrutaAstri::k_paramOsc1Wave, 0);
    synth.setParameter(ScrutaAstri::k_paramOsc2Wave, 1);
    synth.setParameter(ScrutaAstri::k_paramOsc2Mix, 50);

    // Test LFO 3 VCA Tremolo
    synth.setParameter(ScrutaAstri::k_paramL3Wave, 6); // LFO_EXP_DECAY
    synth.setParameter(ScrutaAstri::k_paramL3Rate, 0); // 0.01Hz (Very slow decay)
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

    std::cout << "--- TEST COMPLETE ---\n";
    return 0;
}