"""Render lossless motion frames or finalize the encoded master manifest entry."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path

import bpy


REPO = Path(__file__).resolve().parents[2]
MOTION = REPO / "assets" / "identity" / "motion"
OUTPUT = MOTION / "mobius-identity-master.mp4"
FRAMES = REPO / "build" / "identity-blender" / "motion-frames"
MANIFEST = REPO / "assets" / "identity" / "3d" / "identity-manifest.json"
CLIP_OUTPUTS = {
    "wordmark": MOTION / "mobius-wordmark.mp4",
    "ius": MOTION / "mobius-ius.mp4",
    "ui-hangul": MOTION / "mobius-ui-hangul.mp4",
    "bidi-delta": MOTION / "mobius-bidi-delta.mp4",
    "operator-u": MOTION / "mobius-operator-u.mp4",
    "code-sigil": MOTION / "mobius-code-sigil.mp4",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def finalize() -> None:
    if not OUTPUT.is_file() or OUTPUT.stat().st_size < 50_000:
        raise RuntimeError(f"motion render missing or too small: {OUTPUT}")

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    manifest.setdefault("components", {})["motionMaster"] = "../motion/mobius-identity-master.mp4"
    motion_files = {OUTPUT, *CLIP_OUTPUTS.values()}
    manifest["files"] = [record for record in manifest["files"] if REPO / record["path"] not in motion_files]
    for path in motion_files:
        if not path.is_file() or path.stat().st_size < 10_000:
            raise RuntimeError(f"motion component missing or too small: {path}")
        manifest["files"].append(
            {
                "path": str(path.relative_to(REPO)),
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
        )
    manifest["motionMaster"] = "../motion/mobius-identity-master.mp4"
    manifest["motionClips"] = {name: f"../motion/{path.name}" for name, path in CLIP_OUTPUTS.items()}
    manifest["files"].sort(key=lambda record: record["path"])
    MANIFEST.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("MOBIUS_IDENTITY_MOTION_RENDER_PASS", OUTPUT.stat().st_size, sha256(OUTPUT))


def render_frames() -> None:
    FRAMES.mkdir(parents=True, exist_ok=True)
    for frame in FRAMES.glob("mobius-*.png"):
        frame.unlink()

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 720
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.fps = 24
    scene.frame_start = 1
    scene.frame_end = 600
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.filepath = str(FRAMES / "mobius-")
    bpy.ops.render.render(animation=True)
    frames = sorted(FRAMES.glob("mobius-*.png"))
    expected = scene.frame_end - scene.frame_start + 1
    if len(frames) != expected:
        raise RuntimeError(f"motion frame count mismatch: expected {expected}, found {len(frames)}")
    print("MOBIUS_IDENTITY_FRAME_RENDER_PASS", len(frames), FRAMES)


def main() -> None:
    MOTION.mkdir(parents=True, exist_ok=True)
    if os.environ.get("MOBIUS_FINALIZE_MOTION") == "1":
        finalize()
    else:
        render_frames()


if __name__ == "__main__":
    main()
