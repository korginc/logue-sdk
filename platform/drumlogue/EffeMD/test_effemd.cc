/**
 * @file test_effemd.cc
 * @brief Host-side unit tests for the EffeMD drum models and integration layer.
 *
 * The whole DSP layer under test (fm_perc_synth_process.h + the 11 models)
 * is scalar, so this compiles and runs natively on x86/ARM hosts:
 *
 *   ./run_effemd_tests.sh
 *
 * synth.h (the drumlogue control layer) is excluded: it depends on unit.h
 * and its NEON block path, which are exercised on the device build instead.
 */

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "constants.h"
#include "fm_perc_synth_process.h"

static int g_failures = 0;

static void report_pass(const char* name) {
    std::printf("[PASS] %s\n", name);
}

static void report_fail(const char* name, const char* detail) {
    std::printf("[FAIL] %s: %s\n", name, detail);
    ++g_failures;
}

static constexpr int kRenderLen = 48000;   // 1 s
static constexpr int kTailLen   = 4800;    // last 100 ms
static float g_buf_a[kRenderLen];
static float g_buf_b[kRenderLen];

struct RenderStats {
    bool  finite;
    float peak;
    float tail_peak;
};

static RenderStats render_model(DrumModel* m, float* buf, int len) {
    RenderStats st{true, 0.0f, 0.0f};
    for (int i = 0; i < len; ++i) {
        const float s = m->Process();
        buf[i] = s;
        if (!std::isfinite(s)) st.finite = false;
        const float a = std::fabs(s);
        if (a > st.peak) st.peak = a;
        if (i >= len - kTailLen && a > st.tail_peak) st.tail_peak = a;
    }
    return st;
}

/* ---------------------------------------------------------------------------
 * Per-model tests
 * ------------------------------------------------------------------------- */

// Streams `secs` of audio (no full buffer) and returns the peak magnitude in
// the final 100 ms. Used to verify long-decay instruments still reach silence.
static float streamed_tail_peak(DrumModel* m, float secs) {
    const int total = (int)(secs * 48000.0f);
    float tail = 0.0f;
    for (int i = 0; i < total; ++i) {
        const float s = m->Process();
        if (i >= total - kTailLen) {
            const float a = std::fabs(s);
            if (a > tail) tail = a;
        }
    }
    return tail;
}

static void test_model_render(const char* name, DrumModel* m, bool long_decay) {
    char tname[96];
    char detail[128];

    m->Init();
    m->loadPreset(0);
    m->setPitchRatio(1.0f);
    m->Trigger();
    const RenderStats st = render_model(m, g_buf_a, kRenderLen);

    std::snprintf(tname, sizeof(tname), "%s renders finite output", name);
    if (st.finite) report_pass(tname);
    else report_fail(tname, "non-finite sample");

    std::snprintf(tname, sizeof(tname), "%s is audible", name);
    if (st.peak > 1.0e-3f) report_pass(tname);
    else {
        std::snprintf(detail, sizeof(detail), "peak=%.6f", st.peak);
        report_fail(tname, detail);
    }

    std::snprintf(tname, sizeof(tname), "%s is bounded", name);
    if (st.peak <= 2.5f) report_pass(tname);
    else {
        std::snprintf(detail, sizeof(detail), "peak=%.3f", st.peak);
        report_fail(tname, detail);
    }

    if (long_decay) {
        // Intentionally long-ringing voice (e.g. TRX Gong): just confirm it
        // eventually reaches silence rather than ringing forever.
        m->Init();
        m->loadPreset(0);
        m->setPitchRatio(1.0f);
        m->Trigger();
        const float tail = streamed_tail_peak(m, 6.0f);
        std::snprintf(tname, sizeof(tname), "%s decays within 6s", name);
        if (tail < 0.02f) report_pass(tname);
        else {
            std::snprintf(detail, sizeof(detail), "tail peak=%.4f", tail);
            report_fail(tname, detail);
        }
    } else {
        std::snprintf(tname, sizeof(tname), "%s decays within 1s", name);
        if (st.tail_peak < 0.02f) report_pass(tname);
        else {
            std::snprintf(detail, sizeof(detail), "tail peak=%.4f", st.tail_peak);
            report_fail(tname, detail);
        }
    }
}

