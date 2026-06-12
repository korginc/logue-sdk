/**
 * @file fm_presets.cc
 * @brief Factory preset bank for Sonaglio FM percussion synth.
 *
 * Bank layout (64 presets):
 *   0..46   General-MIDI drum kit, matching the GmDrumNote list of
 *           copych/ESP32-S3_FM_Drum_Synth (notes 35..81). Each preset is a
 *           drum-voice TIMBRE; pitch is taken from the played MIDI note.
 *   47..63  Sonaglio-specific experimental and combo presets.
 *
 * Field order (after name) — 24 values in 6 rows of 4:
 *   instrument, blend, gap, scatter
 *   kick_atk,  kick_body,  snare_atk, snare_body
 *   metal_atk, metal_body, perc_atk,  perc_body
 *   lfo1_shape, lfo1_rate, lfo1_target, lfo1_depth
 *   eucl_tun,  lfo2_rate, lfo2_target, lfo2_depth
 *   env_shape, hit_shape, body_tilt, noise_char
 *
 * Notes:
 *   - The Hat engine is driven by metal_atk / metal_body (shared page).
 *   - The Tom instrument routes to the Perc engine (perc_atk / perc_body).
 *   - env_shape: bits[6:0] index the envelope ROM; bit7 selects the Metal
 *     character (0 = Cymbal, 1 = Gong). Values >127 set the Gong character.
 *   - Only the selected instrument's engine(s) produce sound; inactive engine
 *     params are left at neutral values.
 */

#include "fm_presets.h"
#include <stddef.h>

