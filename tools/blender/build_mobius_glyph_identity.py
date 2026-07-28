"""Build the complete glyph-derived Möbi𝒰s motion identity.

The topology foundation lives in ``build_mobius_identity.py``. This generator
uses that conserved body, then rigs the exact portable wordmark outlines as
independent glyphs. No finished BIDI or Hangul mark is substituted into the
master timeline: every derived state retains inspectable object lineage.
"""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_mobius_identity as base


REPO = base.REPO
IDENTITY = base.IDENTITY
OUTPUT = base.OUTPUT
RENDERS = base.RENDERS
MOTION = base.MOTION
REPORTS = base.REPORTS
PHI = base.PHI
FPS = base.FPS

MASTER_END = 600
HIDDEN_FACTOR = 0.000001

FRAMES = {
    "PRESENCE": (1, 35),
    "WORDMARK_GENERATION": (36, 84),
    "ONE_TURN_WORDMARK": (85, 108),
    "IUS_PHASE": (109, 156),
    "UI_RESTORATION": (157, 192),
    "HANGUL_RESTORATION": (193, 228),
    "RETURN_TO_WORDMARK": (229, 276),
    "BIDI_SELF_EXTRACTION": (277, 368),
    "DELTA_TRIADIC_CLOSURE": (369, 420),
    "OPERATOR_U": (421, 468),
    "CODE_SIGIL": (469, 516),
    "FAMILY_LOCKUP": (517, 600),
}

CLIPS = {
    "wordmark": (1, 108),
    "ius": (109, 156),
    "ui-hangul": (157, 228),
    "bidi-delta": (277, 420),
    "operator-u": (421, 468),
    "code-sigil": (469, 516),
}

