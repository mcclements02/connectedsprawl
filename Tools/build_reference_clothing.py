"""Build a fitted gray long-sleeve shirt and dark straight-leg pants in Blender.

The supplied character reference reads as a close-fitting light-gray crewneck
shirt with full sleeves, paired with relaxed straight dark jeans. The game
export is intentionally split at elbows and knees with rounded transition
shells so Unreal can map rigid, smooth garment sections to the live MetaHuman
bone chain.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = PROJECT_ROOT / "Content/MetaHumans/Source/Clothing"
EXPORT_ROOT = PROJECT_ROOT / "Content/Import/Characters/ReferenceClothing"
BLEND_PATH = SOURCE_ROOT / "ConnectedSprawl_ReferenceClothing.blend"
PREVIEW_PATH = SOURCE_ROOT / "ReferenceClothingPreview.png"
REPORT_PATH = SOURCE_ROOT / "ReferenceClothingReport.json"


def clean_scene() -> None:
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,
    ):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def material(name: str, color: tuple[float, float, float, float],
             roughness: float, sheen: float = 0.0) -> bpy.types.Material:
    result = bpy.data.materials.new(name)
    result.diffuse_color = color
    result.use_nodes = True
    principled = result.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = color
    principled.inputs["Roughness"].default_value = roughness
    if "Sheen Weight" in principled.inputs:
        principled.inputs["Sheen Weight"].default_value = sheen
    return result


def finish_mesh(obj: bpy.types.Object, thickness: float = 0.006,
                bevel: float = 0.003, subdivision: int = 1) -> None:
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    if thickness > 0.0:
        solidify = obj.modifiers.new("GarmentThickness", "SOLIDIFY")
        solidify.thickness = thickness
        solidify.offset = 0.0
        solidify.use_even_offset = True
        bpy.ops.object.modifier_apply(modifier=solidify.name)
    if bevel > 0.0:
        bevel_modifier = obj.modifiers.new("SoftSeams", "BEVEL")
        bevel_modifier.width = bevel
        bevel_modifier.segments = 2
        bpy.ops.object.modifier_apply(modifier=bevel_modifier.name)
    if subdivision > 0:
        smooth = obj.modifiers.new("CleanSubdivision", "SUBSURF")
        smooth.levels = subdivision
        smooth.render_levels = subdivision
        bpy.ops.object.modifier_apply(modifier=smooth.name)
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.select_set(False)


def elliptic_loft(name: str,
                   rings: list[tuple[float, float, float]],
                   target_material: bpy.types.Material,
                   collection: bpy.types.Collection,
                   segments: int = 32) -> bpy.types.Object:
    """Create a torso/pelvis shell in X-depth, Y-width, Z-height space."""
    vertices = []
    faces = []
    for ring_index, (height, depth, width) in enumerate(rings):
        for side in range(segments):
            angle = math.tau * side / segments
            wrinkle = 1.0 + 0.008 * math.sin(angle * 4.0 + ring_index * 0.7)
            vertices.append((
                math.cos(angle) * depth * wrinkle,
                math.sin(angle) * width * wrinkle,
                height,
            ))
    for ring_index in range(len(rings) - 1):
        for side in range(segments):
            nxt = (side + 1) % segments
            a = ring_index * segments + side
            b = ring_index * segments + nxt
            c = (ring_index + 1) * segments + nxt
            d = (ring_index + 1) * segments + side
            faces.append((a, b, c, d))
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate()
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.data.materials.append(target_material)
    finish_mesh(obj)
    return obj


def tube_between(name: str, start: Vector, end: Vector,
                 start_depth: float, start_radius: float,
                 end_depth: float, end_radius: float,
                 target_material: bpy.types.Material,
                 collection: bpy.types.Collection,
                 rings: int = 6, segments: int = 24) -> bpy.types.Object:
    direction = (end - start).normalized()
    depth_axis = Vector((1.0, 0.0, 0.0))
    if abs(direction.dot(depth_axis)) > 0.95:
        depth_axis = Vector((0.0, 1.0, 0.0))
    depth_axis = (depth_axis - direction * direction.dot(depth_axis)).normalized()
    radial_axis = direction.cross(depth_axis).normalized()
    vertices = []
    faces = []
    for ring_index in range(rings):
        progress = ring_index / (rings - 1)
        center = start.lerp(end, progress)
        depth = start_depth + (end_depth - start_depth) * progress
        radius = start_radius + (end_radius - start_radius) * progress
        for side in range(segments):
            angle = math.tau * side / segments
            wrinkle = 1.0 + 0.012 * math.sin(
                angle * 3.0 + ring_index * 1.25)
            vertices.append(tuple(
                center
                + depth_axis * (math.cos(angle) * depth * wrinkle)
                + radial_axis * (math.sin(angle) * radius * wrinkle)
            ))
    for ring_index in range(rings - 1):
        for side in range(segments):
            nxt = (side + 1) % segments
            a = ring_index * segments + side
            b = ring_index * segments + nxt
            c = (ring_index + 1) * segments + nxt
            d = (ring_index + 1) * segments + side
            faces.append((a, b, c, d))
    faces.append(tuple(range(segments - 1, -1, -1)))
    last_ring = (rings - 1) * segments
    faces.append(tuple(last_ring + side for side in range(segments)))
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate()
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj["sprawl_export_axis"] = tuple(direction)
    obj["sprawl_export_center"] = tuple((start + end) * 0.5)
    obj.data.materials.append(target_material)
    finish_mesh(obj)
    return obj


def torus_band(name: str, location: Vector, major_radius: float,
               minor_radius: float, scale_y: float,
               target_material: bpy.types.Material,
               collection: bpy.types.Collection) -> bpy.types.Object:
    bpy.ops.mesh.primitive_torus_add(
        align="WORLD",
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=32,
        minor_segments=8,
        location=location,
    )
    obj = bpy.context.object
    obj.name = name
    for current_collection in list(obj.users_collection):
        current_collection.objects.unlink(obj)
    collection.objects.link(obj)
    obj.scale.y = scale_y
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(target_material)
    finish_mesh(obj, thickness=0.0, bevel=0.0015, subdivision=0)
    return obj


def joint_shell(name: str, center: Vector, radius: float, axis: Vector,
                target_material: bpy.types.Material,
                collection: bpy.types.Collection) -> bpy.types.Object:
    """Create a rounded elbow/knee cover with a normalized export axis."""
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=32, ring_count=16, location=center)
    obj = bpy.context.object
    obj.name = name
    for current_collection in list(obj.users_collection):
        current_collection.objects.unlink(obj)
    collection.objects.link(obj)
    obj.scale = (radius, radius, radius)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=True)
    obj["sprawl_export_axis"] = tuple(axis.normalized())
    obj["sprawl_export_center"] = tuple(center)
    obj.data.materials.append(target_material)
    finish_mesh(obj, thickness=0.0, bevel=0.0015, subdivision=0)
    return obj


def proxy_capsule(name: str, location: tuple[float, float, float],
                  scale: tuple[float, float, float],
                  target_material: bpy.types.Material,
                  collection: bpy.types.Collection) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=32, ring_count=16, location=location)
    obj = bpy.context.object
    obj.name = name
    for current_collection in list(obj.users_collection):
        current_collection.objects.unlink(obj)
    collection.objects.link(obj)
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(target_material)
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return obj


def build_proxy(collection: bpy.types.Collection,
                skin: bpy.types.Material) -> None:
    proxy_capsule("Proxy_Head", (0.0, 0.0, 1.78), (0.105, 0.10, 0.13), skin, collection)
    proxy_capsule("Proxy_Torso", (0.0, 0.0, 1.34), (0.135, 0.225, 0.30), skin, collection)
    proxy_capsule("Proxy_Pelvis", (0.0, 0.0, 0.96), (0.125, 0.185, 0.14), skin, collection)
    for side in (-1.0, 1.0):
        suffix = "L" if side < 0 else "R"
        tube_between(
            f"Proxy_UpperArm_{suffix}",
            Vector((0.0, side * 0.285, 1.545)),
            Vector((0.0, side * 0.485, 1.315)),
            0.070, 0.078, 0.060, 0.068,
            skin, collection, rings=4, segments=16)
        tube_between(
            f"Proxy_LowerArm_{suffix}",
            Vector((0.0, side * 0.465, 1.335)),
            Vector((0.0, side * 0.615, 1.095)),
            0.061, 0.069, 0.050, 0.055,
            skin, collection, rings=4, segments=16)
        proxy_capsule(
            f"Proxy_Hand_{suffix}",
            (0.0, side * 0.655, 1.04), (0.052, 0.075, 0.045),
            skin, collection)
        tube_between(
            f"Proxy_UpperLeg_{suffix}",
            Vector((0.0, side * 0.115, 0.86)),
            Vector((0.0, side * 0.108, 0.50)),
            0.095, 0.090, 0.082, 0.078,
            skin, collection, rings=5, segments=16)
        tube_between(
            f"Proxy_LowerLeg_{suffix}",
            Vector((0.0, side * 0.108, 0.57)),
            Vector((0.0, side * 0.102, 0.09)),
            0.082, 0.078, 0.070, 0.066,
            skin, collection, rings=6, segments=16)


def export_object(obj: bpy.types.Object) -> Path:
    output = EXPORT_ROOT / f"{obj.name}.fbx"
    export_copy = obj.copy()
    export_copy.data = obj.data.copy()
    export_copy.name = obj.name
    bpy.context.scene.collection.objects.link(export_copy)
    bounds_center = sum(
        (Vector(corner) for corner in obj.bound_box), Vector()) / 8.0
    export_center = Vector(obj.get("sprawl_export_center", bounds_center))
    export_axis = Vector(obj.get("sprawl_export_axis", (0.0, 0.0, 1.0)))
    export_basis = Vector((0.0, 0.0, 1.0)).rotation_difference(
        export_axis.normalized())
    inverse_basis = export_basis.inverted()
    for vertex in export_copy.data.vertices:
        vertex.co = inverse_basis @ (vertex.co - export_center)
    export_copy.data.update()

    bpy.ops.object.select_all(action="DESELECT")
    export_copy.select_set(True)
    bpy.context.view_layer.objects.active = export_copy
    bpy.ops.export_scene.fbx(
        filepath=str(output),
        use_selection=True,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        object_types={"MESH"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        add_leaf_bones=False,
        bake_anim=False,
        axis_forward="-Z",
        axis_up="Y",
        path_mode="AUTO",
    )
    bpy.data.objects.remove(export_copy, do_unlink=True)
    return output


def point_camera(camera: bpy.types.Object, target: Vector) -> None:
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()


def build() -> None:
    SOURCE_ROOT.mkdir(parents=True, exist_ok=True)
    EXPORT_ROOT.mkdir(parents=True, exist_ok=True)
    clean_scene()

    garment_collection = bpy.data.collections.new("Reference_Clothing_Game")
    proxy_collection = bpy.data.collections.new("Reference_Human_Preview")
    stage_collection = bpy.data.collections.new("Preview_Stage")
    bpy.context.scene.collection.children.link(garment_collection)
    bpy.context.scene.collection.children.link(proxy_collection)
    bpy.context.scene.collection.children.link(stage_collection)

    shirt_material = material("M_ReferenceShirt_Gray", (0.49, 0.52, 0.55, 1.0), 0.78, 0.22)
    pants_material = material("M_ReferencePants_DarkDenim", (0.035, 0.045, 0.06, 1.0), 0.92, 0.04)
    seam_material = material("M_ReferenceSeams", (0.018, 0.022, 0.03, 1.0), 0.8)
    skin_material = material("M_ReferenceProxySkin", (0.19, 0.075, 0.035, 1.0), 0.62)
    stage_material = material("M_ReferenceStage", (0.025, 0.03, 0.04, 1.0), 0.68)

    garments = []
    garments.append(elliptic_loft(
        "SM_ReferenceShirt_Torso",
        [
            (1.04, 0.125, 0.190),
            (1.12, 0.135, 0.205),
            (1.34, 0.150, 0.245),
            (1.50, 0.160, 0.285),
            (1.56, 0.140, 0.295),
            (1.620, 0.075, 0.100),
        ],
        shirt_material, garment_collection))
    garments.append(torus_band(
        "SM_ReferenceShirt_Collar", Vector((0.0, 0.0, 1.635)),
        0.094, 0.012, 1.18, seam_material, garment_collection))
    garments.append(torus_band(
        "SM_ReferenceShirt_Hem", Vector((0.0, 0.0, 1.045)),
        0.190, 0.014, 1.25, seam_material, garment_collection))

    for side in (-1.0, 1.0):
        suffix = "L" if side < 0 else "R"
        upper = tube_between(
            f"SM_ReferenceShirt_UpperSleeve_{suffix}",
            Vector((0.0, side * 0.285, 1.545)),
            Vector((0.0, side * 0.485, 1.315)),
            0.082, 0.090, 0.073, 0.080,
            shirt_material, garment_collection)
        lower = tube_between(
            f"SM_ReferenceShirt_LowerSleeve_{suffix}",
            Vector((0.0, side * 0.465, 1.335)),
            Vector((0.0, side * 0.615, 1.095)),
            0.074, 0.081, 0.060, 0.065,
            shirt_material, garment_collection)
        elbow_center = Vector((0.0, side * 0.475, 1.325))
        forearm_axis = Vector((0.0, side * 0.15, -0.24)).normalized()
        cuff = joint_shell(
            f"SM_ReferenceShirt_Cuff_{suffix}",
            elbow_center, 0.100, forearm_axis,
            seam_material, garment_collection)
        garments.extend((upper, lower, cuff))

    garments.append(elliptic_loft(
        "SM_ReferencePants_Waist",
        [
            (0.82, 0.135, 0.190),
            (0.94, 0.145, 0.202),
            (1.055, 0.142, 0.208),
            (1.095, 0.138, 0.202),
        ],
        pants_material, garment_collection))
    garments.append(torus_band(
        "SM_ReferencePants_Waistband", Vector((0.0, 0.0, 1.075)),
        0.185, 0.018, 1.22, seam_material, garment_collection))

    for side in (-1.0, 1.0):
        suffix = "L" if side < 0 else "R"
        upper = tube_between(
            f"SM_ReferencePants_UpperLeg_{suffix}",
            Vector((0.0, side * 0.115, 0.86)),
            Vector((0.0, side * 0.108, 0.49)),
            0.115, 0.115, 0.110, 0.110,
            pants_material, garment_collection, rings=7)
        lower = tube_between(
            f"SM_ReferencePants_LowerLeg_{suffix}",
            Vector((0.0, side * 0.108, 0.57)),
            Vector((0.0, side * 0.102, 0.105)),
            0.125, 0.125, 0.105, 0.105,
            pants_material, garment_collection, rings=8)
        knee_center = Vector((0.0, side * 0.108, 0.530))
        calf_axis = Vector((0.0, side * -0.006, -0.465)).normalized()
        cuff = joint_shell(
            f"SM_ReferencePants_Cuff_{suffix}",
            knee_center, 0.125, calf_axis,
            seam_material, garment_collection)
        garments.extend((upper, lower, cuff))

    build_proxy(proxy_collection, skin_material)

    bpy.ops.mesh.primitive_plane_add(size=20.0, location=(0.0, 0.0, 0.0))
    floor = bpy.context.object
    floor.name = "Preview_Floor"
    for current_collection in list(floor.users_collection):
        current_collection.objects.unlink(floor)
    stage_collection.objects.link(floor)
    floor.data.materials.append(stage_material)

    bpy.ops.object.camera_add(location=(3.8, -4.7, 2.45))
    camera = bpy.context.object
    camera.name = "ReferenceClothing_Camera"
    for current_collection in list(camera.users_collection):
        current_collection.objects.unlink(camera)
    stage_collection.objects.link(camera)
    point_camera(camera, Vector((0.0, 0.0, 0.98)))
    camera.data.lens = 58.0
    bpy.context.scene.camera = camera

    for name, location, energy, size in (
        ("Key", (3.5, -3.0, 4.5), 1300.0, 4.0),
        ("Fill", (1.0, 4.0, 2.8), 900.0, 3.5),
        ("Rim", (-3.0, -1.0, 3.5), 1050.0, 3.0),
    ):
        light_data = bpy.data.lights.new(name, "AREA")
        light_data.energy = energy
        light_data.shape = "DISK"
        light_data.size = size
        light = bpy.data.objects.new(name, light_data)
        light.location = location
        point_camera(light, Vector((0.0, 0.0, 1.0)))
        stage_collection.objects.link(light)

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(PREVIEW_PATH)
    scene.render.film_transparent = False
    scene.world.color = (0.012, 0.016, 0.024)

    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    exports = [str(export_object(obj)) for obj in garments]
    bpy.ops.render.render(write_still=True)

    REPORT_PATH.write_text(json.dumps({
        "reference": "light-gray fitted crewneck shirt and dark straight-leg jeans",
        "blend": str(BLEND_PATH),
        "preview": str(PREVIEW_PATH),
        "exports": exports,
        "garmentObjects": [obj.name for obj in garments],
        "garmentVertices": sum(len(obj.data.vertices) for obj in garments),
        "mapping": {
            "shirt": ["spine_03", "upperarm_l/r", "lowerarm_l/r"],
            "pants": ["pelvis", "thigh_l/r", "calf_l/r"],
        },
        "exportSpace": "followers centered on origin with local +Z along limb",
    }, indent=2), encoding="utf-8")
    print(
        f"[ReferenceClothing] saved {BLEND_PATH}; "
        f"exported {len(exports)} pieces; preview {PREVIEW_PATH}")


if __name__ == "__main__":
    build()
