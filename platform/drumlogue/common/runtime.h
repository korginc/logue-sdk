/**
 * @file runtime.h
 * @brief Common runtime definitions
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#ifndef RUNTIME_H_
#define RUNTIME_H_

#include <stdint.h>

#include "sample_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   * Module categories.
   */
enum {
  /** Dummy category, may be used in future. */
  k_unit_module_global = 0U,
  /** Modulation effects */
  k_unit_module_modfx,
  /** Delay effects */
  k_unit_module_delfx,
  /** Reverb effects */
  k_unit_module_revfx,
  /** Oscillators */
  k_unit_module_osc,
  /** Synth voices */
  k_unit_module_synth,
  /** Master effects */
  k_unit_module_masterfx,
  k_num_unit_modules,
};

/**
   * prologue specific platform/module pairs. Passed to user code via initialization callback.
   */
enum {
  k_unit_target_prologue = (1U << 8),
  k_unit_target_prologue_modfx = (1U << 8) | k_unit_module_modfx,
  k_unit_target_prologue_delfx = (1U << 8) | k_unit_module_delfx,
  k_unit_target_prologue_revfx = (1U << 8) | k_unit_module_revfx,
  k_unit_target_prologue_osc = (1U << 8) | k_unit_module_osc,
};

/**
   * minilogue xd specific platform/module pairs. Passed to user code via initialization callback.
   */
enum {
  k_unit_target_miniloguexd = (2U << 8),
  k_unit_target_miniloguexd_modfx = (2U << 8) | k_unit_module_modfx,
  k_unit_target_miniloguexd_delfx = (2U << 8) | k_unit_module_delfx,
  k_unit_target_miniloguexd_revfx = (2U << 8) | k_unit_module_revfx,
  k_unit_target_miniloguexd_osc = (2U << 8) | k_unit_module_osc,
};

/**
   * Nu:Tekt NTS-1 digital specific platform/module pairs. Passed to user code via initialization callback.
   */
enum {
  k_unit_target_nutektdigital = (3U << 8),
  k_unit_target_nutektdigital_modfx = (3U << 8) | k_unit_module_modfx,
  k_unit_target_nutektdigital_delfx = (3U << 8) | k_unit_module_delfx,
  k_unit_target_nutektdigital_revfx = (3U << 8) | k_unit_module_revfx,
  k_unit_target_nutektdigital_osc = (3U << 8) | k_unit_module_osc,
};

/**
   * drumlogue specific platform/module pairs. Passed to user code via initialization callback.
   */
enum {
  k_unit_target_drumlogue = (4U << 8),
  k_unit_target_drumlogue_delfx = (4U << 8) | k_unit_module_delfx,
  k_unit_target_drumlogue_revfx = (4U << 8) | k_unit_module_revfx,
  k_unit_target_drumlogue_synth = (4U << 8) | k_unit_module_synth,
  k_unit_target_drumlogue_masterfx = (4U << 8) | k_unit_module_masterfx,
};

/** Current platform */
#define UNIT_TARGET_PLATFORM (k_unit_target_drumlogue)
#define UNIT_TARGET_PLATFORM_MASK (0x7F << 8)
#define UNIT_TARGET_MODULE_MASK (0x7F)

/** @private */
#define UNIT_TARGET_PLATFORM_IS_COMPAT(tgt) \
  (((tgt)&UNIT_TARGET_PLATFORM_MASK) == k_unit_target_drumlogue)

/**
   *  Existing API versions
   *
   *  Major: breaking changes (7bits, cap to 99)
   *  Minor: additions only   (7bits, cap to 99)
   *  Sub:   bugfixes only    (7bits, cap to 99)
   */
enum {
  k_unit_api_1_0_0 = ((1U << 16) | (0U << 8) | (0U)),
  k_unit_api_1_1_0 = ((1U << 16) | (1U << 8) | (0U)),
  k_unit_api_2_0_0 = ((2U << 16) | (0U << 8) | (0U))
};

/** API version targeted by this code */
#define UNIT_API_VERSION (k_unit_api_2_0_0)
#define UNIT_API_MAJOR_MASK (0x7F << 16)
#define UNIT_API_MINOR_MASK (0x7F << 8)
#define UNIT_API_PATCH_MASK (0x7F)
#define UNIT_API_MAJOR(v) ((v) >> 16 & 0x7F)
#define UNIT_API_MINOR(v) ((v) >> 8 & 0x7F)
#define UNIT_API_PATCH(v) ((v)&0x7F)

/** @private */
#define UNIT_API_IS_COMPAT(api) \
  ((((api)&UNIT_API_MAJOR_MASK) == (UNIT_API_VERSION & UNIT_API_MAJOR_MASK)) && (((api)&UNIT_API_MINOR_MASK) <= (UNIT_API_VERSION & UNIT_API_MINOR_MASK)))

#define UNIT_VERSION_MAJOR_MASK (0x7F << 16)
#define UNIT_VERSION_MINOR_MASK (0x7F << 8)
#define UNIT_VERSION_PATCH_MASK (0x7F)

/** @private */
#define UNIT_VERSION_IS_COMPAT(v1, v2) \
  ((((v1)&UNIT_API_MAJOR_MASK) == ((v2)&UNIT_API_MAJOR_MASK)) && (((v1)&UNIT_API_MINOR_MASK) <= ((v2)&UNIT_API_MINOR_MASK)))

/** @private */
#define UNIT_HEADER_SIZE (0x1000)  // 4KB // TODO: could shrink this

