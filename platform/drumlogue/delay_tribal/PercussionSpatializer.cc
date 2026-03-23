// PercussionSpatializerTables.cpp
#include "PercussionSpatializer.h"

// Single definition of global tables
float lfo_table[LFO_TABLE_SIZE] __attribute__((aligned(16)));

// Initialize LFO table
__attribute__((constructor))
static void init_lfo_table() {
    for (int i = 0; i < LFO_TABLE_SIZE; i++) {
        float phase = (float)i / LFO_TABLE_SIZE;
        lfo_table[i] = 2.0f * fabsf(phase - 0.5f);
    }
}