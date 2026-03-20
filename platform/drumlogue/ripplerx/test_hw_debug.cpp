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
            float a = std::fabs(buf[i]);
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

    // Print voice[0] DSP coefficients — LoadPreset applies setParameter to all voices
    // symmetrically, so every voice holds the same values after Init. No NoteOn needed.
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
    bool mg_ok = std::fabs(st.master_gain - 1.0f) < 1e-6f;
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
            float a = std::fabs(buf[i]);
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
        float a = std::fabs(v);
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
            float a = std::fabs(buf[i]);
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
        // 1500 frames covers the worst-case round-trip for the lowest preset note
        // (note 35 / B1 ≈ 870-sample delay with fasterpowf approximation on x86).
        float peak = run_blocks(s, 1500, 32);
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
    // NoteOn advances next_voice_idx before assigning, so the active voice is
    // always at that index — no hardcoded assumptions about initial state.
    uint8_t active_idx = s.state.next_voice_idx;
    const WaveguideState& resA = s.state.voices[active_idx].resA;
    float dl = resA.delay_length;
    std::cout << "  resA.delay_length after NoteOn(60) = " << dl << " samples\n";
    std::cout << "  Raw C4 period @ 48 kHz = 183.47 samples; after LP+AP compensation (~2 samples) ≈ 181.5\n";

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
        float a = std::fabs(ut_delay_read);
        if (a > max_delay_read) max_delay_read = a;
    }

    std::cout << "  max |ut_delay_read| over " << roundtrip+1
              << " frames = " << max_delay_read << "\n";
    result("T9b delay line returns nonzero after one round trip", max_delay_read > 1e-4f,
           "delay line always reads zero – same symptom as run_test_result.log silent HW");
}

// ════════════════════════════════════════════════════════════════════════════
// T10 — Dedicated Noise SVF (NzFltr / NzFltFrq parameter routing)
//   A structural test: verify that setParameter() routes NzFltr to
//   noise_filter.mode and NzFltFrq to noise_filter.f on all four voices.
//   This directly tests the previously unmapped parameter cases without
//   relying on audio amplitude measurements that can overflow the resonator
//   feedback loop at high noise injection levels.
// ════════════════════════════════════════════════════════════════════════════
static void test_noise_svf() {
    std::cout << "\n── T10: NzFltr/NzFltFrq route to per-voice noise_filter ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // NzFltr=0 → LP mode on all four voices
    s.setParameter(RipplerXWaveguide::k_paramNzFltr, 0);
    bool all_lp = true;
    for (int i = 0; i < 4; ++i)
        if (s.state.voices[i].exciter.noise_filter.mode != 0) all_lp = false;

    // NzFltFrq: higher cutoff → larger SVF f-coefficient (Chamberlin formula)
    s.setParameter(RipplerXWaveguide::k_paramNzFltFrq, 5000);
    float f_5kHz = s.state.voices[0].exciter.noise_filter.f;
    s.setParameter(RipplerXWaveguide::k_paramNzFltFrq, 200);
    float f_200Hz = s.state.voices[0].exciter.noise_filter.f;

    // NzFltr=2 → HP mode on all four voices
    s.setParameter(RipplerXWaveguide::k_paramNzFltr, 2);
    bool all_hp = true;
    for (int i = 0; i < 4; ++i)
        if (s.state.voices[i].exciter.noise_filter.mode != 2) all_hp = false;

    std::cout << "  NzFltr=0 LP mode on all voices : " << (all_lp ? "yes" : "no") << "\n";
    std::cout << "  f@5kHz=" << std::setprecision(4) << f_5kHz
              << "  f@200Hz=" << f_200Hz << "  (higher freq → higher f)\n";
    std::cout << "  NzFltr=2 HP mode on all voices : " << (all_hp ? "yes" : "no") << "\n";

    result("T10a NzFltr=0 sets noise_filter.mode=LP(0) on all 4 voices",
           all_lp,
           "NzFltr not routing to noise_filter.mode — parameter still falls to default");
    result("T10b NzFltFrq=5000 gives larger f-coeff than NzFltFrq=200",
           f_5kHz > f_200Hz,
           "NzFltFrq not routing to noise_filter.set_coeffs");
    result("T10c NzFltr=2 sets noise_filter.mode=HP(2) on all 4 voices",
           all_hp,
           "NzFltr HP mode not routing correctly");
}

