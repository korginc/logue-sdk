/**
 * @file fm_presets.c
 * @brief FM Percussion preset data — compiled as C to allow C99 designated
 *        initializers for char array and sub-array fields (.name, .engine_map).
 *        GCC 6 C++14 rejects these; C99 handles them without issue.
 */

#define FM_PRESETS_DEFINE_DATA
#include "fm_presets.h"