const fm_preset_t FM_PRESETS[NUM_OF_PRESETS] = {

    // =====================================================================
    // General-MIDI kit (GmDrumNote 35..81)
    // =====================================================================

    // 35 Acoustic Bass Drum — deep, boomy kick
    {"AcBassDrum", 0, 50, 0, 0,  35, 85, 40, 50,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  24, 35, 72, 5},
    // 36 Bass Drum 1 — punchy kick
    {"BassDrum1", 0, 50, 0, 0,  60, 68, 40, 50,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  20, 52, 58, 8},
    // 37 Side Stick — tight rim click
    {"SideStick", 1, 50, 0, 0,  50, 55, 72, 15,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  4, 70, 30, 25},
    // 38 Acoustic Snare
    {"AcSnare", 1, 50, 0, 0,  50, 55, 55, 55,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  22, 50, 50, 48},
    // 39 Hand Clap — multi-strike approximation via Gap + scatter + noise
    {"HandClap", 1, 40, 22, 55,  50, 55, 78, 28,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  68, 60, 35, 72},
    // 40 Electric Snare — tighter, brighter
    {"ElSnare", 1, 50, 0, 0,  50, 55, 78, 42,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  18, 58, 45, 55},
    // 41 Low Floor Tom
    {"LowFloorTm", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 30, 85,
     0, 0, 0, 0,  0, 0, 0, 0,  42, 35, 72, 8},
    // 42 Closed Hi-Hat
    {"ClosedHat", 4, 50, 0, 0,  50, 55, 40, 50,  72, 20, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  2, 60, 25, 42},
    // 43 High Floor Tom
    {"HiFloorTom", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 35, 80,
     0, 0, 0, 0,  0, 0, 0, 0,  38, 38, 68, 8},
    // 44 Pedal Hi-Hat
    {"PedalHat", 4, 50, 0, 0,  50, 55, 40, 50,  62, 26, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  6, 55, 28, 38},
    // 45 Low Tom
    {"LowTom", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 42, 72,
     0, 0, 0, 0,  0, 0, 0, 0,  34, 42, 62, 8},
    // 46 Open Hi-Hat
    {"OpenHat", 4, 50, 0, 0,  50, 55, 40, 50,  66, 42, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  40, 50, 42, 48},
    // 47 Low-Mid Tom
    {"LowMidTom", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 46, 64,
     0, 0, 0, 0,  0, 0, 0, 0,  30, 45, 58, 8},
    // 48 Hi-Mid Tom
    {"HiMidTom", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 50, 56,
     0, 0, 0, 0,  0, 0, 0, 0,  28, 48, 52, 8},
    // 49 Crash Cymbal 1 — long metallic ring
    {"Crash1", 3, 50, 0, 0,  50, 55, 40, 50,  70, 75, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  104, 40, 60, 20},
    // 50 High Tom
    {"HighTom", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 56, 48,
     0, 0, 0, 0,  0, 0, 0, 0,  26, 52, 48, 8},
    // 51 Ride Cymbal 1 — pingy, sustained
    {"Ride1", 3, 50, 0, 0,  50, 55, 40, 50,  46, 66, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  100, 35, 62, 12},
    // 52 Chinese Cymbal — Gong character (env bit7=1)
    {"ChinaCym", 3, 50, 0, 0,  50, 55, 40, 50,  76, 70, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  228, 45, 58, 22},
    // 53 Ride Bell — short, bell-like ping
    {"RideBell", 3, 50, 0, 0,  50, 55, 40, 50,  80, 86, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  12, 42, 78, 8},
    // 54 Tambourine — noisy, short
    {"Tambourin", 4, 50, 0, 0,  50, 55, 40, 50,  58, 30, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  10, 58, 30, 72},
    // 55 Splash Cymbal — short crash
    {"SplashCym", 3, 50, 0, 0,  50, 55, 40, 50,  72, 54, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  98, 44, 52, 18},
    // 56 Cowbell — cymbal ratios give the cowbell interval, short decay
    {"Cowbell", 3, 50, 0, 0,  50, 55, 40, 50,  60, 45, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  10, 50, 50, 8},
    // 57 Crash Cymbal 2 — brighter long crash
    {"Crash2", 3, 50, 0, 0,  50, 55, 40, 50,  76, 78, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  108, 46, 62, 22},
    // 58 Vibraslap — rattly snare+metal layer
    {"Vibraslap", 10, 55, 10, 65,  50, 55, 70, 30,  60, 50, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  70, 55, 45, 55},
    // 59 Ride Cymbal 2 — sustained ride
    {"Ride2", 3, 50, 0, 0,  50, 55, 40, 50,  50, 68, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  102, 36, 64, 12},
    // 60 Hi Bongo
    {"HiBongo", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 56, 44,
     0, 0, 0, 0,  0, 0, 0, 0,  14, 55, 42, 10},
    // 61 Low Bongo
    {"LowBongo", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 50, 50,
     0, 0, 0, 0,  0, 0, 0, 0,  16, 52, 46, 10},
    // 62 Mute Hi Conga — very short
    {"MuteHiCng", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 62, 35,
     0, 0, 0, 0,  0, 0, 0, 0,  8, 58, 38, 10},
    // 63 Open Hi Conga
    {"OpenHiCng", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 50, 55,
     0, 0, 0, 0,  0, 0, 0, 0,  20, 48, 52, 10},
    // 64 Low Conga
    {"LowConga", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 44, 62,
     0, 0, 0, 0,  0, 0, 0, 0,  24, 44, 58, 10},
    // 65 High Timbale — bright
    {"HiTimbale", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 62, 50,
     0, 0, 0, 0,  0, 0, 0, 0,  18, 56, 48, 14},
    // 66 Low Timbale
    {"LowTimbale", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 56, 58,
     0, 0, 0, 0,  0, 0, 0, 0,  22, 50, 54, 14},
    // 67 High Agogo — short metallic
    {"HiAgogo", 3, 50, 0, 0,  50, 55, 40, 50,  66, 50, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  10, 54, 50, 8},
    // 68 Low Agogo
    {"LowAgogo", 3, 50, 0, 0,  50, 55, 40, 50,  60, 55, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  14, 50, 54, 8},
    // 69 Cabasa — bright noise, very short
    {"Cabasa", 4, 50, 0, 0,  50, 55, 40, 50,  56, 15, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  2, 58, 20, 75},
    // 70 Maracas — brighter noise shake
    {"Maracas", 4, 50, 0, 0,  50, 55, 40, 50,  66, 12, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  2, 62, 18, 80},
    // 71 Short Whistle — pure tone + vibrato
    {"ShWhistle", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 20, 10,
     0, 45, 1, 15,  0, 0, 0, 0,  14, 40, 20, 5},
    // 72 Long Whistle — sustained tone + vibrato
    {"LnWhistle", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 18, 12,
     0, 35, 1, 18,  0, 0, 0, 0,  24, 38, 25, 5},
    // 73 Short Guiro — scrape (noise + amplitude rasp LFO)
    {"ShGuiro", 4, 50, 0, 0,  50, 55, 40, 50,  50, 20, 50, 40,
     1, 60, 8, 40,  0, 0, 0, 0,  14, 52, 22, 65},
    // 74 Long Guiro — longer scrape
    {"LnGuiro", 4, 50, 0, 0,  50, 55, 40, 50,  48, 22, 50, 40,
     1, 45, 8, 45,  0, 0, 0, 0,  24, 50, 24, 68},
    // 75 Claves — woody, tightest
    {"Claves", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 48, 18,
     0, 0, 0, 0,  0, 0, 0, 0,  2, 58, 22, 5},
    // 76 Hi Wood Block
    {"HiWoodBlk", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 52, 18,
     0, 0, 0, 0,  0, 0, 0, 0,  2, 60, 20, 5},
    // 77 Low Wood Block
    {"LowWoodBlk", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 46, 26,
     0, 0, 0, 0,  0, 0, 0, 0,  4, 55, 28, 5},
    // 78 Mute Cuica — pitched friction, fast pitch LFO
    {"MuteCuica", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 40, 40,
     0, 50, 1, 30,  0, 0, 0, 0,  12, 48, 42, 10},
    // 79 Open Cuica — slower, deeper pitch sweep
    {"OpenCuica", 2, 50, 0, 0,  50, 55, 40, 50,  50, 60, 35, 50,
     0, 35, 1, 42,  0, 0, 0, 0,  24, 44, 50, 10},
    // 80 Mute Triangle — short metallic ping
    {"MuteTrigl", 3, 50, 0, 0,  50, 55, 40, 50,  50, 68, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 52, 62, 5},
    // 81 Open Triangle — long metallic ring
    {"OpenTrigl", 3, 50, 0, 0,  50, 55, 40, 50,  55, 82, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  106, 48, 70, 5},

    // =====================================================================
    // Sonaglio experimental & combo presets
    // =====================================================================

    // Kick+Snare layered clap-ish hit
    {"KS Clap", 5, 50, 8, 15,  60, 65, 70, 35,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  20, 55, 55, 40},
    // Trap-style long sub kick
    {"TrapKick", 0, 50, 0, 0,  45, 92, 40, 50,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  50, 30, 80, 3},
    // 808-style long noisy snare
    {"808 Snare", 1, 50, 0, 0,  50, 55, 60, 65,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  34, 45, 58, 60},
    // Kick + Hat tight combo
    {"K+Hat", 8, 45, 6, 10,  58, 62, 40, 50,  68, 25, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  18, 52, 55, 35},
    // Kick + Tom layered
    {"K+Tom", 6, 50, 8, 12,  55, 68, 40, 50,  50, 60, 45, 60,
     0, 0, 0, 0,  0, 0, 0, 0,  24, 48, 58, 10},
    // Kick + Metal industrial
    {"K+Metal", 7, 45, 5, 10,  60, 65, 40, 50,  70, 70, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  96, 45, 58, 18},
    // Snare + Hat
    {"S+Hat", 11, 50, 6, 15,  50, 55, 68, 40,  66, 28, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  20, 55, 48, 50},
    // Tom + Metal
    {"T+Metal", 12, 50, 10, 20,  50, 55, 40, 50,  65, 68, 48, 58,
     0, 0, 0, 0,  0, 0, 0, 0,  98, 48, 55, 15},
    // Tom + Hat
    {"T+Hat", 13, 45, 8, 18,  50, 55, 40, 50,  64, 30, 50, 55,
     0, 0, 0, 0,  0, 0, 0, 0,  22, 50, 52, 30},
    // Metal + Hat shimmer
    {"M+Hat", 14, 50, 6, 12,  50, 55, 40, 50,  70, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  100, 46, 55, 35},
    // Euclidean-tuned metallic cluster
    {"EuclMetal", 3, 50, 0, 0,  50, 55, 40, 50,  68, 72, 50, 40,
     0, 0, 0, 0,  5, 0, 0, 0,  104, 42, 60, 18},
    // Gong drone — very long, slow FM swell (Gong character)
    {"GongDrone", 3, 50, 0, 0,  50, 55, 40, 50,  72, 85, 50, 40,
     2, 20, 2, 35,  0, 0, 0, 0,  248, 40, 72, 20},
    // Metal wash — slow index LFO
    {"MetalWash", 3, 50, 0, 0,  50, 55, 40, 50,  70, 75, 50, 40,
     0, 25, 2, 40,  0, 0, 0, 0,  110, 44, 62, 20},
    // Glitch perc — fast index LFO + scatter
    {"GlitchPrc", 2, 50, 0, 70,  50, 55, 40, 50,  50, 60, 60, 45,
     1, 75, 2, 55,  0, 0, 0, 0,  16, 58, 45, 20},
    // Flam snare — Gap-driven double hit
    {"FlamSnare", 1, 55, 28, 30,  50, 55, 65, 45,  50, 60, 50, 40,
     0, 0, 0, 0,  0, 0, 0, 0,  22, 52, 50, 50},
    // Chaos hit — dual LFO, high scatter
    {"ChaosHit", 12, 50, 14, 75,  50, 55, 40, 50,  66, 62, 58, 50,
     3, 55, 2, 50,  0, 40, 10, 45,  100, 50, 52, 22},
    // Pitch-drop kick — downward pitch LFO
    {"PitchDrop", 0, 50, 0, 0,  70, 55, 40, 50,  50, 60, 50, 40,
     0, 20, 1, -40,  0, 0, 0, 0,  22, 55, 55, 8},
};


