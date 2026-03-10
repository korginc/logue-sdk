#!/bin/bash

# ============================================================================
# Phase 5 Benchmarks - Percussion Spatializer
# ============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BUILD_DIR="./build"
mkdir -p "$BUILD_DIR"

print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Compile and run benchmark
g++ -O3 -mfpu=neon -mfloat-abi=softfp -o "$BUILD_DIR/bench_delay" benchmark_delay.cpp PercussionSpatializer.h

if [ $? -eq 0 ]; then
    print_header "Running Delay Benchmarks"
    "$BUILD_DIR/bench_delay"
else
    echo -e "${RED}Compilation failed${NC}"
    exit 1
fi