static void test_model_determinism(const char* name, DrumModel* m) {
    char tname[96];

    m->Init();
    m->loadPreset(0);
    m->setPitchRatio(1.0f);
    m->Trigger();
    render_model(m, g_buf_a, kTailLen);

    m->Init();
    m->Trigger();
    render_model(m, g_buf_b, kTailLen);

    std::snprintf(tname, sizeof(tname), "%s retrigger is deterministic", name);
    if (std::memcmp(g_buf_a, g_buf_b, kTailLen * sizeof(float)) == 0)
        report_pass(tname);
    else
        report_fail(tname, "two identical triggers produced different audio");
}

static void test_model_pitch_ratio(const char* name, DrumModel* m) {
    char tname[96];

    m->Init();
    m->loadPreset(0);
    m->setPitchRatio(1.0f);
    m->Trigger();
    render_model(m, g_buf_a, kTailLen);

    m->Init();
    m->setPitchRatio(2.0f);  // +1 octave
    m->Trigger();
    render_model(m, g_buf_b, kTailLen);

    std::snprintf(tname, sizeof(tname), "%s responds to pitch ratio", name);
    if (std::memcmp(g_buf_a, g_buf_b, kTailLen * sizeof(float)) != 0)
        report_pass(tname);
    else
        report_fail(tname, "pitch ratio 2.0 output identical to 1.0");

    m->setPitchRatio(1.0f);
}

static void test_model_parameters(const char* name, DrumModel* m) {
    char tname[96];
    char detail[128];

    m->Init();
    m->loadPreset(0);

    int valid = 0;
    bool ok = true;
    for (int i = 0; i < PARAM_TOTAL; ++i) {
        const fm_param_index_t idx = (fm_param_index_t)i;
        if (idx == K_Instrument || idx == K_Euclidean_Tuning || idx == K_Reserved)
            continue;  // global parameters, not routed to models
        m->setParameter(idx, 50.0f);
        const float v = m->getParameter(idx);
        if (!std::isfinite(v)) ok = false;
        if (v != 255.0f) ++valid;
    }

    std::snprintf(tname, sizeof(tname), "%s parameter set/get", name);
    if (ok && valid >= 4) {
        report_pass(tname);
    } else {
        std::snprintf(detail, sizeof(detail), "ok=%d valid_params=%d", (int)ok, valid);
        report_fail(tname, detail);
    }

    // After parameter edits the model must still render valid audio.
    m->Trigger();
    const RenderStats st = render_model(m, g_buf_a, kTailLen);
    std::snprintf(tname, sizeof(tname), "%s renders after param edits", name);
    if (st.finite && st.peak <= 2.5f) report_pass(tname);
    else {
        std::snprintf(detail, sizeof(detail), "finite=%d peak=%.3f", (int)st.finite, st.peak);
        report_fail(tname, detail);
    }
}

static void test_cymbal_choke() {
    // With sustain > 0 the cymbal floors at `sustain` and rings indefinitely.
    // Without a Release() it must still be audible after 2 s; after Release()
    // it must fall silent within ~0.5 s.
    FmCymbalModel cym;
    cym.Init();
    cym.loadPreset(0);
    cym.setParameter(K_Sustain, 60.0f);  // sustain = 0.6
    cym.setPitchRatio(1.0f);

    cym.Trigger();
    float held = streamed_tail_peak(&cym, 2.0f);  // tail over 2 s, still held

    cym.Trigger();
    streamed_tail_peak(&cym, 0.5f);  // let it ring half a second
    cym.Release();
    float released = streamed_tail_peak(&cym, 0.5f);  // 0.5 s after release

    if (held > 0.02f)
        report_pass("cymbal sustains while held");
    else
        report_fail("cymbal sustains while held", "no sustained tail with sustain>0");

    if (released < 0.01f)
        report_pass("cymbal choke silences sustained tail on note-off");
    else {
        char d[64];
        std::snprintf(d, sizeof(d), "tail after release=%.4f", released);
        report_fail("cymbal choke silences sustained tail on note-off", d);
    }
}

