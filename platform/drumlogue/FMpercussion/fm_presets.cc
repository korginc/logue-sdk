/**
 * @file fm_presets.cc
 * @brief Factory preset data for FM Percussion Synth
 */

#include "fm_presets.h"

// ---------------------------------------------------------
// Preset Name Dictionary
// ---------------------------------------------------------
const char preset_names[NUM_OF_PRESETS][NAME_LENGTH] =
{
    // Iconic Drum Machines
    "TR-808",
    "TR-909",
    "DX7 FM",
    "CR-78",
    // Modern / Genre
    "Techno1",
    "Indstrl1",
    // Resonant Showcases
    "ResoKick",
    "ResoTom",
    "ResoSnare",
    "ResoMetal",
    // Engine Showcases
    "SlwEnv",
    "WahDrum",
    "NoisSwp",
    "FMBuzz",
    "GhstSnr",
    "RimPtch",
    "TomWah",
    "Shaker",
    "GongHit",
    "TmplBell",
    "MetlGong",
    // Euclidean / Advanced
    "DimKit",
    "WholPrc",
    "HiHatSw",
    "WholTone",
    "MetalGate"
};

// ---------------------------------------------------------
// Preset Data Array
// ---------------------------------------------------------
// Struct Layout reminder:
// Page 1: probability for voice 1, 2, 3 and 4
// Page 2: kick_sweep, kick_decay, snare_noise, snare_body
// Page 3: metal_inharm, metal_bright, perc_ratio, perc_var
// Page 4: lfo1_shape, lfo1_rate, lfo1_target, lfo1_depth
// Page 5: lfo2_shape (Eucl), lfo2_rate, lfo2_target, lfo2_depth
// Page 6: env_shape (bit7=Gong), voice_index, resonant_mode, resonant_morph
// not mapped: resonant_res, resonant_center
// aligned to to voice index: voice_engines[4]