// ════════════════════════════════════════════════════════════════════════════
// T11 — TubRad interacts with Mterl to increase lowpass_coeff
//   With TubRad=0 the coefficient is determined by Mterl alone.
//   TubRad=20 (widest tube) must pull the coefficient towards 1.0 (less loss).
// ════════════════════════════════════════════════════════════════════════════
static void test_tubrad_mterl() {
    std::cout << "\n── T11: TubRad combines with Mterl to brighten lowpass_coeff ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Target ResA (default context after Init)
    s.setParameter(RipplerXWaveguide::k_paramMterl,  10); // mid material
    s.setParameter(RipplerXWaveguide::k_paramTubRad,  0); // narrow tube (no adjustment)
    float coeff_narrow = s.state.voices[0].resA.lowpass_coeff;

    s.setParameter(RipplerXWaveguide::k_paramTubRad, 20); // widest tube
    float coeff_wide   = s.state.voices[0].resA.lowpass_coeff;

    std::cout << "  lowpass_coeff TubRad=0  : " << coeff_narrow << "\n";
    std::cout << "  lowpass_coeff TubRad=20 : " << coeff_wide   << "\n";

    result("T11a TubRad=20 raises lowpass_coeff above TubRad=0",
           coeff_wide > coeff_narrow,
           "TubRad had no effect on lowpass_coeff");
    result("T11b TubRad=20 lowpass_coeff < 1.0 (damping still present)",
           coeff_wide < 1.0f,
           "TubRad pushed coeff to 1.0 — all high-frequency damping lost");
}

// ════════════════════════════════════════════════════════════════════════════
// T12 — Partls coupling cross-feeds resonators
//   Partls=0 → ResB inactive, no coupling.  Partls=4 → ResB active, full
//   coupling (ResA←ResB feedback, ResB←ResA feed-forward).  Both must produce
//   audible output; the dual-resonator path must also sustain longer.
// ════════════════════════════════════════════════════════════════════════════
static void test_partls_coupling() {
    std::cout << "\n── T12: Partls coupling — ResA only vs dual with cross-feed ──\n";

    auto run_preset_with_partls = [](int partls_val) -> float {
        unit_runtime_desc_t desc = make_desc();
        RipplerXWaveguide s;
        s.Init(&desc);
        s.setParameter(RipplerXWaveguide::k_paramPartls, partls_val);
        s.GateOn(127);
        return run_blocks(s, 400, 32);
    };

    float peak_resA_only = run_preset_with_partls(0); // Partls=0: 4 partials, ResB off
    float peak_dual      = run_preset_with_partls(4); // Partls=4: 64 partials, ResB + full coupling
    std::cout << "  peak Partls=0 (ResA only) : " << peak_resA_only << "\n";
    std::cout << "  peak Partls=4 (dual+coupling) : " << peak_dual  << "\n";

    result("T12a Partls=0 (ResA only) produces sound",
           peak_resA_only > 1e-9f, "ResA-only mode is silent");
    result("T12b Partls=4 (ResB + coupling) produces sound",
           peak_dual > 1e-9f, "Dual-resonator coupling mode is silent");
}

