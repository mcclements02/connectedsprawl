"""Build Connected Sprawl's reusable Blender streetwear kit.

Run inside Blender (interactive or background). The kit is authored in metres,
uses +X as shoe-forward, saves the editable .blend source, exports one FBX per
modular item, and renders a review image.
"""

import math
import os

import bpy
from mathutils import Vector


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE_DIR = os.path.join(ROOT, "Content", "MetaHumans", "Source", "Streetwear")
EXPORT_DIR = os.path.join(ROOT, "Content", "Import", "Characters", "Streetwear")
BLEND_PATH = os.path.join(SOURCE_DIR, "ConnectedSprawl_StreetwearKit.blend")
PREVIEW_PATH = os.path.join(SOURCE_DIR, "StreetwearKitPreview.png")
SHOE_PREVIEW_PATH = os.path.join(SOURCE_DIR, "ZarriVelocityShoesPreview.png")
REPORT_PATH = os.path.join(SOURCE_DIR, "StreetwearKitReport.txt")


def material(name, color, metallic=0.0, roughness=0.55, micro_detail=False, sheen=0.4):
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*color, 1.0)
    result.use_nodes = True
    nodes = result.node_tree.nodes
    links = result.node_tree.links
    shader = nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1.0)
    metallic_input = "Metallic IOR Level" if "Metallic IOR Level" in shader.inputs else "Metallic"
    shader.inputs[metallic_input].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    if "Sheen Weight" in shader.inputs and sheen > 0.0:
        shader.inputs["Sheen Weight"].default_value = sheen
    elif "Sheen" in shader.inputs and sheen > 0.0:
        shader.inputs["Sheen"].default_value = sheen
    if micro_detail:
        noise = nodes.new("ShaderNodeTexNoise")
        noise.name = name + "_MicroWeave"
        noise.inputs["Scale"].default_value = 220.0
        noise.inputs["Detail"].default_value = 4.0
        noise.inputs["Roughness"].default_value = 0.78
        bump = nodes.new("ShaderNodeBump")
        bump.name = name + "_MicroBump"
        bump.inputs["Strength"].default_value = 0.22
        bump.inputs["Distance"].default_value = 0.002
        links.new(noise.outputs["Fac"], bump.inputs["Height"])
        links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    return result


def finish_object(obj, mat, collection, bevel=0.0):
    for owned in tuple(obj.users_collection):
        owned.objects.unlink(obj)
    collection.objects.link(obj)
    if mat:
        obj.data.materials.append(mat)
    if bevel > 0.0:
        modifier = obj.modifiers.new("Soft construction", "BEVEL")
        modifier.width = bevel
        modifier.segments = 3
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return obj


def rounded_box(name, location, dimensions, mat, collection, bevel=0.012):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish_object(obj, mat, collection, bevel)


def cylinder_between(name, start, end, radius, mat, collection, vertices=16):
    start = Vector(start)
    end = Vector(end)
    delta = end - start
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices, radius=radius, depth=delta.length,
        location=(start + end) * 0.5)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0.0, 0.0, 1.0)).rotation_difference(delta.normalized())
    return finish_object(obj, mat, collection, 0.003)


def loft_shell(name, rings, mat, collection, segments=24, thickness=0.008, wrinkles=True):
    """Create an open garment shell with organic fabric folds from (z, half_depth, half_width) rings."""
    vertices = []
    for ring_idx, (z, depth, width) in enumerate(rings):
        for side in range(segments):
            angle = math.tau * side / segments
            d_val = depth
            w_val = width
            if wrinkles:
                # Multi-harmonic fabric drape and seam wrinkles
                fold = 0.004 * math.sin(angle * 3.0 + ring_idx * 1.5) + 0.002 * math.cos(angle * 5.0)
                d_val += fold
                w_val += fold
            vertices.append((math.cos(angle) * d_val, math.sin(angle) * w_val, z))
    faces = []
    for ring in range(len(rings) - 1):
        for side in range(segments):
            nxt = (side + 1) % segments
            a = ring * segments + side
            b = ring * segments + nxt
            c = (ring + 1) * segments + nxt
            d = (ring + 1) * segments + side
            faces.append((a, b, c, d))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate()
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.data.materials.append(mat)
    solidify = obj.modifiers.new("Fabric thickness", "SOLIDIFY")
    solidify.thickness = thickness
    solidify.offset = 0.0
    bevel = obj.modifiers.new("Soft hems", "BEVEL")
    bevel.width = 0.004
    bevel.segments = 2
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return obj


