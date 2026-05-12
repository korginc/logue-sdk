#!/usr/bin/env bash
set -euo pipefail
shopt -s extglob
# Sonaglio benchmark runner
# Native or WSL cross-compile:
#   CXX=/mnt/d/Fede/drumlogue/arm-unknown-linux-gnueabihf/bin/arm-unknown-linux-gnueabihf-g++
#   RUNNER=qemu-arm
#   ./run_sonaglio_benchmarks.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${ROOT_DIR:-$SCRIPT_DIR}"
if [[ ! -d "$ROOT_DIR/platform/drumlogue/Sonaglio" ]]; then
  ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

SONG_DIR="$ROOT_DIR/platform/drumlogue/Sonaglio"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build/sonaglio}"
mkdir -p "$BUILD_DIR"

BENCH_SRC="${BENCH_SRC:-$SCRIPT_DIR/benchmark_sonaglio.cpp}"
BENCH_BIN="${BENCH_BIN:-$BUILD_DIR/benchmark_sonaglio}"

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

COMMON_FLAGS=(-std=c++17 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
INCLUDES=(-I"$ROOT_DIR" -I"$ROOT_DIR/platform/drumlogue" -I"$ROOT_DIR/platform/drumlogue/common" -I"$ROOT_DIR/platform/common" -I"$SONG_DIR")
if [[ -n "${SDK_INCLUDE_DIR:-}" ]]; then
  INCLUDES+=(-I"$SDK_INCLUDE_DIR")
fi

if [[ "${STATIC_BUILD:-1}" == "1" ]]; then
  COMMON_FLAGS+=(-static)
fi
if [[ -n "${TARGET_TRIPLE:-}" ]]; then
  COMMON_FLAGS+=(-target "$TARGET_TRIPLE")
fi
if [[ -n "${MCPU_FLAGS:-}" ]]; then
  # shellcheck disable=SC2206
  COMMON_FLAGS+=(${MCPU_FLAGS})
fi
if [[ -n "${EXTRA_CXXFLAGS:-}" ]]; then
  # shellcheck disable=SC2206
  COMMON_FLAGS+=(${EXTRA_CXXFLAGS})
fi

echo "CXX      : $CXX"
echo "RUNNER   : ${RUNNER:-<native>}"
echo "BENCH_SRC: $BENCH_SRC"
echo "BENCH_BIN : $BENCH_BIN"
echo "FLAGS    : ${COMMON_FLAGS[*]}"

set -x
"$CXX" "${COMMON_FLAGS[@]}" "${INCLUDES[@]}" "$BENCH_SRC" -o "$BENCH_BIN" -lm
set +x

if [[ -n "${RUNNER:-}" ]]; then
  set -x
  "$RUNNER" "$BENCH_BIN"
  set +x
else
  set -x
  "$BENCH_BIN"
  set +x
fi