// ════════════════════════════════════════════════════════════════════════════
// T13 — Tone tilt EQ: Tone=30 (bright) vs Tone=-10 (dark)
//   Positive Tone boosts the highpass component; negative Tone blends towards
//   the lowpass component.  A percussive mallet has bright attack energy, so
//   Tone=30 must produce a higher peak than Tone=-10.
// ════════════════════════════════════════════════════════════════════════════
static void test_tone_eq() {
    std::cout << "\n── T13: Tone tilt EQ — bright vs dark ──\n";

    // The master limiter clips all output to ±0.99, hiding EQ differences.
    // Instead, capture ut_voice_out — the post-Tone-EQ signal BEFORE master drive
    // and limiter — by running one frame at a time.
    //
    // At the delay-line round-trip (~190 frames), tone_lp ≈ 0 (tracking zero for
    // 190 frames), so the entire signal is "high frequency" from the tilt EQ's
    // perspective.  Tone=30 boosts that component ×3; Tone=-10 crushes it to ~0.
    auto peak_pre_limiter = [](int tone_val) -> float {
        unit_runtime_desc_t desc = make_desc();
        RipplerXWaveguide s;
        s.Init(&desc);
        s.setParameter(RipplerXWaveguide::k_paramTone, tone_val);
        s.GateOn(127);
        float peak = 0.0f;
        for (int i = 0; i < 400; ++i) {
            float buf[2] = {};
            s.processBlock(buf, 1);
            float a = std::fabs(ut_voice_out); // pre-master-effects signal
            if (a > peak) peak = a;
        }
        return peak;
    };

    float peak_neutral = peak_pre_limiter(0);
    float peak_bright  = peak_pre_limiter(30);
    float peak_dark    = peak_pre_limiter(-10);
    std::cout << "  peak (pre-limiter) Tone= 0 (neutral) : " << peak_neutral << "\n";
    std::cout << "  peak (pre-limiter) Tone=30 (bright)  : " << peak_bright  << "\n";
    std::cout << "  peak (pre-limiter) Tone=-10 (dark)   : " << peak_dark    << "\n";

    result("T13a Tone=0 (neutral) produces sound (pre-limiter)",
           peak_neutral > 1e-9f, "Tone=0 is silent");
    result("T13b Tone=30 (bright) pre-limiter peak > Tone=-10 (dark)",
           peak_bright > peak_dark,
           "Tone EQ has no effect or inverted: bright pre-limiter peak <= dark");
}

// ════════════════════════════════════════════════════════════════════════════
// T14 — noise_filter SVF delay state zeroed on NoteOn() and Reset()
//   If lp/bp/hp are not cleared on retrigger, the first noise sample after
//   NoteOn sees stale filter memory, producing a click/pop.
//   This test verifies those fields are exactly 0.0 immediately after both
//   a Reset() and a NoteOn() — before any audio is processed.
// ════════════════════════════════════════════════════════════════════════════
static void test_noise_filter_state_clear() {
    std::cout << "\n── T14: noise_filter SVF state cleared on Reset() and NoteOn() ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Run 200 frames with NzMix=100 to accumulate non-zero filter state
    s.setParameter(RipplerXWaveguide::k_paramNzMix, 100);
    s.NoteOn(60, 127);
    for (int i = 0; i < 200; ++i) { float buf[2]{}; s.processBlock(buf, 1); }

    // Snapshot: confirm states are nonzero after processing noise
    // (voice 1 was allocated by the first NoteOn above — index from T8 shows index 1)
    float lp_before = s.state.voices[1].exciter.noise_filter.lp;
    float bp_before = s.state.voices[1].exciter.noise_filter.bp;

    // Reset() must zero all noise_filter delay states
    s.Reset();
    float lp_after_reset = s.state.voices[0].exciter.noise_filter.lp;
    float bp_after_reset = s.state.voices[0].exciter.noise_filter.bp;

    // A fresh NoteOn() must also zero the states before the first sample
    s.NoteOn(60, 100);
    float lp_after_noteon = s.state.voices[1].exciter.noise_filter.lp;
    float bp_after_noteon = s.state.voices[1].exciter.noise_filter.bp;

    std::cout << "  noise_filter.lp before reset : " << lp_before  << "\n";
    std::cout << "  noise_filter.bp before reset : " << bp_before  << "\n";
    std::cout << "  noise_filter.lp after Reset() : " << lp_after_reset << "\n";
    std::cout << "  noise_filter.lp after NoteOn(): " << lp_after_noteon << "\n";

    result("T14a noise_filter.lp/bp non-zero after 200-frame noise injection",
           lp_before != 0.0f || bp_before != 0.0f,
           "NzMix=100 noise filter state never changed — filter may not be processing");
    result("T14b Reset() zeroes noise_filter.lp",
           lp_after_reset == 0.0f,
           "Reset() left stale noise_filter.lp — would cause click on next NoteOn");
    result("T14c Reset() zeroes noise_filter.bp",
           bp_after_reset == 0.0f,
           "Reset() left stale noise_filter.bp — would cause click on next NoteOn");
    result("T14d NoteOn() zeroes noise_filter.lp",
           lp_after_noteon == 0.0f,
           "NoteOn() left stale noise_filter.lp — retrigger click on poly overlap");
    result("T14e NoteOn() zeroes noise_filter.bp",
           bp_after_noteon == 0.0f,
           "NoteOn() left stale noise_filter.bp — retrigger click on poly overlap");
}