void load_fm_preset(uint8_t idx, int8_t *params) {
    if (idx >= NUM_OF_PRESETS || params == NULL) {
        return;
    }

    const fm_preset_t *p = &FM_PRESETS[idx];

    params[PARAM_INSTRUMENT] = p->instrument_sel;
    params[PARAM_BLEND]      = p->blend;
    params[PARAM_GAP]        = p->gap;
    params[PARAM_SCATTER]    = p->scatter;

    params[PARAM_KICK_ATK]   = p->kick_attack;
    params[PARAM_KICK_BODY]  = p->kick_body;
    params[PARAM_SNARE_ATK]  = p->snare_attack;
    params[PARAM_SNARE_BODY] = p->snare_body;

    params[PARAM_METAL_ATK]  = p->metal_attack;
    params[PARAM_METAL_BODY] = p->metal_body;
    params[PARAM_PERC_ATK]   = p->perc_attack;
    params[PARAM_PERC_BODY]  = p->perc_body;

    params[PARAM_LFO1_SHAPE]  = p->lfo1_shape;
    params[PARAM_LFO1_RATE]   = p->lfo1_rate;
    params[PARAM_LFO1_TARGET] = p->lfo1_target;
    params[PARAM_LFO1_DEPTH]  = p->lfo1_depth;

    params[PARAM_EUCL_TUN]    = p->eucl_tun;
    params[PARAM_LFO2_RATE]   = p->lfo2_rate;
    params[PARAM_LFO2_TARGET] = p->lfo2_target;
    params[PARAM_LFO2_DEPTH]  = p->lfo2_depth;

    params[PARAM_ENV_SHAPE]   = p->env_shape;
    params[PARAM_HIT_SHAPE]   = p->hit_shape;
    params[PARAM_BODY_TILT]   = p->body_tilt;
    params[PARAM_NOISE_CHAR]  = p->noise_char;
}
