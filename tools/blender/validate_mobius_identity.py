"""Visual-semantic release gate for the glyph-derived Möbi𝒰s master.

This validator deliberately checks causality, not only file presence. A
preassembled BIDI logo, pasted Hangul glyph, or flat O substituted for the
embodied mark cannot satisfy the scene contract.
"""

from __future__ import annotations

from collections import defaultdict, deque
import hashlib
import json
import math
from pathlib import Path
import struct

import bpy
from mathutils import Vector


REPO = Path(__file__).resolve().parents[2]
OUTPUT = REPO / "assets" / "identity" / "3d"
RENDERS = REPO / "assets" / "identity" / "renders"
REPORT = REPO / "build" / "identity-blender" / "validation.json"
HIDDEN_LIMIT = 0.01

REQUIRED_GLBS = [
    "mobius-identity-master.glb",
    "mobius-wordmark-animated.glb",
    "mobius-ius-phase-animated.glb",
    "mobius-ui-hangul-animated.glb",
    "mobius-bidi-delta-animated.glb",
    "mobius-u-animated.glb",
    "mobius-u-code-animated.glb",
    "mobius-body.glb",
    "mobius-u-operator.glb",
    "mobius-ius-relational.glb",
    "mobius-u-code-sigil.glb",
    "mobius-bidi-delta.glb",
]

GOLDEN = {
    "presence": 1,
    "wordmark-generated": 84,
    "ius-phase": 132,
    "ui-restored": 180,
    "hangul-restored": 216,
    "bidi-extracted": 360,
    "delta-closed": 408,
    "u-standalone": 456,
    "u-code": 500,
    "family-lockup": 576,
}


def fail(message: str) -> None:
    raise SystemExit(f"identity validation failed: {message}")


def required_object(name: str) -> bpy.types.Object:
    found = bpy.data.objects.get(name)
    if not found:
        fail(f"required object missing: {name}")
    return found


def scale_size(obj: bpy.types.Object) -> float:
    return max(abs(value) for value in obj.matrix_world.to_scale())


def visible(obj: bpy.types.Object) -> bool:
    return scale_size(obj) > HIDDEN_LIMIT


def boundary_components(mesh: bpy.types.Mesh) -> tuple[int, int]:
    edge_faces: dict[tuple[int, int], int] = defaultdict(int)
    for polygon in mesh.polygons:
        vertices = list(polygon.vertices)
        for index, start in enumerate(vertices):
            end = vertices[(index + 1) % len(vertices)]
            edge_faces[tuple(sorted((start, end)))] += 1
    boundary = [edge for edge, count in edge_faces.items() if count == 1]
    graph: dict[int, set[int]] = defaultdict(set)
    for start, end in boundary:
        graph[start].add(end)
        graph[end].add(start)
    if any(len(neighbors) != 2 for neighbors in graph.values()):
        fail("Möbius boundary is not a cycle")
    remaining = set(graph)
    components = 0
    while remaining:
        components += 1
        queue = deque([remaining.pop()])
        while queue:
            current = queue.popleft()
            for neighbor in graph[current]:
                if neighbor in remaining:
                    remaining.remove(neighbor)
                    queue.append(neighbor)
    return len(boundary), components


def glb_ok(path: Path) -> bool:
    if not path.is_file() or path.stat().st_size < 20:
        return False
    with path.open("rb") as handle:
        magic, version, declared = struct.unpack("<4sII", handle.read(12))
    return magic == b"glTF" and version == 2 and declared == path.stat().st_size


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def has_action(owner: object) -> bool:
    animation = getattr(owner, "animation_data", None)
    return bool(animation and animation.action)


def set_frame(scene: bpy.types.Scene, frame: int) -> None:
    scene.frame_set(frame)
    bpy.context.view_layer.update()


def bar_endpoints(bar: bpy.types.Object) -> tuple[Vector, Vector]:
    center = bar.matrix_world.translation
    direction = (bar.matrix_world.to_3x3() @ Vector((1.0, 0.0, 0.0))).normalized()
    half_length = bar.dimensions.x * 0.5
    return center - direction * half_length, center + direction * half_length


def closest_by_y(points: tuple[Vector, Vector], *, high: bool) -> Vector:
    return max(points, key=lambda point: point.y) if high else min(points, key=lambda point: point.y)


