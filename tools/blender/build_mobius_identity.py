"""Build the deterministic Möbi𝒰s master scene and production exports.

Run with:
    blender --background --factory-startup --python tools/blender/build_mobius_identity.py

The master body is one connected Möbius band in every shape-key state. The
visible ö, 𝒰, and Δ are camera-relative projections of the same topology.
"""

from __future__ import annotations

import hashlib
import json
import math
import os
from pathlib import Path

import bpy
from mathutils import Vector


REPO = Path(__file__).resolve().parents[2]
IDENTITY = REPO / "assets" / "identity"
OUTPUT = IDENTITY / "3d"
RENDERS = IDENTITY / "renders"
MOTION = IDENTITY / "motion"
REPORTS = REPO / "build" / "identity-blender"

PHI = 0.125
FPS = 24
SEGMENTS = 192
WIDTH_STEPS = 13
RIBBON_HALF_WIDTH = 0.34

COLORS = {
    "night": (0.007, 0.020, 0.039, 1.0),
    "cyan": (0.031, 0.753, 0.800, 1.0),
    "blue": (0.006, 0.305, 1.000, 1.0),
    "indigo": (0.037, 0.048, 0.450, 1.0),
    "white": (0.939, 0.965, 1.000, 1.0),
    "pupil": (0.004, 0.012, 0.028, 1.0),
}


def log(message: str) -> None:
    print(f"[mobius-identity] {message}")


def ensure_dirs() -> None:
    for path in (OUTPUT, RENDERS, MOTION, REPORTS):
        path.mkdir(parents=True, exist_ok=True)


def reset_scene() -> None:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,
    ):
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)


def collection(name: str) -> bpy.types.Collection:
    found = bpy.data.collections.get(name)
    if found:
        return found
    found = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(found)
    return found


def move_to_collection(obj: bpy.types.Object, target: bpy.types.Collection) -> None:
    for owner in list(obj.users_collection):
        owner.objects.unlink(obj)
    target.objects.link(obj)