def shoe_ring_mesh(name, y_center, mat, collection, sole=False):
    x_values = (-0.135, -0.115, -0.075, -0.020, 0.045, 0.095, 0.130, 0.148)
    widths = (0.038, 0.048, 0.046, 0.044, 0.054, 0.056, 0.050, 0.034)
    vertices = []
    faces = []
    if sole:
        segments = 16
        for station, x_value in enumerate(x_values):
            toe_lift = max(0.0, x_value - 0.095) * 0.25
            arch = 0.008 * max(0.0, 1.0 - abs(x_value + 0.025) / 0.055)
            for side in range(segments):
                angle = math.tau * side / segments
                sine = math.sin(angle)
                z = 0.018 + sine * 0.018 + toe_lift
                if sine < 0.0:
                    z += arch * -sine
                vertices.append((x_value, y_center + math.cos(angle) * widths[station], z))
    else:
        segments = 13
        heights = (0.075, 0.090, 0.082, 0.068, 0.055, 0.047, 0.041, 0.034)
        for station, x_value in enumerate(x_values):
            for side in range(segments):
                alpha = side / (segments - 1)
                angle = math.pi - math.pi * alpha
                cosine = math.cos(angle)
                sine = max(0.0, math.sin(angle))
                y = y_center + math.copysign(abs(cosine) ** 0.76, cosine) * widths[station] * 0.94
                z = 0.030 + (sine ** 0.66) * (heights[station] - 0.030)
                vertices.append((x_value, y, z))
    for station in range(len(x_values) - 1):
        for side in range(segments - (0 if sole else 1)):
            nxt = (side + 1) % segments
            a = station * segments + side
            b = station * segments + nxt
            c = (station + 1) * segments + nxt
            d = (station + 1) * segments + side
            faces.append((a, b, c, d))
    faces.append(tuple(reversed(range(segments))))
    last_ring = (len(x_values) - 1) * segments
    faces.append(tuple(last_ring + side for side in range(segments)))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate()
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.data.materials.append(mat)
    bevel = obj.modifiers.new("Moulded edge", "BEVEL")
    bevel.width = 0.0025 if sole else 0.0035
    bevel.segments = 2
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return obj


