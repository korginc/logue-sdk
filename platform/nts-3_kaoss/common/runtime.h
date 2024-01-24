/*
    BSD 3-Clause License

    Copyright (c) 2018-2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/**
 *  @file runtime.h
 *
 *  @brief Common runtime definitions
 *
 * @addtogroup runtime RUNTIME
 * @{
 */

#ifndef RUNTIME_H_
#define RUNTIME_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Unit module categories.
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
    /** Generic effects, see: NTS-3 kaoss pad kit */
    k_unit_module_genericfx,
    k_num_unit_modules,
  };

  /**
   * prologue specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_prologue        = (1U<<8),
    k_unit_target_prologue_global = (1U<<8) | k_unit_module_global,
    k_unit_target_prologue_modfx  = (1U<<8) | k_unit_module_modfx,
    k_unit_target_prologue_delfx  = (1U<<8) | k_unit_module_delfx,
    k_unit_target_prologue_revfx  = (1U<<8) | k_unit_module_revfx,
    k_unit_target_prologue_osc    = (1U<<8) | k_unit_module_osc,
  };

  /**
   * minilogue-xd specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_miniloguexd        = (2U<<8),
    k_unit_target_miniloguexd_global = (2U<<8) | k_unit_module_global,
    k_unit_target_miniloguexd_modfx  = (2U<<8) | k_unit_module_modfx,
    k_unit_target_miniloguexd_delfx  = (2U<<8) | k_unit_module_delfx,
    k_unit_target_miniloguexd_revfx  = (2U<<8) | k_unit_module_revfx,
    k_unit_target_miniloguexd_osc    = (2U<<8) | k_unit_module_osc,
  };

  /**
   * Nu:Tekt NTS-1 digital kit specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_nutektdigital        = (3U<<8),
    k_unit_target_nutektdigital_global = (3U<<8) | k_unit_module_global,
    k_unit_target_nutektdigital_modfx  = (3U<<8) | k_unit_module_modfx,
    k_unit_target_nutektdigital_delfx  = (3U<<8) | k_unit_module_delfx,
    k_unit_target_nutektdigital_revfx  = (3U<<8) | k_unit_module_revfx,
    k_unit_target_nutektdigital_osc    = (3U<<8) | k_unit_module_osc,
  };

  /**
   * Aliases for Nu:Tekt NTS-1 digital kit specific platform/module pairs.
   */
  enum {
    k_unit_target_nts1        = k_unit_target_nutektdigital,
    k_unit_target_nts1_global = k_unit_target_nutektdigital_global,
    k_unit_target_nts1_modfx  = k_unit_target_nutektdigital_modfx,
    k_unit_target_nts1_delfx  = k_unit_target_nutektdigital_delfx,
    k_unit_target_nts1_revfx  = k_unit_target_nutektdigital_revfx,
    k_unit_target_nts1_osc    = k_unit_target_nutektdigital_osc,
  };

  /**
   * drumlogue specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_drumlogue          = (4U << 8),
    k_unit_target_drumlogue_delfx    = (4U << 8) | k_unit_module_delfx,
    k_unit_target_drumlogue_revfx    = (4U << 8) | k_unit_module_revfx,
    k_unit_target_drumlogue_synth    = (4U << 8) | k_unit_module_synth,
    k_unit_target_drumlogue_masterfx = (4U << 8) | k_unit_module_masterfx,
  };

  /**
   * Nu:Tekt NTS-1 digital kit mkII specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_nts1_mkii        = (5U<<8),
    k_unit_target_nts1_mkii_global = (5U<<8) | k_unit_module_global,
    k_unit_target_nts1_mkii_modfx  = (5U<<8) | k_unit_module_modfx,
    k_unit_target_nts1_mkii_delfx  = (5U<<8) | k_unit_module_delfx,
    k_unit_target_nts1_mkii_revfx  = (5U<<8) | k_unit_module_revfx,
    k_unit_target_nts1_mkii_osc    = (5U<<8) | k_unit_module_osc,
  };

  /**
   * Nu:Tekt NTS-3 kaoss pad kit specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_nts3_kaoss           = (6U<<8),
    k_unit_target_nts3_kaoss_global    = (6U<<8) | k_unit_module_global,
    k_unit_target_nts3_kaoss_genericfx = (6U<<8) | k_unit_module_genericfx,
  };
  
/** Current target platform */
#define UNIT_TARGET_PLATFORM      (k_unit_target_nts3_kaoss)

/** Mask to obtain platform component from a platform/module pair */
#define UNIT_TARGET_PLATFORM_MASK (0x7F<<8)

