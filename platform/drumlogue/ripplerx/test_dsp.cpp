#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// 1. Mock the Drumlogue OS structures
#include "../common/runtime.h"

// Mock implementations of runtime functions
uint8_t mock_get_num_sample_banks() {
    return 1;
}

uint8_t mock_get_num_samples_for_bank(uint8_t bank) {
    (void)bank;
    return 1;
}

const sample_wrapper_t* mock_get_sample(uint8_t bank, uint8_t index) {
    (void)bank;
    (void)index;
    // Return a null pointer or a mock sample_wrapper_t if needed
    return nullptr;
}

// 2. Define UT flags BEFORE including engine
#define UNIT_TEST_DEBUG
float ut_exciter_out = 0.0f;
float ut_delay_read = 0.0f;
float ut_voice_out = 0.0f;

#include "synth_engine.h"

// to test this:
// /mnt/d/Fede/drumlogue/arm-unknown-linux-gnueabihf/bin/arm-unknown-linux-gnueabihf-g++ -static -std=c++17 -O3 -I.. -I. -I../../common -I../common -DRUNTIME_COMMON_H_ test_dsp.cpp -o run_test && qemu-arm ./run_test | tee run_test_result.log
// Include your actual synth engine

int main() {
    std::cout << "--- STARTING RIPPLERX DSP UNIT TEST ---" << std::endl;

    const size_t sample_len = 48000;
    std::vector<float> dummy_sample(sample_len);
    for(size_t i = 0; i < sample_len; ++i) {
        dummy_sample[i] = sinf(2.0f * 3.14159265f * 440.0f * ((float)i / 48000.0f));
    }

    RipplerXWaveguide synth;
    unit_runtime_desc_t desc = {
        .target = k_unit_target_drumlogue_synth,
        .api = UNIT_API_VERSION,
        .samplerate = 48000,
        .frames_per_buffer = 1,
        .input_channels = 0,
        .output_channels = 2,
        .get_num_sample_banks = mock_get_num_sample_banks,
        .get_num_samples_for_bank = mock_get_num_samples_for_bank,
        .get_sample = mock_get_sample
    };
    synth.Init(&desc);

    synth.setParameter(RipplerXWaveguide::k_paramDkay, 1500);
    synth.setParameter(RipplerXWaveguide::k_paramNzMix, 0);
    synth.setParameter(RipplerXWaveguide::k_paramMlltStif, 5000);
    synth.setParameter(RipplerXWaveguide::k_paramGain, 50);

    // TRIGGER NOTE FIRST!
    std::cout << "Triggering NoteOn(60, 127)..." << std::endl;
    synth.NoteOn(60, 127);

    // CRITICAL FIX: Fetch the actual voice that NoteOn activated (Voice 1)
    uint8_t active_idx = synth.state.next_voice_idx;
    synth.state.voices[active_idx].exciter.sample_ptr = dummy_sample.data();
    synth.state.voices[active_idx].exciter.sample_frames = sample_len;
    synth.state.voices[active_idx].exciter.channels = 1;

    // RUN 200 FRAMES (Note 60 takes 183 frames to travel down the tube!)
    float frame_buffer[2] = {0.0f};

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Frame | Exciter In  | Delay Read  | Voice Out | Master L" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    for (int i = 0; i <= 185; ++i) {
        // Run the DSP audio thread for 1 sample
        synth.processBlock(frame_buffer, 1);

        // Print Frame 0 (Mallet Hit), Frame 1 (Sine Wave starts), and Frames 182-185 (Sound Exits Tube)
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