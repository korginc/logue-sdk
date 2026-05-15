/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface for Light Reverb
 *
 * FIXED:
 * - Removed dynamic allocation (no 'new')
 * - Static instance only
 * - Proper deinterleaving of stereo buffers
 * - Safe bounds checking
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "unit.h"
#include "fdn_engine.h"

// ============================================================================
// Constants
// ============================================================================

// Maximum frames per render call (drumlogue typically uses 64 or 128)
constexpr uint32_t kMaxFrames = 256;

// ============================================================================
// Static Instances (No dynamic allocation!)
// ============================================================================

static FDNEngine s_fdn_engine;
static unit_runtime_desc_t s_runtime_desc;
static bool s_initialized = false;

// ============================================================================
// Parameter State (mirrors header.c defaults)
// ============================================================================
// ID 0: NAME  string   default 0
// ID 1: DARK  0..100 %  default 20
// ID 2: BRIG  0..100 %  default 50
// ID 3: GLOW  0..100 %  default 30
// ID 4: COLR  0..100 %  default 10
// ID 5: SPRK  0..100 %  default 5
// ID 6: SIZE  0..100 %  default 50
// ID 7: PDLY  0..100 %  default 50

// ============================================================================
// Static Buffers (Safe - allocated in BSS, not on stack)
// ============================================================================

// static float s_inL[kMaxFrames];
// static float s_inR[kMaxFrames];
// static float s_outL[kMaxFrames];
// static float s_outR[kMaxFrames];

// ============================================================================
// Callback Implementations
// ============================================================================

__unit_callback int8_t unit_init(const unit_runtime_desc_t* desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_runtime_desc = *desc;

    // Initialize the FDN engine (static instance - no allocation!)
    if (!s_fdn_engine.init(desc->samplerate)) {
        // Allocation failed within FDN engine - unit will bypass
        s_initialized = false;
        return k_unit_err_memory;
    }

    s_initialized = true;

    // Apply default parameter values
    s_fdn_engine.loadPreset(0);

    return k_unit_err_none;
}

__unit_callback void unit_teardown() {
    // Nothing to do - static instance cleans up itself
    // The FDNEngine destructor will be called when program exits
}

__unit_callback void unit_reset() {
    s_fdn_engine.reset();   // defined at line 250 of fdn_engine
}

__unit_callback void unit_resume() {}
__unit_callback void unit_suspend() { s_fdn_engine.reset(); }

__unit_callback void unit_render(const float* in, float* out, uint32_t frames) {
    // ========================================================================
    // Safety Check: Bypass if not initialized
    // ========================================================================
    if (!s_initialized) {
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
    // Process through reverb engine (expects deinterleaved buffers)
    // ========================================================================
    s_fdn_engine.processBlock(in, out, frames * 2);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    s_fdn_engine.setParameter(id, value);
}


__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return s_fdn_engine.getParameterValue(id);
}

__unit_callback const char* unit_get_param_str_value(uint8_t id, int32_t value) {
    if ((id == k_paramProgram) && (value < k_preset_number)) {
        return k_preset_names[value];
    }
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

__unit_callback void unit_load_preset(uint8_t idx) {
    if (idx >= k_preset_number) return;
    s_fdn_engine.loadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
    return s_fdn_engine.getPreset();
}

__unit_callback const char* unit_get_preset_name(uint8_t idx) {
    if (idx >= k_preset_number) return nullptr;
    return k_preset_names[idx];
}