def principled_material(
    name: str,
    color: tuple[float, float, float, float],
    *,
    metallic: float = 0.0,
    roughness: float = 0.35,
    emission: float = 0.0,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    material.diffuse_color = color
    node = material.node_tree.nodes.get("Principled BSDF")
    if node:
        if node.inputs.get("Base Color"):
            node.inputs["Base Color"].default_value = color
        if node.inputs.get("Metallic"):
            node.inputs["Metallic"].default_value = metallic
        if node.inputs.get("Roughness"):
            node.inputs["Roughness"].default_value = roughness
        emission_input = node.inputs.get("Emission Color") or node.inputs.get("Emission")
        if emission_input:
            emission_input.default_value = color
        strength_input = node.inputs.get("Emission Strength")
        if strength_input:
            strength_input.default_value = emission
    return material


def catmull_rom(points: list[Vector], t: float) -> Vector:
    count = len(points)
    progress = (t % (2 * math.pi)) / (2 * math.pi) * count
    index = int(math.floor(progress))
    u = progress - index
    p0 = points[(index - 1) % count]
    p1 = points[index % count]
    p2 = points[(index + 1) % count]
    p3 = points[(index + 2) % count]
    return 0.5 * (
        (2.0 * p1)
        + (-p0 + p2) * u
        + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * u * u
        + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * u * u * u
    )


def rest_centerline(t: float) -> Vector:
    angle = t + math.radians(135.0)
    return Vector((1.82 * math.cos(angle), 1.82 * math.sin(angle), 0.0))


U_POINTS = [
    Vector((-1.52, 1.42, 0.02)),
    Vector((-1.58, 0.38, 0.00)),
    Vector((-1.45, -1.02, 0.00)),
    Vector((-0.78, -1.72, 0.00)),
    Vector((0.00, -1.92, 0.00)),
    Vector((0.78, -1.72, 0.00)),
    Vector((1.45, -1.02, 0.00)),
    Vector((1.58, 0.38, 0.00)),
    Vector((1.52, 1.42, 0.02)),
    # The closed return follows the same U silhouette behind the visible sheet.
    # The topology never opens; only the camera-relative projection does.
    Vector((1.48, 1.38, -1.20)),
    Vector((1.53, 0.38, -1.30)),
    Vector((1.40, -0.98, -1.30)),
    Vector((0.74, -1.66, -1.30)),
    Vector((0.00, -1.86, -1.30)),
    Vector((-0.74, -1.66, -1.30)),
    Vector((-1.40, -0.98, -1.30)),
    Vector((-1.53, 0.38, -1.30)),
    Vector((-1.48, 1.38, -1.20)),
]


DELTA_POINTS = [
    Vector((0.00, 2.02, 0.00)),
    Vector((1.18, 0.04, 0.00)),
    Vector((1.84, -1.55, 0.00)),
    Vector((0.00, -1.55, 0.00)),
    Vector((-1.84, -1.55, 0.00)),
    Vector((-1.18, 0.04, 0.00)),
]


def u_centerline(t: float) -> Vector:
    return catmull_rom(U_POINTS, t)


def delta_centerline(t: float) -> Vector:
    point = catmull_rom(DELTA_POINTS, t)
    # The return segment recedes slightly, preserving a visible sheet change.
    point.z += -0.34 * max(0.0, math.sin(t - math.pi * 1.35))
    return point


def sample_centerline(fn) -> list[Vector]:
    return [fn(2.0 * math.pi * i / SEGMENTS) for i in range(SEGMENTS)]


def band_vertices(fn) -> list[Vector]:
    centers = sample_centerline(fn)
    vertices: list[Vector] = []
    for i, center in enumerate(centers):
        t = 2.0 * math.pi * i / SEGMENTS
        previous = centers[(i - 1) % SEGMENTS]
        following = centers[(i + 1) % SEGMENTS]
        tangent = (following - previous).normalized()
        reference = Vector((0.0, 0.0, 1.0))
        if abs(tangent.dot(reference)) > 0.94:
            reference = Vector((0.0, 1.0, 0.0))
        lateral = tangent.cross(reference).normalized()
        normal = lateral.cross(tangent).normalized()
        width_axis = math.cos(t / 2.0) * normal + math.sin(t / 2.0) * lateral
        for width_index in range(WIDTH_STEPS):
            fraction = width_index / (WIDTH_STEPS - 1)
            offset = (fraction * 2.0 - 1.0) * RIBBON_HALF_WIDTH
            vertices.append(center + width_axis * offset)
    return vertices


def band_faces() -> list[tuple[int, int, int, int]]:
    faces: list[tuple[int, int, int, int]] = []

    def index(segment: int, width: int) -> int:
        return segment * WIDTH_STEPS + width

    for segment in range(SEGMENTS):
        next_segment = (segment + 1) % SEGMENTS
        for width in range(WIDTH_STEPS - 1):
            a = index(segment, width)
            b = index(segment, width + 1)
            if segment == SEGMENTS - 1:
                # Reverse the cross-section at the seam: the topological witness.
                c = index(0, WIDTH_STEPS - 2 - width)
                d = index(0, WIDTH_STEPS - 1 - width)
            else:
                c = index(next_segment, width + 1)
                d = index(next_segment, width)
            faces.append((a, b, c, d))
    return faces


def create_master_body(materials: dict[str, bpy.types.Material], target: bpy.types.Collection) -> bpy.types.Object:
    basis = band_vertices(rest_centerline)
    mesh = bpy.data.meshes.new("MobiusBodyMesh")
    mesh.from_pydata([tuple(v) for v in basis], [], band_faces())
    mesh.update()
    body = bpy.data.objects.new("MobiusBody", mesh)
    target.objects.link(body)
    body.data.materials.append(materials["front"])
    body.data.materials.append(materials["reverse"])

    for polygon in body.data.polygons:
        longitudinal = polygon.index // (WIDTH_STEPS - 1)
        polygon.material_index = 0 if longitudinal < SEGMENTS // 2 else 1

    basis_key = body.shape_key_add(name="Basis")
    basis_key.interpolation = "KEY_LINEAR"
    for key_name, generator in (
        ("UProjection", u_centerline),
        ("DeltaProjection", delta_centerline),
    ):
        key = body.shape_key_add(name=key_name)
        for vertex, coordinate in zip(key.data, band_vertices(generator)):
            vertex.co = coordinate
        key.interpolation = "KEY_LINEAR"

    bevel = body.modifiers.new("IdentityEdge", "BEVEL")
    bevel.width = 0.035
    bevel.segments = 2
    body["identity_role"] = "conserved-mobius-body"
    body["topology"] = "mobius-band"
    body["half_twists"] = 1
    body["phase_angle_rad"] = PHI
    body["projection_states"] = "o-umlaut,U,Delta"
    return body


def create_eye(
    name: str,
    location: tuple[float, float, float],
    materials: dict[str, bpy.types.Material],
    target: bpy.types.Collection,
) -> tuple[bpy.types.Object, bpy.types.Object]:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, location=location)
    eye = bpy.context.object
    eye.name = name
    eye.scale = (0.17, 0.17, 0.075)
    eye.data.materials.append(materials["eye"])
    eye["identity_role"] = "literal-animated-eye"
    move_to_collection(eye, target)

    pupil_location = (location[0], location[1], location[2] + 0.071)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, location=pupil_location)
    pupil = bpy.context.object
    pupil.name = f"{name}Pupil"
    pupil.scale = (0.068, 0.068, 0.025)
    pupil.data.materials.append(materials["pupil"])
    pupil.parent = eye
    pupil.matrix_parent_inverse = eye.matrix_world.inverted()
    pupil["identity_role"] = "gaze-direction"
    move_to_collection(pupil, target)
    return eye, pupil