// ════════════════════════════════════════════════════════════════════════════
// T15 — Partls=5/6 (editor-select mode) must not change coupling depth
//   Values 5 and 6 select which resonator subsequent edits target.
//   They must not reset or replace the coupling depth set by values 0–4.
// ════════════════════════════════════════════════════════════════════════════
static void test_partls_mode_select_coupling() {
    std::cout << "\n── T15: Partls=5/6 editor-select leaves coupling depth unchanged ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Set coupling to mid-depth via Partls=2 (16 partials → 0.5 coupling)
    s.setParameter(RipplerXWaveguide::k_paramPartls, 2);
    float depth_before = s.m_coupling_depth_ut();

    // Partls=5 selects ResA for editing — must not touch coupling
    s.setParameter(RipplerXWaveguide::k_paramPartls, 5);
    float depth_after_5 = s.m_coupling_depth_ut();

    // Partls=6 selects ResB for editing — must not touch coupling
    s.setParameter(RipplerXWaveguide::k_paramPartls, 6);
    float depth_after_6 = s.m_coupling_depth_ut();

    std::cout << "  coupling_depth after Partls=2  : " << depth_before  << "\n";
    std::cout << "  coupling_depth after Partls=5  : " << depth_after_5 << "\n";
    std::cout << "  coupling_depth after Partls=6  : " << depth_after_6 << "\n";

    result("T15a Partls=2 sets coupling_depth=0.5",
           depth_before == 0.5f,
           "Partls=2 did not set m_coupling_depth to 0.5");
    result("T15b Partls=5 (ResA select) does not change coupling_depth",
           depth_after_5 == depth_before,
           "Partls=5 overwrote coupling_depth — would enable full coupling unexpectedly");
    result("T15c Partls=6 (ResB select) does not change coupling_depth",
           depth_after_6 == depth_before,
           "Partls=6 overwrote coupling_depth — would enable full coupling unexpectedly");
}