GOLDEN_FRAMES = {
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


@dataclass
class GlyphRig:
    root: bpy.types.Object
    glyphs: dict[str, bpy.types.Object]
    meshes: dict[str, list[bpy.types.Object]]
    base_locations: dict[str, Vector]
    base_scales: dict[str, Vector]
    base_rotations: dict[str, Vector]


def log(message: str) -> None:
    print(f"[mobius-glyph-identity] {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def curve_order(obj: bpy.types.Object) -> int:
    if obj.name == "Curve":
        return 0
    try:
        return int(obj.name.rsplit(".", 1)[1])
    except (IndexError, ValueError):
        return 10_000


def object_bounds(objects: list[bpy.types.Object]) -> tuple[float, float, float, float]:
    corners: list[Vector] = []
    for obj in objects:
        corners.extend(obj.matrix_world @ Vector(corner) for corner in obj.bound_box)
    return (
        min(point.x for point in corners),
        max(point.x for point in corners),
        min(point.y for point in corners),
        max(point.y for point in corners),
    )


def replace_material(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    if obj.type not in {"MESH", "CURVE"}:
        return
    obj.data.materials.clear()
    obj.data.materials.append(material)


def convert_curve(obj: bpy.types.Object) -> bpy.types.Object:
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target="MESH")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
    obj.select_set(False)
    return obj


def import_wordmark_rig(
    materials: dict[str, bpy.types.Material],
    target: bpy.types.Collection,
    target_width: float = 8.6,
) -> GlyphRig:
    before = set(bpy.data.objects)
    source = IDENTITY / "mobius-u-wordmark-dark.svg"
    bpy.ops.import_curve.svg(filepath=str(source))
    curves = sorted(
        (obj for obj in bpy.data.objects if obj not in before and obj.type == "CURVE"),
        key=curve_order,
    )
    roles = ["M", "O", "B", "Eye2D_Left", "Eye2D_Right", "I", "U", "S", "Ground_Left", "Ground_Right"]
    if len(curves) != len(roles):
        raise RuntimeError(f"wordmark SVG anatomy changed: expected {len(roles)} curves, found {len(curves)}")

    min_x, max_x, min_y, max_y = object_bounds(curves)
    factor = target_width / (max_x - min_x)
    offset = Vector((-factor * (min_x + max_x) / 2.0, -factor * (min_y + max_y) / 2.0, 0.0))

    root = bpy.data.objects.new("WordmarkRig", None)
    target.objects.link(root)
    root["identity_role"] = "glyph-derived-wordmark-rig"
    root["source_svg"] = str(source.relative_to(REPO))

    role_material = {
        "M": materials["word"],
        "O": materials["word"],
        "B": materials["word"],
        "Eye2D_Left": materials["eye"],
        "Eye2D_Right": materials["eye"],
        "I": materials["i"],
        "U": materials["white"],
        "S": materials["s"],
        "Ground_Left": materials["i"],
        "Ground_Right": materials["s"],
    }

    glyphs: dict[str, bpy.types.Object] = {}
    meshes: dict[str, list[bpy.types.Object]] = {}
    for curve, role in zip(curves, roles):
        base.move_to_collection(curve, target)
        curve.data.dimensions = "2D"
        curve.data.extrude = 0.014
        curve.data.bevel_depth = 0.002
        curve.scale = (factor, factor, factor)
        curve.location = offset
        replace_material(curve, role_material[role])
        mesh = convert_curve(curve)
        if mesh.dimensions.z > 0.0:
            mesh.scale.z *= 0.045 / mesh.dimensions.z
            bpy.ops.object.select_all(action="DESELECT")
            mesh.select_set(True)
            bpy.context.view_layer.objects.active = mesh
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            mesh.select_set(False)
        mesh.name = f"GlyphMesh_{role}"
        mesh["glyph_role"] = role
        mesh["source_wordmark"] = "Möbi𝒰s"

        pivot = bpy.data.objects.new(f"Glyph_{role}", None)
        target.objects.link(pivot)
        pivot.location = mesh.location.copy()
        pivot.parent = root
        mesh.parent = pivot
        mesh.location = (0.0, 0.0, 0.0)
        mesh.rotation_euler = (0.0, 0.0, 0.0)
        mesh.scale = (1.0, 1.0, 1.0)
        pivot["glyph_role"] = role
        pivot["source_wordmark"] = "Möbi𝒰s"
        glyphs[role] = pivot
        meshes[role] = [mesh]

    # The blue keyline is a second projection of the same U mesh, not a new glyph.
    u_fill = meshes["U"][0]
    u_outline = u_fill.copy()
    u_outline.data = u_fill.data.copy()
    u_outline.name = "GlyphMesh_U_Keyline"
    target.objects.link(u_outline)
    u_outline.parent = glyphs["U"]
    u_outline.location = u_fill.location.copy()
    u_outline.rotation_euler = u_fill.rotation_euler.copy()
    u_outline.scale = tuple(value * 1.045 for value in u_fill.scale)
    u_outline.location.z -= 0.035
    replace_material(u_outline, materials["u_blue"])
    meshes["U"].insert(0, u_outline)

    # White carries more optical weight than pigment; the operator is
    # deliberately smaller than a nominal lowercase glyph, matching the
    # approved wordmark direction rather than letting calligraphy dominate it.
    glyphs["U"].scale = (0.86, 0.86, 0.86)

    # The S is generated by the operator's outgoing terminal, so its animation
    # pivot belongs on the left-hand contact edge rather than at the glyph's
    # geometric center. Move the pivot without moving the visible geometry.
    # This lets the double curve unfurl from the U/S tangent instead of
    # materializing as a compressed shape at the far side of its own box.
    s_vertices = [vertex.co.copy() for vertex in meshes["S"][0].data.vertices]
    s_min_x = min(vertex.x for vertex in s_vertices)
    s_max_x = max(vertex.x for vertex in s_vertices)
    s_edge = [vertex for vertex in s_vertices if vertex.x <= s_min_x + (s_max_x - s_min_x) * 0.035]
    s_anchor = Vector((s_min_x, sum(vertex.y for vertex in s_edge) / len(s_edge), 0.0))
    glyphs["S"].location += s_anchor
    for child in meshes["S"]:
        child.location -= s_anchor
    glyphs["S"]["generation_anchor"] = "left-contact-edge-shared-with-U-terminal"

    base_locations = {role: pivot.location.copy() for role, pivot in glyphs.items()}
    base_scales = {role: pivot.scale.copy() for role, pivot in glyphs.items()}
    base_rotations = {role: Vector(pivot.rotation_euler) for role, pivot in glyphs.items()}
    return GlyphRig(root, glyphs, meshes, base_locations, base_scales, base_rotations)


def clone_glyph(
    rig: GlyphRig,
    source_role: str,
    clone_name: str,
    material: bpy.types.Material,
    target: bpy.types.Collection,
    parent: bpy.types.Object | None = None,
) -> bpy.types.Object:
    source = rig.glyphs[source_role]
    clone = bpy.data.objects.new(clone_name, None)
    target.objects.link(clone)
    clone.parent = parent or rig.root
    clone.location = source.location.copy()
    clone.rotation_euler = source.rotation_euler.copy()
    clone.scale = source.scale.copy()
    clone["identity_role"] = "glyph-lineage-clone"
    clone["derived_from"] = source.name
    clone["source_wordmark"] = "Möbi𝒰s"
    for child in rig.meshes[source_role]:
        # The U keyline is not needed for BI lineage; this remains generic.
        duplicate = child.copy()
        duplicate.data = child.data.copy()
        duplicate.name = f"{clone_name}_{child.name}"
        target.objects.link(duplicate)
        duplicate.parent = clone
        duplicate.location = child.location.copy()
        duplicate.rotation_euler = child.rotation_euler.copy()
        duplicate.scale = child.scale.copy()
        replace_material(duplicate, material)
        duplicate["derived_from"] = child.name
    return clone


def add_u_shear_keys(rig: GlyphRig) -> list[bpy.types.ShapeKey]:
    keys: list[bpy.types.ShapeKey] = []
    for mesh in rig.meshes["U"]:
        if mesh.type != "MESH":
            continue
        basis = mesh.shape_key_add(name="Basis")
        basis.interpolation = "KEY_LINEAR"
        lean = mesh.shape_key_add(name="RelationalLean")
        xs = [vertex.co.x for vertex in mesh.data.vertices]
        ys = [vertex.co.y for vertex in mesh.data.vertices]
        min_x, max_x = min(xs), max(xs)
        center_y = (min(ys) + max(ys)) / 2.0
        width = max(max_x - min_x, 1e-6)
        for target_vertex, source_vertex in zip(lean.data, mesh.data.vertices):
            left_weight = 1.0 - (source_vertex.co.x - min_x) / width
            target_vertex.co.x += math.tan(PHI) * (source_vertex.co.y - center_y) * (0.18 + 0.82 * left_weight)
        lean.interpolation = "KEY_LINEAR"
        keys.append(mesh.data.shape_keys)
    return keys


def make_materials(materials: dict[str, bpy.types.Material]) -> dict[str, bpy.types.Material]:
    # Chromatic hierarchy, not decorative variation:
    # substrate -> directed perspective -> white-hot operator -> relation.
    # Only the U is white.  Keeping the broad word skeleton in indigo prevents
    # the large glyph faces from overwhelming the smaller semantic color
    # transitions under studio lighting.
    materials.update(
        {
            "word": base.principled_material("Wordmark_Substrate_Indigo", (0.035, 0.060, 0.360, 1.0), roughness=0.52, emission=0.18),
            "i": base.principled_material("Perspective_Indigo", (0.075, 0.105, 0.600, 1.0), roughness=0.50, emission=0.20),
            "u_blue": base.principled_material("Operator_Keyline_Blue", (0.006, 0.260, 0.950, 1.0), roughness=0.38, emission=0.28),
            "s": base.principled_material("Relation_Cyan", (0.025, 0.620, 0.700, 1.0), roughness=0.46, emission=0.20),
            "bidi_left": base.principled_material("BIDI_Source_Indigo", (0.055, 0.085, 0.500, 1.0), roughness=0.46, emission=0.18),
            "bidi_right": base.principled_material("BIDI_Reflected_Cyan", (0.025, 0.560, 0.660, 1.0), roughness=0.44, emission=0.20),
            "delta_a": base.principled_material("Delta_Indigo", (0.055, 0.085, 0.500, 1.0), metallic=0.03, roughness=0.40, emission=0.18),
            "delta_b": base.principled_material("Delta_Electric_Blue", (0.080, 0.300, 0.900, 1.0), metallic=0.03, roughness=0.38, emission=0.22),
            "delta_c": base.principled_material("Delta_Cyan", (0.025, 0.620, 0.700, 1.0), metallic=0.03, roughness=0.40, emission=0.20),
        }
    )
    return materials


def key_transform(
    obj: bpy.types.Object,
    frame: int,
    *,
    location: Vector | tuple[float, float, float] | None = None,
    scale: Vector | tuple[float, float, float] | None = None,
    rotation: Vector | tuple[float, float, float] | None = None,
) -> None:
    if location is not None:
        obj.location = location
        obj.keyframe_insert(data_path="location", frame=frame)
    if scale is not None:
        obj.scale = scale
        obj.keyframe_insert(data_path="scale", frame=frame)
    if rotation is not None:
        obj.rotation_euler = rotation
        obj.keyframe_insert(data_path="rotation_euler", frame=frame)


def key_shape(shape_keys: list[bpy.types.ShapeKey], frame: int, value: float) -> None:
    for keys in shape_keys:
        block = keys.key_blocks["RelationalLean"]
        block.value = value
        block.keyframe_insert(data_path="value", frame=frame)


def hidden_scale(base_scale: Vector, factor: float = HIDDEN_FACTOR) -> Vector:
    return Vector(tuple(value * factor for value in base_scale))


def key_visibility(rig: GlyphRig, role: str, frame: int, visible: bool, factor: float = 1.0) -> None:
    base_scale = rig.base_scales[role]
    key_transform(
        rig.glyphs[role],
        frame,
        scale=Vector(tuple(value * factor for value in base_scale)) if visible else hidden_scale(base_scale),
    )


def key_camera(camera: bpy.types.Object, frame: int, *, x: float = 0.0, y: float = 0.0, scale: float) -> None:
    camera.location = (x, y, 10.0)
    camera.keyframe_insert(data_path="location", frame=frame)
    camera.data.ortho_scale = scale
    camera.data.keyframe_insert(data_path="ortho_scale", frame=frame)


def create_delta_strokes(
    materials: dict[str, bpy.types.Material],
    target: bpy.types.Collection,
) -> list[bpy.types.Object]:
    strokes = [
        base.create_bar("DeltaStroke_A_From_M", (0.0, 0.0, 0.32), (1.22, 0.045, 0.035), materials["delta_a"], target),
        base.create_bar("DeltaStroke_B_From_i", (0.0, 0.0, 0.34), (1.22, 0.045, 0.035), materials["delta_b"], target),
        base.create_bar("DeltaStroke_C_From_Ground", (0.0, 0.0, 0.36), (1.06, 0.045, 0.035), materials["delta_c"], target),
    ]
    for stroke, source in zip(strokes, ("Glyph_M", "Glyph_I", "Glyph_Ground_Right")):
        stroke["identity_role"] = "triadic-delta-stroke"
        stroke["derived_from"] = source
        stroke["closure_system"] = "BIDI_DELTA"
    strokes[0]["source_edge"] = "Glyph_M:right-inner-diagonal"
    strokes[0]["extraction_law"] = "highlight-peel-translate"
    return strokes


def create_hangul_bar(
    materials: dict[str, bpy.types.Material],
    target: bpy.types.Collection,
) -> bpy.types.Object:
    bar = base.create_bar("Hangul_I_From_i_Stem", (0.0, 0.0, 0.30), (0.045, 0.90, 0.035), materials["i"], target)
    bar["identity_role"] = "hangul-vertical-derived-from-i"
    bar["derived_from"] = "Glyph_I"
    return bar


def create_code_cursor(
    materials: dict[str, bpy.types.Material],
    target: bpy.types.Collection,
) -> bpy.types.Object:
    cursor = base.create_bar("CodeCursor_From_PhaseGround", (0.0, 0.0, 0.30), (0.82, 0.045, 0.035), materials["u_blue"], target)
    cursor["identity_role"] = "code-cursor-horizontal-invariant"
    cursor["derived_from"] = "Glyph_Ground_Right"
    return cursor


def normalize_interpolation(owners: list[object]) -> None:
    for owner in owners:
        animation = getattr(owner, "animation_data", None)
        action = animation.action if animation else None
        if not action:
            continue
        for curve in getattr(action, "fcurves", ()):
            for point in curve.keyframe_points:
                point.interpolation = "BEZIER"
                point.easing = "AUTO"


def key_body_projection(body: bpy.types.Object, frame: int, *, u: float, delta: float = 0.0) -> None:
    keys = body.data.shape_keys.key_blocks
    keys["UProjection"].value = u
    keys["UProjection"].keyframe_insert(data_path="value", frame=frame)
    keys["DeltaProjection"].value = delta
    keys["DeltaProjection"].keyframe_insert(data_path="value", frame=frame)


def key_object_scale(obj: bpy.types.Object, frame: int, factor: float) -> None:
    key_transform(obj, frame, scale=(factor, factor, factor))


def animate_complete_identity(
    scene: bpy.types.Scene,
    camera: bpy.types.Object,
    rig: GlyphRig,
    body_root: bpy.types.Object,
    body: bpy.types.Object,
    eyes: list[bpy.types.Object],
    pupils: list[bpy.types.Object],
    hangul_root: bpy.types.Object,
    hangul_vertical: bpy.types.Object,
    derivation_root: bpy.types.Object,
    bidi: dict[str, bpy.types.Object],
    delta_strokes: list[bpy.types.Object],
    code_root: bpy.types.Object,
    code_u: bpy.types.Object,
    cursor: bpy.types.Object,
    u_shear_keys: list[bpy.types.ShapeKey],
) -> None:
    """Animate one causal identity system rather than a montage of finished marks."""

    scene.frame_start = 1
    scene.frame_end = MASTER_END
    scene["semantic_timeline"] = json.dumps(FRAMES, sort_keys=True)
    scene["derivation_law"] = "MöBI𝒰s -> BI + reflected DI -> BIDI + three source strokes -> BIDIΔ"
    scene["static_wordmark_frame"] = GOLDEN_FRAMES["wordmark-generated"]
    scene["static_wordmark_body"] = "collapsed MobiusBody plus literal animated eyes in Glyph_O slot"
    scene["square_circle_reading"] = "phase ground plus operator upright bounds the conserved circular aperture"
    scene["wordmark_counter_law"] = "open-M V; filled-B base with one enlarged counter; continuous-S double curve with open counters"
    scene["wordmark_generation_law"] = "U projects from the conserved aperture; S unfurls from the U return terminal"
    scene["us_coupling_law"] = "single generated U-to-S terminal event with distinct open counters"

    # The portable SVG O and its dots are registration guides only. The actual
    # rendered wordmark uses the connected Möbius body and literal 3D eyes.
    for role in ("O", "Eye2D_Left", "Eye2D_Right"):
        for frame in (1, MASTER_END):
            key_visibility(rig, role, frame, False)

    word_roles = ("M", "B", "I", "U", "S", "Ground_Left", "Ground_Right")
    for role in word_roles:
        key_visibility(rig, role, 1, False)
        key_visibility(rig, role, 35, False)

    # 01 — Presence: the embodied closed loop looks through the mark.
    key_transform(body_root, 1, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, 0.0))
    key_transform(body_root, 24, location=(0.0, 0.0, 0.0), scale=(1.015, 1.015, 1.015), rotation=(0.0, 0.0, PHI))
    key_transform(body_root, 35, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, PHI))
    key_body_projection(body, 1, u=0.0)
    key_body_projection(body, 35, u=0.0)
    body.rotation_euler = (0.0, 0.0, 0.0)
    body.keyframe_insert(data_path="rotation_euler", frame=1)
    body.keyframe_insert(data_path="rotation_euler", frame=35)

    # Eyes are literal, paired, and persistent during every body projection.
    eye_closed = [Vector((-0.34, 2.37, 0.42)), Vector((0.34, 2.37, 0.42))]
    eye_open = [Vector((-0.34, 2.18, 0.42)), Vector((0.34, 2.18, 0.42))]
    for index, eye in enumerate(eyes):
        for frame, location in ((1, eye_closed[index]), (35, eye_closed[index]), (456, eye_open[index]), (468, eye_open[index]), (576, eye_closed[index]), (600, eye_closed[index])):
            key_transform(eye, frame, location=location)
        for frame, sy in ((9, 0.17), (11, 0.025), (13, 0.17), (448, 0.17), (450, 0.025), (452, 0.17)):
            eye.scale.y = sy
            eye.keyframe_insert(data_path="scale", index=1, frame=frame)
    for index, pupil in enumerate(pupils):
        origin = pupil.location.copy()
        for frame, dx in ((1, 0.0), (24, 0.022 if index == 0 else -0.022), (35, 0.0), (456, 0.018), (600, 0.0)):
            pupil.location = origin + Vector((dx, 0.0, 0.0))
            pupil.keyframe_insert(data_path="location", frame=frame)

    # 02 — The organism collapses into the ö slot while the rest of its name
    # precipitates around it. This exact frame is also the static artifact.
    o_location = rig.base_locations["O"]
    o_scale = 0.355
    key_transform(body_root, 36, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, PHI))
    key_transform(body_root, 72, location=o_location, scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))
    key_transform(body_root, 108, location=o_location, scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))

    arrivals = {"M": 46, "B": 54, "I": 61, "Ground_Left": 80, "Ground_Right": 80}
    for role, arrival in arrivals.items():
        key_visibility(rig, role, arrival - 8, False)
        key_transform(
            rig.glyphs[role],
            arrival - 8,
            location=rig.base_locations[role] + Vector((0.0, -0.16, 0.0)),
        )
        key_transform(
            rig.glyphs[role],
            arrival,
            location=rig.base_locations[role],
            scale=rig.base_scales[role],
            rotation=rig.base_rotations[role],
        )
        key_transform(
            rig.glyphs[role],
            108,
            location=rig.base_locations[role],
            scale=rig.base_scales[role],
            rotation=rig.base_rotations[role],
        )

    # U is not another letter flown up from below. It projects out of the
    # conserved aperture while the embodied loop seats in the ö slot. The
    # move therefore reads as one body changing register: closed observer to
    # typographic operator.
    u_start = o_location + Vector((0.18, -0.02, 0.0))
    u_mid = u_start.lerp(rig.base_locations["U"], 0.58) + Vector((0.0, 0.10, 0.0))
    key_transform(rig.glyphs["U"], 54, location=u_start, scale=hidden_scale(rig.base_scales["U"]))
    key_transform(
        rig.glyphs["U"],
        63,
        location=u_mid,
        scale=Vector(tuple(value * 0.58 for value in rig.base_scales["U"])),
        rotation=rig.base_rotations["U"],
    )
    key_transform(
        rig.glyphs["U"],
        71,
        location=rig.base_locations["U"],
        scale=rig.base_scales["U"],
        rotation=rig.base_rotations["U"],
    )
    key_transform(
        rig.glyphs["U"],
        108,
        location=rig.base_locations["U"],
        scale=rig.base_scales["U"],
        rotation=rig.base_rotations["U"],
    )

    # S is the outgoing relational state of that same event. It begins at the
    # U terminal as a compressed cyan return, opens its two counters, then
    # settles into the near-tangent static lockup. This makes “us” causal in
    # motion while keeping both glyphs individually legible at rest.
    s_source = rig.base_locations["S"] + Vector((-0.035, -0.025, 0.0))
    s_mid = rig.base_locations["S"] + Vector((-0.012, 0.012, 0.0))
    key_transform(
        rig.glyphs["S"],
        67,
        location=s_source,
        scale=hidden_scale(rig.base_scales["S"]),
        rotation=rig.base_rotations["S"],
    )
    key_transform(
        rig.glyphs["S"],
        68,
        location=s_source,
        scale=(rig.base_scales["S"].x * 0.025, rig.base_scales["S"].y * 0.08, rig.base_scales["S"].z),
        rotation=rig.base_rotations["S"],
    )
    key_transform(
        rig.glyphs["S"],
        75,
        location=s_mid,
        scale=(rig.base_scales["S"].x * 0.48, rig.base_scales["S"].y * 0.88, rig.base_scales["S"].z),
        rotation=rig.base_rotations["S"],
    )
    key_transform(
        rig.glyphs["S"],
        82,
        location=rig.base_locations["S"],
        scale=rig.base_scales["S"],
        rotation=rig.base_rotations["S"],
    )
    key_transform(
        rig.glyphs["S"],
        108,
        location=rig.base_locations["S"],
        scale=rig.base_scales["S"],
        rotation=rig.base_rotations["S"],
    )
    key_shape(u_shear_keys, 1, 0.0)
    key_shape(u_shear_keys, 108, 0.0)

    # 03 — i𝒰s becomes one relational event: approach, crossover, damped return.
    group_shift = Vector((-2.15, 0.0, 0.0))
    for role in ("M", "B"):
        key_visibility(rig, role, 109, True)
        key_visibility(rig, role, 120, False)
    key_transform(body_root, 109, location=o_location, scale=(o_scale, o_scale, o_scale))
    key_transform(body_root, 120, location=o_location, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    for role in ("I", "U", "S", "Ground_Left", "Ground_Right"):
        destination = rig.base_locations[role] + group_shift
        key_transform(rig.glyphs[role], 109, location=rig.base_locations[role], scale=rig.base_scales[role], rotation=rig.base_rotations[role])
        key_transform(rig.glyphs[role], 124, location=destination, scale=rig.base_scales[role], rotation=rig.base_rotations[role])
        key_transform(rig.glyphs[role], 156, location=destination, scale=rig.base_scales[role], rotation=rig.base_rotations[role])
    # A small extra lean and lift makes the already-baked phi bias active.
    ius_i = rig.base_locations["I"] + group_shift + Vector((-0.08, 0.13, 0.0))
    key_transform(rig.glyphs["I"], 132, location=ius_i, rotation=(0.0, 0.0, -PHI * 0.35))
    key_transform(rig.glyphs["I"], 156, location=ius_i, rotation=(0.0, 0.0, -PHI * 0.35))
    key_shape(u_shear_keys, 109, 0.0)
    key_shape(u_shear_keys, 132, 1.0)
    key_shape(u_shear_keys, 156, 1.0)

    # 04 — one-turn i𝒰 folds into orientation-restored UI.
    ui_u = Vector((-0.82, 0.02, 0.0))
    ui_i = Vector((0.92, 0.18, 0.0))
    key_transform(rig.glyphs["S"], 157, scale=rig.base_scales["S"])
    key_transform(rig.glyphs["S"], 168, scale=hidden_scale(rig.base_scales["S"]))
    key_transform(rig.glyphs["U"], 157, location=rig.base_locations["U"] + group_shift, rotation=(0.0, 0.0, 0.0))
    key_transform(rig.glyphs["U"], 180, location=ui_u, rotation=(0.0, math.pi, 0.0))
    key_transform(rig.glyphs["U"], 192, location=ui_u, rotation=(0.0, math.tau, 0.0))
    key_transform(rig.glyphs["I"], 157, location=ius_i, rotation=(0.0, 0.0, -PHI * 0.35))
    key_transform(rig.glyphs["I"], 180, location=ui_i, rotation=(0.0, math.pi, 0.0))
    key_transform(rig.glyphs["I"], 192, location=ui_i, rotation=(0.0, math.tau, 0.0))
    key_shape(u_shear_keys, 157, 1.0)
    key_shape(u_shear_keys, 180, 0.0)
    key_shape(u_shear_keys, 192, 0.0)
    for role in ("Ground_Left", "Ground_Right"):
        key_transform(rig.glyphs[role], 157, location=rig.base_locations[role] + group_shift, scale=rig.base_scales[role])
    key_visibility(rig, "Ground_Left", 192, False)

    # 05 — restored UI structurally reflows into 의. No Korean glyph is pasted:
    # ㅇ is the conserved ribbon, ㅡ is the phase ground, ㅣ is derived from i.
    key_transform(rig.glyphs["U"], 193, location=ui_u, scale=rig.base_scales["U"], rotation=(0.0, math.tau, 0.0))
    key_transform(rig.glyphs["I"], 193, location=ui_i, scale=rig.base_scales["I"], rotation=(0.0, math.tau, 0.0))
    key_transform(rig.glyphs["U"], 205, scale=hidden_scale(rig.base_scales["U"]))
    key_transform(rig.glyphs["I"], 205, scale=hidden_scale(rig.base_scales["I"]))
    hangul_circle = Vector((-0.42, 0.34, 0.0))
    hangul_circle_scale = 0.30
    key_transform(body_root, 193, location=hangul_circle, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR), rotation=(0.0, 0.0, 0.0))
    key_transform(body_root, 216, location=hangul_circle, scale=(hangul_circle_scale, hangul_circle_scale, hangul_circle_scale), rotation=(0.0, 0.0, 0.0))
    key_transform(body_root, 228, location=hangul_circle, scale=(hangul_circle_scale, hangul_circle_scale, hangul_circle_scale), rotation=(0.0, 0.0, 0.0))
    key_body_projection(body, 193, u=0.0)
    key_body_projection(body, 228, u=0.0)
    key_transform(rig.glyphs["Ground_Right"], 193, location=rig.base_locations["Ground_Right"] + group_shift, scale=rig.base_scales["Ground_Right"], rotation=(0.0, 0.0, -PHI))
    # Cancel the phase tilt baked into the source ground so the restored ㅡ is
    # truly horizontal and optically centered under ㅇ.
    key_transform(rig.glyphs["Ground_Right"], 216, location=(-0.42, -0.48, 0.0), scale=(0.44, 0.44, 0.44), rotation=(0.0, 0.0, -PHI))
    key_transform(rig.glyphs["Ground_Right"], 228, location=(-0.42, -0.48, 0.0), scale=(0.44, 0.44, 0.44), rotation=(0.0, 0.0, -PHI))
    key_transform(hangul_root, 1, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(hangul_root, 204, location=(0.18, -0.07, 0.0), scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(hangul_root, 216, location=(0.18, -0.07, 0.0), scale=(0.72, 0.72, 0.72))
    key_transform(hangul_root, 228, location=(0.18, -0.07, 0.0), scale=(0.72, 0.72, 0.72))

    # 06 — return to the exact embodied wordmark state.
    key_transform(hangul_root, 229, location=(0.18, -0.07, 0.0), scale=(0.72, 0.72, 0.72))
    key_transform(hangul_root, 242, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(body_root, 229, location=hangul_circle, scale=(hangul_circle_scale, hangul_circle_scale, hangul_circle_scale))
    key_transform(body_root, 264, location=o_location, scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))
    key_transform(body_root, 276, location=o_location, scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))
    for role in word_roles:
        key_transform(rig.glyphs[role], 229, scale=hidden_scale(rig.base_scales[role]))
        key_transform(rig.glyphs[role], 264, location=rig.base_locations[role], scale=rig.base_scales[role], rotation=rig.base_rotations[role])
        key_transform(rig.glyphs[role], 276, location=rig.base_locations[role], scale=rig.base_scales[role], rotation=rig.base_rotations[role])

    # 07 — the word lifts but remains whole. Its internal BI duplicates itself;
    # the duplicate b reflects through zero into d while i remains invariant.
    key_transform(rig.root, 1, location=(0.0, 0.0, 0.0))
    key_transform(rig.root, 276, location=(0.0, 0.0, 0.0))
    key_transform(rig.root, 300, location=(0.0, 1.72, 0.0))
    key_transform(rig.root, 420, location=(0.0, 1.72, 0.0))
    key_transform(body_root, 277, location=o_location, scale=(o_scale, o_scale, o_scale))
    key_transform(body_root, 300, location=o_location + Vector((0.0, 1.72, 0.0)), scale=(o_scale, o_scale, o_scale))
    key_transform(body_root, 420, location=o_location + Vector((0.0, 1.72, 0.0)), scale=(o_scale, o_scale, o_scale))

    target_positions = {
        "BIDI_B_Source": Vector((-2.65, -0.48, 0.12)),
        "BIDI_I_Source": Vector((-1.28, -0.48, 0.12)),
        "BIDI_D_Reflected": Vector((0.05, -0.48, 0.12)),
        "BIDI_I_Reflected": Vector((1.42, -0.48, 0.12)),
    }
    source_world = {
        "B": rig.base_locations["B"] + Vector((0.0, 1.72, 0.0)),
        "I": rig.base_locations["I"] + Vector((0.0, 1.72, 0.0)),
    }

    # The live BI leaves a real vacancy in Möbi𝒰s.  Source descendants inherit
    # its exact positions before moving outward, so extraction reads as a
    # self-projection rather than four unrelated letters entering the frame.
    for role in ("B", "I"):
        key_transform(rig.glyphs[role], 286, location=rig.base_locations[role], scale=rig.base_scales[role])
        key_transform(rig.glyphs[role], 298, location=rig.base_locations[role], scale=rig.base_scales[role])
        key_transform(rig.glyphs[role], 310, location=rig.base_locations[role] + Vector((0.0, -0.10, 0.0)), scale=hidden_scale(rig.base_scales[role]))
        key_transform(rig.glyphs[role], 408, scale=hidden_scale(rig.base_scales[role]))
        key_transform(rig.glyphs[role], 412, scale=hidden_scale(rig.base_scales[role], 0.12))
        key_transform(rig.glyphs[role], 416, scale=Vector(tuple(value * 0.68 for value in rig.base_scales[role])))
        key_transform(rig.glyphs[role], 420, location=rig.base_locations[role], scale=rig.base_scales[role], rotation=rig.base_rotations[role])

    source_pair = {
        "BIDI_B_Source": "B",
        "BIDI_I_Source": "I",
    }
    for name, source_role in source_pair.items():
        clone = bidi[name]
        key_transform(clone, 1, location=source_world[source_role], scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
        key_transform(clone, 288, location=source_world[source_role], scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
        key_transform(clone, 298, location=source_world[source_role], scale=(0.64, 0.64, 0.64))
        midpoint = source_world[source_role].lerp(target_positions[name], 0.55) + Vector((0.0, 0.18, 0.0))
        key_transform(clone, 312, location=midpoint, scale=(0.68, 0.68, 0.68))
        key_transform(clone, 324, location=target_positions[name], scale=(0.64, 0.64, 0.64))
        key_transform(clone, 420, location=target_positions[name], scale=(0.64, 0.64, 0.64))

    # A second BI is projected out of the settled source pair.  Its B crosses
    # the zero-width inversion plane and crystallizes as D; its i is the phase
    # invariant that simply translates into the second position.
    reflected = bidi["BIDI_D_Reflected"]
    reflected_i = bidi["BIDI_I_Reflected"]
    key_transform(reflected, 1, location=target_positions["BIDI_B_Source"], scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(reflected_i, 1, location=target_positions["BIDI_I_Source"], scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(reflected, 324, location=target_positions["BIDI_B_Source"], scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(reflected_i, 324, location=target_positions["BIDI_I_Source"], scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(reflected, 330, location=target_positions["BIDI_B_Source"], scale=(0.64, 0.64, 0.64))
    key_transform(reflected_i, 330, location=target_positions["BIDI_I_Source"], scale=(0.64, 0.64, 0.64))
    key_transform(reflected, 340, location=(-0.58, -0.30, 0.16), scale=(0.18, 0.68, 0.68))
    key_transform(reflected_i, 344, location=(0.62, -0.30, 0.16), scale=(0.70, 0.70, 0.70))
    key_transform(reflected, 344, location=(-0.24, -0.40, 0.14), scale=(HIDDEN_FACTOR, 0.68, 0.68))
    key_transform(reflected, 352, location=target_positions["BIDI_D_Reflected"], scale=(-0.72, 0.72, 0.72))
    key_transform(reflected_i, 352, location=target_positions["BIDI_I_Reflected"], scale=(0.70, 0.70, 0.70))
    key_transform(reflected, 360, location=target_positions["BIDI_D_Reflected"], scale=(-0.64, 0.64, 0.64))
    key_transform(reflected_i, 360, location=target_positions["BIDI_I_Reflected"], scale=(0.64, 0.64, 0.64))
    key_transform(reflected, 420, location=target_positions["BIDI_D_Reflected"], scale=(-0.64, 0.64, 0.64))
    key_transform(reflected_i, 420, location=target_positions["BIDI_I_Reflected"], scale=(0.64, 0.64, 0.64))

    # 08 — three type-native strokes detach from M, i, and the phase ground.
    # The M-derived edge is not allowed to materialize beside the letter. It
    # first seats exactly on the M's open right-inner diagonal, brightens as a
    # structural seam, then peels away and grows into the first Delta edge.
    # This makes the third lineage as literal as the i and phase-ground edges.
    source_stroke_states = [
        (Vector((rig.base_locations["M"].x + 0.38, 1.92, 0.24)), math.radians(60.0)),
        (Vector((rig.base_locations["I"].x, 2.08, 0.26)), math.pi / 2.0),
        (Vector((rig.base_locations["Ground_Right"].x, 0.78, 0.28)), -PHI),
    ]
    delta_targets = [
        (Vector((3.38, -0.42, 0.24)), math.radians(60.0)),
        (Vector((4.52, -0.42, 0.26)), math.radians(-60.0)),
        (Vector((3.95, -1.41, 0.28)), 0.0),
    ]
    m_stroke = delta_strokes[0]
    m_source, m_source_angle = source_stroke_states[0]
    m_target, m_target_angle = delta_targets[0]
    key_transform(m_stroke, 1, location=m_source, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR), rotation=(0.0, 0.0, m_source_angle))
    key_transform(m_stroke, 360, location=m_source, scale=(HIDDEN_FACTOR, 1.0, 1.0), rotation=(0.0, 0.0, m_source_angle))
    key_transform(m_stroke, 364, location=m_source, scale=(0.08, 1.0, 1.0), rotation=(0.0, 0.0, m_source_angle))
    key_transform(m_stroke, 372, location=m_source, scale=(0.48, 1.0, 1.0), rotation=(0.0, 0.0, m_source_angle))
    peel_midpoint = m_source.lerp(m_target, 0.42) + Vector((0.0, 0.16, 0.0))
    key_transform(m_stroke, 386, location=peel_midpoint, scale=(0.72, 1.0, 1.0), rotation=(0.0, 0.0, m_source_angle))
    key_transform(m_stroke, 408, location=m_target, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, m_target_angle))
    key_transform(m_stroke, 420, location=m_target, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, m_target_angle))

    for stroke, (source_location, source_angle), (target_location, target_angle) in zip(delta_strokes[1:], source_stroke_states[1:], delta_targets[1:]):
        key_transform(stroke, 1, location=source_location, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR), rotation=(0.0, 0.0, source_angle))
        key_transform(stroke, 368, location=source_location, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR), rotation=(0.0, 0.0, source_angle))
        key_transform(stroke, 376, location=source_location, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, source_angle))
        key_transform(stroke, 408, location=target_location, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, target_angle))
        key_transform(stroke, 420, location=target_location, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, target_angle))

    # 09 — standalone operative U is the same connected body opened, eyes intact.
    key_transform(rig.root, 421, location=(0.0, 1.72, 0.0), scale=(1.0, 1.0, 1.0))
    key_transform(rig.root, 434, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(derivation_root, 420, scale=(1.0, 1.0, 1.0))
    key_transform(derivation_root, 434, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(body_root, 421, location=o_location + Vector((0.0, 1.72, 0.0)), scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))
    key_transform(body_root, 442, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, 0.0))
    key_transform(body_root, 468, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, 0.0))
    key_body_projection(body, 420, u=0.0)
    key_body_projection(body, 442, u=0.0)
    key_body_projection(body, 456, u=1.0)
    key_body_projection(body, 468, u=1.0)

    # 10 — reduced typographic body resolves into the complete executable 𝒰_.
    key_transform(body_root, 469, scale=(1.0, 1.0, 1.0))
    key_transform(body_root, 482, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(code_root, 1, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(code_root, 469, location=(0.0, 0.0, 0.0), scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(code_root, 482, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0))
    key_transform(code_root, 516, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0))
    key_transform(code_u, 469, location=(0.0, 0.14, 0.0), scale=(1.18, 1.18, 1.18))
    key_transform(code_u, 516, location=(0.0, 0.14, 0.0), scale=(1.18, 1.18, 1.18))
    for frame, factor in ((469, HIDDEN_FACTOR), (486, HIDDEN_FACTOR), (492, 1.0), (498, HIDDEN_FACTOR), (504, 1.0), (510, HIDDEN_FACTOR), (516, 1.0)):
        key_transform(cursor, frame, location=(1.06, -1.18, 0.26), scale=(factor, factor, factor), rotation=(0.0, 0.0, PHI if frame >= 504 else 0.0))

    # 11 — family lockup: parent word above, extracted BIDIΔ below, canonical
    # Universal Operator sigil at right.
    key_transform(code_root, 517, location=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0))
    # Resolve the Universal Operator sigil inside Delta without sacrificing its
    # live horizon. The
    # smaller, centered lockup lifts the underscore clear of the Delta base;
    # its right-biased overhang remains visible but no longer collides with the
    # triangle's right edge.
    key_transform(code_root, 540, location=(4.02, -0.48, 0.0), scale=(0.48, 0.48, 0.48))
    key_transform(code_root, 600, location=(4.02, -0.48, 0.0), scale=(0.48, 0.48, 0.48))
    key_transform(rig.root, 517, location=(0.0, 1.72, 0.0), scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(rig.root, 540, location=(0.0, 1.72, 0.0), scale=(1.0, 1.0, 1.0))
    key_transform(rig.root, 600, location=(0.0, 1.72, 0.0), scale=(1.0, 1.0, 1.0))
    key_transform(derivation_root, 517, scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(derivation_root, 540, scale=(1.0, 1.0, 1.0))
    key_transform(derivation_root, 600, scale=(1.0, 1.0, 1.0))
    key_transform(body_root, 517, location=(0.0, 0.0, 0.0), scale=(HIDDEN_FACTOR, HIDDEN_FACTOR, HIDDEN_FACTOR))
    key_transform(body_root, 540, location=o_location + Vector((0.0, 1.72, 0.0)), scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))
    key_transform(body_root, 600, location=o_location + Vector((0.0, 1.72, 0.0)), scale=(o_scale, o_scale, o_scale), rotation=(0.0, 0.0, PHI))
    key_body_projection(body, 517, u=1.0)
    key_body_projection(body, 540, u=0.0)
    key_body_projection(body, 600, u=0.0)

    # Camera choreography is deliberately orthographic; typography is never
    # distorted merely to make the 3D state feel dramatic.
    for frame, x, y, scale in (
        (1, 0.0, 0.0, 6.3),
        (35, 0.0, 0.0, 6.3),
        (84, 0.0, 0.0, 10.5),
        (108, 0.0, 0.0, 10.5),
        (132, 0.0, 0.0, 6.2),
        (156, 0.0, 0.0, 6.2),
        (180, 0.0, 0.0, 5.8),
        (216, 0.0, 0.0, 4.8),
        (276, 0.0, 0.0, 10.5),
        (340, 0.35, 0.45, 11.8),
        (408, 0.35, 0.20, 12.2),
        (420, 0.35, 0.20, 12.2),
        (456, 0.0, 0.0, 6.3),
        (468, 0.0, 0.0, 6.3),
        (500, 0.0, 0.0, 5.8),
        (516, 0.0, 0.0, 5.8),
        (576, 0.4, 0.30, 13.1),
        (600, 0.4, 0.30, 13.1),
    ):
        key_camera(camera, frame, x=x, y=y, scale=scale)

    animated_owners: list[object] = [
        rig.root,
        body_root,
        body,
        body.data.shape_keys,
        hangul_root,
        hangul_vertical,
        derivation_root,
        code_root,
        code_u,
        cursor,
        camera,
        camera.data,
        *rig.glyphs.values(),
        *eyes,
        *pupils,
        *bidi.values(),
        *delta_strokes,
        *u_shear_keys,
    ]
    normalize_interpolation(animated_owners)


def build_scene() -> dict[str, object]:
    base.ensure_dirs()
    base.reset_scene()
    camera, materials = base.setup_scene()
    materials = make_materials(materials)
    scene = bpy.context.scene
    scene.frame_end = MASTER_END
    scene.render.resolution_x = 960
    scene.render.resolution_y = 960
    scene["identity_version"] = "glyph-derived-v2"
    scene["conserved_aperture"] = True
    scene["literal_eyes"] = True
    scene["finished_asset_substitution_allowed"] = False

    body_collection = base.collection("EMBODIED_MOBIUS")
    glyph_collection = base.collection("GLYPH_RIG")
    derived_collection = base.collection("DERIVED_STATES")
    witness_collection = base.collection("STATIC_EXPORT_WITNESSES")

    body_root = bpy.data.objects.new("EmbodiedMobiusRig", None)
    body_collection.objects.link(body_root)
    body_root["identity_role"] = "embodied-conserved-aperture-rig"
    body_root["static_wordmark_role"] = "literal-collapsed-o-with-eyes"
    body = base.create_master_body(materials, body_collection)
    body.parent = body_root
    body.location = (0.0, 0.0, 0.0)
    left_eye, left_pupil = base.create_eye("Eye_Left", (-0.34, 2.37, 0.42), materials, body_collection)
    right_eye, right_pupil = base.create_eye("Eye_Right", (0.34, 2.37, 0.42), materials, body_collection)
    for eye in (left_eye, right_eye):
        eye.parent = body_root
    eyes = [left_eye, right_eye]
    pupils = [left_pupil, right_pupil]

    rig = import_wordmark_rig(materials, glyph_collection)
    u_shear_keys = add_u_shear_keys(rig)

    hangul_root = bpy.data.objects.new("HangulRestorationRig", None)
    derived_collection.objects.link(hangul_root)
    hangul_root["identity_role"] = "structural-hangul-restoration"
    hangul_root["assembly"] = "MobiusBody(ㅇ)+phase-ground(ㅡ)+i-stem(ㅣ)"
    hangul_vertical = create_hangul_bar(materials, derived_collection)
    hangul_vertical.parent = hangul_root

    derivation_root = bpy.data.objects.new("BIDIDeltaDerivationRig", None)
    derived_collection.objects.link(derivation_root)
    derivation_root["identity_role"] = "literal-self-extraction-rig"
    derivation_root["law"] = "BI duplicates; duplicate B reflects into D; three type-native strokes close Delta"
    bidi = {
        "BIDI_B_Source": clone_glyph(rig, "B", "BIDI_B_Source", materials["bidi_left"], derived_collection, derivation_root),
        "BIDI_I_Source": clone_glyph(rig, "I", "BIDI_I_Source", materials["bidi_left"], derived_collection, derivation_root),
        "BIDI_D_Reflected": clone_glyph(rig, "B", "BIDI_D_Reflected", materials["bidi_right"], derived_collection, derivation_root),
        "BIDI_I_Reflected": clone_glyph(rig, "I", "BIDI_I_Reflected", materials["bidi_right"], derived_collection, derivation_root),
    }
    bidi["BIDI_D_Reflected"]["transformation"] = "literal-x-reflection-through-zero"
    bidi["BIDI_I_Reflected"]["transformation"] = "duplicate-invariant-i"
    delta_strokes = create_delta_strokes(materials, derived_collection)
    for stroke in delta_strokes:
        stroke.parent = derivation_root

    code_root = bpy.data.objects.new("CodeSigilRig", None)
    derived_collection.objects.link(code_root)
    code_root["identity_role"] = "standalone-operator-and-terminal-sigil"
    code_u = clone_glyph(rig, "U", "OperatorU_Standalone", materials["white"], derived_collection, code_root)
    for child in code_u.children:
        replace_material(child, materials["u_blue"] if "Keyline" in child.name else materials["white"])
    cursor = create_code_cursor(materials, derived_collection)
    cursor.parent = code_root

    animate_complete_identity(
        scene,
        camera,
        rig,
        body_root,
        body,
        eyes,
        pupils,
        hangul_root,
        hangul_vertical,
        derivation_root,
        bidi,
        delta_strokes,
        code_root,
        code_u,
        cursor,
        u_shear_keys,
    )

    # Static, topology-preserving web components are baked from the same body.
    baked_body = base.bake_body(body, "MobiusBody_StaticClosed", 0.0, 0.0, witness_collection)
    baked_u = base.bake_body(body, "MobiusBody_StaticU", 1.0, 0.0, witness_collection)
    baked_delta = base.bake_body(body, "MobiusBody_SecondaryDeltaProjection", 0.0, 1.0, witness_collection)
    for obj in (baked_body, baked_u, baked_delta):
        obj.hide_render = True
        obj["master_visibility"] = "static-export-witness-only"
    baked_delta["identity_role"] = "secondary-topological-delta-projection"
    baked_delta["primary_bidi_delta_logo"] = False

    return {
        "scene": scene,
        "camera": camera,
        "materials": materials,
        "rig": rig,
        "body_root": body_root,
        "body": body,
        "eyes": eyes,
        "pupils": pupils,
        "hangul_root": hangul_root,
        "hangul_vertical": hangul_vertical,
        "derivation_root": derivation_root,
        "bidi": bidi,
        "delta_strokes": delta_strokes,
        "code_root": code_root,
        "code_u": code_u,
        "cursor": cursor,
        "baked_body": baked_body,
        "baked_u": baked_u,
        "baked_delta": baked_delta,
    }


def export_scene_assets(state: dict[str, object]) -> list[Path]:
    scene: bpy.types.Scene = state["scene"]  # type: ignore[assignment]
    rig: GlyphRig = state["rig"]  # type: ignore[assignment]
    body_root: bpy.types.Object = state["body_root"]  # type: ignore[assignment]
    derivation_root: bpy.types.Object = state["derivation_root"]  # type: ignore[assignment]
    hangul_root: bpy.types.Object = state["hangul_root"]  # type: ignore[assignment]
    code_root: bpy.types.Object = state["code_root"]  # type: ignore[assignment]
    cursor: bpy.types.Object = state["cursor"]  # type: ignore[assignment]
    baked_body: bpy.types.Object = state["baked_body"]  # type: ignore[assignment]
    baked_u: bpy.types.Object = state["baked_u"]  # type: ignore[assignment]

    exports: list[Path] = []

    def emit(name: str, objects: list[bpy.types.Object], *, animations: bool) -> None:
        path = OUTPUT / name
        base.export_selected(path, objects, animations=animations)
        exports.append(path)

    # Full animated system and independently usable animated projections.
    emit("mobius-identity-master.glb", [rig.root, body_root, derivation_root, hangul_root, code_root], animations=True)
    emit("mobius-wordmark-animated.glb", [rig.root, body_root], animations=True)
    emit("mobius-ius-phase-animated.glb", [rig.glyphs[role] for role in ("I", "U", "S", "Ground_Left", "Ground_Right")], animations=True)
    emit("mobius-ui-hangul-animated.glb", [body_root, rig.glyphs["I"], rig.glyphs["U"], rig.glyphs["Ground_Right"], hangul_root], animations=True)
    emit("mobius-bidi-delta-animated.glb", [derivation_root], animations=True)
    emit("mobius-u-animated.glb", [body_root], animations=True)
    emit("mobius-u-code-animated.glb", [code_root], animations=True)

    # Stable component names retained for downstream consumers.
    scene.frame_set(1)
    emit("mobius-body.glb", [baked_body], animations=False)
    emit("mobius-u-operator.glb", [baked_u], animations=False)
    scene.frame_set(GOLDEN_FRAMES["ius-phase"])
    emit("mobius-ius-relational.glb", [rig.glyphs[role] for role in ("I", "U", "S", "Ground_Left", "Ground_Right")], animations=False)
    scene.frame_set(GOLDEN_FRAMES["u-code"])
    emit("mobius-u-code-sigil.glb", [code_root, cursor], animations=False)
    scene.frame_set(GOLDEN_FRAMES["delta-closed"])
    emit("mobius-bidi-delta.glb", [derivation_root], animations=False)
    scene.frame_set(1)
    return exports


def render_static_wordmark(scene: bpy.types.Scene, camera: bpy.types.Object) -> Path:
    """Render the canonical still as a committed frame of the live organism."""
    target = RENDERS / "mobius-wordmark-static.png"
    scene.frame_set(GOLDEN_FRAMES["wordmark-generated"])
    scene.render.resolution_x = 1440
    scene.render.resolution_y = 420
    scene.render.resolution_percentage = 100
    camera.data.ortho_scale = 3.25
    scene.render.filepath = str(target)
    bpy.ops.render.render(write_still=True)
    scene.render.resolution_x = 960
    scene.render.resolution_y = 960
    scene.frame_set(1)
    log(f"rendered {target.relative_to(REPO)}")
    return target


def write_glyph_manifest(generated: list[Path]) -> Path:
    manifest_path = OUTPUT / "identity-manifest.json"
    motion_master = MOTION / "mobius-identity-master.mp4"
    records = []
    for path in sorted(set(generated)):
        # The native .blend is saved once more after manifest generation so it
        # can embed the final manifest-facing frame. Interchange artifacts are
        # checksum-addressed; the authoring file is validated structurally by
        # Blender and intentionally omitted from this immutable file ledger.
        if path.is_file() and path.suffix != ".blend":
            records.append({"path": str(path.relative_to(REPO)), "bytes": path.stat().st_size, "sha256": sha256(path)})
    if motion_master.is_file():
        records.append({"path": str(motion_master.relative_to(REPO)), "bytes": motion_master.stat().st_size, "sha256": sha256(motion_master)})
    manifest = {
        "schemaVersion": 2,
        "identity": "Möbi𝒰s",
        "formalKernel": "bidiγΔ",
        "universalOperator": {
            "canonicalSigil": "𝒰_",
            "reducedMathematicalBody": "𝒰",
            "indexedNotation": "𝒰_R",
            "law": "one operator at multiple resolution levels; underscore is the executable live horizon",
        },
        "status": "working-candidate-awaiting-final-pencil-refinement",
        "governingLaw": "One geometry. Many projections. Parent contains and generates kernel.",
        "phaseAngle": {"radians": PHI, "degrees": math.degrees(PHI)},
        "staticArtifact": {
            "frame": GOLDEN_FRAMES["wordmark-generated"],
            "path": "../renders/mobius-wordmark-static.png",
            "goldenFrame": "../renders/mobius-wordmark-generated.png",
            "construction": "connected collapsed MobiusBody plus literal eyes seated in the ö slot",
            "squareCircle": "the circular aperture meets the orthogonal phase-ground and operator upright",
        },
        "topology": {"surface": "mobius-band", "halfTwists": 1, "connectedComponents": 1, "eulerCharacteristic": 0, "boundaryComponents": 1},
        "components": {
            "embodiedBody": "mobius-body.glb",
            "operatorU": "mobius-u-operator.glb",
            "universalOperatorSigil": "mobius-u-code-sigil.glb",
            "relationalIUs": "mobius-ius-relational.glb",
            "codeSigil": "mobius-u-code-sigil.glb",
            "kernelBiDiDelta": "mobius-bidi-delta.glb",
            "animatedMaster": "mobius-identity-master.glb",
            "animatedWordmark": "mobius-wordmark-animated.glb",
            "animatedIUs": "mobius-ius-phase-animated.glb",
            "animatedUIHangul": "mobius-ui-hangul-animated.glb",
            "animatedBiDiDelta": "mobius-bidi-delta-animated.glb",
            "animatedOperatorU": "mobius-u-animated.glb",
            "animatedCodeSigil": "mobius-u-code-animated.glb",
        },
        "components2d": {
            "embodiedBody": "../mobius-embodied-mark.svg",
            "operatorU": "../mobius-u-operator.svg",
            "universalOperatorSigil": "../mobius-u-code-sigil.svg",
            "relationalIUs": "../mobius-ius-relational.svg",
            "codeSigil": "../mobius-u-code-sigil.svg",
            "codeSigilDark": "../mobius-u-code-sigil-dark.svg",
            "biSeed": "../mobius-bi-seed.svg",
            "bidiKernel": "../mobius-bidi-kernel.svg",
            "kernelBiDiDelta": "../mobius-bidi-delta.svg",
            "wordmarkDark": "../mobius-u-wordmark-dark.svg",
            "wordmarkLight": "../mobius-u-wordmark-light.svg",
        },
        "compatibilityAliases": {
            "codeSigil": "universalOperatorSigil",
            "CODE_SIGIL": "UNIVERSAL_OPERATOR_SIGIL",
            "operatorU": "reducedMathematicalBody",
        },
        "lineage": {
            "BIDI_B_Source": "Glyph_B",
            "BIDI_I_Source": "Glyph_I",
            "BIDI_D_Reflected": "Glyph_B reflected through scale.x = 0",
            "BIDI_I_Reflected": "Glyph_I duplicated invariantly",
            "DeltaStroke_A_From_M": "Glyph_M",
            "DeltaStroke_B_From_i": "Glyph_I",
            "DeltaStroke_C_From_Ground": "Glyph_Ground_Right",
            "HangulRestoration": "MobiusBody(ㅇ) + phase-ground(ㅡ) + i-stem(ㅣ)",
            "RelationalUS": "U projects from conserved aperture; S unfurls from the U return terminal",
        },
        "documentation": {
            "sourceOfTruth": "../../../docs/identity/MOBIUS_U_RECONCILED_SOURCE_OF_TRUTH.md",
            "workingHandoff": "../../../docs/identity/MOBIUS_IDENTITY_WORKING_HANDOFF_2026-07-22.md",
            "publicInstrument": "../../../docs/identity/ETELLIS_XYZ_IDENTITY_INTEGRATION_DRAFT.md",
            "visualRefinement": "../../../docs/identity/MOBIUS_U_VISUAL_REFINEMENT_WORKING_PAPER.md",
            "languageAndRepository": "../../../docs/identity/MOBIUS_U_LANGUAGE_AND_REPOSITORY_INTEGRATION.md",
            "internalFoldMap": "../../../docs/identity/MOBIUS_INTERNAL_SYSTEM_FOLD_MAP.md",
        },
        "provisionalKineticReveals": {
            "reverseSheet": "Möbi𝒰s -> s𝒰iböM; SUI is inspected as a separate Latin resonance",
            "terminalOm": "the final öM may resolve to OM only as an authored motion state after the eyes lift",
            "yiProgression": "一 -> 意 -> 易 -> 義 is an authored semantic progression, not an etymology",
        },
        "stateRanges": FRAMES,
        "clipRanges": {name: {"frames": list(bounds), "fps": FPS} for name, bounds in CLIPS.items()},
        "goldenFrames": {name: {"frame": frame, "path": f"../renders/mobius-{name}.png"} for name, frame in GOLDEN_FRAMES.items()},
        "motionMaster": "../motion/mobius-identity-master.mp4" if motion_master.is_file() else None,
        "files": sorted(records, key=lambda record: record["path"]),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return manifest_path


def main() -> None:
    state = build_scene()
    scene: bpy.types.Scene = state["scene"]  # type: ignore[assignment]
    master = OUTPUT / "mobius-identity-master.blend"
    scene.frame_set(1)
    bpy.ops.wm.save_as_mainfile(filepath=str(master))
    exports = export_scene_assets(state)
    base.render_frames(scene, GOLDEN_FRAMES)
    static_wordmark = render_static_wordmark(scene, state["camera"])  # type: ignore[arg-type]
    generated = [master, *exports, static_wordmark, *(RENDERS / f"mobius-{label}.png" for label in GOLDEN_FRAMES)]
    manifest = write_glyph_manifest(generated)
    bpy.ops.wm.save_as_mainfile(filepath=str(master))
    log(f"manifest {manifest.relative_to(REPO)}")
    log("build complete")


if __name__ == "__main__":
    main()