def create_bar(
    name: str,
    location: tuple[float, float, float],
    scale: tuple[float, float, float],
    material: bpy.types.Material,
    target: bpy.types.Collection,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location)
    bar = bpy.context.object
    bar.name = name
    bar.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bar.data.materials.append(material)
    bar["identity_role"] = "horizontal-invariant"
    move_to_collection(bar, target)
    return bar


def import_svg_group(
    path: Path,
    name: str,
    target_width: float,
    material: bpy.types.Material,
    target: bpy.types.Collection,
) -> bpy.types.Object:
    before = set(bpy.data.objects)
    bpy.ops.import_curve.svg(filepath=str(path))
    imported = [obj for obj in bpy.data.objects if obj not in before]
    if not imported:
        raise RuntimeError(f"SVG import produced no objects: {path}")
    parent = bpy.data.objects.new(name, None)
    target.objects.link(parent)
    for obj in imported:
        move_to_collection(obj, target)
        obj.parent = parent
        if obj.type == "CURVE":
            obj.data.dimensions = "2D"
            obj.data.extrude = 0.012
            obj.data.bevel_depth = 0.002
            obj.data.materials.clear()
            obj.data.materials.append(material)

    corners = []
    for obj in imported:
        corners.extend(obj.matrix_world @ Vector(corner) for corner in obj.bound_box)
    min_x = min(v.x for v in corners)
    max_x = max(v.x for v in corners)
    min_y = min(v.y for v in corners)
    max_y = max(v.y for v in corners)
    width = max_x - min_x
    height = max_y - min_y
    if width <= 0 or height <= 0:
        raise RuntimeError(f"SVG import has empty bounds: {path}")
    factor = target_width / width
    parent.scale = (factor, factor, factor)
    parent.location = (-factor * (min_x + max_x) / 2.0, -factor * (min_y + max_y) / 2.0, 0.0)
    parent["identity_role"] = name
    parent["source_svg"] = str(path.relative_to(REPO))
    parent["identity_base_scale"] = [factor, factor, factor]
    return parent


def set_keyframe(object_or_data, path: str, frame: int, value, index: int | None = None) -> None:
    if index is None:
        setattr(object_or_data, path, value)
        object_or_data.keyframe_insert(data_path=path, frame=frame)
    else:
        values = getattr(object_or_data, path)
        values[index] = value
        object_or_data.keyframe_insert(data_path=path, index=index, frame=frame)


