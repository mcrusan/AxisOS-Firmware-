#!/usr/bin/env bash
# ==============================================================================
# File:        firmware/tools/build.sh
# Project:     Crusan & Co. // AxisOS — Sovereign Pico 2 W Firmware
# Author:      Marc F. Crusan / Crusan & Co. w/ AGY (AXEL)
# Date:        2026-10-30
# Version:     v1.7.0-Silicon
# Description: Clean build script for AxisOS Raspberry Pi Pico 2 W (RP2350)
#              with automatic Sovereign toolchain & Pico SDK discovery.
# Changelog:
#   - 2026-10-30 (MFC/AGY): Auto-discovered PICO_SDK_PATH and ARM GCC toolchain,
#     added Ninja/Make fallback, and replaced brittle Perl lookbehind regex.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-release}"
BUILD_DIR="${ROOT_DIR}/build"
RELEASE_DIR="${ROOT_DIR}/release"

# Auto-discover Sovereign ARM toolchain & Pico SDK if not already in environment
if [[ -d "/home/mcrusan/toolchains/axisos-arm-10.3/gcc-arm-10.3-2021.07-x86_64-arm-none-eabi/bin" ]]; then
    export PICO_TOOLCHAIN_PATH="/home/mcrusan/toolchains/axisos-arm-10.3/gcc-arm-10.3-2021.07-x86_64-arm-none-eabi"
    export PATH="${PICO_TOOLCHAIN_PATH}/bin:${PATH}"
fi
if [[ -z "${PICO_SDK_PATH:-}" ]] && [[ -d "/home/mcrusan/pico-sdk" ]]; then
    export PICO_SDK_PATH="/home/mcrusan/pico-sdk"
fi

# Capitalise for CMake
CMAKE_BUILD_TYPE="$(echo "${BUILD_TYPE}" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')"

echo "=============================================="
echo "  Crusan & Co. // AxisOS Build — ${CMAKE_BUILD_TYPE}"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

# ── Prerequisites ──────────────────────────────────────────────────────────
if [[ -z "${PICO_SDK_PATH:-}" ]]; then
    echo "[ERROR] PICO_SDK_PATH is not set. Export it before running this script."
    exit 1
fi

for cmd in cmake arm-none-eabi-gcc; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "[ERROR] Required tool not found: $cmd"
        exit 1
    fi
done

CMAKE_GEN="Unix Makefiles"
if command -v ninja &>/dev/null; then
    CMAKE_GEN="Ninja"
fi

# ── Clean ─────────────────────────────────────────────────────────────────
echo "[STEP 1] Cleaning previous build..."
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${RELEASE_DIR}"

# ── Configure ─────────────────────────────────────────────────────────────
echo "[STEP 2] Configuring CMake (${CMAKE_BUILD_TYPE}, generator: ${CMAKE_GEN})..."
cmake -S "${ROOT_DIR}" \
      -B "${BUILD_DIR}" \
      -G "${CMAKE_GEN}" \
      -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
      -DPICO_BOARD=pico2_w \
      -DCMAKE_BUILD_DATE="$(date '+%Y%m%d')"

# ── Build ─────────────────────────────────────────────────────────────────
echo "[STEP 3] Building..."
cmake --build "${BUILD_DIR}" --parallel "$(nproc)"

# ── Collect artefacts ─────────────────────────────────────────────────────
echo "[STEP 4] Collecting release artefacts..."
UF2_SRC="${BUILD_DIR}/axisOS.uf2"
if [[ -f "$UF2_SRC" ]]; then
    VERSION=$(awk -F'"' '/#define[[:space:]]+AXISOS_VERSION_STRING/ {print $2; exit}' \
              "${ROOT_DIR}/include/axisOS_version.h")
    [[ -z "${VERSION}" ]] && VERSION="unknown"
    DEST="${RELEASE_DIR}/axisOS-v${VERSION}.uf2"
    cp "$UF2_SRC" "$DEST"
    sha256sum "$DEST" > "${DEST}.sha256"
    echo "[OK] UF2: ${DEST}"
    echo "[OK] SHA256: $(awk '{print $1}' "${DEST}.sha256")"
else
    echo "[WARN] UF2 not found at ${UF2_SRC}. Check build output."
fi

echo ""
echo "Build complete. Artefacts in: ${RELEASE_DIR}"
echo "=============================================="
