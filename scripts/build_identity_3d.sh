#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

MOBIUS_BLENDER_BIN="${MOBIUS_BLENDER_BIN:-$(command -v blender || true)}"
if [[ -z "$MOBIUS_BLENDER_BIN" ]]; then
  echo "Blender is required to build the Möbi𝒰s 3D identity" >&2
  exit 1
fi

mkdir -p build/identity-blender
BUILD_LOG="build/identity-blender/build.log"

echo "+ $MOBIUS_BLENDER_BIN --background --factory-startup --python tools/blender/build_mobius_glyph_identity.py"
"$MOBIUS_BLENDER_BIN" --background --factory-startup \
  --python tools/blender/build_mobius_glyph_identity.py 2>&1 | tee "$BUILD_LOG"

# Blender can return zero after a Python exception, so the build is fail-closed
# on the explicit completion witness rather than process status alone.
grep -q '\[mobius-glyph-identity\] build complete' "$BUILD_LOG" || {
  echo "Blender identity build did not emit its completion witness" >&2
  exit 1
}

echo "MOBIUS_IDENTITY_BUILD_PASS"
