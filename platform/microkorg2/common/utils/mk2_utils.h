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

int32_t * GetModIndex(void * data)
{
  return static_cast<int32_t *>(data);
}

float * GetModDepth(void * data)
{
  // depth data is stored after index
  return static_cast<float *>(data) + kNumMk2ModSrc;
}

float * GetModData(void * data)
{
  // mod data is stored after depth and index values
  const uint32_t offset = (kNumMk2ModSrc * 2);
  return (static_cast<float *>(data) + offset);
}

float * GetModSourceData(void * data, int sourceIndex, int numVoices, int voice)
{
  // mod data is stored after depth and index values
  const uint32_t dataOffset = (kNumMk2ModSrc * 2);
  const uint32_t sourceOffset = sourceIndex * numVoices;
  float * dataPtr = static_cast<float *>(data);
  return (dataPtr + (dataOffset + sourceOffset + voice));
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