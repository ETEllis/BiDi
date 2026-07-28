#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

MOBIUS_BLENDER_BIN="${MOBIUS_BLENDER_BIN:-$(command -v blender || true)}"
MOBIUS_FFMPEG_BIN="${MOBIUS_FFMPEG_BIN:-$(command -v ffmpeg || true)}"
MASTER="assets/identity/3d/mobius-identity-master.blend"
LOG="build/identity-blender/motion-render.log"
FRAME_DIR="build/identity-blender/motion-frames"
OUTPUT="assets/identity/motion/mobius-identity-master.mp4"

test -n "$MOBIUS_BLENDER_BIN" || { echo "Blender is required for motion rendering" >&2; exit 1; }
test -n "$MOBIUS_FFMPEG_BIN" || { echo "FFmpeg is required for motion encoding" >&2; exit 1; }
test -s "$MASTER" || { echo "missing Blender master: $MASTER" >&2; exit 1; }
mkdir -p "$FRAME_DIR" assets/identity/motion

echo "+ $MOBIUS_BLENDER_BIN --background $MASTER --python tools/blender/render_mobius_identity.py"
"$MOBIUS_BLENDER_BIN" --background "$MASTER" \
  --python tools/blender/render_mobius_identity.py 2>&1 | tee "$LOG"
grep -q 'MOBIUS_IDENTITY_FRAME_RENDER_PASS' "$LOG" || {
  echo "motion frame render did not emit its completion witness" >&2
  exit 1
}

"$MOBIUS_FFMPEG_BIN" -hide_banner -loglevel warning -y \
  -framerate 24 -start_number 1 -i "$FRAME_DIR/mobius-%04d.png" \
  -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p -movflags +faststart \
  "$OUTPUT"

encode_clip() {
  local name="$1"
  local start="$2"
  local duration="$3"
  "$MOBIUS_FFMPEG_BIN" -hide_banner -loglevel warning -y \
    -ss "$start" -i "$OUTPUT" -t "$duration" -an \
    -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p -movflags +faststart \
    "assets/identity/motion/$name"
}

# Independent, causally complete projection clips. Times are exact frame
# ranges from the 24 fps Blender master, not editorial approximations.
encode_clip "mobius-wordmark.mp4" 0 4.5
encode_clip "mobius-ius.mp4" 4.5 2
encode_clip "mobius-ui-hangul.mp4" 6.5 3
encode_clip "mobius-bidi-delta.mp4" 11.5 6
encode_clip "mobius-operator-u.mp4" 17.5 2
encode_clip "mobius-code-sigil.mp4" 19.5 2

MOBIUS_FINALIZE_MOTION=1 "$MOBIUS_BLENDER_BIN" --background "$MASTER" \
  --python tools/blender/render_mobius_identity.py 2>&1 | tee -a "$LOG"
grep -q 'MOBIUS_IDENTITY_MOTION_RENDER_PASS' "$LOG" || {
  echo "motion encode did not emit its completion witness" >&2
  exit 1
}

rm -f "$FRAME_DIR"/mobius-*.png

echo "MOBIUS_IDENTITY_MOTION_PASS"