/** Mask to obtain module component from a platform/module pair */
#define UNIT_TARGET_MODULE_MASK   (0x7F)

/** Checks whether a given target platform is compatible with the current platform */
#define UNIT_TARGET_PLATFORM_IS_COMPAT(tgt)                             \
  ((((tgt)&UNIT_TARGET_PLATFORM_MASK)==k_unit_target_nts3_kaoss))

  /** Valid API versions.
   *  Major: breaking changes (7bits, cap to 99)
   *  Minor: additions only   (7bits, cap to 99)
   *  Sub:   bugfixes only    (7bits, cap to 99)
   */
  enum {
    k_unit_api_1_0_0 = ((1U<<16) | (0U<<8) | (0U)),
    k_unit_api_1_1_0 = ((1U<<16) | (1U<<8) | (0U)),
    k_unit_api_2_0_0 = ((2U<<16) | (0U<<8) | (0U))
  };

/** Current API version */
#define UNIT_API_VERSION    (k_unit_api_2_0_0)

/** Mask for major component of an API version */
#define UNIT_API_MAJOR_MASK (0x7F<<16)

/** Mask for minor component of an API version */
#define UNIT_API_MINOR_MASK (0x7F<<8)

/** Mask for patch component of an API version */
#define UNIT_API_PATCH_MASK (0x7F)

/** Obtain major component of an API version */
#define UNIT_API_MAJOR(v)   ((v)>>16 & 0x7F)

/** Obtain minor component of an API version */
#define UNIT_API_MINOR(v)   ((v)>>8  & 0x7F)

/** Obtain patch component of an API version */
#define UNIT_API_PATCH(v)   ((v)     & 0x7F)

/** Check whether a given API version is compatible with the current API version */
#define UNIT_API_IS_COMPAT(api)                                         \
  ((((api) & UNIT_API_MAJOR_MASK) == (UNIT_API_VERSION & UNIT_API_MAJOR_MASK)) \
   && (((api) & UNIT_API_MINOR_MASK) <= (UNIT_API_VERSION & UNIT_API_MINOR_MASK)))

  /**
   * Base type for runtime contexts.
   * Currently void as there are no common contextual fields accross unit types.
   */
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

  /**
   * Runtime descriptor.
   * Passed from a runtime to units during initialization to provide information about the current runtime environment and expose additional APIs.
   */
#pragma pack(push, 1)
  typedef struct unit_runtime_desc {
    uint32_t target;            /** Target platform/module pair corresponding to runtime. */
    uint32_t api;               /** API version used by runtime. */
    uint32_t samplerate;        /** Sample rate used by runtime. */
    uint16_t frames_per_buffer; /** Frames per buffer used by runtime in render calls. */
    uint8_t input_channels;     /** Number of input channels in render calls. */ 
    uint8_t output_channels;    /** Number of output channels in render calls. */
    unit_runtime_hooks_t hooks; /** Additional API hooks exposed by the runtime. @see unit_runtime_hooks_t */
  } unit_runtime_desc_t;
#pragma pack(pop)

/** Maximum number of exposed parameters per unit
 * @note Each unit module may also have a maximum parameter count less or equal to this limit. See unit_<module>.h headers.
 */
#define UNIT_MAX_PARAM_COUNT (8)

/** Maximum length for parameter names in descriptors */
#define UNIT_PARAM_NAME_LEN  (21)
#define UNIT_PARAM_NAME_SIZE (UNIT_PARAM_NAME_LEN+1)

