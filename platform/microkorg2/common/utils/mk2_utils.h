/**
 * @file mk2_utils.h
 * @brief Useful functions for microkorg2 units
 *
 * Copyright (c) 2025 KORG Inc. All rights reserved.
 *
 */

#include <cstring>

#include "attributes.h"
#include "utils/int_math.h"
#include "runtime.h"

#ifndef MK2_UTILS_H_
#define MK2_UTILS_H_

// wrappers and utilities to make code more readable
enum { kMk2LocalityNone = 0, kMk2LocalityL2, kMk2LocalityL1 };
fast_inline void PrefetchMemory(void * memory, bool isRead = true, int locality = kMk2LocalityL1)
{
  __builtin_prefetch(memory, isRead, locality);
}

int32_t * GetCurrentVoiceMaxData(void * data)
{
  return static_cast<int32_t *>(data);
}

int32_t * GetCurrentVoiceOffsetData(void * data)
{
  return static_cast<int32_t *>(data);
}

int32_t * GetCurrentBufferOffsetData(void * data)
{
  return static_cast<int32_t *>(data);
}

int32_t * GetCurrentBufferOutputStrideData(void * data)
{
  return static_cast<int32_t *>(data);
}

float * GetModDepth(void * data)
{
  return static_cast<float *>(data);
}

float * GetModData(void * data)
{
  // shift num mod dest up by 1 to account for 2 timbres
  const uint32_t offset = (kNumModDest << 1);
  return (static_cast<float *>(data) + offset);
}

char * GetModDestNameData(void * data)
{
  return static_cast<char *>(data);
}

enum
{
  kMicrokorg2FontSizeSmall,
  kMicrokorg2FontSizeMedium,
  kMicrokorg2FontSizeLarge
};
void AddCustomParameterUnit(char * value, const char * unit, int size) 
{
  const char * sizeChar;
  switch (size)
  {
  case kMicrokorg2FontSizeMedium:
    sizeChar = "|";
    break;
  
  case kMicrokorg2FontSizeLarge:
    sizeChar = "}";
    break;

  case kMicrokorg2FontSizeSmall:
  default:
    sizeChar = "{";
    break;
  }

  strncat(value + strlen(value), sizeChar, strlen(sizeChar));
  strncat(value + strlen(value), unit, strlen(unit));
}
#endif // MK2_UTILS_H_