// ════════════════════════════════════════════════════════════════════════════
// T16 — Dynamic Energy Squelch reclaims CPU from an inaudible voice
//   A voice with near-zero feedback_gain decays to silence in milliseconds.
//   After GateOff, it should become is_active=false well before the envelope's
//   theoretical worst-case release time expires.  A voice still sustaining
//   above threshold must remain alive.
// ════════════════════════════════════════════════════════════════════════════
static void test_energy_squelch() {
    std::cout << "\n── T16: Dynamic Energy Squelch kills inaudible releasing voices ──\n";

    unit_runtime_desc_t desc = make_desc();

    // Sub-test A: a voice with very low feedback_gain dies quickly after GateOff.
    {
        RipplerXWaveguide s;
        s.Init(&desc);
        // Preset 0 has feedback_gain ≈ 0.87.  Override to near-zero so the waveguide
        // loses energy almost instantly (one round-trip ≈ 190 samples).
        s.state.voices[1].resA.feedback_gain = 0.001f;
        s.state.voices[1].resB.feedback_gain = 0.001f;

        s.NoteOn(60, 127);
        // Let the exciter fire and the delay line fill for ~300 frames
        for (int i = 0; i < 300; ++i) { float buf[2]{}; s.processBlock(buf, 1); }
        s.GateOff();

        int frames_to_death = 0;
        for (int i = 0; i < 5000; ++i) {
            float buf[2]{}; s.processBlock(buf, 1);
            if (!s.state.voices[1].is_active) { frames_to_death = i + 1; break; }
        }

        std::cout << "  low-gain voice killed after " << frames_to_death << " frames post-GateOff\n";
        result("T16a near-zero feedback_gain voice is killed after GateOff",
               frames_to_death > 0 && frames_to_death < 4000,
               "Squelch never fired — voice consumed CPU for the full envelope release");
    }

    // Sub-test B: a voice that is still sustaining (normal feedback_gain) stays active.
    {
        RipplerXWaveguide s;
        s.Init(&desc);
        s.NoteOn(60, 127);
        for (int i = 0; i < 200; ++i) { float buf[2]{}; s.processBlock(buf, 1); }
        // Do NOT call GateOff — voice should remain active.
        bool still_active = s.state.voices[1].is_active;
        std::cout << "  sustaining voice still active after 200 frames: " << (still_active ? "yes" : "no") << "\n";
        result("T16b sustaining voice (no GateOff) remains active",
               still_active,
               "Squelch incorrectly killed a sustaining voice");
    }
}

// ════════════════════════════════════════════════════════════════════════════
// T17 — PitchBend changes delay_length proportionally and symmetrically
//   Bending up (bend > 8192) must shorten the delay; down must lengthen it.
//   The unbent length must be restored when bend returns to centre (8192).
// ════════════════════════════════════════════════════════════════════════════
static void test_pitch_bend() {
    std::cout << "\n── T17: PitchBend adjusts active-voice delay_length ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);
    s.NoteOn(60, 127);

    float centre = s.state.voices[1].resA.delay_length;

    s.PitchBend(16383); // maximum up (~+2 semitones)
    float bent_up = s.state.voices[1].resA.delay_length;

    s.PitchBend(0);     // maximum down (~−2 semitones)
    float bent_down = s.state.voices[1].resA.delay_length;

    s.PitchBend(8192);  // centre — should restore the original length
    float restored = s.state.voices[1].resA.delay_length;

    std::cout << "  delay_length centre=" << centre
              << "  up=" << bent_up << "  down=" << bent_down
              << "  restored=" << restored << "\n";

    result("T17a bend-up shortens delay_length (higher pitch)",
           bent_up < centre,
           "PitchBend up did not shorten delay_length");
    result("T17b bend-down lengthens delay_length (lower pitch)",
           bent_down > centre,
           "PitchBend down did not lengthen delay_length");
    result("T17c centre bend (8192) restores delay_length within 0.1 sample",
           std::fabs(restored - centre) < 0.1f,
           "PitchBend centre did not restore the original delay_length");
}

// ════════════════════════════════════════════════════════════════════════════
// T18 — Pitch bend held before NoteOn is applied immediately to new notes
//   A bend wheel held at max-up when a new note is struck must shorten that
//   note's delay_length from the start.  Also verifies the base_delay fields
//   are stored correctly so a subsequent centre-bend restores the root pitch.
// ════════════════════════════════════════════════════════════════════════════
static void test_pitch_bend_persists_to_new_note() {
    std::cout << "\n── T18: Held pitch bend applies to notes struck while bent ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Establish the nominal (no-bend) delay for note 60
    s.NoteOn(60, 127);
    float nominal = s.state.voices[1].resA.delay_length;

    // Hold bend up, then strike the same pitch — allocates voices[2]
    s.PitchBend(16383);
    s.NoteOn(60, 127);
    float bent_at_noteon = s.state.voices[2].resA.delay_length; // voices[2]: second NoteOn

    // Return to centre — delay should snap back to root pitch
    s.PitchBend(8192);
    float after_centre = s.state.voices[2].resA.delay_length;

    std::cout << "  nominal delay=" << nominal
              << "  delay while bent=" << bent_at_noteon
              << "  after centre=" << after_centre << "\n";

    result("T18a note struck while bent up has shorter delay_length",
           bent_at_noteon < nominal,
           "Held pitch bend was not applied to the new note's delay_length");
    result("T18b returning to centre restores root delay_length within 0.1 sample",
           std::fabs(after_centre - nominal) < 0.1f,
           "Centre bend after bent NoteOn did not restore the root delay_length");
}

