#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

REBUILD=0
REQUIRE_MOTION=0
while (($#)); do
  case "$1" in
    --rebuild) REBUILD=1 ;;
    --require-motion) REQUIRE_MOTION=1 ;;
    *) echo "usage: $0 [--rebuild] [--require-motion]" >&2; exit 2 ;;
  esac
  shift
done

if ((REBUILD)); then
  ./scripts/build_identity_3d.sh
fi

MOBIUS_BLENDER_BIN="${MOBIUS_BLENDER_BIN:-$(command -v blender || true)}"
MASTER="assets/identity/3d/mobius-identity-master.blend"
VALIDATION_LOG="build/identity-blender/validation.log"
mkdir -p "$(dirname "$VALIDATION_LOG")"

test -s "$MASTER" || { echo "missing Blender master: $MASTER" >&2; exit 1; }

if [[ -n "$MOBIUS_BLENDER_BIN" ]]; then
  echo "+ $MOBIUS_BLENDER_BIN --background $MASTER --python tools/blender/validate_mobius_identity.py"
  "$MOBIUS_BLENDER_BIN" --background "$MASTER" \
    --python tools/blender/validate_mobius_identity.py 2>&1 | tee "$VALIDATION_LOG"
  grep -q 'MOBIUS_IDENTITY_VALIDATION_PASS' "$VALIDATION_LOG" || {
    echo "Blender visual-semantic validation did not emit its pass witness" >&2
    exit 1
  }
else
  echo "Blender unavailable; validating committed interchange artifacts only"
fi

MOBIUS_REQUIRE_MOTION="$REQUIRE_MOTION" python3 - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import shutil
import struct
import subprocess
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(".")
identity = root / "assets" / "identity"
output = identity / "3d"
renders = identity / "renders"
motion_dir = identity / "motion"

svgs = [
    identity / "mobius-embodied-mark.svg",
    identity / "mobius-u-operator.svg",
    identity / "mobius-u-code-sigil.svg",
    identity / "mobius-u-code-sigil-dark.svg",
    identity / "mobius-ius-relational.svg",
    identity / "mobius-bi-seed.svg",
    identity / "mobius-bidi-kernel.svg",
    identity / "mobius-bidi-delta.svg",
    identity / "mobius-u-wordmark-dark.svg",
    identity / "mobius-u-wordmark-light.svg",
]
for path in svgs:
    if not path.is_file():
        raise SystemExit(f"missing SVG component: {path}")
    node = ET.parse(path).getroot()
    if "viewBox" not in node.attrib or not any(child.tag.endswith("title") for child in node):
        raise SystemExit(f"SVG lacks accessible portable anatomy: {path}")

manifest_path = output / "identity-manifest.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
if manifest.get("schemaVersion") != 2 or manifest.get("identity") != "Möbi𝒰s" or manifest.get("formalKernel") != "bidiγΔ":
    raise SystemExit("manifest identity hierarchy is incorrect")
if manifest.get("universalOperator") != {
    "canonicalSigil": "𝒰_",
    "reducedMathematicalBody": "𝒰",
    "indexedNotation": "𝒰_R",
    "law": "one operator at multiple resolution levels; underscore is the executable live horizon",
}:
    raise SystemExit("Universal Operator sigil hierarchy is incorrect")
if manifest.get("compatibilityAliases") != {
    "codeSigil": "universalOperatorSigil",
    "CODE_SIGIL": "UNIVERSAL_OPERATOR_SIGIL",
    "operatorU": "reducedMathematicalBody",
}:
    raise SystemExit("legacy identity handles are not mapped to the canonical operator hierarchy")
if manifest.get("topology") != {
    "surface": "mobius-band",
    "halfTwists": 1,
    "connectedComponents": 1,
    "eulerCharacteristic": 0,
    "boundaryComponents": 1,
}:
    raise SystemExit("manifest topology mismatch")