def main() -> None:
    scene = bpy.context.scene
    if scene.frame_end != 600:
        fail(f"master timeline must end at frame 600, got {scene.frame_end}")
    if scene.get("finished_asset_substitution_allowed") is not False:
        fail("master does not explicitly forbid finished-asset substitution")

    body = required_object("MobiusBody")
    body_root = required_object("EmbodiedMobiusRig")
    word_root = required_object("WordmarkRig")
    if body.type != "MESH" or body.get("topology") != "mobius-band" or body.get("half_twists") != 1:
        fail("connected Möbius body metadata missing or incorrect")
    vertices = len(body.data.vertices)
    edges = len(body.data.edges)
    faces = len(body.data.polygons)
    euler = vertices - edges + faces
    boundary_edges, boundaries = boundary_components(body.data)
    if euler != 0 or boundaries != 1:
        fail(f"invalid Möbius topology: chi={euler}, boundaries={boundaries}")
    if set(body.data.shape_keys.key_blocks.keys()) != {"Basis", "UProjection", "DeltaProjection"}:
        fail("body projection shape keys incomplete")

    eyes = [obj for obj in bpy.data.objects if obj.get("identity_role") == "literal-animated-eye"]
    if len(eyes) != 2 or not all(has_action(eye) for eye in eyes):
        fail("exactly two literal animated eyes are required")

    # The master may import only the source wordmark anatomy. A completed BIDI
    # asset would violate the derivation requirement even if it looked right.
    imported_sources = [str(obj.get("source_svg", "")) for obj in bpy.data.objects if obj.get("source_svg")]
    if any("bidi-kernel" in source or "bidi-delta" in source for source in imported_sources):
        fail("preassembled BIDI asset was imported into the master")

    required_objects = [
        "Glyph_M", "Glyph_O", "Glyph_B", "Glyph_I", "Glyph_U", "Glyph_S",
        "Glyph_Ground_Left", "Glyph_Ground_Right", "HangulRestorationRig",
        "Hangul_I_From_i_Stem", "BIDIDeltaDerivationRig", "BIDI_B_Source",
        "BIDI_I_Source", "BIDI_D_Reflected", "BIDI_I_Reflected",
        "DeltaStroke_A_From_M", "DeltaStroke_B_From_i", "DeltaStroke_C_From_Ground",
        "CodeSigilRig", "OperatorU_Standalone", "CodeCursor_From_PhaseGround",
    ]
    objects = {name: required_object(name) for name in required_objects}

    # Frame 84: the public still is the live organism seated inside its name.
    set_frame(scene, 84)
    for name in ("Glyph_M", "Glyph_B", "Glyph_I", "Glyph_U", "Glyph_S", "Glyph_Ground_Left", "Glyph_Ground_Right"):
        if not visible(objects[name]):
            fail(f"wordmark frame is missing {name}")
    if visible(objects["Glyph_O"]):
        fail("flat SVG O is visible in the embodied static wordmark")
    if not visible(body_root) or not all(visible(eye) for eye in eyes):
        fail("collapsed Möbius body and eyes are not present in the static wordmark")
    substrate = bpy.data.materials.get("Wordmark_Substrate_Indigo")
    if substrate is None or max(substrate.diffuse_color[:3]) > 0.5:
        fail("wordmark substrate is missing or has regressed to a snow-white value")
    for role in ("M", "B"):
        material_names = {
            material.name
            for child in objects[f"Glyph_{role}"].children
            for material in getattr(child.data, "materials", ())
        }
        if "Wordmark_Substrate_Indigo" not in material_names:
            fail(f"Glyph_{role} does not carry the indigo substrate material")
    if max((light.energy for light in bpy.data.lights), default=0.0) > 300.0:
        fail("studio lighting exceeds the palette-preserving energy ceiling")
    o_slot = objects["Glyph_O"].matrix_world.translation
    if (body_root.matrix_world.translation - o_slot).length > 0.04:
        fail("collapsed Möbius body is not seated in the O slot")

    # During generation the operator must originate at the conserved aperture,
    # then its outgoing terminal must generate S. This rejects a regression to
    # independent bottom-up glyph arrivals that only happen to end adjacent.
    set_frame(scene, 63)
    u_location = objects["Glyph_U"].matrix_world.translation
    if (u_location - o_slot).length >= 2.8:
        fail("wordmark U does not visibly project from the conserved aperture")
    if visible(objects["Glyph_S"]):
        fail("S appears before the projected U establishes the relational hinge")
    set_frame(scene, 75)
    if not visible(objects["Glyph_U"]) or not visible(objects["Glyph_S"]):
        fail("U-to-S generation event is missing a visible participant")
    s_scale_x = abs(objects["Glyph_S"].scale.x)
    if not 0.15 < s_scale_x < 0.80:
        fail("S does not visibly unfurl from a compressed U-terminal state")

    # Frame 132: i𝒰s is isolated and the operator carries the shear transition.
    set_frame(scene, 132)
    if visible(objects["Glyph_M"]) or visible(objects["Glyph_B"]):
        fail("i𝒰s frame did not isolate the terminal relational triad")
    for name in ("Glyph_I", "Glyph_U", "Glyph_S"):
        if not visible(objects[name]):
            fail(f"i𝒰s frame missing {name}")
    u_meshes = [child for child in objects["Glyph_U"].children if child.data and getattr(child.data, "shape_keys", None)]
    if not u_meshes or not all(mesh.data.shape_keys.key_blocks["RelationalLean"].value > 0.8 for mesh in u_meshes):
        fail("operator U does not carry its internal relational shear")

    # Frame 180: order and sheet orientation are both restored, not relabeled.
    set_frame(scene, 180)
    if objects["Glyph_U"].matrix_world.translation.x >= objects["Glyph_I"].matrix_world.translation.x:
        fail("UI restoration did not swap the U and I into readable order")
    if visible(objects["Glyph_S"]):
        fail("S remained visible during the UI restoration witness")
    if abs(abs(objects["Glyph_U"].rotation_euler.y) - math.pi) > 0.08:
        fail("UI restoration lacks the one-sheet flip")

    # Frame 216: 의 is assembled from body, ground, and i-stem—not type.
    set_frame(scene, 216)
    if not visible(body_root) or not visible(objects["HangulRestorationRig"]) or not visible(objects["Glyph_Ground_Right"]):
        fail("structural Hangul restoration components are incomplete")
    if visible(objects["Glyph_I"]) or visible(objects["Glyph_U"]):
        fail("Latin I/U glyphs were not released before the structural 의 state")
    if objects["Hangul_I_From_i_Stem"].get("derived_from") != "Glyph_I":
        fail("Hangul vertical lacks inspectable i-stem lineage")
    circle_location = body_root.matrix_world.translation
    ground_location = objects["Glyph_Ground_Right"].matrix_world.translation
    vertical_location = objects["Hangul_I_From_i_Stem"].matrix_world.translation
    if abs(circle_location.x - ground_location.x) > 0.16 or ground_location.y >= circle_location.y - 0.55:
        fail("Hangul ㅇ and ㅡ are not locked into one centered syllabic column")
    if vertical_location.x <= circle_location.x + 0.54 or abs(vertical_location.y - ground_location.y) > 0.58:
        fail("Hangul ㅣ is not seated tightly beside the restored ㅇ/ㅡ block")

    # Frame 340: the projected B visibly collapses through the inversion plane
    # while the original BI has vacated the parent word.
    set_frame(scene, 340)
    if not visible(objects["BIDI_B_Source"]) or not visible(objects["BIDI_I_Source"]):
        fail("source BI did not visibly leave the parent word")
    if abs(objects["BIDI_D_Reflected"].scale.x) >= 0.25:
        fail("projected B does not pass through the zero-width inversion state")
    if visible(objects["Glyph_B"]) or visible(objects["Glyph_I"]):
        fail("original BI did not vacate its Möbi𝒰s slots during self-projection")

    # Frame 360: the projected pair crystallizes as DI beside source BI.
    set_frame(scene, 360)
    clone_names = ("BIDI_B_Source", "BIDI_I_Source", "BIDI_D_Reflected", "BIDI_I_Reflected")
    if not all(visible(objects[name]) for name in clone_names):
        fail("BIDI extraction is missing one or more live BI descendants")
    if objects["BIDI_D_Reflected"].scale.x >= -0.5:
        fail("D is not a literal negative-scale reflection of source B")
    if objects["BIDI_D_Reflected"].get("derived_from") != "Glyph_B":
        fail("reflected D lacks source-B lineage")
    if objects["BIDI_I_Reflected"].get("derived_from") != "Glyph_I":
        fail("duplicate I lacks source-I lineage")
    if visible(objects["Glyph_B"]) or visible(objects["Glyph_I"]) or not visible(word_root):
        fail("parent word did not preserve the visible BI vacancy during crystallization")

    # Frame 372: the M-derived Delta edge must be visibly seated on the M's
    # actual inner diagonal before it peels away. This forbids a detached bar
    # simply materializing beside the word.
    set_frame(scene, 372)
    m_seed = objects["DeltaStroke_A_From_M"]
    if not visible(m_seed) or m_seed.get("source_edge") != "Glyph_M:right-inner-diagonal":
        fail("M-derived Delta edge lacks a visible inner-diagonal seed")
    if (m_seed.matrix_world.translation - objects["Glyph_M"].matrix_world.translation).length > 0.95:
        fail("M-derived Delta edge is not seated within the source glyph")
    if abs(m_seed.rotation_euler.z - math.radians(60.0)) > math.radians(1.0):
        fail("M-derived Delta seed does not share the M inner-diagonal angle")

    # Frame 408: exactly three source strokes close one triangle next to BIDI.
    set_frame(scene, 408)
    strokes = [objects[name] for name in ("DeltaStroke_A_From_M", "DeltaStroke_B_From_i", "DeltaStroke_C_From_Ground")]
    if not all(visible(stroke) for stroke in strokes):
        fail("triadic Delta closure is incomplete")
    if {stroke.get("derived_from") for stroke in strokes} != {"Glyph_M", "Glyph_I", "Glyph_Ground_Right"}:
        fail("Delta strokes do not preserve three distinct type-source lineages")
    a_ends, b_ends, c_ends = (bar_endpoints(stroke) for stroke in strokes)
    a_top, b_top = closest_by_y(a_ends, high=True), closest_by_y(b_ends, high=True)
    a_low, b_low = closest_by_y(a_ends, high=False), closest_by_y(b_ends, high=False)
    c_left, c_right = sorted(c_ends, key=lambda point: point.x)
    closure_gaps = [(a_top - b_top).length, (a_low - c_left).length, (b_low - c_right).length]
    if max(closure_gaps) > 0.22:
        fail(f"three Delta strokes do not close geometrically: gaps={closure_gaps}")
    if visible(objects["Glyph_B"]) or visible(objects["Glyph_I"]):
        fail("original BI returned before the generated BIDI Delta closure completed")

    set_frame(scene, 420)
    if not visible(objects["Glyph_B"]) or not visible(objects["Glyph_I"]):
        fail("original BI did not resolve back into Möbi𝒰s after closure")

    # Standalone connected U and terminal U_ each earn their own frame.
    set_frame(scene, 456)
    if not visible(body_root) or body.data.shape_keys.key_blocks["UProjection"].value < 0.95:
        fail("standalone U is not the opened connected Möbius body")
    if not all(visible(eye) for eye in eyes):
        fail("literal eyes did not survive the operative U projection")
    set_frame(scene, 500)
    if not visible(objects["CodeSigilRig"]) or not visible(objects["OperatorU_Standalone"]) or not visible(objects["CodeCursor_From_PhaseGround"]):
        fail("terminal-ready 𝒰_ frame is incomplete")
    cursor_overhang = objects["CodeCursor_From_PhaseGround"].matrix_world.translation.x - objects["OperatorU_Standalone"].matrix_world.translation.x
    if cursor_overhang < 0.98:
        fail("code cursor lacks the intentional right-hand terminal overhang")

    # In the resolved family lockup, the same overhang remains present inside
    # Delta but lifts clear of its base and stays within the triangle's span.
    set_frame(scene, 576)
    cursor = objects["CodeCursor_From_PhaseGround"]
    delta_base = objects["DeltaStroke_C_From_Ground"]
    cursor_ends = bar_endpoints(cursor)
    base_ends = bar_endpoints(delta_base)
    if cursor.matrix_world.translation.y - delta_base.matrix_world.translation.y < 0.22:
        fail("resolved code cursor clashes with the Delta base")
    if max(point.x for point in cursor_ends) > max(point.x for point in base_ends) + 0.05:
        fail("resolved code cursor overhang escapes the Delta frame")

    # Required animation owners prove that the scene is rigged, not a still montage.
    animated = [
        body_root, body.data.shape_keys, word_root, objects["Glyph_I"], objects["Glyph_U"], objects["Glyph_S"],
        objects["HangulRestorationRig"], objects["BIDIDeltaDerivationRig"], *[objects[name] for name in clone_names],
        *strokes, objects["CodeSigilRig"], objects["CodeCursor_From_PhaseGround"], *eyes,
    ]
    if not all(has_action(owner) for owner in animated):
        missing = [getattr(owner, "name", type(owner).__name__) for owner in animated if not has_action(owner)]
        fail(f"required animation tracks missing: {missing}")

    bad_glbs = [name for name in REQUIRED_GLBS if not glb_ok(OUTPUT / name)]
    if bad_glbs:
        fail(f"invalid GLB exports: {bad_glbs}")
    oversized = [name for name in REQUIRED_GLBS if (OUTPUT / name).stat().st_size > 5_000_000]
    if oversized:
        fail(f"web GLB exceeds 5 MB target: {oversized}")

    render_hashes: dict[str, str] = {}
    for label in GOLDEN:
        path = RENDERS / f"mobius-{label}.png"
        if not path.is_file() or path.stat().st_size < 10_000:
            fail(f"golden render missing or too small: {path.name}")
        with path.open("rb") as handle:
            header = handle.read(24)
        if header[:8] != b"\x89PNG\r\n\x1a\n" or struct.unpack(">II", header[16:24]) != (960, 960):
            fail(f"invalid golden render: {path.name}")
        render_hashes[label] = sha256(path)
    if len(set(render_hashes.values())) != len(render_hashes):
        fail("two semantic golden frames are visually identical")

    static_wordmark = RENDERS / "mobius-wordmark-static.png"
    with static_wordmark.open("rb") as handle:
        static_header = handle.read(24)
    if static_header[:8] != b"\x89PNG\r\n\x1a\n" or struct.unpack(">II", static_header[16:24]) != (1440, 420):
        fail("canonical embodied static wordmark is missing or has the wrong production dimensions")

    manifest_path = OUTPUT / "identity-manifest.json"
    if not manifest_path.is_file():
        fail("identity manifest missing")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schemaVersion") != 2 or manifest.get("formalKernel") != "bidiγΔ":
        fail("glyph-derived manifest schema or hierarchy is incorrect")
    if manifest.get("universalOperator") != {
        "canonicalSigil": "𝒰_",
        "reducedMathematicalBody": "𝒰",
        "indexedNotation": "𝒰_R",
        "law": "one operator at multiple resolution levels; underscore is the executable live horizon",
    }:
        fail("manifest does not canonize 𝒰_ as the complete Universal Operator sigil")
    if manifest.get("compatibilityAliases", {}).get("codeSigil") != "universalOperatorSigil":
        fail("legacy code-sigil handle is not mapped to the canonical Universal Operator sigil")
    if manifest.get("staticArtifact", {}).get("frame") != 84:
        fail("manifest does not expose the embodied static wordmark")
    if manifest.get("staticArtifact", {}).get("path") != "../renders/mobius-wordmark-static.png":
        fail("manifest static artifact does not point to the embodied production still")
    if set(manifest.get("goldenFrames", {})) != set(GOLDEN):
        fail("manifest golden-frame surface is incomplete")
    if manifest.get("lineage", {}).get("BIDI_D_Reflected") != "Glyph_B reflected through scale.x = 0":
        fail("manifest omits the literal B-to-D reflection law")
    if scene.get("wordmark_counter_law") != "open-M V; filled-B base with one enlarged counter; continuous-S double curve with open counters":
        fail("wordmark negative-space contract is missing")
    if scene.get("wordmark_generation_law") != "U projects from the conserved aperture; S unfurls from the U return terminal":
        fail("wordmark U-to-S generation contract is missing")
    if scene.get("us_coupling_law") != "single generated U-to-S terminal event with distinct open counters":
        fail("U/S relational coupling contract is missing")

    report = {
        "status": "PASS",
        "blender": bpy.app.version_string,
        "scene": scene.name,
        "timeline": [scene.frame_start, scene.frame_end],
        "topology": {
            "vertices": vertices,
            "edges": edges,
            "faces": faces,
            "eulerCharacteristic": euler,
            "boundaryEdges": boundary_edges,
            "boundaryComponents": boundaries,
            "halfTwists": body.get("half_twists"),
        },
        "semanticWitnesses": GOLDEN,
        "deltaClosureGaps": closure_gaps,
        "eyes": [eye.name for eye in eyes],
        "glbFiles": {name: (OUTPUT / name).stat().st_size for name in REQUIRED_GLBS},
        "renderHashes": render_hashes,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("MOBIUS_IDENTITY_VALIDATION_PASS", json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
