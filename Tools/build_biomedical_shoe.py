"""Build the project-owned Cordero Biomedical Bio-Circuit high-top pair.

Run inside Blender 4.3+ (validated with Blender 5.1). The deterministic scene
uses metres, +X toe-forward, and separate named parts suitable for later
MetaHuman weight transfer and LOD authoring.
"""

from __future__ import annotations

import math
from pathlib import Path

import bpy
from mathutils import Vector


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT_ROOT / "Content/MetaHumans/Source/Footwear/CorderoBiomedical"
IMPORT_DIR = PROJECT_ROOT / "Content/Import/Characters/Footwear"
BLEND_PATH = SOURCE_DIR / "CorderoBiomedical_HighTop.blend"
PREVIEW_PATH = SOURCE_DIR / "CorderoBiomedical_HighTopPreview.png"
REPORT_PATH = SOURCE_DIR / "CorderoBiomedical_HighTopReport.txt"
FBX_PATH = IMPORT_DIR / "SM_CorderoBiomedical_HighTopPair.fbx"


def reset_scene() -> None:
    if bpy.context.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials,
                       bpy.data.cameras, bpy.data.lights):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def material(name: str, color: tuple[float, float, float, float],
             metallic: float = 0.0, roughness: float = 0.45,
             transmission: float = 0.0) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if "Transmission Weight" in bsdf.inputs:
        bsdf.inputs["Transmission Weight"].default_value = transmission
    if transmission:
        bsdf.inputs["Alpha"].default_value = color[3]
        mat.surface_render_method = "DITHERED"
    return mat


def assign(obj: bpy.types.Object, mat: bpy.types.Material) -> bpy.types.Object:
    if hasattr(obj.data, "materials"):
        obj.data.materials.append(mat)
    return obj


def rounded_box(name: str, location: tuple[float, float, float],
                dimensions: tuple[float, float, float], radius: float,
                mat: bpy.types.Material,
                rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
                segments: int = 3) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bevel = obj.modifiers.new("MouldedEdge", "BEVEL")
    bevel.width = min(radius, min(dimensions) * 0.48)
    bevel.segments = segments
    bevel.limit_method = "ANGLE"
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return assign(obj, mat)


def cylinder_between(name: str, start: Vector, end: Vector, radius: float,
                     mat: bpy.types.Material, vertices: int = 12) -> bpy.types.Object:
    delta = end - start
    midpoint = (start + end) * 0.5
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices, radius=radius, depth=delta.length, location=midpoint)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = delta.to_track_quat("Z", "Y")
    return assign(obj, mat)


def torus(name: str, location: tuple[float, float, float], major: float,
          minor: float, scale: tuple[float, float, float],
          mat: bpy.types.Material,
          rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)) -> bpy.types.Object:
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major, minor_radius=minor, major_segments=20,
        minor_segments=6, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return assign(obj, mat)


def mesh_upper(name: str, side_y: float, mat: bpy.types.Material) -> bpy.types.Object:
    """Make a tapered, closed toe/forefoot nanofiber shell."""
    rings = [
        (0.145, 0.032, 0.046),
        (0.112, 0.047, 0.060),
        (0.050, 0.050, 0.070),
        (-0.018, 0.047, 0.082),
        (-0.066, 0.044, 0.090),
    ]
    vertices: list[tuple[float, float, float]] = []
    ring_count = 12
    for x, half_width, height in rings:
        for index in range(ring_count):
            angle = (2.0 * math.pi * index / ring_count)
            y = side_y + half_width * math.cos(angle)
            z = 0.042 + max(0.003, height * math.sin(angle))
            if math.sin(angle) < 0.0:
                z = 0.038 + 0.006 * (1.0 + math.sin(angle))
            vertices.append((x, y, z))
    faces: list[tuple[int, ...]] = []
    for ring in range(len(rings) - 1):
        for index in range(ring_count):
            next_index = (index + 1) % ring_count
            a = ring * ring_count + index
            b = ring * ring_count + next_index
            c = (ring + 1) * ring_count + next_index
            d = (ring + 1) * ring_count + index
            faces.append((a, b, c, d))
    faces.append(tuple(reversed(range(ring_count))))
    final_start = (len(rings) - 1) * ring_count
    faces.append(tuple(final_start + index for index in range(ring_count)))
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign(obj, mat)
    bevel = obj.modifiers.new("SoftNanofiberEdge", "BEVEL")
    bevel.width = 0.0025
    bevel.segments = 2
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return obj


