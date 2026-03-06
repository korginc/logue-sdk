#!/bin/bash

# ============================================================================
# FM Percussion Synth - Benchmark Runner
# ============================================================================
# Runs CPU cycle benchmarks for all DSP components
# 
# Usage: ./run_benchmarks.sh
# ============================================================================

set -e

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BUILD_DIR="./build"
mkdir -p "$BUILD_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   FM PERCUSSION SYNTH - BENCHMARKS    ${NC}"
echo -e "${BLUE}========================================${NC}"

# Compiler flags with cycle counting enabled
CFLAGS="-O3 -Wall -Wextra -std=c99 -lm -DARM_NEON_OPTIMIZATION"

# Check architecture
ARCH=$(uname -m)
if [[ "$ARCH" == "armv7l" ]] || [[ "$ARCH" == "aarch64" ]]; then
    echo -e "${YELLOW}➜ Running on ARM ($ARCH) - hardware cycle counting available${NC}"
    CFLAGS="$CFLAGS -mfpu=neon -mfloat-abi=softfp"
    CYCLE_FLAG="-DARM_CYCLE_COUNT"
else
    echo -e "${YELLOW}➜ Running on $ARCH - cycle counting simulated${NC}"
    CFLAGS="$CFLAGS -DARM_NEON_SIMULATE"
    CYCLE_FLAG=""
fi

# Build benchmark
echo -e "${YELLOW}➜ Compiling benchmarks...${NC}"

cat > "$BUILD_DIR/benchmark_runner.c" << EOF
#define TEST_BENCHMARK
#include "../tests.h"
int main() {
    run_benchmarks();
    return 0;
}
EOF

gcc $CFLAGS $CYCLE_FLAG -o "$BUILD_DIR/benchmark" "$BUILD_DIR/benchmark_runner.c"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
    echo
    
    # Run benchmark
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}   Running Benchmarks...                ${NC}"
    echo -e "${BLUE}========================================${NC}"
    "$BUILD_DIR/benchmark"
else
    echo -e "${RED}✗ Compilation failed${NC}"
    exit 1
fi