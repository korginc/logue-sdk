/**
 * @file attributes.h
 * @brief Some useful compiler attribute shorthands 
 * common to all logue sdk platforms
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#ifndef ATTRIBUTES_COMMON_H_
#define ATTRIBUTES_COMMON_H_

#define noopt __attribute__((optimize("O0")))
#define fast __attribute__((optimize("Ofast")))

#define force_inline inline __attribute__((always_inline))
#define fast_inline inline __attribute__((always_inline, optimize("Ofast")))

#define __unit_callback __attribute__((used))
#define __unit_header __attribute__((used, section(".unit_header")))

#endif // ATTRIBUTES_COMMON_H_