// ════════════════════════════════════════════════════════════════════════════
// T19 — Loop filter pitch compensation accuracy
//   After NoteOn(60) the effective loop period (delay_length + τ_LP + τ_AP)
//   should equal srate/f₀ (183.47 samples for C4 at 48 kHz) to within 2 cents.
//   This verifies that both the table generation (powf, not fasterpowf) and the
//   pitch compensation formula (pa/(1-pa) for LP, (1+c)/(1-c) for AP) are correct.
// ════════════════════════════════════════════════════════════════════════════
static void test_pitch_compensation_accuracy() {
    std::cout << "\n── T19: Loop filter pitch compensation accuracy ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    s.NoteOn(60, 127);
    uint8_t vi = s.state.next_voice_idx;

    float dl  = s.state.voices[vi].resA.delay_length;
    float lc  = s.state.voices[vi].resA.lowpass_coeff;
    float ac  = s.state.voices[vi].resA.ap_coeff;

    // Reconstruct the group delays using the same DC-limit formulas as NoteOn
    float pa     = 1.0f - lc;
    float tau_lp = pa / (1.0f - pa);                    // τ_LP = pa/(1-pa)
    float tau_ap = (1.0f + ac) / (1.0f - ac);           // τ_AP = (1+c)/(1-c)
    float effective_period = dl + tau_lp + tau_ap;

    // C4 exact at A4=440 Hz, 48 kHz
    float expected_period  = 48000.0f / (440.0f * powf(2.0f, (60.0f - 69.0f) / 12.0f));
    float error_cents      = 1200.0f * log2f(effective_period / expected_period);

    std::cout << "  delay_length    = " << dl              << " samples\n";
    std::cout << "  lowpass_coeff   = " << lc              << "  τ_LP=" << tau_lp << "\n";
    std::cout << "  ap_coeff        = " << ac              << "  τ_AP=" << tau_ap << "\n";
    std::cout << "  effective period= " << effective_period << " samples (target=" << expected_period << ")\n";
    std::cout << "  pitch error     = " << error_cents      << " cents\n";

    result("T19a effective loop period within 2 cents of C4 target",
           std::fabs(error_cents) < 2.0f,
           "Pitch compensation inaccurate: effective period too far from 48000/261.63");
}

// ════════════════════════════════════════════════════════════════════════════
// T20 — Same-tick GateOn + GateOff (Drumlogue one-shot drum trigger model)
//
//   On the Drumlogue the internal sequencer fires gate_on THEN gate_off in the
//   same scheduler tick, before any audio block is rendered.  The master_env
//   therefore starts at value=0 (just triggered) and is immediately released.
//
//   Before the fix: release() with value=0 → ENV_RELEASE → ENV_IDLE in one
//   audio sample → damper_fade=0 → voice_out *= 0 → permanent silence.
//
//   After the fix: NoteOn calls process() once after trigger(), advancing
//   value to 1.0 before GateOff can call release().  The first audio block
//   then sees value=1.0 releasing normally and hears the strike.
// ════════════════════════════════════════════════════════════════════════════
static void test_same_tick_gate() {
    std::cout << "\n── T20: Same-tick GateOn + GateOff (Drumlogue drum trigger model) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Replicate the hardware sequence: both events fire before any audio block
    s.GateOn(127);    // → NoteOn → trigger() + process() pre-advance
    s.GateOff();      // → release(); value must be 1.0 here, not 0

    // Now render 300 frames (covers delay round-trip + release fade)
    float peak = run_blocks(s, 300, 32);

    std::cout << "  peak output over 300 frames: " << peak << "\n";

    result("T20 same-tick GateOn+GateOff produces audible sound",
           peak > 1e-4f,
           "voice was silent after same-tick trigger+release — master_env killed at value=0");
}