/* ---------------------------------------------------------------------------
 * Euclidean tuning table tests
 * ------------------------------------------------------------------------- */

static void test_euclid_table() {
    bool zero_ok = true;
    for (int lane = 0; lane < 4; ++lane) {
        if (EUCLID_OFFSETS[EUCLID_MODE_OFF][lane] != 0.0f) zero_ok = false;
    }
    if (zero_ok) report_pass("euclid mode Off has zero offsets");
    else report_fail("euclid mode Off has zero offsets", "non-zero entry");

    bool mono_ok = true;
    for (int mode = 0; mode < EUCLID_MODE_COUNT; ++mode) {
        for (int lane = 1; lane < 4; ++lane) {
            if (EUCLID_OFFSETS[mode][lane] < EUCLID_OFFSETS[mode][lane - 1]) mono_ok = false;
        }
    }
    if (mono_ok) report_pass("euclid offsets monotonic per mode");
    else report_fail("euclid offsets monotonic per mode", "decreasing offset found");

    bool lane_ok = true;
    for (int i = 0; i < INST_COUNT; ++i) {
        const int lane = fm_engine_to_euclid_lane((engine_id_t)i);
        if (lane < 0 || lane > 3) lane_ok = false;
    }
    if (lane_ok) report_pass("every instrument maps to a euclid lane");
    else report_fail("every instrument maps to a euclid lane", "lane out of range");
}

/* ---------------------------------------------------------------------------
 * Integration layer tests (fm_perc_synth_*)
 * ------------------------------------------------------------------------- */

static fm_perc_synth_t g_synth;

static float render_synth_peak(fm_perc_synth_t* s, int len, float* buf) {
    float peak = 0.0f;
    for (int i = 0; i < len; ++i) {
        const float v = fm_perc_synth_process(s);
        if (buf) buf[i] = v;
        const float a = std::fabs(v);
        if (a > peak) peak = a;
    }
    return peak;
}

static void test_synth_basics() {
    fm_perc_synth_init(&g_synth);

    if (fm_perc_synth_is_idle(&g_synth))
        report_pass("synth starts idle");
    else
        report_fail("synth starts idle", "idle flag not set after init");

    fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 127);
    if (!fm_perc_synth_is_idle(&g_synth))
        report_pass("note-on clears idle");
    else
        report_fail("note-on clears idle", "still idle after note-on");

    const float peak = render_synth_peak(&g_synth, kRenderLen, g_buf_a);
    if (peak > 1.0e-3f && peak < 1.0f)
        report_pass("synth output audible and soft-clipped below 1.0");
    else {
        char d[64];
        std::snprintf(d, sizeof(d), "peak=%.4f", peak);
        report_fail("synth output audible and soft-clipped below 1.0", d);
    }

    if (fm_perc_synth_is_idle(&g_synth))
        report_pass("synth returns to idle after decay");
    else
        report_fail("synth returns to idle after decay", "idle not set after 1s of kick");
}

static void test_synth_velocity() {
    fm_perc_synth_init(&g_synth);
    fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 127);
    const float loud = render_synth_peak(&g_synth, 9600, nullptr);

    fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 16);
    const float soft = render_synth_peak(&g_synth, 9600, nullptr);

    if (loud > soft && soft > 0.0f)
        report_pass("velocity scales output");
    else {
        char d[64];
        std::snprintf(d, sizeof(d), "loud=%.4f soft=%.4f", loud, soft);
        report_fail("velocity scales output", d);
    }
}

static void test_synth_instrument_switch() {
    fm_perc_synth_init(&g_synth);
    bool ok = true;
    for (int inst = 0; inst < INST_COUNT; ++inst) {
        fm_perc_synth_update_params(&g_synth, K_Instrument, inst);
        fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 100);
        const float peak = render_synth_peak(&g_synth, 9600, nullptr);
        if (!(peak > 1.0e-4f) || !std::isfinite(peak)) {
            std::printf("       instrument %d (%s) peak=%.6f\n",
                        inst, instruments_strings[inst], peak);
            ok = false;
        }
    }
    if (ok) report_pass("all 11 instruments render through the synth layer");
    else report_fail("all 11 instruments render through the synth layer", "see above");
}

