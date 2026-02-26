/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface (Data-Oriented Waveguide Port)
 */

#include "unit.h"
#include "synth_engine.h" // This should #include "dsp_core.h" and "dsp_process.h" internally

// ==============================================================================
// 1. Global Synth Instance
// ==============================================================================
// By declaring this static, it is allocated safely in the Drumlogue's BSS memory
// segment at boot, avoiding any real-time allocation (malloc/new) hazards.
static RipplerXWaveguide s_synth;
static unit_runtime_desc_t s_runtime_desc;

// ==============================================================================
// 2. Lifecycle Callbacks
// ==============================================================================
__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_runtime_desc = *desc;

    // Hand over initialization to our synth engine
    return s_synth.Init(desc);
}

__unit_callback void unit_teardown() { s_synth.Teardown(); }
__unit_callback void unit_reset()    { s_synth.Reset(); }
__unit_callback void unit_resume()   { s_synth.Resume(); }
__unit_callback void unit_suspend()  { s_synth.Suspend(); }

// ==============================================================================
// 3. The Real-Time Audio Thread
// ==============================================================================
__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    // The OS gives us the output buffer ('out') and the number of frames to calculate.
    // We pass it directly to our lightning-fast inline DSP loop.
    s_synth.processBlock(out, frames);
}

// ==============================================================================
// 4. UI / Parameter Callbacks
// ==============================================================================
__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    // Routes UI knob turns to your pre-calculation logic
    s_synth.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    // TODO: Return your UI tracking variables (m_ui_note, a_b_decay, etc.)
    // based on the requested 'id' so the screen updates correctly on preset load.
    return 0;
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr; // Drumlogue defaults to rendering the raw number if nullptr
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
    return nullptr;
}

// ==============================================================================
// 5. MIDI & Sequencer Callbacks
// ==============================================================================
__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {
    s_synth.NoteOn(note, velocity);
}

__unit_callback void unit_note_off(uint8_t note) {
    // s_synth.NoteOff(note); // Uncomment when Envelope Release is implemented
}

__unit_callback void unit_gate_on(uint8_t velocity) {
    s_synth.GateOn(velocity);
}

__unit_callback void unit_gate_off() {
    // s_synth.GateOff(); // Uncomment when Envelope Release is implemented
}

__unit_callback void unit_all_note_off() {
    // Zero out the delay lines or fast-release all active voices here
}

__unit_callback void unit_pitch_bend(uint16_t bend) {
    // s_synth.PitchBend(bend); // Uncomment when bend math is implemented
}

__unit_callback void unit_channel_pressure(uint8_t pressure) { }
__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) { }