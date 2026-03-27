#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <cassert>

// ==============================================================================
// instructions fro Windows/WSL (NOTE do not remove these lines!!)
// /mnt/d/Fede/drumlogue/arm-unknown-linux-gnueabihf/bin/arm-unknown-linux-gnueabihf-g++ -static -std=c++17 -O3 -I.. -I. -I../../common -I../common -DRUNTIME_COMMON_H_ test_dsp.cpp -o run_test && qemu-arm ./run_test | tee run_test_result.log
// ==============================================================================


// 1. Mock the Drumlogue OS structures
#include "../common/runtime.h"

uint8_t mock_get_num_sample_banks() { return 1; }
uint8_t mock_get_num_samples_for_bank(uint8_t bank) { (void)bank; return 1; }
const sample_wrapper_t* mock_get_sample(uint8_t bank, uint8_t index) { return nullptr; }

// 2. Define UT flags BEFORE including engine
#define UNIT_TEST_DEBUG
float ut_exciter_out = 0.0f;
float ut_delay_read = 0.0f;
float ut_voice_out = 0.0f;

#include "synth_engine.h"

void run_active_test() {
    std::cout << "--- STARTING ACTIVE DSP UNIT TEST 1 ---\n";

    RipplerXWaveguide synth;
    unit_runtime_desc_t desc = {0};
    desc.samplerate = 48000;
    desc.output_channels = 2;
    synth.Init(&desc);

    synth.LoadPreset(10);

    std::cout << "Triggering NoteOn(60, 127)...\n";
    synth.NoteOn(60, 127);

    uint8_t active_idx = synth.state.next_voice_idx;

    // [UT-ASSERT 1]: Verify Delay Length Generation
    float delay_len = synth.state.voices[active_idx].resA.delay_length;
    if (delay_len <= 12.0f || delay_len > 4096.0f) {
        std::cerr << "\n[FATAL ERROR] Delay length is invalid: " << delay_len
                  << "\nReason: Korg SDK fasterpowf() may be failing on your PC compiler.\n";
        exit(1);
    }

    float frame_buffer[2] = {0.0f};
    bool exciter_fired = false;
    bool signal_emerged = false;

    std::cout << "Simulating 200 frames of audio...\n";

    for (int i = 0; i <= 200; ++i) {
        // SIMULATE DRUMLOGUE SEQUENCER: Short 1ms drum trigger!
        if (i == 48) {
            std::cout << "Frame 48: Sequencer sends GateOff()...\n";
            synth.GateOff();
        }

        synth.processBlock(frame_buffer, 1);

        if (i == 0 && ut_exciter_out > 1.0f) exciter_fired = true;
        if (i > 150 && std::abs(frame_buffer[0]) > 0.0001f) signal_emerged = true;

        // [UT-ASSERT 2]: Check for premature voice death (The Squelch Bug)
        if (i > 48 && i < 180 && !synth.state.voices[active_idx].is_active) {
            std::cerr << "\n[FATAL ERROR] Voice was prematurely killed at frame " << i
                      << " before sound emerged!\nReason: The Squelch logic detected silence while the wave was still travelling silently down the delay line.\n";
            exit(1);
        }

        // [UT-ASSERT 3]: Check for memory corruption
        if (std::isnan(frame_buffer[0]) || std::isinf(frame_buffer[0])) {
            std::cerr << "\n[FATAL ERROR] NaN / Infinity detected at output!\n";
            exit(1);
        }
    }

    if (!exciter_fired) {
        std::cerr << "\n[FATAL ERROR] Exciter never injected energy.\n";
        exit(1);
    }

    if (!signal_emerged) {
        std::cerr << "\n[FATAL ERROR] Sound never emerged from the master output.\n";
        exit(1);
    }

    std::cout << "\n--- 1st DIAGNOSTIC COMPLETE ---\n";
}

void run_active_test2() {
    std::cout << "--- STARTING DIAGNOSTIC DSP UNIT TEST 2 ---\n";

    RipplerXWaveguide synth;
    unit_runtime_desc_t desc = {0};
    desc.samplerate = 48000;
    desc.output_channels = 2;
    synth.Init(&desc);

    synth.LoadPreset(0);
    synth.setParameter(RipplerXWaveguide::k_paramDkay, 1500);
    synth.setParameter(RipplerXWaveguide::k_paramNzMix, 0);

    synth.NoteOn(60, 127);
    uint8_t active_idx = synth.state.next_voice_idx;
    VoiceState& v = synth.state.voices[active_idx];

    // DIAGNOSTIC X-RAY: Run exactly 1 frame of audio
    float frame_buffer[2] = {0.0f};
    synth.processBlock(frame_buffer, 1);

    std::cout << "\n[X-RAY 1: PITCH & TUNING]\n";
    std::cout << "Note 60 Delay Length: " << v.resA.delay_length << " (Should be ~183.47)\n";
    std::cout << "Read Pointer Offset : " << (float)v.resA.write_ptr - v.resA.delay_length << "\n";

    std::cout << "\n[X-RAY 2: EXCITERS & UI BINDING]\n";
    std::cout << "Mallet Stiffness  : " << v.exciter.mallet_stiffness << "\n";
    std::cout << "Mallet Res Coeff  : " << v.exciter.mallet_res_coeff << " (If 0.0, UI matrix is broken!)\n";
    std::cout << "Exciter Output    : " << ut_exciter_out << " (Should be ~15.0)\n";

    std::cout << "\n[X-RAY 3: WAVEGUIDE MEMORY]\n";
    std::cout << "Phase Multiplier  : " << v.resA.phase_mult << " (Should be 1.0 or -1.0)\n";
    std::cout << "Buffer[0] Memory  : " << v.resA.buffer[0] << " (If 0.0, the waveguide multiplied the Mallet by zero!)\n";

    std::cout << "\n--- 2nd DIAGNOSTIC COMPLETE ---\n";
    return;
}

int main() {
    run_active_test();
    run_active_test2();
    return 0;
}