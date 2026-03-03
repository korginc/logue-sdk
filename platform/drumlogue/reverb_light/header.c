#include "unit.h"

__attribute__((section(".unit_header"), used))
const unit_header_t unit_header = {
  .header_size = sizeof(unit_header_t),
  .target = k_unit_module_revfx,
  .api = UNIT_API_VERSION,
  .dev_id = 0x00000000U,
  .unit_id = 0x00020000U,
  .version = 0x00010000U,
  .name = "LuceAlNeon",
  .num_presets = 0,
  .num_params = 6,
  .params = {
        { .id = 0, .name = "DARK", .min = 0, .max = 1023, .center = 512, .default = 200, .type = k_unit_param_type_percent },
        { .id = 1, .name = "BRIG", .min = 0, .max = 1023, .center = 512, .default = 512, .type = k_unit_param_type_percent },
        { .id = 2, .name = "GLOW", .min = 0, .max = 1023, .center = 512, .default = 300, .type = k_unit_param_type_percent },
        { .id = 3, .name = "COLR", .min = 0, .max = 1023, .center = 512, .default = 100, .type = k_unit_param_type_percent },
        { .id = 4, .name = "SPRK", .min = 0, .max = 1023, .center = 512, .default = 50,  .type = k_unit_param_type_percent },
        { .id = 5, .name = "SIZE", .min = 0, .max = 1023, .center = 512, .default = 512, .type = k_unit_param_type_percent },

        // Pad remainder of 24 slots required by SDK
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};