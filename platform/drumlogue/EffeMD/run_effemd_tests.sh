#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob extglob

# EffeMD unit test runner
#
# The DSP layer under test is scalar C++, so the tests build and run natively
# on any host:
#   ./run_effemd_tests.sh
#
# Optional ARM cross-compile + qemu (exercises the NEON paths):
#   CXX=arm-linux-gnueabihf-g++ RUNNER=qemu-arm ./run_effemd_tests.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${ROOT_DIR:-$(cd "$SCRIPT_DIR/../../.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build/effemd}"
mkdir -p "$BUILD_DIR"

TEST_SRC="${TEST_SRC:-$SCRIPT_DIR/test_effemd.cc}"
TEST_BIN="${TEST_BIN:-$BUILD_DIR/test_effemd}"

if [[ -z "${CXX:-}" ]]; then
  if [[ -n "${CROSS_PREFIX:-}" ]]; then
    CXX="${CROSS_PREFIX}-g++"
  elif command -v clang++ >/dev/null 2>&1; then
    CXX=clang++
  else
    CXX=g++
  fi
fi

if [[ -z "${RUNNER:-}" ]]; then
  case "$CXX" in
    *arm-unknown-linux-gnueabihf-g++*|*arm-linux-gnueabihf-g++*|*arm-none-eabi-g++*)
      RUNNER="qemu-arm"
      ;;
  esac
fi

COMMON_FLAGS=(-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
INCLUDES=(-I"$SCRIPT_DIR")
if [[ -n "${EXTRA_CXXFLAGS:-}" ]]; then
  # shellcheck disable=SC2206
  COMMON_FLAGS+=(${EXTRA_CXXFLAGS})
fi

echo "CXX      : $CXX"
echo "RUNNER   : ${RUNNER:-<native>}"
echo "TEST_SRC : $TEST_SRC"
echo "TEST_BIN : $TEST_BIN"

# Model sources (exclude unit.cc — it needs the drumlogue SDK headers —
# and other test files).
MODEL_SOURCES=("$SCRIPT_DIR"/!(test_*|unit|benchmark_*).cc)

set -x
"$CXX" "${COMMON_FLAGS[@]}" "${INCLUDES[@]}" "$TEST_SRC" "${MODEL_SOURCES[@]}" -o "$TEST_BIN" -lm
set +x

if [[ -n "${RUNNER:-}" ]]; then
  "$RUNNER" "$TEST_BIN"
else
  "$TEST_BIN"
fi
