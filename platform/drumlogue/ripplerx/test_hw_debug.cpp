/**
 * @file test_hw_debug.cpp
 * @brief Diagnostic unit tests targeting the UT-passes / HW-silent gap.
 *
 * Compile (from the ripplerx/ directory):
 *   g++ -std=c++14 -O2 -DUNIT_TEST_DEBUG \
 *       -I../common -I. \
 *       -o test_hw_debug test_hw_debug.cpp -lm
 * Run:
 *   ./test_hw_debug
 *
 * Each test is independent and prints PASS/FAIL with a reason.
 * Silent-on-HW hypotheses covered:
 *   H1 – feedback_gain or mallet_stiffness left at zero by default
 *   H2 – Init preset has Gain=0, making master_drive only 1.0 (UT masked with Gain=50)
 *   H3 – Denormals: feedback ring decays into sub-normal range and is flushed to 0
 *   H4 – GateOn → NoteOn(m_ui_note, vel) path differs from UT's NoteOn(60, 127)
 *   H5 – Larger block size (32/64 frames) exposes a buffer-clear or indexing bug
 *   H6 – After unit_reset() the parameters are intact but delay buffers zeroed;
 *         subsequent GateOn must still produce sound
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <cstring>

// ── Mock Drumlogue OS runtime ────────────────────────────────────────────────
#include "../common/runtime.h"

uint8_t mock_get_num_sample_banks()                        { return 1; }
uint8_t mock_get_num_samples_for_bank(uint8_t)             { return 1; }
const sample_wrapper_t* mock_get_sample(uint8_t, uint8_t)  { return nullptr; }

// ── UT probe hooks ───────────────────────────────────────────────────────────
#define UNIT_TEST_DEBUG
float ut_exciter_out = 0.0f;
float ut_delay_read  = 0.0f;
float ut_voice_out   = 0.0f;

#include "synth_engine.h"

// ── Helpers ──────────────────────────────────────────────────────────────────

/** Build a unit_runtime_desc_t identical to what the Drumlogue OS provides. */
static unit_runtime_desc_t make_desc() {
    unit_runtime_desc_t d = {0};
    d.samplerate                = 48000;
    d.output_channels           = 2;
    d.get_num_sample_banks      = mock_get_num_sample_banks;
    d.get_num_samples_for_bank  = mock_get_num_samples_for_bank;
    d.get_sample                = mock_get_sample;
    return d;
}

static bool is_nan_or_inf(float v) {
    return std::isnan(v) || std::isinf(v);
}

/** Process N frames in blocks of block_size.  Returns max absolute sample seen. */
static float run_blocks(RipplerXWaveguide& s, int total_frames, int block_size = 32) {
    float buf[256] = {0.0f};  // Max block_size we test is 64 → 128 floats, 256 is safe
    float peak = 0.0f;
    for (int done = 0; done < total_frames; done += block_size) {
        int frames = std::min(block_size, total_frames - done);
        std::memset(buf, 0, sizeof(buf));
        s.processBlock(buf, (size_t)frames);
        for (int i = 0; i < frames * 2; ++i) {
            float a = std::fabsf(buf[i]);
            if (a > peak) peak = a;
        }
    }
    return peak;
}

/** Check left-channel of a single frame for nonzero; returns true if sound present. */
static float single_frame(RipplerXWaveguide& s) {
    float buf[2] = {0.0f, 0.0f};
    s.processBlock(buf, 1);
    return buf[0];
}

// ── Test runner bookkeeping ──────────────────────────────────────────────────
static int g_pass = 0, g_fail = 0;

