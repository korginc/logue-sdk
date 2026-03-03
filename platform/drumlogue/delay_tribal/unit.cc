/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface for Percussion Spatializer
 */

#include "unit.h"
#include "PercussionSpatializer.h"

static PercussionSpatializer s_delay_instance;
static unit_runtime_desc_t s_runtime_desc;

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_runtime_desc = *desc;
    return s_delay_instance.Init(desc);
}

__unit_callback void unit_teardown() {
    // Engine automatically cleans up in destructor
}

__unit_callback void unit_reset() {
    s_delay_instance.Reset();
}

__unit_callback void unit_resume() {}
__unit_callback void unit_suspend() {}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    // drumlogue provides interleaved stereo blocks.
    // The class handles the interleaved extraction natively now.
    s_delay_instance.Process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    s_delay_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return 0;
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr;
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
    return nullptr;
}