def build_shoe(side_name, y_center, collection, mats):
    objects = []
    objects.append(shoe_ring_mesh(side_name + "_Sole", y_center, mats["sole"], collection, True))
    objects.append(shoe_ring_mesh(side_name + "_Upper", y_center, mats["upper"], collection, False))
    objects.append(rounded_box(
        side_name + "_Tongue", (-0.020, y_center, 0.079),
        (0.090, 0.058, 0.012), mats["upper2"], collection, 0.006))
    objects[-1].rotation_euler[1] = math.radians(-14.0)
    vamp = rounded_box(
        side_name + "_LayeredVamp", (0.078, y_center, 0.053),
        (0.078, 0.074, 0.007), mats["upper2"], collection, 0.008)
    vamp.rotation_euler[1] = math.radians(-8.0)
    objects.append(vamp)
    strap = rounded_box(
        side_name + "_InstepStrap", (0.035, y_center, 0.064),
        (0.052, 0.108, 0.008), mats["accent"], collection, 0.005)
    strap.rotation_euler[1] = math.radians(-7.0)
    objects.append(strap)
    objects.append(rounded_box(
        side_name + "_ToeBumper", (0.144, y_center, 0.027),
        (0.018, 0.068, 0.032), mats["panel"], collection, 0.007))
    objects.append(rounded_box(
        side_name + "_HeelClip", (-0.120, y_center, 0.055),
        (0.020, 0.086, 0.050), mats["accent"], collection, 0.006))
    for lace_index, x_value in enumerate((-0.060, -0.040, -0.020, 0.000)):
        width = 0.030 + lace_index * 0.002
        objects.append(cylinder_between(
            side_name + "_Lace_%02d" % lace_index,
            (x_value, y_center - width, 0.086 - lace_index * 0.004),
            (x_value, y_center + width, 0.086 - lace_index * 0.004),
            0.0018, mats["lace"], collection, 8))
    for flank in (-1.0, 1.0):
        quarter_panel = rounded_box(
            side_name + ("_OuterQuarterPanel" if flank > 0 else "_InnerQuarterPanel"),
            (-0.020, y_center + flank * 0.045, 0.058),
            (0.112, 0.004, 0.018), mats["accent2"], collection, 0.003)
        quarter_panel.rotation_euler[1] = math.radians(-14.0)
        objects.append(quarter_panel)
        objects.append(rounded_box(
            side_name + ("_StrapTabOuter" if flank > 0 else "_StrapTabInner"),
            (0.035, y_center + flank * 0.061, 0.079),
            (0.048, 0.010, 0.016), mats["accent2"], collection, 0.003))
    tread_stations = (-0.120, -0.082, -0.042, 0.000, 0.044, 0.086, 0.124)
    for tread_index, x_value in enumerate(tread_stations):
        width = 0.022 + 0.007 * math.sin(
            math.pi * tread_index / (len(tread_stations) - 1))
        toe_lift = max(0.0, x_value - 0.095) * 0.25
        arch_lift = 0.008 * max(0.0, 1.0 - abs(x_value + 0.025) / 0.055)
        for flank in (-1.0, 1.0):
            tread = rounded_box(
                side_name + "_Tread_%02d_%s" % (
                    tread_index, "L" if flank < 0 else "R"),
                (x_value, y_center + flank * width,
                 toe_lift + arch_lift - 0.003),
                (0.018, 0.016, 0.006), mats["outsole"], collection, 0.0015)
            tread.rotation_euler[2] = math.radians(flank * (10.0 if tread_index % 2 else -10.0))
            objects.append(tread)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.030, minor_radius=0.006, major_segments=20,
        minor_segments=8, location=(-0.090, y_center, 0.092))
    collar = bpy.context.object
    collar.name = side_name + "_Collar"
    collar.scale.y = 1.20
    objects.append(finish_object(collar, mats["lining"], collection))
    return objects


def build_hoodie(collection, mats):
    objects = [loft_shell(
        "SW_OversizedHoodie_Body",
        ((1.08, 0.18, 0.28), (1.18, 0.19, 0.30),
         (1.48, 0.18, 0.31), (1.64, 0.16, 0.25)),
        mats["hoodie"], collection, thickness=0.008)]
    for side in (-1.0, 1.0):
        objects.append(cylinder_between(
            "SW_OversizedHoodie_Sleeve_%s" % ("L" if side < 0 else "R"),
            (0.0, side * 0.28, 1.55), (0.02, side * 0.43, 1.12),
            0.095, mats["hoodie"], collection, 20))
        objects.append(rounded_box(
            "SW_OversizedHoodie_Cuff_%s" % ("L" if side < 0 else "R"),
            (0.02, side * 0.43, 1.10), (0.12, 0.10, 0.075),
            mats["rib"], collection, 0.018))
    objects.append(rounded_box(
        "SW_OversizedHoodie_KangarooPocket", (0.181, 0.0, 1.23),
        (0.035, 0.35, 0.17), mats["hoodie2"], collection, 0.022))
    objects.append(rounded_box(
        "SW_OversizedHoodie_Hem", (0.0, 0.0, 1.09),
        (0.36, 0.56, 0.060), mats["rib"], collection, 0.020))
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.16, minor_radius=0.038, major_segments=28,
        minor_segments=10, location=(-0.045, 0.0, 1.68),
        rotation=(0.0, math.radians(90.0), 0.0))
    hood = bpy.context.object
    hood.name = "SW_OversizedHoodie_Hood"
    hood.scale.z = 1.16
    objects.append(finish_object(hood, mats["hoodie2"], collection))
    for side in (-1.0, 1.0):
        objects.append(cylinder_between(
            "SW_OversizedHoodie_Drawcord_%s" % ("L" if side < 0 else "R"),
            (0.19, side * 0.055, 1.66), (0.20, side * 0.070, 1.48),
            0.003, mats["lace"], collection, 8))
    return objects