required_components = {
    "embodiedBody", "operatorU", "universalOperatorSigil", "relationalIUs", "codeSigil", "kernelBiDiDelta", "animatedMaster",
    "animatedWordmark", "animatedIUs", "animatedUIHangul", "animatedBiDiDelta", "animatedOperatorU", "animatedCodeSigil",
}
if not required_components.issubset(manifest.get("components", {})):
    raise SystemExit("manifest 3D component family is incomplete")
for value in manifest["components"].values():
    path = (manifest_path.parent / value).resolve()
    if not path.is_file():
        raise SystemExit(f"manifest 3D projection missing: {value}")
for value in manifest["components2d"].values():
    path = (manifest_path.parent / value).resolve()
    if not path.is_file():
        raise SystemExit(f"manifest 2D projection missing: {value}")

required_docs = {
    "sourceOfTruth", "workingHandoff", "publicInstrument",
    "visualRefinement", "languageAndRepository", "internalFoldMap",
}
if set(manifest.get("documentation", {})) != required_docs:
    raise SystemExit("identity documentation handoff surface is incomplete")
for value in manifest["documentation"].values():
    path = (manifest_path.parent / value).resolve()
    if not path.is_file():
        raise SystemExit(f"manifest documentation target missing: {value}")

expected_ranges = {
    "PRESENCE": [1, 35],
    "WORDMARK_GENERATION": [36, 84],
    "ONE_TURN_WORDMARK": [85, 108],
    "IUS_PHASE": [109, 156],
    "UI_RESTORATION": [157, 192],
    "HANGUL_RESTORATION": [193, 228],
    "RETURN_TO_WORDMARK": [229, 276],
    "BIDI_SELF_EXTRACTION": [277, 368],
    "DELTA_TRIADIC_CLOSURE": [369, 420],
    "OPERATOR_U": [421, 468],
    "CODE_SIGIL": [469, 516],
    "FAMILY_LOCKUP": [517, 600],
}
if manifest.get("stateRanges") != expected_ranges:
    raise SystemExit("manifest state ranges do not match the 600-frame master")
if manifest.get("staticArtifact", {}).get("frame") != 84:
    raise SystemExit("embodied static wordmark frame is not exposed")
static_wordmark = (manifest_path.parent / manifest["staticArtifact"]["path"]).resolve()
with static_wordmark.open("rb") as handle:
    static_header = handle.read(24)
if static_header[:8] != b"\x89PNG\r\n\x1a\n" or struct.unpack(">II", static_header[16:24]) != (1440, 420):
    raise SystemExit("canonical embodied static wordmark is missing or incorrectly sized")
if manifest.get("lineage", {}).get("BIDI_D_Reflected") != "Glyph_B reflected through scale.x = 0":
    raise SystemExit("BIDI reflection lineage is missing")
if manifest.get("lineage", {}).get("HangulRestoration") != "MobiusBody(ㅇ) + phase-ground(ㅡ) + i-stem(ㅣ)":
    raise SystemExit("structural Hangul lineage is missing")

def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()

for record in manifest["files"]:
    path = root / record["path"]
    if not path.is_file() or path.stat().st_size != record["bytes"]:
        raise SystemExit(f"manifest file missing or byte count stale: {path}")
    if path.suffix != ".blend" and sha256(path) != record["sha256"]:
        raise SystemExit(f"manifest checksum mismatch: {path}")

required_glbs = {value for key, value in manifest["components"].items() if key != "motionMaster" and value.endswith(".glb")}
for name in required_glbs:
    path = output / name
    with path.open("rb") as handle:
        magic, version, length = struct.unpack("<4sII", handle.read(12))
        json_length, json_kind = struct.unpack("<I4s", handle.read(8))
        gltf = json.loads(handle.read(json_length).decode("utf-8").rstrip(" \x00"))
    if magic != b"glTF" or version != 2 or length != path.stat().st_size or json_kind != b"JSON" or not gltf.get("meshes"):
        raise SystemExit(f"invalid GLB scene payload: {path}")
    if path.stat().st_size > 5_000_000:
        raise SystemExit(f"GLB exceeds five-megabyte web target: {path}")