// ════════════════════════════════════════════════════════════════════════════
// T21 — OS parameter-initialisation sequence
//
//   The Drumlogue SDK documentation states: "Unit parameters are expected to be
//   set to [their init value] after the initialization phase."  In practice the
//   OS calls unit_set_param_value(i, header_default[i]) for every parameter
//   AFTER unit_init() returns.
//
//   One header.c default differs from the Init preset:
//     k_paramModel (index 9): header default = 3 (Membrane), Init preset = 0 (String)
//   If the OS overrides the preset after init, the model is Membrane when the
//   first note is played.  This test verifies that the unit still produces sound
//   even when all 24 header defaults are sent after init (simulating OS init).
// ════════════════════════════════════════════════════════════════════════════
static void test_os_param_init_sequence() {
    std::cout << "\n── T21: OS parameter-init sequence (header defaults sent after Init) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Send all 24 header.c default values exactly as the Drumlogue OS does
    // Format: setParameter(index, header_default_value)
    // Source: header.c params array — columns are min,max,center,DEFAULT,type,...
    static const int32_t header_defaults[24] = {
        0,      // 0: Program
        60,     // 1: Note
        0,      // 2: Bank
        1,      // 3: Sample
        500,    // 4: MlltRes
        2500,   // 5: MlltStif
        0,      // 6: VlMllRes
        0,      // 7: VlMllStf
        3,      // 8: Partls
        3,      // 9: Model  ← DIFFERS: header=3 (Membrane), Init preset=0 (String)
        250,    // 10: Dkay
        10,     // 11: Mterl
        0,      // 12: Tone
        26,     // 13: HitPos
        10,     // 14: Rel
        300,    // 15: Inharm
        1,      // 16: LowCut
        5,      // 17: TubRad
        0,      // 18: Gain
        0,      // 19: NzMix
        0,      // 20: NzRes  ← DIFFERS: header=0, Init preset=300
        0,      // 21: NzFltr
        20,     // 22: NzFltFrq  ← DIFFERS: header=20, Init preset=12000
        707,    // 23: Resnc
    };

    for (int i = 0; i < 24; ++i) {
        s.setParameter((uint8_t)i, header_defaults[i]);
    }

    std::cout << "  All 24 header.c defaults sent. Model is now 3 (Membrane).\n";

    // Same-tick trigger: matches the most demanding hardware scenario
    s.GateOn(127);
    s.GateOff();

    float peak = run_blocks(s, 300, 32);
    std::cout << "  peak output over 300 frames: " << peak << "\n";

    result("T21 same-tick trigger after OS param init produces sound",
           peak > 1e-4f,
           "silent after OS sends header defaults then same-tick GateOn+GateOff");
}

