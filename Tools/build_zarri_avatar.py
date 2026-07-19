"""Build Zarri's game-ready avatar from the project's animated Cappy base.

The source model already has the lightweight mobile rig and all seven gameplay
clips.  This pass creates a distinct Zarri derivative with medium-deep skin,
dark hair, and a darker tech-streetwear palette, then validates the exported
FBX from a clean import and renders Walk/Sit QA frames.

Run with Blender 5.1+:
    blender --background --python Tools/build_zarri_avatar.py
"""

import colorsys
import math
import shutil
import tempfile
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
SOURCE_FBX = ROOT / "Content/Import/Pedestrians/Cappy.fbx"
OUTPUT_FBX = ROOT / "staging/avatars_out/Zarri.fbx"
QA_DIR = ROOT / "staging/qa"
EXPECTED_CLIPS = {
    "A_Idle", "A_Walk", "A_Jog", "A_Talk", "A_WalkFormal",
    "A_Sprint", "A_Sit",
}


def lerp_color(shadow, highlight, amount):
    amount = max(0.0, min(1.0, amount))
    return tuple(a + (b - a) * amount for a, b in zip(shadow, highlight))


def recolor_zarri_atlas(image, texture_path):
    """Recolor the packed Cappy atlas without changing its UV layout."""
    width, height = image.size
    pixels = list(image.pixels[:])
    changed_pixels = 0

    for y in range(height):
        atlas_y = height - 1 - y
        for x in range(width):
            offset = (y * width + x) * 4
            red, green, blue, alpha = pixels[offset:offset + 4]
            if alpha < 0.05:
                continue

            hue, saturation, value = colorsys.rgb_to_hsv(red, green, blue)

            # Cappy's two pale side-hair UV islands.  The cap stays in place,
            # while the visible hair now reads as a close black fade.
            is_hair = (
                x < 142 and 18 <= atlas_y <= 286
                and value > 0.72 and saturation < 0.18
            )
            if is_hair:
                new_rgb = lerp_color(
                    (0.015, 0.020, 0.028), (0.055, 0.070, 0.090), value)
            else:
                # Skin occupies a narrow peach/orange band in the source
                # atlas.  The extra bounded test covers the stylized nose.
                is_skin = (
                    0.070 <= hue <= 0.125
                    and 0.16 <= saturation <= 0.72
                    and value >= 0.55
                )
                is_nose = (
                    220 <= x <= 315 and 105 <= atlas_y <= 205
                    and 0.012 <= hue <= 0.052
                    and saturation >= 0.35 and value >= 0.65
                )
                if is_skin or is_nose:
                    shade = (value - 0.52) / 0.48
                    new_rgb = lerp_color(
                        (0.20, 0.070, 0.035),
                        (0.47, 0.245, 0.135),
                        shade,
                    )
                # Shift the cream hoodie/sneaker panels into graphite while
                # preserving the white eyes and brighter linework.
                elif (
                    0.125 <= hue <= 0.235
                    and 0.055 <= saturation <= 0.24
                    and value >= 0.74
                ):
                    new_rgb = lerp_color(
                        (0.075, 0.095, 0.145),
                        (0.18, 0.23, 0.34),
                        value,
                    )
                # Deepen the existing teal torso panels into navy techwear.
                elif (
                    0.40 <= hue <= 0.58
                    and 0.12 <= saturation <= 0.38
                    and 0.22 <= value <= 0.72
                ):
                    new_rgb = lerp_color(
                        (0.045, 0.060, 0.095),
                        (0.13, 0.19, 0.28),
                        value,
                    )
                else:
                    continue

            pixels[offset:offset + 3] = new_rgb
            changed_pixels += 1

    image.pixels[:] = pixels
    image.update()
    image.name = "T_Zarri_Albedo"
    image.filepath_raw = str(texture_path)
    image.file_format = "PNG"
    image.save()
    if changed_pixels < 1000:
        raise RuntimeError(
            "Zarri atlas recolor changed too few pixels: {}".format(
                changed_pixels))
    print("Recolored {:,} Zarri atlas pixels".format(changed_pixels))


