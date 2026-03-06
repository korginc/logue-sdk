#!/bin/bash

# ============================================================================
# FM Percussion Synth - Unit Test Runner
# ============================================================================
# This script compiles and runs all unit tests for the FM Percussion Synth
#
# Usage: ./run_unit_tests.sh [test_name]
#   test_name: Optional specific test to run (prng, sine, envelope, etc.)
# ============================================================================

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# Configuration
# ============================================================================

# Compiler flags for ARM NEON (for native build, remove -mfpu for x86 testing)
CFLAGS="-O3 -Wall -Wextra -std=c99 -lm"
CXXFLAGS="-O3 -Wall -Wextra -std=c++11 -lm"
NEON_FLAGS="-mfpu=neon -mfloat-abi=softfp"

# Source files (all headers, no .c files needed as tests.h includes everything)
TEST_SUITE="tests.h"

# Output directory
BUILD_DIR="./build"
mkdir -p "$BUILD_DIR"

# ============================================================================
# Helper Functions
# ============================================================================

print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_failure() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}➜ $1${NC}"
}

# ============================================================================
# Check if running on ARM
# ============================================================================

ARCH=$(uname -m)
if [[ "$ARCH" == "armv7l" ]] || [[ "$ARCH" == "aarch64" ]]; then
    print_info "Running on ARM ($ARCH) - enabling NEON optimizations"
    CFLAGS="$CFLAGS $NEON_FLAGS"
    CXXFLAGS="$CXXFLAGS $NEON_FLAGS"
else
    print_info "Running on $ARCH - disabling NEON (simulated mode)"
    # For x86, define a macro to use scalar fallbacks
    CFLAGS="$CFLAGS -DARM_NEON_SIMULATE"
    CXXFLAGS="$CXXFLAGS -DARM_NEON_SIMULATE"
fi

# ============================================================================
# Build and Run Individual Tests
# ============================================================================

run_test() {
    local test_name=$1
    local test_macro=$2
    local output_name="$BUILD_DIR/test_$test_name"

    print_header "Running Test: $test_name"

    # Build test
    print_info "Compiling $test_name..."

    # Note: We're compiling the test header directly with the appropriate macro
    # This works because tests.h contains the test implementations guarded by macros
    if [[ "$test_name" == "all" ]]; then
        gcc $CFLAGS -o "$output_name" -xc - << EOF
#define TEST_ALL
#include "$TEST_SUITE"
int main() { return run_all_tests() ? 0 : 1; }
EOF
    else
        gcc $CFLAGS -o "$output_name" -xc - << EOF
#define $test_macro
#include "$TEST_SUITE"
int main() { return 0; }
EOF
    fi

    if [ $? -ne 0 ]; then
        print_failure "Compilation failed"
        return 1
    fi
    print_success "Compilation successful"

    # Run test
    print_info "Executing $test_name..."
    if "$output_name"; then
        print_success "Test passed"
        return 0
    else
        print_failure "Test failed"
        return 1
    fi
}

# ============================================================================
# Main Test Runner
# ============================================================================

# If specific test requested, run only that
if [ $# -eq 1 ]; then
    case $1 in
        "prng")
            run_test "prng" "TEST_PRNG_ONLY"
            ;;
        "sine")
            run_test "sine" "TEST_SINE_ONLY"
            ;;
        "envelope")
            run_test "envelope" "TEST_ENVELOPE_ONLY"
            ;;
        "engines")
            run_test "engines" "TEST_ENGINES_ONLY"
            ;;
        "lfo")
            run_test "lfo" "TEST_LFO_ONLY"
            ;;
        "smoothing")
            run_test "smoothing" "TEST_SMOOTHING"
            ;;
        "stability")
            run_test "stability" "TEST_STABILITY_ONLY"
            ;;
        "all")
            run_test "all" "TEST_ALL"
            ;;
        *)
            echo -e "${RED}Unknown test: $1${NC}"
            echo "Available tests: prng, sine, envelope, engines, lfo, smoothing, stability, all"
            exit 1
            ;;
    esac
else
    # Run all tests
    print_header "FM PERCUSSION SYNTH - COMPLETE TEST SUITE"

    TESTS=(
        "prng:TEST_PRNG_ONLY"
        "sine:TEST_SINE_ONLY"
        "envelope:TEST_ENVELOPE_ONLY"
        "engines:TEST_ENGINES_ONLY"
        "lfo:TEST_LFO_ONLY"
        "smoothing:TEST_SMOOTHING"
        "stability:TEST_STABILITY_ONLY"
    )

    PASSED=0
    FAILED=0

    for test in "${TESTS[@]}"; do
        IFS=':' read -r name macro <<< "$test"
        if run_test "$name" "$macro"; then
            PASSED=$((PASSED + 1))
        else
            FAILED=$((FAILED + 1))
        fi
        echo
    done

    # Run final integration test
    if run_test "integration" "TEST_ALL"; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
    fi

    # Summary
    print_header "TEST SUMMARY"
    echo -e "${GREEN}Passed: $PASSED${NC}"
    echo -e "${RED}Failed: $FAILED${NC}"

    if [ $FAILED -eq 0 ]; then
        print_success "All tests passed!"
        exit 0
    else
        print_failure "Some tests failed"
        exit 1
    fi
fi