/** Maximum length for unit name in descriptor */
#define UNIT_NAME_LEN        (19)
#define UNIT_NAME_SIZE       (UNIT_NAME_LEN+1)

  /**
   * Valid parameter value types.
   * Influences how the value is displayed, but can be limited by the display technology used for the current target platform.
   */
  enum {
    k_unit_param_type_none = 0U, /** Describes a typeless value. The value will be displayed as is, while taking into account the fractional part. */
    k_unit_param_type_percent,   /** Describe a percent value. A '%' character will be automatically appended to the displayed value. */
    k_unit_param_type_db,        /** Describes a decibel value. 'dB' will be automatically appended to the displayed value. */
    k_unit_param_type_cents,     /** Describes a pitch cents value. 'C' will be automatically appended to the displayed value. Positive values will be prepended with a '+' to indicate it is an offset. */
    k_unit_param_type_semi,      /** Describes a pitch semitone value. Positive values will be prepended with a '+' to indicate it is an offset. */
    k_unit_param_type_oct,       /** Describes an octave offset value. Positive values will be prepended with a '+' to indicate it is an offset. */
    k_unit_param_type_hertz,     /** Describes a Hertz value. 'Hz' will be automatically appended to the displayed value. */
    k_unit_param_type_khertz,    /** Describes a kilo Hertz value. 'kHz' will be automatically appended to the displayed value. */
    k_unit_param_type_bpm,       /** Describes a beat per minute value. */
    k_unit_param_type_msec,      /** Describes a milliseconds value. 'ms' will be automatically appended to the displayed value. */
    k_unit_param_type_sec,       /** Describes a seconds value. 's' will be automatically appended to the displayed value. */
    k_unit_param_type_enum,      /** Describes a numerical enumeration value. If the value minimum is set to 0, the value will be incremented by 1 when displayed. */
    k_unit_param_type_strings,   /** Describes a value with custom string representation. The numerical value will be passed in a call to unit_get_param_str_value(..) in order to obtain the string representation. */
    k_unit_param_type_reserved0, /** Reserved value unused on this platform. */
    k_unit_param_type_drywet,    /** Describes a dry/wet value. Negative values will be prepended with D for dry, positive values with W for wet, and zero value replaced with BAL to indicate a balanced mix. */
    k_unit_param_type_pan,       /** Describes a stereo pan value. % will automatically be appended to the displayed value. Negative values will be prepended with L for left, positive values with R for right, and zero value replaced with C to indicate centered panning. */
    k_unit_param_type_spread,    /** Describes a stereo spread value. Negative values will be prepended with <, positive values with >. */
    k_unit_param_type_onoff,     /** Describes an on/off toggle value. 0 will be displayed as off, and 1 will be displayed as on. */
    k_unit_param_type_midi_note, /** Describes a MIDI note value. The numerical note value will be displayed as musical pitches (e.g.: C0, A3). */
    k_unit_param_type_count
  };

  /** Fractional value interpretations. */
  enum {
    k_unit_param_frac_mode_fixed = 0U,  // Fraction bits interpreted as fixed point value.
    k_unit_param_frac_mode_decimal = 1U, // Fraction bits interpreted as number of decimals in base 10 representation.
  };

  /** Unit parameter descriptor. */
#pragma pack(push, 1)
  typedef struct unit_param {
    int16_t min;                         /** Minimum value. */
    int16_t max;                         /** Maximum value. */
    int16_t center;                      /** Logical center value. */
    int16_t init;                        /** Initial/default value. */
    uint8_t type;                        /** Value type. See k_unit_param_type_* above. */
    uint8_t frac : 4;                    /** Fractional bits / decimals according to frac_mode. */
    uint8_t frac_mode : 1;               /** See k_unit_param_frac_mode_* above. */
    uint8_t reserved : 3;                /** Reserved bits. Keep to zero. */
    char    name[UNIT_PARAM_NAME_SIZE];  /** Parameter name. */
  } unit_param_t;