def prepare_material(meshes, texture_path):
    images = {
        node.image
        for mesh in meshes
        for slot in mesh.material_slots
        if slot.material and slot.material.node_tree
        for node in slot.material.node_tree.nodes
        if node.type == "TEX_IMAGE" and node.image
    }
    if len(images) != 1:
        raise RuntimeError(
            "Zarri base must contain exactly one texture image; found {}"
            .format(len(images)))

    source_image = next(iter(images))
    zarri_image = bpy.data.images.new(
        "T_Zarri_Albedo",
        width=source_image.size[0],
        height=source_image.size[1],
        alpha=True,
    )
    zarri_image.colorspace_settings.name = source_image.colorspace_settings.name
    zarri_image.alpha_mode = source_image.alpha_mode
    zarri_image.pixels[:] = source_image.pixels[:]
    zarri_image.update()
    recolor_zarri_atlas(zarri_image, texture_path)

    materials = []
    for mesh in meshes:
        for slot in mesh.material_slots:
            material = slot.material
            if not material:
                continue
            if material not in materials:
                materials.append(material)
            material.name = "M_Zarri"
            material.use_nodes = True
            nodes = material.node_tree.nodes
            links = material.node_tree.links
            texture_node = next(
                (node for node in nodes if node.type == "TEX_IMAGE"), None)
            principled = next(
                (node for node in nodes if node.type == "BSDF_PRINCIPLED"),
                None,
            )
            output = next(
                (node for node in nodes if node.type == "OUTPUT_MATERIAL"),
                None,
            )
            if texture_node is None or principled is None or output is None:
                raise RuntimeError("Zarri base material lacks FBX-safe nodes")
            texture_node.image = zarri_image
            links.new(texture_node.outputs["Color"], principled.inputs["Base Color"])
            links.new(principled.outputs["BSDF"], output.inputs["Surface"])
            principled.inputs["Metallic"].default_value = 0.0
            principled.inputs["Roughness"].default_value = 0.78
            principled.inputs["Alpha"].default_value = 1.0
    return zarri_image


def export_avatar(armature, meshes):
    armature.name = "Armature_Zarri"
    armature.data.name = "Zarri_Skeleton"
    for index, mesh in enumerate(meshes):
        mesh.name = "Zarri" if index == 0 else "Zarri_{:02d}".format(index)
        mesh.data.name = "SK_Zarri_Source_{:02d}".format(index)

    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    for mesh in meshes:
        mesh.select_set(True)
    bpy.context.view_layer.objects.active = armature

    OUTPUT_FBX.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=str(OUTPUT_FBX),
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        add_leaf_bones=False,
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=True,
        bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=0.0,
        mesh_smooth_type="FACE",
        path_mode="COPY",
        embed_textures=True,
    )


def action_clip_name(action):
    return action.name.rsplit("|", 1)[-1]


