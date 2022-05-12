/**
 * @file sample_wrapper.h
 * @brief Sample wrapper structure
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#ifndef SAMPLE_WRAPPER_H_
#define SAMPLE_WRAPPER_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNIT_SAMPLE_WRAPPER_MAX_NAME_LEN 31

typedef struct sample_wrapper {
  uint8_t bank;
  uint8_t index;
  uint8_t channels;
  uint8_t _padding;
  char name[UNIT_SAMPLE_WRAPPER_MAX_NAME_LEN + 1];
  size_t frames;
  const float * sample_ptr;
} sample_wrapper_t;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SAMPLE_WRAPPER_H_