def build_bomber(collection, mats):
    objects = [loft_shell(
        "SW_TechBomber_Body",
        ((1.12, 0.17, 0.27), (1.22, 0.20, 0.30),
         (1.48, 0.20, 0.31), (1.63, 0.16, 0.24)),
        mats["bomber"], collection, thickness=0.009)]
    for side in (-1.0, 1.0):
        objects.append(cylinder_between(
            "SW_TechBomber_Sleeve_%s" % ("L" if side < 0 else "R"),
            (0.0, side * 0.27, 1.54), (0.01, side * 0.44, 1.10),
            0.090, mats["bomber2"], collection, 20))
        objects.append(rounded_box(
            "SW_TechBomber_Cuff_%s" % ("L" if side < 0 else "R"),
            (0.01, side * 0.44, 1.09), (0.11, 0.095, 0.065),
            mats["rib2"], collection, 0.016))
    objects.append(rounded_box(
        "SW_TechBomber_Hem", (0.0, 0.0, 1.13),
        (0.36, 0.56, 0.065), mats["rib2"], collection, 0.018))
    objects.append(rounded_box(
        "SW_TechBomber_Zipper", (0.204, 0.0, 1.39),
        (0.008, 0.012, 0.49), mats["metal"], collection, 0.002))
    for side in (-1.0, 1.0):
        pocket = rounded_box(
            "SW_TechBomber_Pocket_%s" % ("L" if side < 0 else "R"),
            (0.196, side * 0.16, 1.29), (0.020, 0.16, 0.075),
            mats["bomber2"], collection, 0.010)
        pocket.rotation_euler[0] = math.radians(side * 12.0)
        objects.append(pocket)
    return objects


def build_cargo_joggers(collection, mats):
    objects = []
    waist = loft_shell(
        "SW_CargoJoggers_Waist",
        ((0.98, 0.145, 0.23), (1.12, 0.16, 0.25)),
        mats["cargo"], collection, thickness=0.009)
    objects.append(waist)
    objects.append(rounded_box(
        "SW_CargoJoggers_Waistband", (0.0, 0.0, 1.10),
        (0.30, 0.48, 0.070), mats["rib"], collection, 0.020))
    for side in (-1.0, 1.0):
        y = side * 0.12
        objects.append(cylinder_between(
            "SW_CargoJoggers_Leg_%s" % ("L" if side < 0 else "R"),
            (0.0, y, 1.02), (0.0, y * 0.90, 0.14),
            0.115, mats["cargo"], collection, 20))
        objects.append(rounded_box(
            "SW_CargoJoggers_Pocket_%s" % ("L" if side < 0 else "R"),
            (0.115, y, 0.69), (0.055, 0.20, 0.22),
            mats["cargo2"], collection, 0.015))
        objects.append(rounded_box(
            "SW_CargoJoggers_Cuff_%s" % ("L" if side < 0 else "R"),
            (0.0, y * 0.90, 0.12), (0.22, 0.20, 0.080),
            mats["rib"], collection, 0.020))
    return objects


def build_beanie(collection, mats):
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=28, ring_count=16, location=(0.0, 0.0, 1.93),
        scale=(0.145, 0.145, 0.16))
    crown = bpy.context.object
    crown.name = "SW_RibBeanie_Crown"
    objects = [finish_object(crown, mats["beanie"], collection)]
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.135, minor_radius=0.035, major_segments=28,
        minor_segments=10, location=(0.0, 0.0, 1.84))
    cuff = bpy.context.object
    cuff.name = "SW_RibBeanie_Cuff"
    objects.append(finish_object(cuff, mats["beanie2"], collection))
    return objects


def move_group(objects, offset):
    for obj in objects:
        obj.location += Vector(offset)


