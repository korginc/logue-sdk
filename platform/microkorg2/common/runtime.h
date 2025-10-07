/**
 * @file runtime.h
 * @brief Common runtime definitions
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#ifndef RUNTIME_H_
#define RUNTIME_H_

#include "runtime_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "platform_definitions.h"

/** Current platform */
#define UNIT_TARGET_PLATFORM (k_unit_target_microkorg2)
#define UNIT_TARGET_PLATFORM_MASK (0x7F << 8)
#define UNIT_TARGET_MODULE_MASK (0x7F)

/** @private */
#define UNIT_TARGET_PLATFORM_IS_COMPAT(tgt) \
  (((tgt)&UNIT_TARGET_PLATFORM_MASK) == k_unit_target_microkorg2)

enum
{
  kMk2PlatformExclusiveModData,
  kMk2PlatformExclusiveModDestName,
  kMk2PlatformExclusiveUserModSource,
  kNumMk2PlatformExclusive
};

enum
{
  kModDest1 = 0,
  kModDest2,
  kModDest3,
  kModDest4,
  kModDest5,
  kModDest6,
  kModDest7,
  kModDest8,
  kModDest9,
  kModDest10,
  kNumModDest
};

enum
{
  kMk2MaxVoices = 8,
  kMk2HalfVoices = 4,
  kMk2QuarterVoices = 2,
  kMk2SingleVoice = 1,

  kMk2NumModSources = 10,
  kMk2BufferSize = 64
};

/** API version targeted by this code */
#define UNIT_API_VERSION (k_unit_api_2_1_0)
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

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RUNTIME_H_