master_gltf = output / "mobius-identity-master.glb"
with master_gltf.open("rb") as handle:
    handle.read(12)
    json_length, _ = struct.unpack("<I4s", handle.read(8))
    master_json = json.loads(handle.read(json_length).decode("utf-8").rstrip(" \x00"))
animation_names = {animation.get("name") for animation in master_json.get("animations", [])}
required_animation_names = {
    "MobiusBodyMeshAction", "Eye_LeftAction", "Eye_RightAction", "WordmarkRigAction",
    "Glyph_IAction", "Glyph_UAction", "Glyph_SAction", "HangulRestorationRigAction",
    "BIDIDeltaDerivationRigAction", "BIDI_B_SourceAction", "BIDI_D_ReflectedAction",
    "BIDI_I_ReflectedAction", "DeltaStroke_A_From_MAction", "DeltaStroke_B_From_iAction",
    "DeltaStroke_C_From_GroundAction", "CodeSigilRigAction", "CodeCursor_From_PhaseGroundAction",
}
if not required_animation_names.issubset(animation_names):
    raise SystemExit(f"animated master is missing causal tracks: {required_animation_names - animation_names}")

golden = manifest.get("goldenFrames", {})
if len(golden) != 10:
    raise SystemExit("golden-frame release surface is incomplete")
render_hashes = set()
for label, witness in golden.items():
    path = (manifest_path.parent / witness["path"]).resolve()
    with path.open("rb") as handle:
        header = handle.read(24)
    if header[:8] != b"\x89PNG\r\n\x1a\n" or struct.unpack(">II", header[16:24]) != (960, 960):
        raise SystemExit(f"invalid semantic render: {label}")
    render_hashes.add(sha256(path))
if len(render_hashes) != len(golden):
    raise SystemExit("semantic render witnesses are not visually distinct")

if os.environ.get("MOBIUS_REQUIRE_MOTION") == "1":
    ffprobe = shutil.which("ffprobe")
    if not ffprobe:
        raise SystemExit("ffprobe is required for the full motion release gate")
    expected_motion = {
        "mobius-identity-master.mp4": 25.0,
        "mobius-wordmark.mp4": 4.5,
        "mobius-ius.mp4": 2.0,
        "mobius-ui-hangul.mp4": 3.0,
        "mobius-bidi-delta.mp4": 6.0,
        "mobius-operator-u.mp4": 2.0,
        "mobius-code-sigil.mp4": 2.0,
    }
    records = {record["path"]: record for record in manifest["files"]}
    for name, duration in expected_motion.items():
        path = motion_dir / name
        if not path.is_file() or path.stat().st_size < 10_000:
            raise SystemExit(f"motion component missing or too small: {name}")
        key = str(path.relative_to(root))
        if key not in records or records[key]["sha256"] != sha256(path):
            raise SystemExit(f"motion component absent or stale in manifest: {name}")
        probe = json.loads(subprocess.check_output([
            ffprobe, "-v", "error", "-select_streams", "v:0",
            "-show_entries", "stream=codec_name,width,height,r_frame_rate,pix_fmt:format=duration",
            "-of", "json", str(path),
        ], text=True))
        stream = probe["streams"][0]
        if (stream["codec_name"], stream["width"], stream["height"], stream["r_frame_rate"], stream["pix_fmt"]) != ("h264", 720, 720, "24/1", "yuv420p"):
            raise SystemExit(f"motion encoding contract mismatch: {name}: {stream}")
        if abs(float(probe["format"]["duration"]) - duration) > 0.08:
            raise SystemExit(f"motion duration mismatch: {name}")
    if set(manifest.get("motionClips", {})) != {"wordmark", "ius", "ui-hangul", "bidi-delta", "operator-u", "code-sigil"}:
        raise SystemExit("independent motion clip manifest is incomplete")

print(
    "MOBIUS_IDENTITY_ARTIFACT_PASS",
    f"svg={len(svgs)}",
    f"glb={len(required_glbs)}",
    f"golden={len(golden)}",
    "causal=BIDI-reflection+triadic-Delta+structural-Hangul",
)
PY

echo "MOBIUS_IDENTITY_RELEASE_GATE_PASS"