def export_group(objects, filename, preview_offset=(0.0, 0.0, 0.0)):
    offset = Vector(preview_offset)
    for obj in objects:
        obj.location -= offset
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    path = os.path.join(EXPORT_DIR, filename + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"},
        apply_unit_scale=True, global_scale=1.0, bake_space_transform=True,
        use_mesh_modifiers=True, colors_type="SRGB", add_leaf_bones=False,
        path_mode="AUTO", mesh_smooth_type="FACE")
    for obj in objects:
        obj.location += offset
    return path


def look_at(camera, target):
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()


def main():
    os.makedirs(SOURCE_DIR, exist_ok=True)
    os.makedirs(EXPORT_DIR, exist_ok=True)
    # Keep the active Blender workspace/console alive while replacing only the
    # scene contents; factory reset also destroys the console that launched us.
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for collection in tuple(bpy.data.collections):
        if collection.name != "Collection":
            bpy.data.collections.remove(collection)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = PREVIEW_PATH
    if scene.world is None:
        scene.world = bpy.data.worlds.new("Streetwear_Review_World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (
        0.018, 0.022, 0.032, 1.0)

    mats = {
        "upper": material("M_Shoe_Navy", (0.025, 0.055, 0.12), 0.02, 0.46, True),
        "upper2": material("M_Shoe_Knit", (0.045, 0.09, 0.16), 0.0, 0.68, True),
        "sole": material("M_Shoe_Midsole", (0.72, 0.86, 0.84), 0.0, 0.54),
        "outsole": material("M_Shoe_Outsole", (0.018, 0.30, 0.28), 0.0, 0.72),
        "accent": material("M_Shoe_Teal", (0.07, 0.64, 0.57), 0.02, 0.56, True),
        "accent2": material("M_Shoe_DarkTeal", (0.025, 0.24, 0.24), 0.02, 0.62),
        "panel": material("M_Shoe_CloudPanel", (0.78, 0.86, 0.88), 0.0, 0.44),
        "lace": material("M_Shoe_Lace", (0.88, 0.90, 0.88), 0.0, 0.72),
        "lining": material("M_Shoe_Lining", (0.018, 0.022, 0.03), 0.0, 0.82),
        "hoodie": material("M_Hoodie_Charcoal", (0.055, 0.065, 0.08), 0.0, 0.78),
        "hoodie2": material("M_Hoodie_Navy", (0.03, 0.07, 0.13), 0.0, 0.72),
        "bomber": material("M_Bomber_Plum", (0.18, 0.035, 0.19), 0.03, 0.46),
        "bomber2": material("M_Bomber_Black", (0.025, 0.025, 0.032), 0.02, 0.55),
        "cargo": material("M_Cargo_Olive", (0.13, 0.16, 0.09), 0.0, 0.80),
        "cargo2": material("M_Cargo_Dark", (0.055, 0.07, 0.045), 0.0, 0.76),
        "rib": material("M_Rib_Charcoal", (0.035, 0.04, 0.05), 0.0, 0.88),
        "rib2": material("M_Rib_Plum", (0.09, 0.02, 0.10), 0.0, 0.84),
        "beanie": material("M_Beanie_Rust", (0.62, 0.16, 0.055), 0.0, 0.82),
        "beanie2": material("M_Beanie_Cuff", (0.33, 0.07, 0.035), 0.0, 0.86),
        "metal": material("M_Zipper_Metal", (0.28, 0.31, 0.34), 0.80, 0.20),
    }

    shoes_collection = bpy.data.collections.new("01_ZarriVelocity_Shoes")
    hoodie_collection = bpy.data.collections.new("02_Oversized_Hoodie")
    bomber_collection = bpy.data.collections.new("03_Tech_Bomber")
    cargo_collection = bpy.data.collections.new("04_Cargo_Joggers")
    beanie_collection = bpy.data.collections.new("05_Rib_Beanie")
    for collection in (shoes_collection, hoodie_collection, bomber_collection,
                       cargo_collection, beanie_collection):
        scene.collection.children.link(collection)

    shoes = build_shoe("SM_ZarriVelocity_L", -0.068, shoes_collection, mats)
    shoes += build_shoe("SM_ZarriVelocity_R", 0.068, shoes_collection, mats)
    hoodie = build_hoodie(hoodie_collection, mats)
    bomber = build_bomber(bomber_collection, mats)
    cargo = build_cargo_joggers(cargo_collection, mats)
    beanie = build_beanie(beanie_collection, mats)

    preview_offsets = {
        "hoodie": (-1.25, 0.0, 0.0),
        "cargo": (0.0, 0.0, 0.0),
        "bomber": (1.25, 0.0, 0.0),
        "beanie": (1.25, 0.0, 0.0),
        "shoes": (0.0, -0.55, 0.03),
    }
    move_group(hoodie, preview_offsets["hoodie"])
    move_group(cargo, preview_offsets["cargo"])
    move_group(bomber, preview_offsets["bomber"])
    move_group(beanie, preview_offsets["beanie"])
    move_group(shoes, preview_offsets["shoes"])

    exports = [
        export_group(shoes, "SM_ZarriVelocity_ShoePair", preview_offsets["shoes"]),
        export_group(hoodie, "SM_Streetwear_OversizedHoodie", preview_offsets["hoodie"]),
        export_group(bomber, "SM_Streetwear_TechBomber", preview_offsets["bomber"]),
        export_group(cargo, "SM_Streetwear_CargoJoggers", preview_offsets["cargo"]),
        export_group(beanie, "SM_Streetwear_RibBeanie", preview_offsets["beanie"]),
    ]

    display = bpy.data.collections.new("90_Preview_Environment")
    scene.collection.children.link(display)
    rounded_box("Preview_Plinth", (0.0, 0.15, 0.025), (3.7, 1.2, 0.05),
                material("M_Plinth", (0.04, 0.045, 0.055), 0.0, 0.48), display, 0.025)
    bpy.ops.object.light_add(type="AREA", location=(-2.5, -3.0, 4.5))
    key = bpy.context.object
    key.name = "Key_Light"
    key.data.energy = 1300
    key.data.shape = "DISK"
    key.data.size = 4.0
    look_at(key, (0.0, 0.0, 1.0))
    bpy.ops.object.light_add(type="AREA", location=(3.0, 1.0, 3.0))
    fill = bpy.context.object
    fill.name = "Teal_Rim"
    fill.data.energy = 900
    fill.data.color = (0.08, 0.65, 0.52)
    fill.data.size = 3.0
    look_at(fill, (0.0, 0.0, 1.1))
    bpy.ops.object.camera_add(location=(4.2, -6.8, 3.0))
    camera = bpy.context.object
    camera.name = "Streetwear_Review_Camera"
    camera.data.lens = 58
    look_at(camera, (0.0, 0.0, 1.0))
    scene.camera = camera

    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
    bpy.ops.render.render(write_still=True)
    camera.location = (0.72, -1.38, 0.56)
    camera.data.lens = 68
    look_at(camera, (0.0, -0.55, 0.075))
    scene.render.filepath = SHOE_PREVIEW_PATH
    scene.render.resolution_x = 1100
    scene.render.resolution_y = 700
    bpy.ops.render.render(write_still=True)
    triangles = sum(
        len(p.vertices) - 2 for obj in scene.objects if obj.type == "MESH"
        for p in obj.data.polygons)
    with open(REPORT_PATH, "w", encoding="utf-8") as report:
        report.write("Connected Sprawl Streetwear Kit\n")
        report.write("Editable source: %s\n" % BLEND_PATH)
        report.write("Preview: %s\n" % PREVIEW_PATH)
        report.write("Shoe preview: %s\n" % SHOE_PREVIEW_PATH)
        report.write("Mesh objects: %d\n" % sum(o.type == "MESH" for o in scene.objects))
        report.write("Base triangles before modifiers: %d\n" % triangles)
        report.write("Exports:\n")
        for path in exports:
            report.write("- %s\n" % path)
    print("[StreetwearKit] saved", BLEND_PATH)
    print("[StreetwearKit] rendered", PREVIEW_PATH)
    for path in exports:
        print("[StreetwearKit] exported", path)


main()