#pragma pack(pop)
  
  /**
   * Unit initialization callback type
   *
   * The callback is called to initialize units immediately after loading them into the runtime.
   * \param desc Pointer to the runtime descriptor. @see unit_runtime_desc_t.
   * \return 0 shall be returned upon successful inialization, otherwise an error code shall be returned. See k_unit_err_* definitions.
   */
  typedef int8_t (*unit_init_func)(const unit_runtime_desc_t * desc);

  /**
   * Unit teardown callback type
   *
   * The callback is called before the unit is unloaded from the runtime environment.
   */  
  typedef void (*unit_teardown_func)(void);

  /**
   * Unit reset callback type
   */
  typedef void (*unit_reset_func)(void);

  /**
   * Unit resume rendering callback type
   *
   * The callback is called when the unit shall exit suspended state and be ready for its render callback to be called.
   */
  typedef void (*unit_resume_func)(void);

  /**
   * Unit suspend rendering callback type
   *
   * The callback is called when the unit shall enter suspended state.
   */
  typedef void (*unit_suspend_func)(void);

  /**
   * Unit render callback type
   *
   * After initialization, and after exiting suspended state, the render callback is called at each audio processing cycle.
   * Note that input and output buffers either overlap completely or not at all.
   * Input/output channel geometry is determined by the runtime descriptor passed via the initialization callback.
   * @see unit_init_func
   * @see unit_runtime_desc_t
   *
   * \param in     Input audio data buffer pointer.
   * \param out    Output audio data buffer pointer.
   * \param frames Number of audio frames in input and output buffers.
   */
  typedef void (*unit_render_func)(const float * in, float * out, uint32_t frames);

  /**
   * Unit get parameter value callback type
   *
   * After initialization, the callback may be called at any time to obtain the current value of the given parameter.
   *
   * \param param_id Parameter identifier.
   * \return Current value of parameter according to value range and format defined in the parameter's descriptor.
   */
  typedef int32_t (*unit_get_param_value_func)(uint8_t param_id);

  /**
   * Unit get parameter string value callback type
   *
   * After initialization, the callback may be called at any time to convert a k_unit_param_type_strings typed parameter's numerical value into a string representation.
   * It can be assumed that the string data will have been used or cached by the runtime before the callback is called again.
   *
   * \param param_id Parameter identifier.
   * \param value    Numerical value of parameter according to range defined in the parameter's descriptor.
   * \return String representation of value.
   */  
  typedef const char * (*unit_get_param_str_value_func)(uint8_t param_id, int32_t value);

  /**
   * Unit set parameter value callback type
   *
   * After initialization, the callback may be called at any time to set the current value of the given parameter.
   *
   * \param param_id Parameter identifier.
   * \param value    Value of parameter according to value range and format defined in the parameter's descriptor.
   */  
  typedef void (*unit_set_param_value_func)(uint8_t param_id, int32_t value);

  /**
   * Unit set tempo callback type
   *
   * After initialization, the callback may be called at any time to notify the unit of the current master tempo.
   *
   * \param tempo Current tempo in UQ16.16 representation (i.e.: fractional part in lower 16 bits).
   */    
  typedef void (*unit_set_tempo_func)(uint32_t tempo);

  /**
   * Unit tempo 4ppqn tick callback type
   *
   * After initialization, the callback may be called at any time to notify the unit of a clock event (4PPQN interval, ie: 16th notes with regards to tempo).
   *
   * \param counter Clock event counter since last transport reset.
   */    
  typedef void (*unit_tempo_4ppqn_tick_func)(uint32_t counter);

  /**
   * Unit touch phases
   *
   * Used in touch events to indicate the current phase in a touch lifecyle.
   */
  typedef enum {
    k_unit_touch_phase_began = 0U, /** A new touch was detected. */
    k_unit_touch_phase_moved,      /** Motion to new coordinates. */
    k_unit_touch_phase_ended,      /** End of touch lifecycle. */
    k_unit_touch_phase_stationary, /** Used to force-refresh current coordinates. */
    k_unit_touch_phase_cancelled,  /** Touch lifecyle forcibly ended. e.g.: by a UI mode change */
    k_num_unit_touch_phases,
  } unit_touch_phase_t;
  
  /**
   * Unit touch event callback type
   *
   * After initialization, the callback may be called at any time to notify the unit of a touch event.
   *
   * \param id    The id of the touch to which the event is related. Always 0 for single touch platforms. (NTS-3 kaoss pad kit is single touch)
   * \param phase Current phase of the touch. See #unit_touch_phase_t
   * \param x     X-axis coordinates of touch.
   * \param y     y-axis coordinates of touch.
   *
   * @note The actual x/y touch area width/height can be obtained programatically via the runtime context passed in #unit_runtime_hooks_t
   *       (On NTS-3 kaoss pad kit, the touch area is 1024x1024 with origin at the bottom left)
   */    
  typedef void (*unit_touch_event_func)(uint8_t, uint8_t, uint32_t, uint32_t);
  
  /**
   * Header units must provide to the runtime.
   * Each unit must define and expose an instance of unit_header_t to the runtime.
   */
#pragma pack(push, 1)
  typedef struct unit_header {
    uint32_t header_size;                      /** Size of the header. */
    uint32_t target;                           /** Platform and module pair the unit is targeted for */
    uint32_t api;                              /** API version for which the unit was built. See k_unit_api_* above. */
    uint32_t dev_id;                           /** Developer ID. See https://github.com/korginc/logue-sdk/blob/master/developer_ids.md */
    uint32_t unit_id;                          /** ID for this unit. Scoped within the context of a given dev_id. */
    uint32_t version;                          /** The unit's version following the same major.minor.patch format and rules as the API version */
    char     name[UNIT_NAME_SIZE];             /** Unit name. */
    uint32_t reserved0;                        /** Reserved for future use. */
    uint32_t reserved1;                        /** Reserved for future use. */
    uint32_t num_params;                       /** Number of valid parameter descriptors. */
    unit_param_t params[UNIT_MAX_PARAM_COUNT]; /** Parameter descriptors. */
  } unit_header_t;
#pragma pack(pop)

  /** Result/error codes expected from initilization callback. */
  enum {
    k_unit_err_none = 0,
    k_unit_err_target = -1,
    k_unit_err_api_version = -2,
    k_unit_err_samplerate = -4,
    k_unit_err_geometry = -8,
    k_unit_err_memory = -16,
    k_unit_err_undef = -32,
  };
    
#ifdef __cplusplus
} // extern "C"
#endif
  
#endif // RUNTIME_H_

/** @} */

