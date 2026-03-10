/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface for Labirinto Reverb
 *
 * FIXED:
 * - Proper deinterleaving of input buffers
 * - Correct interleaving of output buffers
 * - Static buffers with bounds checking
 * - Safe bypass mode
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "unit.h"
#include "NeonAdvancedLabirinto.h"

// ============================================================================
// Constants
// ============================================================================

// Maximum frames per render call (drumlogue typically uses 64 or 128)
constexpr uint32_t kMaxFrames = 256;

// ============================================================================
// Static Instances
// ============================================================================

static NeonAdvancedLabirinto* s_reverb = nullptr;
static unit_runtime_desc_t s_runtime_desc;
static bool s_initialized = false;
static bool s_bypass = true;

// ============================================================================
// Parameter State (mirrors header.c defaults)
// ============================================================================
// ID 0: MIX  0..1000 (x0.1%)   default 300
// ID 1: TIME 1..100             default 20
// ID 2: LOW  1..100             default 20
// ID 3: HIGH 1..100             default 10
// ID 4: DAMP 200..10000 Hz      default 2500
// ID 5: WIDE 0..200 %           default 100
// ID 6: COMP 0..1000 (x0.1%)   default 1000
// ID 7: PILL 0..3               default 3
static int32_t s_params[8] = { 300, 20, 20, 10, 2500, 100, 1000, 3 };

// ============================================================================
// Static Buffers (Safe - allocated in BSS, not on stack)
// ============================================================================

static float s_inL[kMaxFrames];
static float s_inR[kMaxFrames];
static float s_outL[kMaxFrames];
static float s_outR[kMaxFrames];

// ============================================================================
// Callback Implementations
// ============================================================================

__unit_callback int8_t unit_init(const unit_runtime_desc_t* desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_runtime_desc = *desc;

    // Create reverb instance
    s_reverb = new NeonAdvancedLabirinto();
    if (!s_reverb) {
        s_initialized = false;
        s_bypass = true;
        return k_unit_err_memory;
    }

    // Initialize with memory allocation
    if (!s_reverb->init()) {
        delete s_reverb;
        s_reverb = nullptr;
        s_initialized = false;
        s_bypass = true;
        return k_unit_err_memory;
    }

    s_initialized = true;
    s_bypass = false;

    // Apply default parameter values
    for (uint8_t i = 0; i < 8; i++) {
        unit_set_param_value(i, s_params[i]);
    }

    return k_unit_err_none;
}

__unit_callback void unit_teardown() {
    if (s_reverb) {
        delete s_reverb;
        s_reverb = nullptr;
    }
    s_initialized = false;
    s_bypass = true;
}

__unit_callback void unit_reset() {
    // Reset reverb state if needed
    // Note: Would need a reset method in NeonAdvancedLabirinto
}

__unit_callback void unit_resume() {}
__unit_callback void unit_suspend() {}

__unit_callback void unit_render(const float* in, float* out, uint32_t frames) {
    // ========================================================================
    // Safety Check: Bypass if not initialized
    // ========================================================================
    if (!s_initialized || !s_reverb || s_bypass) {
        memcpy(out, in, frames * 2 * sizeof(float));
        return;
    }

    // ========================================================================
    // Bounds Check - Prevent buffer overflow
    // ========================================================================
    if (frames > kMaxFrames) {
        // This should never happen with drumlogue, but safety first
        memcpy(out, in, frames * 2 * sizeof(float));
        return;
    }

    // ========================================================================
    // FIXED: Deinterleave input: [L,R,L,R,...] -> separate L and R buffers
    // ========================================================================
    for (uint32_t i = 0; i < frames; i++) {
        s_inL[i] = in[i * 2];
        s_inR[i] = in[i * 2 + 1];
    }

    // ========================================================================
    // Process through reverb engine (expects deinterleaved buffers)
    // ========================================================================
    s_reverb->process(s_inL, s_inR, s_outL, s_outR, frames);

    // ========================================================================
    // FIXED: Interleave output: separate L/R buffers -> [L,R,L,R,...]
    // ========================================================================
    for (uint32_t i = 0; i < frames; i++) {
        out[i * 2] = s_outL[i];
        out[i * 2 + 1] = s_outR[i];
    }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    if (id >= 8) return;
    s_params[id] = value;
    if (!s_reverb) return;

    switch (id) {
        case 0: // MIX  0..1000 → 0.0..1.0
            s_reverb->setMix(value / 1000.0f);
            break;
        case 1: // TIME  1..100 → decay 0.01..0.99  (linear scale of RT60 x0.1s)
            s_reverb->setDecay(0.01f + (value - 1) / 99.0f * 0.98f);
            break;
        case 2: // LOW  1..100 → low-freq decay multiplier
            s_reverb->setLowDecay((float)value);
            break;
        case 3: // HIGH  1..100 → high-freq decay multiplier
            s_reverb->setHighDecay((float)value);
            break;
        case 4: // DAMP  200..10000 Hz
            s_reverb->setDamping((float)value);
            break;
        case 5: // WIDE  0..200 → stereo width 0.0..2.0
            s_reverb->setWidth(value / 100.0f);
            break;
        case 6: // COMP  0..1000 → diffusion 0.0..1.0
            s_reverb->setDiffusion(value / 1000.0f);
            break;
        case 7: // PILL  0..3  (pillar count index, stored for info)
            // FDN channel count is fixed at compile time; store only
            break;
        default:
            break;
    }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    if (id >= 8) return 0;
    return s_params[id];
}

__unit_callback const char* unit_get_param_str_value(uint8_t id, int32_t value) {
    (void)id;
    (void)value;
    return nullptr;
}

__unit_callback const uint8_t* unit_get_param_bmp_value(uint8_t id, int32_t value) {
    (void)id;
    (void)value;
    return nullptr;
}

// Unused MIDI callbacks
__unit_callback void unit_set_tempo(uint32_t tempo) { (void)tempo; }
__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
__unit_callback void unit_note_off(uint8_t note) { (void)note; }
__unit_callback void unit_gate_on(uint8_t velocity) { (void)velocity; }
__unit_callback void unit_gate_off() {}
__unit_callback void unit_all_note_off() {}
__unit_callback void unit_pitch_bend(uint16_t bend) { (void)bend; }
__unit_callback void unit_channel_pressure(uint8_t pressure) { (void)pressure; }
__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note;
    (void)aftertouch;
}

__unit_callback void unit_load_preset(uint8_t idx) { (void)idx; }
__unit_callback uint8_t unit_get_preset_index() { return 0; }
__unit_callback const char* unit_get_preset_name(uint8_t idx) { return nullptr; }