def animate_scene(
    root: bpy.types.Object,
    body: bpy.types.Object,
    eyes: list[bpy.types.Object],
    pupils: list[bpy.types.Object],
    ground: bpy.types.Object,
    hangul_vertical: bpy.types.Object,
    kernel_word: bpy.types.Object,
    camera: bpy.types.Object,
) -> None:
    u_key = body.data.shape_keys.key_blocks["UProjection"]
    delta_key = body.data.shape_keys.key_blocks["DeltaProjection"]

    for frame, u_value, delta_value in (
        (1, 0.0, 0.0),
        (24, 0.0, 0.0),
        (72, 1.0, 0.0),
        (132, 1.0, 0.0),
        (168, 0.34, 0.0),
        (180, 0.0, 0.0),
        (192, 0.0, 0.0),
        (216, 0.0, 0.0),
        (228, 0.0, 0.0),
        (264, 0.0, 1.0),
        (300, 0.0, 1.0),
    ):
        u_key.value = u_value
        u_key.keyframe_insert(data_path="value", frame=frame)
        delta_key.value = delta_value
        delta_key.keyframe_insert(data_path="value", frame=frame)

    for frame, z_rotation in ((1, 0.0), (24, PHI), (72, PHI), (216, 0.0)):
        set_keyframe(root, "rotation_euler", frame, z_rotation, index=2)
    for frame, location, scale in (
        (1, (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
        (168, (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
        (180, (-0.62, 0.42, 0.0), (0.62, 0.62, 0.62)),
        (192, (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
        (300, (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)),
    ):
        root.location = location
        root.scale = scale
        root.keyframe_insert(data_path="location", frame=frame)
        root.keyframe_insert(data_path="scale", frame=frame)
    for frame, y_rotation in ((1, 0.0), (120, 0.0), (150, math.pi), (192, math.tau), (300, math.tau)):
        set_keyframe(body, "rotation_euler", frame, y_rotation, index=1)

    rest_positions = [(-0.34, 2.37, 0.42), (0.34, 2.37, 0.42)]
    open_positions = [(-0.34, 2.18, 0.42), (0.34, 2.18, 0.42)]
    delta_positions = [(-0.28, 2.35, 0.42), (0.28, 2.35, 0.42)]
    for index, eye in enumerate(eyes):
        for frame, location in (
            (1, rest_positions[index]),
            (24, rest_positions[index]),
            (72, open_positions[index]),
            (192, rest_positions[index]),
            (228, rest_positions[index]),
            (264, delta_positions[index]),
            (300, delta_positions[index]),
        ):
            eye.location = location
            eye.keyframe_insert(data_path="location", frame=frame)
        for frame, y_scale in ((10, 0.17), (12, 0.025), (14, 0.17), (204, 0.17), (206, 0.025), (208, 0.17)):
            eye.scale.y = y_scale
            eye.keyframe_insert(data_path="scale", index=1, frame=frame)

    for index, pupil in enumerate(pupils):
        base = pupil.location.copy()
        for frame, dx in ((1, 0.0), (24, 0.025 if index == 0 else -0.025), (72, 0.015), (216, 0.0), (300, 0.0)):
            pupil.location = base + Vector((dx, 0.0, 0.0))
            pupil.keyframe_insert(data_path="location", frame=frame)

    for frame, scale in (
        (1, (1.0, 1.0, 1.0)),
        (132, (1.0, 1.0, 1.0)),
        (150, (0.04, 1.0, 1.0)),
        (168, (1.0, 1.0, 1.0)),
        (216, (1.0, 1.0, 1.0)),
        (228, (1.0, 1.0, 1.0)),
        (252, (0.001, 0.001, 0.001)),
        (300, (0.001, 0.001, 0.001)),
    ):
        ground.scale = scale
        ground.keyframe_insert(data_path="scale", frame=frame)
    for frame, angle in ((1, PHI), (168, PHI), (180, 0.0), (192, PHI), (300, PHI)):
        set_keyframe(ground, "rotation_euler", frame, angle, index=2)

    for frame, scale in (
        (1, (0.001, 0.001, 0.001)),
        (168, (0.001, 0.001, 0.001)),
        (176, (1.0, 1.0, 1.0)),
        (184, (1.0, 1.0, 1.0)),
        (192, (0.001, 0.001, 0.001)),
        (300, (0.001, 0.001, 0.001)),
    ):
        hangul_vertical.scale = scale
        hangul_vertical.keyframe_insert(data_path="scale", frame=frame)

    base_scale = tuple(kernel_word.get("identity_base_scale", (1.0, 1.0, 1.0)))
    hidden_scale = tuple(value * 0.001 for value in base_scale)
    kernel_word.scale = hidden_scale
    kernel_word.keyframe_insert(data_path="scale", frame=1)
    kernel_word.keyframe_insert(data_path="scale", frame=228)
    kernel_word.scale = base_scale
    kernel_word.keyframe_insert(data_path="scale", frame=252)
    kernel_word.keyframe_insert(data_path="scale", frame=300)

    body.location.x = 0.0
    body.keyframe_insert(data_path="location", frame=228)
    body.location.x = 3.05
    body.keyframe_insert(data_path="location", frame=264)
    body.keyframe_insert(data_path="location", frame=300)
    for eye in eyes:
        eye.keyframe_insert(data_path="location", frame=228)
        eye.location.x += 3.05
        eye.keyframe_insert(data_path="location", frame=264)
        eye.keyframe_insert(data_path="location", frame=300)

    camera.data.ortho_scale = 6.3
    camera.data.keyframe_insert(data_path="ortho_scale", frame=1)
    camera.data.keyframe_insert(data_path="ortho_scale", frame=228)
    camera.data.ortho_scale = 10.2
    camera.data.keyframe_insert(data_path="ortho_scale", frame=264)
    camera.data.keyframe_insert(data_path="ortho_scale", frame=300)

    for owner in (body.data.shape_keys, root, body, ground, hangul_vertical, kernel_word, camera.data, *eyes, *pupils):
        animation = getattr(owner, "animation_data", None)
        action = animation.action if animation else None
        if not action:
            continue
        # Blender 5 layered actions no longer expose a direct fcurves list.
        # Their default interpolation is already Bezier; legacy actions retain
        # the explicit normalization below.
        for curve in getattr(action, "fcurves", ()): 
            for point in curve.keyframe_points:
                point.interpolation = "BEZIER"
                point.easing = "AUTO"


def aim_camera(camera: bpy.types.Object, target: Vector) -> None:
    direction = target - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def setup_scene() -> tuple[bpy.types.Object, dict[str, bpy.types.Material]]:
    scene = bpy.context.scene
    # Blender 5.2 exposes the next-generation engine under BLENDER_EEVEE.
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 960
    scene.render.resolution_y = 960
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = True
    scene.render.fps = FPS
    scene.frame_start = 1
    scene.frame_end = 300
    if scene.world is None:
        scene.world = bpy.data.worlds.new("MobiusIdentityWorld")
    scene.world.color = COLORS["night"][:3]
    try:
        scene.view_settings.look = "AgX - Medium High Contrast"
    except TypeError:
        pass
    scene["identity"] = "Möbi𝒰s"
    scene["formal_kernel"] = "bidiγΔ"
    scene["governing_law"] = "one geometry, many projections"
    scene["phase_angle_rad"] = PHI

    materials = {
        "front": principled_material("Ribbon_Cyan", COLORS["cyan"], metallic=0.24, roughness=0.27, emission=0.12),
        "reverse": principled_material("Ribbon_Indigo", COLORS["indigo"], metallic=0.32, roughness=0.31, emission=0.08),
        "eye": principled_material("Eyes_Cyan", COLORS["cyan"], roughness=0.19, emission=2.0),
        "pupil": principled_material("Pupils_Night", COLORS["pupil"], roughness=0.25),
        "white": principled_material("Operator_White", COLORS["white"], metallic=0.05, roughness=0.28, emission=0.08),
        "kernel": principled_material("Kernel_Indigo", COLORS["indigo"], metallic=0.0, roughness=0.62, emission=0.10),
    }

    bpy.ops.object.camera_add(location=(0.0, 0.0, 10.0))
    camera = bpy.context.object
    camera.name = "Camera_Identity_Ortho"
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 6.3
    aim_camera(camera, Vector((0.0, 0.0, 0.0)))
    move_to_collection(camera, collection("CAMERAS"))
    scene.camera = camera

    # The identity palette is carried by the materials.  These broad lights
    # reveal the shallow extrusion and sheet crossover without bleaching the
    # indigo/cyan polarity into a field of near-white diffuse highlights.
    for name, location, energy, size in (
        ("Key_Cyan", (-4.5, 4.0, 6.0), 260.0, 5.0),
        ("Fill_Indigo", (4.5, -2.0, 5.0), 150.0, 4.0),
    ):
        data = bpy.data.lights.new(name, "AREA")
        data.energy = energy
        data.shape = "DISK"
        data.size = size
        light = bpy.data.objects.new(name, data)
        light.location = location
        aim_camera(light, Vector((0.0, 0.0, 0.0)))
        collection("LIGHTS").objects.link(light)
    return camera, materials


def convert_group_to_mesh(parent: bpy.types.Object) -> list[bpy.types.Object]:
    converted: list[bpy.types.Object] = []
    descendants = [obj for obj in bpy.data.objects if obj.parent == parent]
    for obj in descendants:
        if obj.type != "CURVE":
            continue
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.convert(target="MESH")
        obj.select_set(False)
        converted.append(obj)
    return converted


def descendants(parent: bpy.types.Object) -> list[bpy.types.Object]:
    found: list[bpy.types.Object] = []
    queue = list(parent.children)
    while queue:
        current = queue.pop(0)
        found.append(current)
        queue.extend(current.children)
    return found


def set_group_renderable(parent: bpy.types.Object, renderable: bool) -> None:
    for obj in (parent, *descendants(parent)):
        obj.hide_render = not renderable


def set_shape_state(body: bpy.types.Object, u: float = 0.0, delta: float = 0.0) -> None:
    body.data.shape_keys.key_blocks["UProjection"].value = u
    body.data.shape_keys.key_blocks["DeltaProjection"].value = delta
    bpy.context.view_layer.update()


def bake_body(body: bpy.types.Object, name: str, u: float, delta: float, target: bpy.types.Collection) -> bpy.types.Object:
    set_shape_state(body, u=u, delta=delta)
    evaluated = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = bpy.data.meshes.new_from_object(evaluated)
    baked = bpy.data.objects.new(name, mesh)
    target.objects.link(baked)
    for material in body.data.materials:
        if material.name not in baked.data.materials:
            baked.data.materials.append(material)
    baked["identity_role"] = name
    baked["source_body"] = "MobiusBody"
    set_shape_state(body)
    return baked


def export_selected(path: Path, objects: list[bpy.types.Object], *, animations: bool) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    expanded: list[bpy.types.Object] = []
    for obj in objects:
        expanded.append(obj)
        expanded.extend(descendants(obj))
    exportable = []
    visibility: dict[bpy.types.Object, bool] = {}
    for obj in dict.fromkeys(expanded):
        if obj.type in {"MESH", "CURVE", "EMPTY"}:
            visibility[obj] = obj.hide_render
            obj.hide_render = False
            obj.hide_set(False)
            obj.select_set(True)
            exportable.append(obj)
    if not exportable:
        raise RuntimeError(f"No exportable objects for {path}")
    bpy.context.view_layer.objects.active = exportable[0]
    bpy.ops.export_scene.gltf(
        filepath=str(path),
        export_format="GLB",
        use_selection=True,
        export_animations=animations,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
        export_extras=True,
        export_yup=True,
    )
    for obj, was_hidden in visibility.items():
        obj.hide_render = was_hidden
    log(f"exported {path.relative_to(REPO)} ({path.stat().st_size} bytes)")


def render_frames(scene: bpy.types.Scene, frames: dict[str, int]) -> None:
    for label, frame in frames.items():
        scene.frame_set(frame)
        target = RENDERS / f"mobius-{label}.png"
        scene.render.filepath = str(target)
        bpy.ops.render.render(write_still=True)
        log(f"rendered {target.relative_to(REPO)}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_manifest(generated: list[Path]) -> None:
    motion_master = MOTION / "mobius-identity-master.mp4"
    manifest = {
        "schemaVersion": 1,
        "identity": "Möbi𝒰s",
        "formalKernel": "bidiγΔ",
        "governingLaw": "One geometry. Many projections.",
        "phaseAngle": {"radians": PHI, "degrees": math.degrees(PHI)},
        "topology": {
            "surface": "mobius-band",
            "halfTwists": 1,
            "connectedComponents": 1,
            "eulerCharacteristic": 0,
            "boundaryComponents": 1,
        },
        "components": {
            "embodiedBody": "mobius-body.glb",
            "operatorU": "mobius-u-operator.glb",
            "relationalIUs": "mobius-ius-relational.glb",
            "codeSigil": "mobius-u-code-sigil.glb",
            "kernelBiDiDelta": "mobius-bidi-delta.glb",
            "animatedMaster": "mobius-identity-master.glb",
        },
        "components2d": {
            "embodiedBody": "../mobius-embodied-mark.svg",
            "operatorU": "../mobius-u-operator.svg",
            "relationalIUs": "../mobius-ius-relational.svg",
            "codeSigil": "../mobius-u-code-sigil.svg",
            "codeSigilDark": "../mobius-u-code-sigil-dark.svg",
            "kernelBiDi": "../mobius-bidi-kernel.svg",
            "kernelBiDiDelta": "../mobius-bidi-delta.svg",
            "wordmarkDark": "../mobius-u-wordmark-dark.svg",
            "wordmarkLight": "../mobius-u-wordmark-light.svg",
        },
        "renderWitnesses": {
            "presence": "../renders/mobius-presence.png",
            "operator": "../renders/mobius-u-operative.png",
            "oneTurn": "../renders/mobius-one-turn.png",
            "restoration": "../renders/mobius-restoration.png",
            "kernelBiDiDelta": "../renders/mobius-bidi-delta.png",
        },
        "animationRanges": {
            "identityCycle": {"frames": [1, 216], "fps": FPS},
            "kernelDecomposition": {"frames": [228, 300], "fps": FPS},
        },
        "stateRanges": {
            "PRESENCE": [1, 23],
            "ATTENTION": [24, 47],
            "U_OPERATIVE": [48, 119],
            "ONE_TURN": [120, 149],
            "FRAME_FLIP": [150, 167],
            "EUI_RESTORATION": [168, 191],
            "RETURN": [192, 227],
            "BIDI_DELTA_EXTRACTION": [228, 300],
        },
        "semanticStates": [
            "PRESENCE",
            "ATTENTION",
            "U_OPERATIVE",
            "ONE_TURN",
            "FRAME_FLIP",
            "EUI_RESTORATION",
            "RETURN",
            "BIDI_DELTA_EXTRACTION",
        ],
        "files": [],
    }
    if motion_master.is_file():
        manifest["components"]["motionMaster"] = "../motion/mobius-identity-master.mp4"
        generated = [*generated, motion_master]
    for path in sorted(generated):
        manifest["files"].append(
            {
                "path": str(path.relative_to(REPO)),
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
        )
    target = OUTPUT / "identity-manifest.json"
    target.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    log(f"wrote {target.relative_to(REPO)}")


def main() -> None:
    ensure_dirs()
    reset_scene()
    camera, materials = setup_scene()
    export_collection = collection("EXPORT_MASTER")
    component_collection = collection("COMPONENTS")

    root = bpy.data.objects.new("MobiusIdentityRoot", None)
    export_collection.objects.link(root)
    root["identity_role"] = "identity-root"
    root["phase_angle_rad"] = PHI

    body = create_master_body(materials, export_collection)
    body.parent = root
    eyes_and_pupils = [
        create_eye("Eye_Left", (-0.34, 2.37, 0.42), materials, export_collection),
        create_eye("Eye_Right", (0.34, 2.37, 0.42), materials, export_collection),
    ]
    eyes = [pair[0] for pair in eyes_and_pupils]
    pupils = [pair[1] for pair in eyes_and_pupils]
    for eye in eyes:
        eye.parent = root
    ground = create_bar("PhaseGround", (0.0, -2.32, 0.0), (1.42, 0.028, 0.025), materials["kernel"], export_collection)
    ground.rotation_euler.z = PHI
    ground.parent = root
    hangul_vertical = create_bar(
        "Hangul_Vertical_i",
        (2.35, 0.10, 0.0),
        (0.035, 1.05, 0.025),
        materials["kernel"],
        export_collection,
    )
    hangul_vertical.parent = root
    hangul_vertical["identity_role"] = "hangul-vertical-subjective-i"

    kernel_word = import_svg_group(
        IDENTITY / "mobius-bidi-kernel.svg",
        "Kernel_BiDi",
        4.4,
        materials["kernel"],
        export_collection,
    )
    kernel_word.location += Vector((-2.65, -0.72, 0.24))
    convert_group_to_mesh(kernel_word)

    animate_scene(root, body, eyes, pupils, ground, hangul_vertical, kernel_word, camera)
    scene = bpy.context.scene
    scene.frame_set(1)

    # Component instances are baked from the same source body.
    body_rest = bake_body(body, "Component_EmbodiedBody", 0.0, 0.0, component_collection)
    body_u = bake_body(body, "Component_OperatorU", 1.0, 0.0, component_collection)
    body_delta = bake_body(body, "Component_Delta", 0.0, 1.0, component_collection)
    body_rest.hide_render = True
    body_u.hide_render = True
    body_delta.hide_render = True

    underscore = create_bar("Component_CodeCursor", (0.38, -2.28, 0.0), (0.78, 0.038, 0.025), materials["kernel"], component_collection)
    underscore.hide_render = True
    relational = import_svg_group(IDENTITY / "mobius-ius-relational.svg", "Component_iUs", 4.4, materials["kernel"], component_collection)
    code_sigil = import_svg_group(IDENTITY / "mobius-u-code-sigil-dark.svg", "Component_UCursor", 3.4, materials["white"], component_collection)
    bidi_word = import_svg_group(IDENTITY / "mobius-bidi-kernel.svg", "Component_BiDi", 4.4, materials["kernel"], component_collection)
    convert_group_to_mesh(relational)
    convert_group_to_mesh(code_sigil)
    convert_group_to_mesh(bidi_word)
    set_group_renderable(relational, False)
    set_group_renderable(code_sigil, False)
    set_group_renderable(bidi_word, False)

    master_blend = OUTPUT / "mobius-identity-master.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master_blend), compress=True)
    log(f"saved {master_blend.relative_to(REPO)}")

    master_objects = [root, body, ground, hangul_vertical, kernel_word, *eyes, *pupils]
    export_selected(OUTPUT / "mobius-identity-master.glb", master_objects, animations=True)
    export_selected(OUTPUT / "mobius-body.glb", [body_rest], animations=False)
    export_selected(OUTPUT / "mobius-u-operator.glb", [body_u], animations=False)
    export_selected(OUTPUT / "mobius-u-code-sigil.glb", [code_sigil], animations=False)
    export_selected(OUTPUT / "mobius-ius-relational.glb", [relational], animations=False)
    export_selected(OUTPUT / "mobius-bidi-delta.glb", [bidi_word, body_delta], animations=False)

    render_frames(
        scene,
        {
            "presence": 1,
            "u-operative": 72,
            "one-turn": 132,
            "restoration": 180,
            "bidi-delta": 264,
        },
    )

    generated = [
        master_blend,
        *(OUTPUT / filename for filename in (
            "mobius-identity-master.glb",
            "mobius-body.glb",
            "mobius-u-operator.glb",
            "mobius-u-code-sigil.glb",
            "mobius-ius-relational.glb",
            "mobius-bidi-delta.glb",
        )),
        *(RENDERS / f"mobius-{label}.png" for label in (
            "presence",
            "u-operative",
            "one-turn",
            "restoration",
            "bidi-delta",
        )),
    ]
    write_manifest(generated)
    scene.frame_set(1)
    log("build complete")


if __name__ == "__main__":
    main()
