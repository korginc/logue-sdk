#include "unit.h"
#include "synth.h"

static ScrutaAstri s_synth;

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    return s_synth.Init(desc);
}

__unit_callback void unit_teardown() { }
__unit_callback void unit_reset() { s_synth.Reset(); }
__unit_callback void unit_resume() { }
__unit_callback void unit_suspend() { s_synth.Reset(); }

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    s_synth.processBlock(out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    s_synth.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return 0; // Handled by OS cache
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr; // Handled by OS unless custom strings are needed
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {
    s_synth.NoteOn(note, velocity);
}

__unit_callback void unit_note_off(uint8_t note) { }
__unit_callback void unit_gate_on(uint8_t velocity) { }
__unit_callback void unit_gate_off() { }
__unit_callback void unit_all_note_off() { }
__unit_callback void unit_pitch_bend(uint16_t bend) { }
__unit_callback void unit_channel_pressure(uint8_t pressure) { }
__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) { }