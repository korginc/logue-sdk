#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// 1. Mock the Drumlogue OS structures so your header compiles on PC
#define UNIT_API_VERSION 0
#define UNIT_TARGET_PLATFORM 0
#define k_unit_module_synth 0
typedef struct { int samplerate; int output_channels; void* get_num_sample_banks; void* get_num_samples_for_bank; void* get_sample; } unit_runtime_desc_t;
typedef void* unit_runtime_get_num_sample_banks_ptr;
typedef void* unit_runtime_get_num_samples_for_bank_ptr;
typedef void* unit_runtime_get_sample_ptr;

// 2. Include your actual synth engine!
#include "synth_engine.h"

int main() {
    std::cout << "--- STARTING RIPPLERX DSP UNIT TEST ---" << std::endl;

    // 3. Create a Dummy Sine Wave Sample (440 Hz)
    const size_t sample_len = 48000;
    std::vector<float> dummy_sample(sample_len);
    for(size_t i = 0; i < sample_len; ++i) {
        dummy_sample[i] = sinf(2.0f * 3.14159265f * 440.0f * ((float)i / 48000.0f));
    }

    // 4. Instantiate the Synth
    RipplerXWaveguide synth;
    unit_runtime_desc_t desc = { 48000, 2, nullptr, nullptr, nullptr };
    synth.Init(&desc);

    // 5. Set up a basic preset (Init Patch)
    synth.setParameter(RipplerXWaveguide::k_paramDkay, 1500); // 75% Decay
    synth.setParameter(RipplerXWaveguide::k_paramNzMix, 0);   // No Noise
    synth.setParameter(RipplerXWaveguide::k_paramMlltStif, 5000); // Max Mallet
    synth.setParameter(RipplerXWaveguide::k_paramGain, 50);   // Mild Overdrive

    // 6. Manually load the dummy sample into Voice 0
    synth.state.voices[0].exciter.sample_ptr = dummy_sample.data();
    synth.state.voices[0].exciter.sample_frames = sample_len;
    synth.state.voices[0].exciter.channels = 1;

    // 7. Trigger the Note (MIDI 60 = Middle C, Max Velocity)
    std::cout << "Triggering NoteOn(60, 127)..." << std::endl;
    synth.NoteOn(60, 127);

    // 8. Run 10 frames of audio and print the signal path
    float audio_buffer[20] = {0.0f}; // 10 frames * 2 (Stereo)

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Frame | Exciter Out | ResA Delay  | ResA Filter | Voice Out | Master L" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    for (int i = 0; i < 10; ++i) {
        // We will do a micro-step of processBlock to capture internal state
        VoiceState& v = synth.state.voices[0];

        float exciter_sig = 0.0f;
        float delay_read = 0.0f;
        float filter_out = 0.0f;
        float voice_out = 0.0f;

        if (v.is_active) {
            // Replicate process_exciter
            exciter_sig = v.exciter.sample_ptr[v.exciter.current_frame];
            float mallet_imp = (v.exciter.current_frame == 0) ? 1.0f : 0.0f;
            v.exciter.mallet_lp = (mallet_imp * v.exciter.mallet_stiffness) + (v.exciter.mallet_lp * (1.0f - v.exciter.mallet_stiffness));
            exciter_sig += v.exciter.mallet_lp * 15.0f;
            v.exciter.current_frame++;

            // Replicate process_waveguide (Res A only for logs)
            float read_idx = (float)v.resA.write_ptr - v.resA.delay_length;
            while (read_idx < 0.0f) read_idx += (float)DELAY_BUFFER_SIZE;
            uint32_t idx_A = ((uint32_t)read_idx) & DELAY_MASK;
            uint32_t idx_B = (idx_A + 1) & DELAY_MASK;
            float frac = read_idx - (float)((uint32_t)read_idx);

            delay_read = (v.resA.buffer[idx_A] * (1.0f - frac)) + (v.resA.buffer[idx_B] * frac);
            v.resA.z1 = (delay_read * v.resA.lowpass_coeff) + (v.resA.z1 * (1.0f - v.resA.lowpass_coeff));
            filter_out = v.resA.z1;

            float new_val = exciter_sig + (filter_out * v.resA.feedback_gain);
            v.resA.buffer[v.resA.write_ptr] = new_val;
            v.resA.write_ptr = (v.resA.write_ptr + 1) & DELAY_MASK;

            voice_out = delay_read * v.current_velocity;
#ifdef ENABLE_PHASE_5_EXCITERS
            voice_out *= v.exciter.master_env.process();
#endif
        }

        // Run the actual block to get Master L
        float frame_buffer[2] = {0.0f};
        synth.processBlock(frame_buffer, 1);

        std::cout << std::setw(5) << i << " | "
                  << std::setw(11) << exciter_sig << " | "
                  << std::setw(11) << delay_read << " | "
                  << std::setw(11) << filter_out << " | "
                  << std::setw(9) << voice_out << " | "
                  << std::setw(8) << frame_buffer[0] << std::endl;
    }

    std::cout << "--- TEST COMPLETE ---" << std::endl;
    return 0;
}