/** @private */
#define UNIT_MAX_PARAM_COUNT (24)

/** @private */
#define UNIT_PARAM_NAME_LEN (12)

/** @private */
#define UNIT_NAME_LEN (13)

/** @private */
enum {
  k_unit_param_type_none = 0U,
  k_unit_param_type_percent,
  k_unit_param_type_db,
  k_unit_param_type_cents,
  k_unit_param_type_semi,
  k_unit_param_type_oct,
  k_unit_param_type_hertz,
  k_unit_param_type_khertz,
  k_unit_param_type_bpm,
  k_unit_param_type_msec,
  k_unit_param_type_sec,
  k_unit_param_type_enum,
  k_unit_param_type_strings,  // key for a string value table
  k_unit_param_type_bitmaps,  // key for a bitmap value table
  k_unit_param_type_drywet,
  k_unit_param_type_pan,
  k_unit_param_type_spread,
  k_unit_param_type_onoff,
  k_unit_param_type_midi_note,
  k_num_unit_param_type
};

/** @private */
enum {
  k_unit_param_frac_mode_fixed = 0U,
  k_unit_param_frac_mode_decimal = 1U,
};
  
/** @private */
#pragma pack(push, 1)
typedef struct unit_param {
  int16_t min;
  int16_t max;
  int16_t center;
  int16_t init;
  uint8_t type;
  uint8_t frac : 4;       // fractional bits / decimals according to frac_mode
  uint8_t frac_mode : 1;  // 0: fixed point, 1: decimal
  uint8_t reserved : 3;
  char name[UNIT_PARAM_NAME_LEN + 1];
} unit_param_t;  // 24 bytes
#pragma pack(pop)

/** @private */
#pragma pack(push, 1)
typedef struct unit_header {
  uint32_t header_size;
  uint16_t target;
  uint32_t api;
  uint32_t dev_id;
  uint32_t unit_id;
  uint32_t version;
  char name[UNIT_NAME_LEN + 1];
  uint32_t num_presets;
  uint32_t num_params;
  unit_param_t params[UNIT_MAX_PARAM_COUNT];
} unit_header_t;
#pragma pack(pop)

typedef uint8_t (*unit_runtime_get_num_sample_banks_ptr)();
typedef uint8_t (*unit_runtime_get_num_samples_for_bank_ptr)(uint8_t);
typedef const sample_wrapper_t * (*unit_runtime_get_sample_ptr)(uint8_t, uint8_t);

/** @private */
#pragma pack(push, 1)
typedef struct unit_runtime_desc {
  uint16_t target;
  uint32_t api;
  uint32_t samplerate;
  uint16_t frames_per_buffer;
  uint8_t input_channels;
  uint8_t output_channels;
  unit_runtime_get_num_sample_banks_ptr get_num_sample_banks;
  unit_runtime_get_num_samples_for_bank_ptr get_num_samples_for_bank;
  unit_runtime_get_sample_ptr get_sample;
} unit_runtime_desc_t;
#pragma pack(pop)

enum {
  k_unit_err_none = 0,
  k_unit_err_target = -1,
  k_unit_err_api_version = -2,
  k_unit_err_samplerate = -4,
  k_unit_err_geometry = -8,
  k_unit_err_memory = -16,
  k_unit_err_undef = -32,
};

/** @private */
typedef int8_t (*unit_init_func)(const unit_runtime_desc_t *);               // sym: unit_init
typedef void (*unit_teardown_func)();                                        // sym: unit_teardown
typedef void (*unit_reset_func)();                                           // sym: unit_reset
typedef void (*unit_resume_func)();                                          // sym: unit_resume
typedef void (*unit_suspend_func)();                                         // sym: unit_suspend
typedef void (*unit_render_func)(const float *, float *, uint32_t);          // sym: unit_render
typedef uint8_t (*unit_get_preset_index_func)();                             // sym: unit_get_preset_index
typedef const char * (*unit_get_preset_name_func)(uint8_t);                  // sym: unit_get_preset_name
typedef void (*unit_load_preset_func)(uint8_t);                              // sym: unit_load_preset
typedef int32_t (*unit_get_param_value_func)(uint8_t);                       // sym: unit_get_param_value
typedef const char * (*unit_get_param_str_value_func)(uint8_t, int32_t);     // sym: unit_get_param_str_value
typedef const uint8_t * (*unit_get_param_bmp_value_func)(uint8_t, int32_t);  // sym: unit_get_param_bmp_value
typedef void (*unit_set_param_value_func)(uint8_t, int32_t);                 // sym: unit_set_param_value
typedef void (*unit_set_tempo_func)(uint32_t);                               // sym: unit_set_tempo
typedef void (*unit_note_on_func)(uint8_t, uint8_t);                         // sym: unit_note_on
typedef void (*unit_note_off_func)(uint8_t);                                 // sym: unit_note_off
typedef void (*unit_gate_on_func)(uint8_t);                                  // sym: unit_gate_on
typedef void (*unit_gate_off_func)();                                        // sym: unit_gate_off
typedef void (*unit_all_note_off_func)(void);                                // sym: unit_all_note_off
typedef void (*unit_pitch_bend_func)(uint16_t);                              // sym: unit_pitch_bend
typedef void (*unit_channel_pressure_func)(uint8_t);                         // sym: unit_channel_pressure
typedef void (*unit_aftertouch_func)(uint8_t, uint8_t);                      // sym: unit_aftertouch

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RUNTIME_H_
