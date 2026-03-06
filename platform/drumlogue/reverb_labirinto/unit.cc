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
    if (!s_reverb) return;

    // Map parameters to reverb controls
    switch (id) {
        case 0: // Decay (0-100%)
            s_reverb->setDecay(value / 100.0f);
            break;
        case 1: // Diffusion (0-100%)
            s_reverb->setDiffusion(value / 100.0f);
            break;
        case 2: // Modulation Depth (0-100%)
            s_reverb->setModDepth(value / 100.0f);
            break;
        case 3: // Modulation Rate (0-100% scaled to 0.1-10 Hz)
            s_reverb->setModRate(0.1f + (value / 100.0f) * 9.9f);
            break;
        default:
            break;
    }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    // Return current parameter values
    // This would need to be implemented based on your parameter scheme
    (void)id;
    return 0;
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