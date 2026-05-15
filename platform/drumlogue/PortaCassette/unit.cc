/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface for PortaCassette
 */

#include "unit.h"
#include "PortaCassette.h" // We will build this next
#include <cstdio>

// Static instance of the DSP engine
static PortaCassette s_porta_instance;

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    return s_porta_instance.Init(desc);
}

__unit_callback void unit_teardown() {
    // Engine automatically cleans up in destructor
}

__unit_callback void unit_reset() {
    s_porta_instance.Reset();
}

__unit_callback void unit_resume() {
    s_porta_instance.SetBypass(false);
}

__unit_callback void unit_suspend() {
    s_porta_instance.SetBypass(true);
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    s_porta_instance.ProcessBlock(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    s_porta_instance.SetParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return s_porta_instance.GetParameter(id);
}

// Custom String formatting for the OLED Display
__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    static char buf[16];

    switch(id) {
        // Model Selection
        case PortaCassette::k_param_model:
            if (value == PortaCassette::k_244)   return "T-244";
            if (value == PortaCassette::k_424)   return "T-424";
            if (value == PortaCassette::k_488)   return "T-488";
            if (value == PortaCassette::k_vinyl) return "Vinyl";
            break;

        // dbx Noise Reduction Modes
        case PortaCassette::k_param_dbx:
            if (value == PortaCassette::k_active)       return "Active";
            if (value == PortaCassette::k_encode_only)  return "EncOnly"; // The classic Portastudio abuse trick
            if (value == PortaCassette::k_off)          return "Off";
            break;

        // EQ Frequencies
        case PortaCassette::k_param_eq_low_hz:
        case PortaCassette::k_param_eq_mid_hz:
            snprintf(buf, sizeof(buf), "%d Hz", (int)(value * 10));
            return buf;

        case PortaCassette::k_param_eq_high_hz:
            // Multiply by 100 for high frequencies (e.g., 20 = 2000Hz)
            snprintf(buf, sizeof(buf), "%d.0k", (int)(value / 10));
            return buf;

        // EQ Gains (-12dB to +12dB)
        case PortaCassette::k_param_eq_low_gain:
        case PortaCassette::k_param_eq_mid_gain:
        case PortaCassette::k_param_eq_high_gain:
            {
                // UI is 0-240. Center is 120. Scale to -12.0 to +12.0
                float db = (float)(value - 120) * 0.1f;
                snprintf(buf, sizeof(buf), "%+.1f dB", db);
                return buf;
            }

        // Crosstalk Bleed
        case PortaCassette::k_crosstalk:
            snprintf(buf, sizeof(buf), "%d %%", (int)value);
            return buf;
    }

    return nullptr; // Let the OS handle the standard % displays
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
    return nullptr;
}

// ---- MIDI / Tempo stubs (not used) ----------------------
__unit_callback void unit_set_tempo(uint32_t tempo) { (void)tempo; }
__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {}
__unit_callback void unit_note_off(uint8_t note) {}
__unit_callback void unit_gate_on(uint8_t velocity) {}
__unit_callback void unit_gate_off() {}
__unit_callback void unit_all_note_off() {}
__unit_callback void unit_pitch_bend(uint16_t bend) {}
__unit_callback void unit_channel_pressure(uint8_t pressure) {}
__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) {}