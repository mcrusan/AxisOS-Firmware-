#!/usr/bin/env bash
# =============================================================================
#  AxisOS Clean Build Script
#  Targets: Raspberry Pi Pico 2W (RP2350)
#  Usage:   ./tools/build.sh [release|debug]
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-release}"
BUILD_DIR="${ROOT_DIR}/build"
RELEASE_DIR="${ROOT_DIR}/release"

# Capitalise for CMake
CMAKE_BUILD_TYPE="$(echo "${BUILD_TYPE}" | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')"

echo "=============================================="
echo "  AxisOS Build — ${CMAKE_BUILD_TYPE}"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

# ── Prerequisites ──────────────────────────────────────────────────────────
if [[ -z "${PICO_SDK_PATH:-}" ]]; then
    echo "[ERROR] PICO_SDK_PATH is not set. Export it before running this script."
    exit 1
fi

for cmd in cmake ninja arm-none-eabi-gcc; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "[ERROR] Required tool not found: $cmd"
        exit 1
    fi
done

# ── Clean ─────────────────────────────────────────────────────────────────
echo "[STEP 1] Cleaning previous build..."
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${RELEASE_DIR}"

# ── Configure ─────────────────────────────────────────────────────────────
echo "[STEP 2] Configuring CMake (${CMAKE_BUILD_TYPE})..."
cmake -S "${ROOT_DIR}" \
      -B "${BUILD_DIR}" \
      -G Ninja \
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
    VERSION=$(grep -oP '(?<=VERSION_STRING\s{3}")[^"]+' \
              "${ROOT_DIR}/include/axisOS_version.h" || echo "unknown")
    DEST="${RELEASE_DIR}/axisOS-v${VERSION}.uf2"
    cp "$UF2_SRC" "$DEST"
    sha256sum "$DEST" > "${DEST}.sha256"
    echo "[OK] UF2: ${DEST}"
    echo "[OK] SHA256: $(cat "${DEST}.sha256" | awk '{print $1}')"
else
    echo "[WARN] UF2 not found at ${UF2_SRC}. Check build output."
fi

echo ""
echo "Build complete. Artefacts in: ${RELEASE_DIR}"
echo "=============================================="