static void result(const char* name, bool pass, const char* detail = "") {
    if (pass) {
        std::cout << "[PASS] " << name << "\n";
        ++g_pass;
    } else {
        std::cout << "[FAIL] " << name << "  ← " << detail << "\n";
        ++g_fail;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// T0 — Parameter audit (H1, H2)
//   After Init() + LoadPreset(0) – exactly what the HW does – print every
//   critical DSP coefficient and assert nothing is zero where it shouldn't be.
// ════════════════════════════════════════════════════════════════════════════
static void test_param_audit() {
    std::cout << "\n── T0: Parameter audit after Init + LoadPreset(0) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);
    // NO manual LoadPreset here — Init already calls it.

    // Access internal state through the public SynthState member
    const SynthState& st = s.state;

    // Print the values a developer would want to see on a logic analyser
    auto& v  = st.voices[1]; // voice 1 is allocated by the first NoteOn
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  master_gain          = " << st.master_gain    << "\n";
    std::cout << "  master_drive         = " << st.master_drive   << "\n";
    std::cout << "  mix_ab               = " << st.mix_ab         << "\n";
    std::cout << "  resA.feedback_gain   = " << st.voices[0].resA.feedback_gain  << "\n";
    std::cout << "  resA.lowpass_coeff   = " << st.voices[0].resA.lowpass_coeff  << "\n";
    std::cout << "  resA.ap_coeff        = " << st.voices[0].resA.ap_coeff       << "\n";
    std::cout << "  exciter.mallet_stiff = " << st.voices[0].exciter.mallet_stiffness << "\n";
    std::cout << "  exciter.mallet_res   = " << st.voices[0].exciter.mallet_res_coeff << "\n";
    std::cout << "  exciter.noise_decay  = " << st.voices[0].exciter.noise_decay_coeff << "\n";

    // H1: feedback_gain must not be zero – zero means silence after frame 0
    bool fg_ok = st.voices[0].resA.feedback_gain > 0.01f;
    result("T0a feedback_gain > 0.01 after LoadPreset(0)", fg_ok,
           "feedback_gain is zero – resonator will not sustain");

    // H2: master_drive at Gain=0 → 1.0f (unity, not muted).
    //     The UT hid this by setting Gain=50 → drive=11.0.
    //     Verify drive>=1.0 so the signal isn't attenuated.
    bool drv_ok = st.master_drive >= 1.0f;
    result("T0b master_drive >= 1.0 (Gain=0 preset gives unity, not mute)", drv_ok,
           "master_drive < 1 would attenuate signal below audibility");

    // master_gain should always be 1.0 (never changed by any parameter)
    bool mg_ok = std::fabsf(st.master_gain - 1.0f) < 1e-6f;
    result("T0c master_gain == 1.0 (never changed by UI)", mg_ok,
           "master_gain changed unexpectedly");
}

// ════════════════════════════════════════════════════════════════════════════
// T1 — HW boot sequence: Init → GateOn → processBlock in 32-frame blocks (H4, H5)
//   The UT uses NoteOn(60,127) + frame-by-frame loop.  The HW uses GateOn
//   + block-based render.  This test replicates the exact OS call order.
// ════════════════════════════════════════════════════════════════════════════
static void test_hw_boot_sequence() {
    std::cout << "\n── T1: HW boot sequence (GateOn + 32-frame blocks) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);
    // Hardware fires GateOn, NOT NoteOn directly
    s.GateOn(127);

    // Process 10 blocks of 32 frames (= 6.7 ms, well beyond delay-line roundtrip of ~3.8 ms)
    float buf[64] = {0.0f};
    float peak = 0.0f;
    std::cout << "  Block | L-peak\n";
    for (int b = 0; b < 10; ++b) {
        std::memset(buf, 0, sizeof(buf));
        s.processBlock(buf, 32);
        float blk_peak = 0.0f;
        for (int i = 0; i < 64; ++i) {
            float a = std::fabsf(buf[i]);
            if (a > blk_peak) blk_peak = a;
            if (a > peak) peak = a;
        }
        std::cout << "  " << std::setw(5) << b << " | " << blk_peak << "\n";
    }

    result("T1 GateOn + 32-frame blocks produces nonzero audio", peak > 1e-9f,
           "all 320 frames were zero after GateOn(127)");
}

// ════════════════════════════════════════════════════════════════════════════
// T2 — Default preset (no UT override): Dkay=250, Gain=0 (H1, H2)
//   The existing UT overrides k_paramDkay=1500 and k_paramGain=50.
//   This test uses the raw preset 0 values so we see what the hardware gets.
// ════════════════════════════════════════════════════════════════════════════
static void test_default_preset_no_override() {
    std::cout << "\n── T2: Default preset, no UT overrides (Dkay=250, Gain=0) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);
    s.NoteOn(60, 127);

    // Sample output at specific frames that bracket the delay-line roundtrip
    std::cout << "  Frame | L output\n";
    float peak = 0.0f;
    for (int i = 0; i < 400; ++i) {
        float v = single_frame(s);
        float a = std::fabsf(v);
        if (a > peak) peak = a;
        if (i < 5 || (i >= 180 && i <= 195) || i >= 395) {
            std::cout << "  " << std::setw(5) << i << " | " << std::setprecision(8) << v << "\n";
        } else if (i == 6) {
            std::cout << "  ... (travelling down delay line) ...\n";
        }
    }

    result("T2 preset-0 defaults produce nonzero output", peak > 1e-9f,
           "completely silent with default Dkay=250 / Gain=0 – hardware would be mute");
}

// ════════════════════════════════════════════════════════════════════════════
// T3 — Denormal / flush-to-zero decay (H3)
//   ARM Cortex-A7 NEON typically runs with FTZ.  If the feedback ring decays
//   into the sub-normal float range (~1e-38), those samples become exactly
//   0.0.  A sustained note should stay audible for at least 100 ms (4800 frames).
// ════════════════════════════════════════════════════════════════════════════
static void test_denormal_decay() {
    std::cout << "\n── T3: Denormal / FTZ decay check (100 ms of sustained sound) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);
    // Use the default preset (Dkay=250 → feedback_gain≈0.869) but hold the gate open.
    s.NoteOn(60, 127);

    float buf[64] = {};
    bool still_audible_at_50ms  = false;
    bool still_audible_at_100ms = false;
    bool nan_or_inf_detected    = false;

    // 100 ms = 4800 frames; process in 32-frame blocks
    for (int frame = 0; frame < 4800; frame += 32) {
        std::memset(buf, 0, sizeof(buf));
        s.processBlock(buf, 32);
        for (int i = 0; i < 64; ++i) {
            float a = std::fabsf(buf[i]);
            if (is_nan_or_inf(buf[i])) nan_or_inf_detected = true;
            if (a > 1e-9f) {
                if (frame >= 2400) still_audible_at_50ms  = true;
                if (frame >= 4768) still_audible_at_100ms = true;
            }
        }
    }

    result("T3a no NaN/Inf in 100 ms of output", !nan_or_inf_detected,
           "NaN or Inf detected – would be clamped to 0.99 by limiter, masking silence");
    result("T3b signal > 1e-9 at 50 ms (feedback_gain=0.869 not decaying into denormals)",
           still_audible_at_50ms,
           "signal flushed to zero before 50 ms – FTZ/denormal issue on hardware");
    result("T3c signal > 1e-9 at 100 ms",
           still_audible_at_100ms,
           "signal gone before 100 ms – sound may be inaudible before DAC transduces it");
}

// ════════════════════════════════════════════════════════════════════════════
// T4 — GateOn / GateOff cycle (H4)
//   Verifies the release envelope fades rather than hard-zeros the signal,
//   and that a second GateOn after GateOff produces sound.
// ════════════════════════════════════════════════════════════════════════════
static void test_gate_on_off_cycle() {
    std::cout << "\n── T4: GateOn → 250 frames → GateOff → 250 frames → GateOn again ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    s.GateOn(127);
    float peak_during  = run_blocks(s, 250, 32);

    s.GateOff();
    float peak_release = run_blocks(s, 250, 32);

    s.GateOn(127);
    float peak_second  = run_blocks(s, 250, 32);

    std::cout << "  peak_during (first gate):  " << peak_during  << "\n";
    std::cout << "  peak_release (after off):  " << peak_release << "\n";
    std::cout << "  peak_second (second gate): " << peak_second  << "\n";

    result("T4a first GateOn produces sound",   peak_during  > 1e-9f,
           "no audio during first gate hold");
    result("T4b GateOff enters release (not hard zero)", peak_release > 1e-9f,
           "signal hard-zeroed on GateOff – release not working; hardware would click");
    result("T4c second GateOn also produces sound", peak_second > 1e-9f,
           "re-trigger after GateOff produces no sound – voice state corrupt");
}

// ════════════════════════════════════════════════════════════════════════════
// T5 — Block-size sensitivity (H5)
//   Drumlogue may pass different block sizes.  Verify output is nonzero for
//   1, 16, 32 and 64 frames per processBlock call.
// ════════════════════════════════════════════════════════════════════════════
static void test_block_sizes() {
    std::cout << "\n── T5: Block-size sensitivity (1, 16, 32, 64 frames) ──\n";

    int sizes[] = {1, 16, 32, 64};
    for (int sz : sizes) {
        unit_runtime_desc_t desc = make_desc();
        RipplerXWaveguide s;
        s.Init(&desc);
        s.GateOn(127);
        // Process enough frames to complete at least one delay-line round trip (~184 frames for C4)
        float peak = run_blocks(s, 400, sz);
        char label[64];
        std::snprintf(label, sizeof(label), "T5 block_size=%d produces nonzero audio", sz);
        result(label, peak > 1e-9f, "silent at this block size");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// T6 — unit_reset() does not kill subsequent GateOn (H6)
//   The OS calls unit_reset() between patterns.  It wipes delay buffers and
//   is_active flags but must not zero feedback_gain / mallet_stiffness.
// ════════════════════════════════════════════════════════════════════════════
static void test_reset_then_gate_on() {
    std::cout << "\n── T6: unit_reset() then GateOn still produces audio ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Simulate OS: fire one note, then reset (e.g. pattern change), then play again
    s.GateOn(127);
    run_blocks(s, 64, 32);  // partial play

    s.Reset();              // unit_reset() — wipes buffers, keeps parameters

    s.GateOn(127);
    float peak = run_blocks(s, 400, 32);

    std::cout << "  peak after Reset + GateOn: " << peak << "\n";
    result("T6 post-Reset GateOn produces sound", peak > 1e-9f,
           "Reset() wiped a critical DSP parameter (feedback_gain / mallet_stiffness)");
}

// ════════════════════════════════════════════════════════════════════════════
// T7 — Preset sweep: every preset must produce nonzero audio on first GateOn
//   If a preset has a zero-killing default (e.g. all-zero noise with Gain=0
//   AND zero mallet somehow), this catches it.
// ════════════════════════════════════════════════════════════════════════════
static void test_all_presets_audible() {
    std::cout << "\n── T7: All 28 presets produce nonzero audio ──\n";

    bool any_fail = false;
    for (int p = 0; p < 28; ++p) {
        unit_runtime_desc_t desc = make_desc();
        RipplerXWaveguide s;
        s.Init(&desc);
        s.LoadPreset((uint8_t)p);
        s.GateOn(127);
        float peak = run_blocks(s, 500, 32);
        if (peak < 1e-9f) {
            std::cout << "  [SILENT] preset " << p << " (" << RipplerXWaveguide::getPresetName(p) << ")"
                      << "  peak=" << peak << "\n";
            any_fail = true;
        }
    }
    result("T7 all 28 presets audible on first GateOn", !any_fail,
           "one or more presets produce no audio – hardware would be silent on those presets");
}

// ════════════════════════════════════════════════════════════════════════════
// T8 — Voice allocation: NoteOn activates a voice that processBlock can see
//   Regression for next_voice_idx advancing before indexing (voice 0 skipped).
// ════════════════════════════════════════════════════════════════════════════
static void test_voice_allocation() {
    std::cout << "\n── T8: Voice allocation – first NoteOn activates a processed voice ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Count active voices before NoteOn
    int active_before = 0;
    for (int i = 0; i < 4; ++i)
        if (s.state.voices[i].is_active) ++active_before;

    s.NoteOn(60, 127);

    int active_after = 0;
    int active_idx = -1;
    for (int i = 0; i < 4; ++i) {
        if (s.state.voices[i].is_active) { ++active_after; active_idx = i; }
    }

    std::cout << "  active voices before: " << active_before << "\n";
    std::cout << "  active voices after:  " << active_after  << "\n";
    std::cout << "  active voice index:   " << active_idx    << "\n";

    result("T8a exactly 1 voice active after NoteOn",
           active_before == 0 && active_after == 1,
           "unexpected voice count");

    // Verify that voice is within the range processBlock iterates
    result("T8b active voice index in [0,3]",
           active_idx >= 0 && active_idx < 4,
           "voice index out of range");
}

// ════════════════════════════════════════════════════════════════════════════
// T9 — Delay-line round-trip (THE root cause from run_test_result.log)
//
//   The existing UT log (run_test_result.log) shows:
//     Frame 0:   Exciter = 15.0  →  Delay Read = 0.000  ← exciter fired
//     Frames 182-185: Delay Read = 0.000 STILL zero after full round trip!
//   This means the signal enters the waveguide but never comes back.
//
//   Most likely cause: resA.delay_length = 0 after NoteOn, so read_ptr ==
//   write_ptr — always reading the slot that is *about* to be written
//   (still zero from last frame).  Zero delay_length → permanent silence.
//
//   This test:
//     a) Asserts delay_length is sane (> 10 samples, < 4096) after NoteOn
//     b) Asserts that after exactly ceil(delay_length) frames the delay
//        line returns a nonzero value (the round-trip echo).
// ════════════════════════════════════════════════════════════════════════════
static void test_delay_roundtrip() {
    std::cout << "\n── T9: Delay-line round-trip echo (root cause from run_test_result.log) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);
    s.NoteOn(60, 127);

    // Inspect the allocated voice's delay_length directly.
    // NoteOn increments next_voice_idx before assigning, so the active voice is
    // at index (initial 0 +1) = 1.
    const WaveguideState& resA = s.state.voices[1].resA;
    float dl = resA.delay_length;
    std::cout << "  resA.delay_length after NoteOn(60) = " << dl << " samples\n";
    std::cout << "  Expected for C4 @ 48kHz            ≈ 183.5 samples\n";

    bool dl_sane = dl > 10.0f && dl < 4090.0f;
    result("T9a delay_length in [10, 4090] after NoteOn(60)", dl_sane,
           "delay_length is 0 or huge – waveguide permanently silent (matches log)");

    if (!dl_sane) {
        std::cout << "  *** SKIP round-trip check because delay_length=" << dl << " ***\n";
        return;
    }

    // Process ceil(delay_length)+2 frames one at a time and capture outA via
    // the ut_delay_read hook.  At least one frame after the roundtrip must be
    // nonzero.
    int roundtrip = (int)dl + 4;   // a few extra frames for the fractional part
    float max_delay_read = 0.0f;
    for (int i = 0; i <= roundtrip; ++i) {
        float buf[2] = {};
        s.processBlock(buf, 1);
        float a = std::fabsf(ut_delay_read);
        if (a > max_delay_read) max_delay_read = a;
    }

    std::cout << "  max |ut_delay_read| over " << roundtrip+1
              << " frames = " << max_delay_read << "\n";
    result("T9b delay line returns nonzero after one round trip", max_delay_read > 1e-4f,
           "delay line always reads zero – same symptom as run_test_result.log silent HW");
}

// ════════════════════════════════════════════════════════════════════════════
int main() {
    std::cout << "=== RIPPLERX HW-DEBUG UNIT TESTS ===\n";
    std::cout << "Testing HW-vs-UT discrepancies that could cause hardware silence.\n";

    test_param_audit();
    test_hw_boot_sequence();
    test_default_preset_no_override();
    test_denormal_decay();
    test_gate_on_off_cycle();
    test_block_sizes();
    test_reset_then_gate_on();
    test_all_presets_audible();
    test_voice_allocation();

    std::cout << "\n=== RESULTS: " << g_pass << " passed, " << g_fail << " failed ===\n";
    return g_fail == 0 ? 0 : 1;
}
