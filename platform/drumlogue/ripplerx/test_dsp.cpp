#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// 1. Mock the Drumlogue OS structures
#include "../common/runtime.h"

uint8_t mock_get_num_sample_banks() { return 1; }
uint8_t mock_get_num_samples_for_bank(uint8_t bank) { (void)bank; return 1; }
const sample_wrapper_t* mock_get_sample(uint8_t bank, uint8_t index) { return nullptr; }

// 2. Define UT flags
#define UNIT_TEST_DEBUG
float ut_exciter_out = 0.0f;
float ut_delay_read = 0.0f;
float ut_voice_out = 0.0f;

#include "synth_engine.h"

int main() {
    std::cout << "--- STARTING RIPPLERX DSP UNIT TEST ---" << std::endl;

    RipplerXWaveguide synth;
    unit_runtime_desc_t desc = {0};
    desc.samplerate = 48000;
    desc.output_channels = 2;
    synth.Init(&desc);

    // [UT1/UT2 FIX TEST] - Load the Init patch to correctly set all filter states!
    synth.LoadPreset(0);
    // Tweak parameters for the test
    synth.setParameter(RipplerXWaveguide::k_paramDkay, 1500);
    synth.setParameter(RipplerXWaveguide::k_paramNzMix, 0);
    synth.setParameter(RipplerXWaveguide::k_paramGain, 50);

    std::cout << "Triggering NoteOn(60, 127)..." << std::endl;
    synth.NoteOn(60, 127);

    float frame_buffer[2] = {0.0f};

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Frame | Exciter In  | Delay Read  | Voice Out | Master L" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    for (int i = 0; i <= 185; ++i) {
        synth.processBlock(frame_buffer, 1);

        if (i < 2 || i >= 182) {
            std::cout << std::setw(5) << i << " | "
                      << std::setw(11) << ut_exciter_out << " | "
                      << std::setw(11) << ut_delay_read << " | "
                      << std::setw(9) << ut_voice_out << " | "
                      << std::setw(8) << frame_buffer[0] << std::endl;
        } else if (i == 2) {
            std::cout << "  ... | Sound wave is travelling down the delay line... " << std::endl;
        }
    }

    std::cout << "--- TEST COMPLETE ---" << std::endl;
    return 0;
}