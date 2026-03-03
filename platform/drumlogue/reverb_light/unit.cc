#include "unit.h"
#include "fdn_engine.h"

static FDNLightReverb s_reverb;

// ---- Callback entry points from drumlogue runtime ----

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_reverb.Init(desc->samplerate);
    return k_unit_err_none;
}

__unit_callback void unit_teardown() {
    s_reverb.Teardown();
}

__unit_callback void unit_reset() {
    s_reverb.Reset();
}

__unit_callback void unit_resume() {}
__unit_callback void unit_suspend() {}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
    // Drumlogue processing block: Split interleaved to L/R
    float inL[frames], inR[frames];
    float outL[frames], outR[frames];

    for (uint32_t i = 0; i < frames; ++i) {
        inL[i] = in[i * 2];
        inR[i] = in[i * 2 + 1];
    }

    s_reverb.process(inL, inR, outL, outR, frames);

    // Write back to master interleaved bus
    for (uint32_t i = 0; i < frames; ++i) {
        out[i * 2]     = outL[i];
        out[i * 2 + 1] = outR[i];
    }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    const float inv1023 = 1.0f / 1023.0f;
    float valF = (float)value * inv1023;

    switch (id) {
        case 0: s_reverb.p.darkness = valF; break;
        case 1: s_reverb.p.brightness = valF; break;
        case 2: s_reverb.p.glow = valF; break;
        case 3: s_reverb.p.color = valF * 0.7f; break; // Limitata per sicurezza
        case 4: s_reverb.p.sparkle = valF; break;
        case 5: s_reverb.p.size = 0.5f + (valF * 9.5f); break; // 0.5x -> 10.0x
    }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return 0; // Not strictly necessary unless you want to sync UI updates back
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
    return nullptr;
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
    return nullptr;
}