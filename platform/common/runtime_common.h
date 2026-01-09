/**
 * @file runtime_common.h
 * @brief runtime definitions common to all platforms
 *
 * Copyright (c) 2025 KORG Inc. All rights reserved.
 *
 */

#ifndef RUNTIME_COMMON_H_
#define RUNTIME_COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include "attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UNIT_TARGET_PLATFORM_INVALID (-1)
#define UNIT_API_VERSION_INVALID (-1)

/** @private */
#define UNIT_HEADER_SIZE (0x1000)  // 4KB // TODO: could shrink this

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

typedef void unit_runtime_base_context_t;

#define unit_runtime_base_context_fields  // none for now

/**
 * SDRAM allocator callback type
 *
 * The callback is used to allocate memory in a given unit runtime's dedicated SDRAM area.
 * \param size Size in bytes of desired memory allocation.
 * \return     Pointer to allocated memory area, or NULL if unsucessful.
 */
typedef uint8_t * (*unit_runtime_sdram_alloc_ptr)(size_t size);

/**
 * SDRAM deallocation callback type
 *
 * The callback is used to deallocate memory from the runtime's dedicated SDRAM area.
 * \param mem Pointer to memory area previously allocated via a corresponsding unit_runtime_sdram_alloc_ptr callback.
 */
typedef void (*unit_runtime_sdram_free_ptr)(const uint8_t *mem);

/**
 * SDRAM allocation availability callback type
 *
 * The callback is used to verify the amount of allocatable memory in the runtime's dedicated SDRAM area.
 * \return Size in bytes of the allocatable memory.
 */
typedef size_t (*unit_runtime_sdram_avail_ptr)(void);

/**
 * Runtime hooks.
 * Passed from the unit runtime to the unit during initialization to provide access to runtime APIs that are not known at compile time.
 * Note: Member fields will vary from platform to platform
 */
typedef struct unit_runtime_hooks {
  const unit_runtime_base_context_t *runtime_context; /** Ref. to contextual data exposed by the runtime. (Req. cast to appropriate module-specific type) */
  unit_runtime_sdram_alloc_ptr sdram_alloc;           /** SDRAM allocation callback. */
  unit_runtime_sdram_free_ptr sdram_free;             /** SDRAM deallocation callback. */
  unit_runtime_sdram_avail_ptr sdram_avail;           /** SDRAM availability callback. */
} unit_runtime_hooks_t;

/** @private */
#pragma pack(push, 1)
typedef struct unit_runtime_desc {
  uint16_t target;
  uint32_t api;
  uint32_t samplerate;
  uint16_t frames_per_buffer;
  uint8_t input_channels;
  uint8_t output_channels;
  unit_runtime_hooks_t hooks;
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
typedef void (*unit_platform_exclusive_func)(uint8_t, void *, uint32_t);     // sym: unit_platform_exclusive

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RUNTIME_COMMON_H_