def text_mesh(name: str, body: str, location: tuple[float, float, float],
              size: float, extrude: float, mat: bpy.types.Material,
              rotation: tuple[float, float, float]) -> bpy.types.Object:
    bpy.ops.object.text_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.body = body
    obj.data.align_x = "CENTER"
    obj.data.align_y = "CENTER"
    obj.data.size = size
    obj.data.extrude = extrude
    obj.data.bevel_depth = extrude * 0.25
    obj.data.materials.append(mat)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    return obj


def make_shoe(label: str, center_y: float, mirror: float,
              mats: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    parts: list[bpy.types.Object] = []
    add = parts.append

    add(rounded_box(f"{label}_Outsole", (0.008, center_y, 0.014),
                    (0.292, 0.108, 0.020), 0.010, mats["red"]))
    add(rounded_box(f"{label}_Midsole", (0.010, center_y, 0.029),
                    (0.300, 0.112, 0.026), 0.012, mats["sole"]))
    add(mesh_upper(f"{label}_NanofiberUpper", center_y, mats["mesh"]))

    add(rounded_box(f"{label}_HeelQuarter", (-0.078, center_y, 0.086),
                    (0.110, 0.094, 0.094), 0.025, mats["upper"],
                    rotation=(0.0, -0.12, 0.0), segments=4))
    add(rounded_box(f"{label}_AnkleCuff", (-0.096, center_y, 0.146),
                    (0.088, 0.089, 0.126), 0.028, mats["upper"],
                    rotation=(0.0, -0.10, 0.0), segments=4))
    add(torus(f"{label}_PaddedCollar", (-0.101, center_y, 0.203),
              0.034, 0.0075, (1.15, 0.82, 0.78), mats["collar"]))

    for rail_side in (-1.0, 1.0):
        add(rounded_box(
            f"{label}_RedEyestay_{'L' if rail_side < 0.0 else 'R'}",
            (-0.006, center_y + rail_side * 0.034, 0.091),
            (0.112, 0.012, 0.012), 0.005, mats["red"],
            rotation=(0.0, -0.26, 0.0)))
    add(rounded_box(f"{label}_Tongue", (-0.025, center_y, 0.098),
                    (0.108, 0.047, 0.011), 0.005, mats["tongue"],
                    rotation=(0.0, -0.28, 0.0)))
    add(rounded_box(f"{label}_ToeGuard", (0.128, center_y, 0.050),
                    (0.070, 0.101, 0.035), 0.016, mats["sole"],
                    rotation=(0.0, 0.10, 0.0)))
    add(rounded_box(f"{label}_HeelSupport", (-0.116, center_y, 0.093),
                    (0.060, 0.101, 0.050), 0.012, mats["red"],
                    rotation=(0.0, -0.16, 0.0)))

    outside_y = center_y + mirror * 0.052
    add(rounded_box(f"{label}_CircuitFrame", (-0.020, outside_y, 0.097),
                    (0.100, 0.006, 0.054), 0.008, mats["red"],
                    rotation=(0.0, -0.10, 0.0)))
    add(rounded_box(f"{label}_CircuitWindow", (-0.017, outside_y + mirror * 0.004, 0.098),
                    (0.078, 0.005, 0.038), 0.007, mats["glass"],
                    rotation=(0.0, -0.10, 0.0)))
    add(rounded_box(f"{label}_BioChip", (-0.017, outside_y + mirror * 0.008, 0.098),
                    (0.025, 0.004, 0.017), 0.003, mats["chip"],
                    rotation=(0.0, -0.10, 0.0)))
    for trace_index, (x, z, length) in enumerate((
            (-0.053, 0.110, 0.022), (0.012, 0.111, 0.020),
            (-0.050, 0.085, 0.020), (0.013, 0.085, 0.022))):
        add(rounded_box(f"{label}_CircuitTrace_{trace_index}",
                        (x, outside_y + mirror * 0.011, z),
                        (length, 0.0025, 0.0025), 0.001, mats["gold"],
                        rotation=(0.0, -0.10, 0.0), segments=1))

    for lace_index, x in enumerate((0.042, 0.020, -0.002, -0.024, -0.046)):
        z = 0.094 + (-x + 0.042) * 0.24
        add(cylinder_between(
            f"{label}_Lace_{lace_index}",
            Vector((x, center_y - 0.031, z)),
            Vector((x, center_y + 0.031, z)),
            0.0022, mats["lace"], vertices=10))

    add(rounded_box(f"{label}_AirWindow", (-0.101, outside_y, 0.034),
                    (0.060, 0.0055, 0.015), 0.004, mats["glass"]))
    add(cylinder_between(
        f"{label}_AirCore",
        Vector((-0.122, outside_y + mirror * 0.004, 0.034)),
        Vector((-0.080, outside_y + mirror * 0.004, 0.034)),
        0.0035, mats["red"], vertices=10))

    pod_x = (-0.110, -0.055, 0.000, 0.055, 0.110)
    for row, local_y in enumerate((-0.033, 0.033)):
        for column, x in enumerate(pod_x):
            add(rounded_box(
                f"{label}_TractionPod_{row}_{column}",
                (x, center_y + local_y, 0.0005),
                (0.035, 0.024, 0.009), 0.004, mats["tread"],
                rotation=(0.0, 0.0, (column % 2 - 0.5) * 0.12),
                segments=2))

    add(torus(f"{label}_PullLoop", (-0.133, center_y, 0.192),
              0.022, 0.004, (0.65, 0.35, 1.4), mats["upper"],
              rotation=(math.radians(90.0), 0.0, 0.0)))

    badge_y = center_y - mirror * 0.031
    add(rounded_box(f"{label}_TongueBadge", (-0.066, badge_y, 0.175),
                    (0.045, 0.004, 0.042), 0.007, mats["badge"],
                    rotation=(0.0, -0.25, 0.0)))
    add(text_mesh(f"{label}_CBMark", "CB", (-0.065, badge_y - mirror * 0.003, 0.177),
                  0.017, 0.001, mats["red"],
                  (math.radians(90.0), 0.0, math.radians(180.0 if mirror > 0 else 0.0))))
    return parts


def add_floor(mats: dict[str, bpy.types.Material]) -> None:
    rounded_box("PresentationPlinth", (0.005, 0.0, -0.027),
                (0.48, 0.40, 0.035), 0.012, mats["plinth"], segments=4)
    rounded_box("PresentationInset", (0.005, 0.0, -0.007),
                (0.42, 0.34, 0.006), 0.004, mats["inset"], segments=2)


def set_camera() -> None:
    bpy.ops.object.camera_add(location=(0.48, -0.58, 0.255))
    camera = bpy.context.object
    camera.name = "CorderoBiomedicalPreviewCamera"
    direction = Vector((0.0, 0.0, 0.085)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    camera.data.lens = 62
    bpy.context.scene.camera = camera


def add_lighting() -> None:
    for name, location, energy, size, color in (
            ("Key", (0.20, -0.30, 0.55), 80.0, 0.35, (1.0, 0.92, 0.88)),
            ("Fill", (-0.35, -0.10, 0.35), 50.0, 0.28, (0.82, 0.90, 1.0)),
            ("Rim", (-0.05, 0.38, 0.50), 65.0, 0.22, (1.0, 0.72, 0.70))):
        bpy.ops.object.light_add(type="AREA", location=location)
        light = bpy.context.object
        light.name = f"{name}Light"
        light.data.energy = energy
        light.data.shape = "DISK"
        light.data.size = size
        light.data.color = color
        direction = Vector((0.0, 0.0, 0.07)) - light.location
        light.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def export_pair(shoe_parts: list[bpy.types.Object]) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    for part in shoe_parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = shoe_parts[0]
    bpy.ops.export_scene.fbx(
        filepath=str(FBX_PATH),
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="X",
        axis_up="Z",
        bake_space_transform=True,
        add_leaf_bones=False,
        path_mode="AUTO",
        embed_textures=False,
    )


def build() -> None:
    SOURCE_DIR.mkdir(parents=True, exist_ok=True)
    IMPORT_DIR.mkdir(parents=True, exist_ok=True)
    reset_scene()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(PREVIEW_PATH)
    scene.render.film_transparent = False
    scene.view_settings.exposure = -0.8
    scene.world.color = (0.035, 0.040, 0.050)

    mats = {
        "upper": material("CB_OffWhite_Synthetic", (0.42, 0.45, 0.46, 1.0), roughness=0.34),
        "mesh": material("CB_Nanofiber_Mesh", (0.30, 0.34, 0.35, 1.0), roughness=0.67),
        "red": material("CB_Biomedical_Red", (0.38, 0.012, 0.026, 1.0), roughness=0.30),
        "gold": material("CB_Circuit_Gold", (0.88, 0.47, 0.08, 1.0), metallic=0.75, roughness=0.22),
        "chip": material("CB_Active_Chip", (0.64, 0.05, 0.03, 1.0), metallic=0.55, roughness=0.20),
        "sole": material("CB_Polymeric_White", (0.50, 0.52, 0.51, 1.0), roughness=0.42),
        "tread": material("CB_Traction_Red", (0.29, 0.008, 0.016, 1.0), roughness=0.54),
        "lace": material("CB_Nanofiber_Laces", (0.50, 0.52, 0.52, 1.0), roughness=0.74),
        "tongue": material("CB_Tongue_Mesh", (0.30, 0.33, 0.34, 1.0), roughness=0.78),
        "collar": material("CB_Collar", (0.20, 0.22, 0.24, 1.0), roughness=0.75),
        "badge": material("CB_Badge", (0.52, 0.54, 0.53, 1.0), roughness=0.38),
        "glass": material("CB_BioGlass", (0.50, 0.68, 0.70, 0.36), metallic=0.05, roughness=0.16, transmission=0.70),
        "plinth": material("CB_Platform", (0.055, 0.060, 0.070, 1.0), metallic=0.20, roughness=0.28),
        "inset": material("CB_PlatformInset", (0.12, 0.025, 0.035, 1.0), metallic=0.32, roughness=0.25),
    }

    left = make_shoe("CB_Left", -0.075, -1.0, mats)
    right = make_shoe("CB_Right", 0.075, 1.0, mats)
    shoe_parts = left + right
    add_floor(mats)
    set_camera()
    add_lighting()

    export_pair(shoe_parts)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH), compress=True)
    Path(f"{BLEND_PATH}1").unlink(missing_ok=True)
    scene.render.filepath = str(PREVIEW_PATH)
    bpy.ops.render.render(write_still=True)

    mesh_objects = [obj for obj in shoe_parts if obj.type == "MESH"]
    triangle_count = 0
    for obj in mesh_objects:
        evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        mesh.calc_loop_triangles()
        triangle_count += len(mesh.loop_triangles)
        evaluated.to_mesh_clear()
    REPORT_PATH.write_text(
        "Cordero Biomedical Bio-Circuit High-Top\n"
        "================================================\n"
        f"Blender source: {BLEND_PATH.relative_to(PROJECT_ROOT)}\n"
        f"Unreal FBX source: {FBX_PATH.relative_to(PROJECT_ROOT)}\n"
        f"Preview: {PREVIEW_PATH.relative_to(PROJECT_ROOT)}\n"
        f"Shoe mesh parts: {len(mesh_objects)}\n"
        f"Evaluated triangles: {triangle_count}\n"
        "Scale: metres; reference shoe length 0.28 m\n"
        "Orientation: +X toe-forward, +Z up\n"
        "Signature details: nanofiber forefoot, deep-red support cage, "
        "transparent circuit window, active chip, heel air unit, 10 traction "
        "pods per shoe, padded high-top collar, CB tongue badge.\n"
        "Runtime integration: USprawlBiomedicalShoeModule; fit and bind the "
        "imported pair to MetaHuman foot skeletons before shipping.\n",
        encoding="utf-8",
    )
    print(f"CORDERO_BIOMEDICAL_BUILD_COMPLETE {BLEND_PATH}")


if __name__ == "__main__":
    build()