// ════════════════════════════════════════════════════════════════════════════
// T22 — master_env value trace at critical frames
//
//   Directly traces the master_env value through the same-tick scenario to
//   verify that the Phase 18 pre-advance fix actually produces value=1.0 before
//   the first audio block, and that the release decays slowly (not instantly).
//   If this test fails, the pre-advance fix is not working correctly.
// ════════════════════════════════════════════════════════════════════════════
static void test_master_env_trace() {
    std::cout << "\n── T22: master_env value trace through same-tick trigger ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // Probe master_env by reading the voice's exciter state after each event.
    // Voice index 1 (next_voice_idx advances from 0 to 1 on first NoteOn).
    s.GateOn(127);   // NoteOn: trigger() + process() → value should be 1.0
    float val_after_noteon = s.state.voices[1].exciter.master_env.value;
    int   state_after_noteon = (int)s.state.voices[1].exciter.master_env.state;

    s.GateOff();     // NoteOff: release() → state should be ENV_RELEASE (2)
    float val_after_gateoff = s.state.voices[1].exciter.master_env.value;
    int   state_after_gateoff = (int)s.state.voices[1].exciter.master_env.state;

    std::cout << "  After GateOn:  value=" << val_after_noteon
              << " state=" << state_after_noteon << " (expect: 1.0 / ENV_DECAY=2)\n";
    std::cout << "  After GateOff: value=" << val_after_gateoff
              << " state=" << state_after_gateoff << " (expect: 1.0 / ENV_RELEASE=3)\n";

    // Render one sample to get the damper_fade value in frame 0
    float buf[2] = {0.0f, 0.0f};
    s.processBlock(buf, 1);
    std::cout << "  Frame 0 output: L=" << buf[0] << " (should be ~0.5 or higher)\n";

    result("T22a master_env value=1.0 after NoteOn pre-advance",
           val_after_noteon >= 0.99f,
           "pre-advance did not advance value to 1.0 — same-tick GateOff will silence voice");

    result("T22b master_env in ENV_RELEASE (not ENV_IDLE) after GateOff",
           state_after_gateoff == 3,  // ENV_RELEASE = 3
           "GateOff put master_env in wrong state (IDLE=0, ATTACK=1, DECAY=2, RELEASE=3)");

    result("T22c frame 0 output is audible (> 0.1)",
           buf[0] > 0.1f,
           "first audio sample is near-zero despite pre-advance fix");
}

// ════════════════════════════════════════════════════════════════════════════
// T23 — Exciter output independent of master_env
//
//   Verifies that the mallet strike at frame 0 is nonzero WITHOUT any envelope
//   involvement, by checking the exciter output before the master_env is applied.
//   This isolates the waveguide exciter path from the envelope gate path.
//   Uses the UNIT_TEST_DEBUG hooks (ut_exciter_out, ut_voice_out).
// ════════════════════════════════════════════════════════════════════════════
static void test_exciter_independent_of_env() {
    std::cout << "\n── T23: Exciter output at frame 0 (mallet strike, no sample) ──\n";

    unit_runtime_desc_t desc = make_desc();
    RipplerXWaveguide s;
    s.Init(&desc);

    // GateOn only — no GateOff — so master_env stays at 1.0 (sustained, ENV_DECAY)
    s.GateOn(127);

    // Process exactly 1 frame to capture frame-0 values via the UT debug hooks
    ut_exciter_out = 0.0f;
    ut_voice_out   = 0.0f;
    float buf[2] = {0.0f, 0.0f};
    s.processBlock(buf, 1);

    std::cout << "  Frame 0 exciter_out = " << ut_exciter_out << " (mallet impulse, expect ~3-5)\n";
    std::cout << "  Frame 0 voice_out   = " << ut_voice_out   << " (post-env, expect > 0)\n";
    std::cout << "  Frame 0 L channel   = " << buf[0]         << " (final output, expect > 0.1)\n";

    result("T23a mallet produces nonzero exciter at frame 0",
           ut_exciter_out > 0.5f,
           "mallet impulse is near-zero — mallet_stiffness or mallet_res_coeff may be 0");

    result("T23b voice_out is nonzero (waveguide+env path works)",
           ut_voice_out > 0.1f,
           "voice_out is near-zero despite nonzero exciter — check feedback/env path");

    result("T23c final frame 0 output > 0.1 (whole chain passes through)",
           buf[0] > 0.1f,
           "final output near-zero — master_filter or master_drive may be wrong");
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
    test_delay_roundtrip();
    test_noise_svf();
    test_tubrad_mterl();
    test_partls_coupling();
    test_tone_eq();
    test_noise_filter_state_clear();
    test_partls_mode_select_coupling();
    test_energy_squelch();
    test_pitch_bend();
    test_pitch_bend_persists_to_new_note();
    test_pitch_compensation_accuracy();
    test_same_tick_gate();
    test_os_param_init_sequence();
    test_master_env_trace();
    test_exciter_independent_of_env();

    std::cout << "\n=== RESULTS: " << g_pass << " passed, " << g_fail << " failed ===\n";
    return g_fail == 0 ? 0 : 1;
}
