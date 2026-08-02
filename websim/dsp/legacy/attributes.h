// websim host shim for the gen-2 "attributes.h" that the shared websim/dsp
// ROM-data headers include. The gen-1 platforms (prologue / minilogue xd /
// NTS-1 mkI) have no such header, so this provides the few constants those data
// headers reference. Only seen by the gen-1 build (via -I websim/dsp/legacy).

#ifndef ATTRIBUTES_H_
#define ATTRIBUTES_H_

#ifndef UNIT_NAME_LEN
#define UNIT_NAME_LEN 8
#endif
#ifndef UNIT_MAX_PARAM_COUNT
#define UNIT_MAX_PARAM_COUNT 13
#endif
#ifndef UNIT_PARAM_NAME_LEN
#define UNIT_PARAM_NAME_LEN 8
#endif

// gen-2 spells this without underscores (gen-1 uses __fast_inline); the shared
// websim/dsp data headers use the gen-2 spelling.
#ifndef fast_inline
#define fast_inline inline __attribute__((always_inline, optimize("Ofast")))
#endif

#endif // ATTRIBUTES_H_