def render_qa(armature, meshes):
    scene = bpy.context.scene
    for obj in bpy.data.objects:
        obj.hide_render = obj not in meshes

    camera = bpy.data.objects.new("QACam", bpy.data.cameras.new("QACam"))
    scene.collection.objects.link(camera)
    camera.hide_render = False
    scene.camera = camera

    sun = bpy.data.objects.new("QASun", bpy.data.lights.new("QASun", "SUN"))
    scene.collection.objects.link(sun)
    sun.hide_render = False
    sun.data.energy = 4.0
    sun.rotation_euler = (math.radians(50), math.radians(15), 0.0)

    scene.world = bpy.data.worlds.new("QAWorld")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs[0].default_value = (
        0.7, 0.8, 1.0, 1.0)
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.render.resolution_x = 420
    scene.render.resolution_y = 520
    scene.render.resolution_percentage = 100
    QA_DIR.mkdir(parents=True, exist_ok=True)

    for clip_name in ("A_Walk", "A_Sit"):
        action = next(
            item for item in bpy.data.actions
            if action_clip_name(item) == clip_name)
        armature.animation_data_create()
        armature.animation_data.action = action
        object_slots = [
            slot for slot in action.slots if slot.target_id_type == "OBJECT"]
        if object_slots:
            armature.animation_data.action_slot = object_slots[0]

        start, end = (int(value) for value in action.frame_range)
        scene.frame_set(start + (end - start) // 2)
        bpy.context.view_layer.update()

        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = [mesh.evaluated_get(depsgraph) for mesh in meshes]
        points = [
            mesh.matrix_world @ Vector(corner)
            for mesh in evaluated for corner in mesh.bound_box
        ]
        minimum = Vector(tuple(min(point[i] for point in points) for i in range(3)))
        maximum = Vector(tuple(max(point[i] for point in points) for i in range(3)))
        center = (minimum + maximum) * 0.5
        size = maximum - minimum
        frame_size = max(1.0, size.z, size.x * 1.35, size.y * 1.35)
        camera.location = center + Vector((
            1.10 * frame_size, -2.45 * frame_size, 0.10 * size.z))
        camera.rotation_euler = (
            center - camera.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(
            QA_DIR / "Zarri_{}.png".format(clip_name.removeprefix("A_")))
        bpy.ops.render.render(write_still=True)


def validate_export():
    with tempfile.TemporaryDirectory(prefix="sprawl-zarri-validate-") as directory:
        import_path = Path(directory) / OUTPUT_FBX.name
        shutil.copy2(OUTPUT_FBX, import_path)
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.wm.fbx_import(
            filepath=str(import_path), use_anim=True, ignore_leaf_bones=True)

        armatures = [obj for obj in bpy.data.objects if obj.type == "ARMATURE"]
        meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
        clips = {action_clip_name(action) for action in bpy.data.actions}
        assigned_images = {
            node.image
            for mesh in meshes
            for slot in mesh.material_slots
            if slot.material and slot.material.node_tree
            for node in slot.material.node_tree.nodes
            if node.type == "TEX_IMAGE" and node.image
            and node.image.packed_file
        }
        if len(armatures) != 1 or not meshes:
            raise RuntimeError("clean Zarri FBX import lost its rig or mesh")
        if clips != EXPECTED_CLIPS:
            raise RuntimeError(
                "clean Zarri FBX clips: expected {}, found {}"
                .format(sorted(EXPECTED_CLIPS), sorted(clips)))
        if len(assigned_images) != 1:
            raise RuntimeError(
                "clean Zarri FBX must contain one packed texture; found {}"
                .format(len(assigned_images)))

        render_qa(armatures[0], meshes)
        print("Validated Zarri: 1 rig, {} mesh(es), 1 texture, 7 clips".format(
            len(meshes)))


def main():
    if not SOURCE_FBX.exists():
        raise RuntimeError("Missing animated Cappy source: {}".format(SOURCE_FBX))

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.wm.fbx_import(
        filepath=str(SOURCE_FBX), use_anim=True, ignore_leaf_bones=True)
    armatures = [obj for obj in bpy.data.objects if obj.type == "ARMATURE"]
    meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
    if len(armatures) != 1 or not meshes:
        raise RuntimeError("Cappy source must contain one armature and a mesh")

    clips = {action_clip_name(action) for action in bpy.data.actions}
    if clips != EXPECTED_CLIPS:
        raise RuntimeError(
            "Cappy source clips: expected {}, found {}"
            .format(sorted(EXPECTED_CLIPS), sorted(clips)))

    with tempfile.TemporaryDirectory(prefix="sprawl-zarri-texture-") as directory:
        texture_path = Path(directory) / "T_Zarri_Albedo.png"
        prepare_material(meshes, texture_path)
        export_avatar(armatures[0], meshes)

    validate_export()
    print("Exported {} ({} KB)".format(
        OUTPUT_FBX, OUTPUT_FBX.stat().st_size // 1024))


if __name__ == "__main__":
    main()