const fm_preset_t FM_PRESETS[NUM_OF_PRESETS] = {
    // =====================================================================
    // 1. ICONIC DRUM MACHINES
    // =====================================================================

    // Preset 0: "TR-808 Kit"
    // Deep, booming kick with a slow decay. Snare has high noise sizzle.
    // Metal is a pure cymbal. Perc is a low-ratio tom.
    {
        "TR-808 Kit",
        80, 80, 80, 80,
        60, 90,                  // Kick: Deep sweep, long decay
        85, 30,                  // Snare: High noise (rattle), low body
        40, 60, 20, 10,          // Metal: Clean cymbal. Perc: Low ratio tom (1.2)
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,   // EuclTun=0 (Off)
        0, 0, 0, 100,               // env_shape 0 = Cymbal character
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 1: "TR-909 Kit"
    // Punchy, tight kick. Snare has aggressive body and moderate noise.
    // Metal is slightly inharmonic (crash). Perc is a tighter, higher tom.
    {
        "TR-909 Kit",
        50, 85, 50, 85,
        90, 40,                  // Kick: Aggressive sweep, short punchy decay
        40, 80,                  // Snare: Lower noise, punchy body
        60, 80, 35, 20,          // Metal: Inharmonic crash. Perc: Mid tom
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,   // EuclTun=0 (Off)
        0, 0, 0, 100,               // env_shape 0 = Cymbal character
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 2: "DX7 FM Kit"
    // Pure frequency modulation. Kick has no sweep, just pure sine sub.
    // Metal is set to GONG character (EnvShape bit 7). Perc is bell-like.
    {
        "DX7 FM Kit",
        70, 80, 70, 80,
        0, 70,                   // Kick: No sweep (pure FM sub)
        10, 95,                  // Snare: Almost no noise, pure FM body
        80, 90, 70, 80,          // Metal: Highly inharmonic. Perc: Bell ratio
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,   // EuclTun=0 (Off)
        128, 0,  0, 100,            // env_shape 128 = GONG character!
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 3: "CR-78 Kit"
    // Very short, blippy drum machine. High noise, high pitch.
    {
        "CR-78 Kit",
        20, 70, 20, 70,
        30, 20,                  // Kick: Tiny blip
        90, 10,                  // Snare: Mostly noise burst
        20, 40, 80, 0,           // Metal: Simple closed hat. Perc: Woodblock (high ratio, no var)
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,   // EuclTun=0 (Off)
        0, 0, 0, 100,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // =====================================================================
    // 2. MODERN & GENRE
    // =====================================================================

    // Preset 4: "Techno1"
    // Huge distorted kick, rattling snare.
    {
        "Techno1",
        60, 90, 60, 90,
        100, 60,                 // Kick: Massive transient
        60, 60,                  // Snare: Balanced
        80, 100, 15, 30,         // Metal: Bright. Perc: Low rumble tom
        1, 10, LFO_TARGET_PITCH, -10, // LFO: Slight pitch drop
        0, 0, LFO_TARGET_NONE, 0,     // EuclTun=0 (Off)
        0, 0, 0, 100,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 5: "Indstrl1"
    // Industrial. Heavy modulation, gong metal, high variation percs.
    {
        "Indstrl1",
        40, 95, 40, 95,
        80, 50,                  // Kick: Hard punch
        100, 80,                 // Snare: White noise blast
        100, 100, 50, 100,       // Metal: Chaotic bright gong. Perc: High variance fm
        5, 80, LFO_TARGET_INDEX, 50, // LFO: Fast S&H on FM Index!
        0, 0, LFO_TARGET_NONE, 0,   // EuclTun=0 (Off)
        128, 0, 0, 100,             // env_shape 128 = GONG character
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },


    // ===== NEW RESONANT PRESETS =====

    // Preset 6: "ResoKick" - Resonant kick drum
    {
        "ResoKick",
        100, 0, 0, 0,               //  probability
        70, 60, 0, 0,
        0, 0, 0, 0,
        0, 10, LFO_TARGET_PITCH, 20,
        0, 5, LFO_TARGET_NONE, 0,   // EuclTun=0 (Off)
        25, 8,
        RESONANT_MODE_LOWPASS, 20,
        {ENGINE_RESONANT, ENGINE_SNARE, ENGINE_KICK, ENGINE_PERC}
    },

    // Preset 7: "ResoTom" - Resonant tom
    {
        "ResoTom",
        0, 0, 0, 100,               //  probability
        50, 50, 0, 0,
        0, 0, 50, 50,
        1, 15, LFO_TARGET_PITCH, 30,
        0, 8, LFO_TARGET_INDEX, 20,  // EuclTun=0 (Off)
        35, 1,
        RESONANT_MODE_BANDPASS, 20,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_RESONANT}
    },

    // Preset 8: "ResoSnare" - Resonant snare
    {
        "ResoSnare",
        0, 100, 0, 0,               //  probability
        0, 0, 50, 50,
        0, 0, 0, 0,
        2, 20, LFO_TARGET_PITCH, 40,
        0, 10, LFO_TARGET_INDEX, 30,  // EuclTun=0 (Off)
        20, 3,
        RESONANT_MODE_HIGHPASS, 20,
        {ENGINE_KICK, ENGINE_RESONANT, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 9: "ResoMetal" - Resonant metal/cymbal
    {
        "ResoMetal",
        0, 0, 100, 0,               //  probability
        0, 0, 0, 0,
        70, 80, 0, 0,
        8, 30, LFO_TARGET_PITCH, 50,
        0, 40, LFO_TARGET_INDEX, 40,  // EuclTun=0 (Off)
        10, 2,
        RESONANT_MODE_PEAK, 20,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_RESONANT, ENGINE_PERC}
    },

    // ===== NEW PRESETS (exploiting phase sync, NOISE_MIX, RES_MORPH) =====

    // Preset 10: "SlwEnv" - Slow ramp LFO (0.5 Hz) on ENV as a secondary
    // envelope shaper: the kick swells in, the snare gets a slow fade-in.
    // Because LFO phase resets on each trigger, this is one-shot per hit.
    {
        "SlwEnv",
        100, 80, 0, 0,              //  probability
        70, 60, 20, 40,
        0, 0, 0, 0,
        1, 5, LFO_TARGET_ENV, 60,      // LFO1: 0.5 Hz ramp → ENV swell
        0, 0, LFO_TARGET_NONE, 0,      // EuclTun=0 (Off)
        30, 0,
        RESONANT_MODE_LOWPASS, 20,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 11: "WahDrum" - LFO→RES_MORPH creates auto-wah filter sweep.
    // Resonant is on voice 2 (metal slot).  Medium rate (4 Hz) triangle
    // gives a fast wah on every hit; synced to trigger so it starts fresh.
    {
        "WahDrum",
        80, 60, 0, 70,              //  probability
        60, 50, 30, 40,
        0, 0, 60, 50,
        0, 35, LFO_TARGET_RES_MORPH, 80,  // LFO1: triangle 4 Hz → filter morph
        0, 10, LFO_TARGET_PITCH, 20,      // LFO2: slow pitch drift; EuclTun=0
        25, 2,
        RESONANT_MODE_BANDPASS, 50,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_RESONANT, ENGINE_PERC}
    },

    // Preset 12: "NoisSwp" - LFO→NOISE_MIX sweeps snare from pure tone to
    // full noise and back over 2 seconds (0.5 Hz).  Interesting on slow
    // patterns: each snare hit starts with crack, fades to hiss.
    {
        "NoisSwp",
        40, 100, 0, 50,             //  probability
        30, 50, 50, 60,
        50, 60, 0, 0,
        1, 5, LFO_TARGET_NOISE_MIX, 90,  // LFO1: slow ramp → noise blend
        0, 20, LFO_TARGET_PITCH, -20,    // LFO2: slight negative pitch wobble
        20, 0,
        RESONANT_MODE_HIGHPASS, 30,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 13: "FMBuzz" - Both LFOs near audio rate targeting INDEX.
    // LFO1 at ~15 Hz and LFO2 at ~20 Hz create amplitude/FM beating.
    // The difference frequency (5 Hz) causes a slow tremolo on top.
    {
        "FMBuzz",
        90, 60, 80, 60,             //  probability
        50, 40, 40, 60,
        70, 80, 50, 50,
        0, 82, LFO_TARGET_INDEX, 70,   // LFO1: ~15 Hz triangle → FM index buzz
        0, 100, LFO_TARGET_INDEX, 50,  // LFO2: ~20 Hz triangle → beating
        15, 0,
        RESONANT_MODE_PEAK, 60,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 14: "GhstSnr" - Ghost snare: low probability (30%), gentle
    // RES_MORPH sweep on the resonant filter gives each ghost hit a slightly
    // different tonal colour.  Voice alloc K-S-M-R.
    {
        "GhstSnr",
        70, 30, 50, 40,             //  probability
        40, 40, 70, 50,
        40, 50, 0, 0,
        0, 18, LFO_TARGET_RES_MORPH, 60,  // LFO1: 2 Hz triangle → filter colour
        0, 8,  LFO_TARGET_PITCH, -30,     // LFO2: slow negative drift; EuclTun=0
        35, 1,
        RESONANT_MODE_BANDPASS, 40,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_RESONANT}
    },

    // Preset 15: "RimPtch" - Ramp LFO on pitch creates a falling tone edge
    // (rim-shot / stick click character) on metal and perc voices.
    // Short env (tight), medium ramp rate (3 Hz → 333 ms period, plenty
    // for a single hit since LFO resets on trigger).
    {
        "RimPtch",
        50, 60, 80, 70,             //  probability
        20, 30, 40, 50,
        60, 70, 50, 40,
        1, 25, LFO_TARGET_PITCH, -70,  // LFO1: ramp, negative → falling pitch
        0, 0,  LFO_TARGET_NONE, 0,     // EuclTun=0 (Off)
        10, 0,
        RESONANT_MODE_HIGHPASS, 70,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 16: "TomWah" - Resonant tom with RES_MORPH modulated by a slow
    // triangle (1 Hz).  Each tom hit sweeps the filter from low to high and
    // back, giving a pitch/wah character.  Voice alloc K-S-M-R.
    {
        "TomWah",
        0, 0, 0, 100,               //  probability
        50, 60, 0, 0,
        0, 0, 60, 60,
        0, 10, LFO_TARGET_RES_MORPH, 100, // LFO1: 1 Hz triangle → full morph sweep
        0, 5,  LFO_TARGET_PITCH, 20,      // LFO2: subtle pitch rise; EuclTun=0
        40, 1,
        RESONANT_MODE_LOWPASS, 30,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_RESONANT}
    },

    // Preset 17: "Shaker" - High density rattling: all four voices active,
    // low probabilities for irregular hitting, high metal brightness, fast
    // near-audio LFO on NOISE_MIX for textured shimmer (metal and snare).
    {
        "Shaker",
        60, 50, 90, 70,             //  probability
        20, 40, 80, 70,
        80, 90, 60, 70,
        0, 75, LFO_TARGET_NOISE_MIX, 80,  // LFO1: ~10 Hz triangle → noise shimmer
        0, 50, LFO_TARGET_PITCH, 15,      // LFO2: slow pitch arp; EuclTun=0
        8, 0,
        RESONANT_MODE_HIGHPASS, 80,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // ===== CHARACTER 1 (GONG) PRESETS — EnvShape = 128+index =====
    // EnvShape bit 7 = 1 selects gong ratios (1.0, 2.756, 3.752, 5.404)
    // which produce widely-spaced inharmonic partials vs. the DX7 cymbal cluster.

    // Preset 18: "GongHit" - Single gong strike with long resonant tail.
    // Metal engine only (all voices routed to metal), high inharmonicity for
    // rich gong spectrum, slow LFO pitch sag (gong pitch drops after strike).
    {
        "GongHit",
        0, 0, 100, 0,               //  probability
        0, 0, 0, 0,
        70, 60, 0, 0,
        1, 8,  LFO_TARGET_PITCH, -40,  // LFO1: slow ramp → pitch sag after hit
        0, 0,  LFO_TARGET_NONE, 0,
        128 + 110, 0,                  // EnvShape: char=1(Gong) + env=110 (long decay)
        RESONANT_MODE_PEAK, 0,
        {ENGINE_METAL, ENGINE_METAL, ENGINE_METAL, ENGINE_METAL}
    },

    // Preset 19: "TmplBell" - Temple bell: low inharmonicity (partials less
    // spread) gives a clearer pitch centre; LFO index sweep brightens the
    // attack transient then softens.  Long decay envelope.
    {
        "TmplBell",
        0, 0, 100, 0,               //  probability
        0, 0, 0, 0,
        20, 80, 0, 0,
        0, 12, LFO_TARGET_INDEX, 60,   // LFO1: slow ramp → brightness decay
        0, 0,  LFO_TARGET_NONE, 0,     // EuclTun=0 (Off)
        128 + 100, 0,                  // EnvShape: char=1(Gong) + env=100 (long)
        RESONANT_MODE_LOWPASS, 0,
        {ENGINE_METAL, ENGINE_METAL, ENGINE_METAL, ENGINE_METAL}
    },

    // Preset 20: "MetlGong" - Hybrid: kick gives low body, metal uses gong
    // character.  LFO NOISE_MIX on both snare and metal creates evolving
    // texture.  Medium decay, mid-range voices active.
    {
        "MetlGong",
        60, 30, 80, 0,              //  probability
        50, 40, 20, 30,
        60, 70, 0, 0,
        0, 40, LFO_TARGET_NOISE_MIX, 70, // LFO1: triangle → noise texture
        0, 6,  LFO_TARGET_PITCH, -20,     // LFO2: slow pitch drop; EuclTun=0
        128 + 60, 0,                      // EnvShape: char=1(Gong) + env=60 (medium)
        RESONANT_MODE_BANDPASS, 0,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },


    // =====================================================================
    // 3. EUCLIDEAN & ADVANCED SHOWCASES
    // =====================================================================

    // Preset 21: "DimKit" — EuclTun=Dim7 (E(4,12)=[0,3,6,9]).
    // All 4 voices active with dim7 chord spread: Kick at root, Snare +3st,
    // Metal +6st, Perc +9st.  Every trigger fires a diminished 7th chord of
    // percussion.  Slow ramp LFO→Pitch adds a falling pitch tail to each hit.
    {
        "DimKit",
        100, 90, 80, 100,               //  probability
        60, 50, 25, 55,
        50, 65, 55, 35,
        1, 12, LFO_TARGET_PITCH, -30,  // LFO1: slow ramp → falling pitch tail
        6, 0,  LFO_TARGET_NONE, 0,     // EuclTun=6 (Dim7 [0,3,6,9])
        30, 0,
        RESONANT_MODE_LOWPASS, 30,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 22: "WholPrc" — EuclTun=Whole (E(4,8)=[0,2,4,6]).
    // Perc-forward whole-tone tuning: all 4 voices spread across a whole-tone
    // scale (0,2,4,6 semitones from the incoming note).  Low kick/snare so the
    // tuned perc and metal voices dominate.  Short env for staccato character.
    {
        "WholPrc",
        40, 30, 70, 100,                //  probability
        30, 40, 15, 35,
        35, 55, 70, 45,
        1, 18, LFO_TARGET_PITCH, -20,  // LFO1: ramp → slight pitch drop after hit
        4, 0,  LFO_TARGET_NONE, 0,     // EuclTun=4 (Whole [0,2,4,6])
        22, 0,
        RESONANT_MODE_LOWPASS, 20,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 23: "HiHatSw" — MetalGate open/closed hi-hat.
    // Metal engine routed to voice 2 (slot 2).  LFO1 uses Ramp shape targeting
    // METAL_GATE: phase resets on trigger → one-shot gate that closes from 1→0
    // over the LFO period.  Rate=30 (~3 Hz period=330ms) → medium-length open hat.
    // Increase rate for closed hat, decrease for longer open shimmer.
    {
        "HiHatSw",
        70, 50, 100, 0,             //  probability
        25, 45, 40, 45,
        55, 90, 0, 0,
        1, 30, LFO_TARGET_METAL_GATE, 90,  // LFO1: Ramp → open→closed gate
        0, 0,  LFO_TARGET_NONE, 0,         // EuclTun=0 (Off)
        10, 0,
        RESONANT_MODE_HIGHPASS, 20,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 24: "WholTone"
    // Demonstrating the EuclTun (Euclidean Tuning) feature on the Perc engine.
    // LFO2 shape is used as the tuning mode (e.g. 4 = Whole Tone scale spread).
    {
        "WholTone",
        70, 80, 70, 80,
        40, 30,
        30, 40,
        35, 55, 70, 45,          // Perc is highly tuned
        1, 18, LFO_TARGET_PITCH, -10,
        4, 0,  LFO_TARGET_NONE, 0, // EuclTun = 4 (Whole Tone spread across 4 voices)
        0, 0, 0, 100,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 25: "MetalGate"
    // Demonstrating the MetalGate LFO target (chops the cymbal tail).
    {
        "MetalGate",
        60, 80, 60, 80,
        60, 50,
        50, 50,
        50, 80, 30, 20,          // Metal is bright
        3, 60, LFO_TARGET_METAL_GATE, 100, // LFO1: Square wave gating the Metal engine!
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, 0, 100,
        {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    }
};