static void test_synth_euclid_modes() {
    // Metal-lane instrument (cowbell) + tritone spread: offset 12 semitones
    // must change the rendered audio relative to mode Off.
    fm_perc_synth_init(&g_synth);
    fm_perc_synth_update_params(&g_synth, K_Instrument, ID_FmCowbellModel);

    fm_perc_synth_update_params(&g_synth, K_Euclidean_Tuning, EUCLID_MODE_OFF);
    fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 100);
    render_synth_peak(&g_synth, kTailLen, g_buf_a);

    fm_perc_synth_update_params(&g_synth, K_Euclidean_Tuning, EUCLID_MODE_TRIT);
    fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 100);
    render_synth_peak(&g_synth, kTailLen, g_buf_b);

    if (std::memcmp(g_buf_a, g_buf_b, kTailLen * sizeof(float)) != 0)
        report_pass("euclidean tuning transposes the voice");
    else
        report_fail("euclidean tuning transposes the voice",
                    "mode Trit output identical to Off");

    // MIDI note transposition independent of euclid
    fm_perc_synth_update_params(&g_synth, K_Euclidean_Tuning, EUCLID_MODE_OFF);
    fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE + 7, 100);
    render_synth_peak(&g_synth, kTailLen, g_buf_b);
    if (std::memcmp(g_buf_a, g_buf_b, kTailLen * sizeof(float)) != 0)
        report_pass("MIDI note transposes the voice");
    else
        report_fail("MIDI note transposes the voice", "note +7 identical to root");
}

static void test_presets() {
    bool ok = true;
    for (int i = 0; i < NUM_OF_PRESETS; ++i) {
        if (!FM_PRESET_NAMES[i] || !FM_PRESET_NAMES[i][0]) ok = false;
    }
    if (ok) report_pass("preset names defined");
    else report_fail("preset names defined", "empty preset name");

    fm_perc_synth_init(&g_synth);
    for (int p = 0; p < NUM_OF_PRESETS; ++p) {
        load_fm_preset(&g_synth, p);
        fm_perc_synth_note_on(&g_synth, EFFEMD_ROOT_NOTE, 100);
        const float peak = render_synth_peak(&g_synth, 9600, nullptr);
        char tname[64];
        std::snprintf(tname, sizeof(tname), "preset %d renders", p);
        if (peak > 1.0e-4f && std::isfinite(peak)) report_pass(tname);
        else report_fail(tname, "silent or invalid output");
    }
}

/* ------------------------------------------------------------------------- */

int main() {
    std::printf("EffeMD unit tests\n=================\n");

    test_euclid_table();
    test_cymbal_choke();
    test_presets();
    test_synth_basics();
    test_synth_velocity();
    test_synth_instrument_switch();
    test_synth_euclid_modes();

    fm_perc_synth_init(&g_synth);
    struct { const char* name; DrumModel* m; bool long_decay; } models[] = {
        {"FmKick",       &g_synth.kick,       false},
        {"FmSnare",      &g_synth.snare,      false},
        {"FmTom",        &g_synth.tom,        false},
        {"FmClap",       &g_synth.clap,       false},
        {"FmRimshot",    &g_synth.rimshot,    false},
        {"FmCowbell",    &g_synth.cowbell,    false},
        {"FmCymbal",     &g_synth.cymbal,     false},
        {"TRXBassDrum",  &g_synth.bass,       false},
        {"TRXSnareDrum", &g_synth.snare_drum, false},
        {"TRXClaves",    &g_synth.claves,     false},
        {"TRXHiHat",     &g_synth.hi_hat,     false},
        {"FmWhistle",    &g_synth.whistle,    false},
        {"TRXGong",      &g_synth.gong,       true},
    };

    for (auto& e : models) {
        test_model_render(e.name, e.m, e.long_decay);
        test_model_determinism(e.name, e.m);
        test_model_pitch_ratio(e.name, e.m);
        test_model_parameters(e.name, e.m);
    }

    std::printf("=================\n");
    if (g_failures == 0) {
        std::printf("All tests passed.\n");
        return 0;
    }
    std::printf("%d test(s) FAILED.\n", g_failures);
    return 1;
}
