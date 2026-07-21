"""Author the authentic-city kit in headless Blender.

Bakes three seamless 1K PBR texture sets (worn asphalt street, jointed
concrete quay, patchy grass field) and models two meshes (a bent multi-blade
grass clump with vertex colours, and a centred, finely tessellated water
surface so the GPU wave displacement actually ripples). Everything lands in
Content/Import/CityKit/ for import_city_kit.py to pick up.

Seamlessness: every noise/voronoi lookup is fed 4D torus-embedded UV
coordinates (sin/cos of u and v), so the bake tiles perfectly in both axes —
required because M_RealGround projects these maps in world space across the
whole city floor. Concrete expansion joints use fract()-based stripes, which
tile by construction.

Run (deterministic, ~1 min):
  /opt/homebrew/bin/blender --background --python Tools/build_city_kit.py
"""
import math
import os
import random

import bpy

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, "Content", "Import", "CityKit")
TEX_DIR = os.path.join(OUT_DIR, "Textures")
SIZE = 1024
SEED = 20260720

lines = []


def out(text=""):
    lines.append(text)
    print("[CityKit] " + text)


# ---------------------------------------------------------------- scene ----
def reset_scene():
    bpy.ops.wm.read_homefile(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 16
    scene.render.bake.margin = 16
    scene.render.bake.use_selected_to_active = False


def new_image(name, is_data):
    img = bpy.data.images.new(name, SIZE, SIZE, alpha=False, float_buffer=False)
    if is_data:
        img.colorspace_settings.name = "Non-Color"
    return img


def save_image(img, name):
    os.makedirs(TEX_DIR, exist_ok=True)
    path = os.path.join(TEX_DIR, name + ".png")
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    out("baked %s.png" % name)


# ------------------------------------------------------------ node soup ----
def add_torus_coords(nt):
    """UV -> seamless 4D torus embedding: Vector=(sin u, cos u, sin v), W=cos v."""
    uv = nt.nodes.new("ShaderNodeTexCoord")
    sep = nt.nodes.new("ShaderNodeSeparateXYZ")
    nt.links.new(uv.outputs["UV"], sep.inputs["Vector"])

    def trig(src, op):
        mul = nt.nodes.new("ShaderNodeMath")
        mul.operation = "MULTIPLY"
        mul.inputs[1].default_value = 2.0 * math.pi
        nt.links.new(src, mul.inputs[0])
        fn = nt.nodes.new("ShaderNodeMath")
        fn.operation = op
        nt.links.new(mul.outputs[0], fn.inputs[0])
        return fn.outputs[0]

    su = trig(sep.outputs["X"], "SINE")
    cu = trig(sep.outputs["X"], "COSINE")
    sv = trig(sep.outputs["Y"], "SINE")
    cv = trig(sep.outputs["Y"], "COSINE")
    comb = nt.nodes.new("ShaderNodeCombineXYZ")
    nt.links.new(su, comb.inputs["X"])
    nt.links.new(cu, comb.inputs["Y"])
    nt.links.new(sv, comb.inputs["Z"])
    return comb.outputs["Vector"], cv, sep.outputs


def add_noise(nt, vec, w, scale, detail=2.0):
    n = nt.nodes.new("ShaderNodeTexNoise")
    n.noise_dimensions = "4D"
    n.inputs["Scale"].default_value = scale
    n.inputs["Detail"].default_value = detail
    nt.links.new(vec, n.inputs["Vector"])
    nt.links.new(w, n.inputs["W"])
    return n.outputs["Fac"]


def add_crack_lines(nt, vec, w, scale):
    """Voronoi distance-to-edge -> thin dark crack mask (1 inside a crack)."""
    v = nt.nodes.new("ShaderNodeTexVoronoi")
    v.voronoi_dimensions = "4D"
    v.feature = "DISTANCE_TO_EDGE"
    v.inputs["Scale"].default_value = scale
    nt.links.new(vec, v.inputs["Vector"])
    nt.links.new(w, v.inputs["W"])
    lt = nt.nodes.new("ShaderNodeMath")
    lt.operation = "LESS_THAN"
    lt.inputs[1].default_value = 0.012
    nt.links.new(v.outputs["Distance"], lt.inputs[0])
    return lt.outputs[0]


def add_joint_stripes(nt, uv_sep, period=0.5, width=0.02):
    """Grooved expansion-joint mask along both UV axes (tiles by construction)."""
    def axis_groove(src):
        mul = nt.nodes.new("ShaderNodeMath")
        mul.operation = "MULTIPLY"
        mul.inputs[1].default_value = 1.0 / period
        nt.links.new(src, mul.inputs[0])
        fract = nt.nodes.new("ShaderNodeMath")
        fract.operation = "FRACT"
        nt.links.new(mul.outputs[0], fract.inputs[0])
        # distance to the nearest stripe centre: min(f, 1-f)
        inv = nt.nodes.new("ShaderNodeMath")
        inv.operation = "SUBTRACT"
        inv.inputs[0].default_value = 1.0
        nt.links.new(fract.outputs[0], inv.inputs[1])
        mn = nt.nodes.new("ShaderNodeMath")
        mn.operation = "MINIMUM"
        nt.links.new(fract.outputs[0], mn.inputs[0])
        nt.links.new(inv.outputs[0], mn.inputs[1])
        lt = nt.nodes.new("ShaderNodeMath")
        lt.operation = "LESS_THAN"
        lt.inputs[1].default_value = width
        nt.links.new(mn.outputs[0], lt.inputs[0])
        return lt.outputs[0]

    gu = axis_groove(uv_sep["X"])
    gv = axis_groove(uv_sep["Y"])
    mx = nt.nodes.new("ShaderNodeMath")
    mx.operation = "MAXIMUM"
    nt.links.new(gu, mx.inputs[0])
    nt.links.new(gv, mx.inputs[1])
    return mx.outputs[0]


def mix_colors(nt, fac, col_a, col_b):
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    nt.links.new(fac, mix.inputs["Factor"])
    mix.inputs["A"].default_value = (*col_a, 1.0)
    mix.inputs["B"].default_value = (*col_b, 1.0)
    return mix.outputs["Result"]


def map_range(nt, src, to_min, to_max):
    mr = nt.nodes.new("ShaderNodeMapRange")
    mr.inputs["To Min"].default_value = to_min
    mr.inputs["To Max"].default_value = to_max
    nt.links.new(src, mr.inputs["Value"])
    return mr.outputs["Result"]


# ------------------------------------------------------------- surfaces ----
def build_surface_fields(nt, kind):
    """Return (color_socket, roughness_socket, height_socket) for a kind."""
    vec, w, uv_sep = add_torus_coords(nt)

    if kind == "asphalt":
        mottle = add_noise(nt, vec, w, 5.0, 3.0)
        speckle = add_noise(nt, vec, w, 60.0, 2.0)
        patches = add_noise(nt, vec, w, 1.6, 2.0)
        crack = add_crack_lines(nt, vec, w, 3.0)
        # only some regions are cracked: gate the crack lines by broad noise
        crack_region = nt.nodes.new("ShaderNodeMath")
        crack_region.operation = "GREATER_THAN"
        crack_region.inputs[1].default_value = 0.52
        nt.links.new(add_noise(nt, vec, w, 1.1, 1.0), crack_region.inputs[0])
        crack_mask = nt.nodes.new("ShaderNodeMath")
        crack_mask.operation = "MULTIPLY"
        nt.links.new(crack.outputs[0] if hasattr(crack, "outputs") else crack,
                     crack_mask.inputs[0])
        nt.links.new(crack_region.outputs[0], crack_mask.inputs[1])
        crack = crack_mask.outputs[0]

        base = mix_colors(nt, mottle, (0.030, 0.030, 0.033), (0.048, 0.048, 0.050))
        patched = nt.nodes.new("ShaderNodeMix")
        patched.data_type = "RGBA"
        nt.links.new(map_range(nt, patches, 0.0, 0.35), patched.inputs["Factor"])
        nt.links.new(base, patched.inputs["A"])
        patched.inputs["B"].default_value = (0.020, 0.020, 0.023, 1.0)
        lit = nt.nodes.new("ShaderNodeMix")
        lit.data_type = "RGBA"
        nt.links.new(map_range(nt, speckle, 0.0, 0.30), lit.inputs["Factor"])
        nt.links.new(patched.outputs["Result"], lit.inputs["A"])
        lit.inputs["B"].default_value = (0.075, 0.075, 0.078, 1.0)
        color = nt.nodes.new("ShaderNodeMix")
        color.data_type = "RGBA"
        nt.links.new(crack, color.inputs["Factor"])
        nt.links.new(lit.outputs["Result"], color.inputs["A"])
        color.inputs["B"].default_value = (0.012, 0.012, 0.014, 1.0)

        rough = map_range(nt, speckle, 0.92, 0.78)
        # height: speckle aggregate up, cracks carved down
        hs = nt.nodes.new("ShaderNodeMath")
        hs.operation = "MULTIPLY_ADD"
        nt.links.new(speckle, hs.inputs[0])
        hs.inputs[1].default_value = 0.6
        nt.links.new(map_range(nt, mottle, 0.0, 0.4), hs.inputs[2])
        height = nt.nodes.new("ShaderNodeMath")
        height.operation = "SUBTRACT"
        nt.links.new(hs.outputs[0], height.inputs[0])
        nt.links.new(crack, height.inputs[1])
        return color.outputs["Result"], rough, height.outputs[0]

    if kind == "concrete":
        mottle = add_noise(nt, vec, w, 4.0, 3.0)
        speckle = add_noise(nt, vec, w, 80.0, 2.0)
        stain = add_noise(nt, vec, w, 1.3, 2.0)
        joints = add_joint_stripes(nt, uv_sep, period=0.5, width=0.015)

        base = mix_colors(nt, mottle, (0.235, 0.230, 0.215), (0.300, 0.295, 0.278))
        stained = nt.nodes.new("ShaderNodeMix")
        stained.data_type = "RGBA"
        nt.links.new(map_range(nt, stain, 0.0, 0.30), stained.inputs["Factor"])
        nt.links.new(base, stained.inputs["A"])
        stained.inputs["B"].default_value = (0.165, 0.160, 0.148, 1.0)
        color = nt.nodes.new("ShaderNodeMix")
        color.data_type = "RGBA"
        nt.links.new(joints, color.inputs["Factor"])
        nt.links.new(stained.outputs["Result"], color.inputs["A"])
        color.inputs["B"].default_value = (0.120, 0.118, 0.110, 1.0)

        rough_base = map_range(nt, speckle, 0.68, 0.55)
        rough = nt.nodes.new("ShaderNodeMix")
        rough.data_type = "FLOAT"
        nt.links.new(joints, rough.inputs["Factor"])
        nt.links.new(rough_base, rough.inputs[2])  # Float A
        rough.inputs[3].default_value = 0.8        # Float B
        height = nt.nodes.new("ShaderNodeMath")
        height.operation = "SUBTRACT"
        nt.links.new(map_range(nt, speckle, 0.0, 0.25), height.inputs[0])
        nt.links.new(joints, height.inputs[1])
        return color.outputs["Result"], rough.outputs["Result"], height.outputs[0]

    # grass field
    broad = add_noise(nt, vec, w, 2.0, 2.0)
    fine = add_noise(nt, vec, w, 45.0, 3.0)
    dry = add_noise(nt, vec, w, 1.2, 2.0)
    base = mix_colors(nt, fine, (0.055, 0.110, 0.030), (0.110, 0.190, 0.052))
    shade = nt.nodes.new("ShaderNodeMix")
    shade.data_type = "RGBA"
    nt.links.new(map_range(nt, broad, 0.0, 0.5), shade.inputs["Factor"])
    nt.links.new(base, shade.inputs["A"])
    shade.inputs["B"].default_value = (0.040, 0.085, 0.022, 1.0)
    dry_gate = nt.nodes.new("ShaderNodeMath")
    dry_gate.operation = "GREATER_THAN"
    dry_gate.inputs[1].default_value = 0.62
    nt.links.new(dry, dry_gate.inputs[0])
    color = nt.nodes.new("ShaderNodeMix")
    color.data_type = "RGBA"
    nt.links.new(dry_gate.outputs[0], color.inputs["Factor"])
    nt.links.new(shade.outputs["Result"], color.inputs["A"])
    color.inputs["B"].default_value = (0.150, 0.140, 0.060, 1.0)
    rough = map_range(nt, fine, 0.90, 0.80)
    return color.outputs["Result"], rough, fine


def bake_set(kind, prefix):
    """Bake color/roughness/normal for one surface kind onto the unit plane."""
    bpy.ops.mesh.primitive_plane_add(size=2)
    plane = bpy.context.active_object
    mat = bpy.data.materials.new(prefix + "_mat")
    mat.use_nodes = True
    plane.data.materials.append(mat)

    def rebuild(mode):
        nt = mat.node_tree
        nt.nodes.clear()
        out_node = nt.nodes.new("ShaderNodeOutputMaterial")
        color, rough, height = build_surface_fields(nt, kind)
        if mode == "NORMAL":
            bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
            bump = nt.nodes.new("ShaderNodeBump")
            bump.inputs["Strength"].default_value = 0.35
            nt.links.new(height, bump.inputs["Height"])
            nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
            nt.links.new(bsdf.outputs["BSDF"], out_node.inputs["Surface"])
        else:
            emit = nt.nodes.new("ShaderNodeEmission")
            src = color if mode == "COLOR" else rough
            nt.links.new(src, emit.inputs["Color"])
            nt.links.new(emit.outputs["Emission"], out_node.inputs["Surface"])
        img_node = nt.nodes.new("ShaderNodeTexImage")
        img = new_image(prefix + "_" + mode, is_data=(mode != "COLOR"))
        img_node.image = img
        nt.nodes.active = img_node
        return img

    bpy.ops.object.select_all(action="DESELECT")
    plane.select_set(True)
    bpy.context.view_layer.objects.active = plane

    img = rebuild("COLOR")
    bpy.context.scene.cycles.samples = 1
    bpy.ops.object.bake(type="EMIT")
    save_image(img, prefix + "_Color")

    img = rebuild("ROUGH")
    bpy.ops.object.bake(type="EMIT")
    save_image(img, prefix + "_Roughness")

    img = rebuild("NORMAL")
    bpy.context.scene.cycles.samples = 16
    bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT")
    save_image(img, prefix + "_NormalGL")

    bpy.data.objects.remove(plane, do_unlink=True)


# --------------------------------------------------------------- meshes ----
def build_grass_clump():
    """14 bent, tapered, solid blades with a root->tip vertex-colour ramp."""
    rand = random.Random(SEED)
    verts, faces, vcols = [], [], []

    def ring(center, direction, width, depth):
        """Triangular cross-section ring perpendicular to the blade axis."""
        ax = direction.normalized() if hasattr(direction, "normalized") else None
        # simple fixed-frame ribbon: orient by yaw only, close enough for grass
        return [
            (center[0] + width, center[1], center[2]),
            (center[0] - width * 0.5, center[1] + depth, center[2]),
            (center[0] - width * 0.5, center[1] - depth, center[2]),
        ]

    for b in range(14):
        yaw = rand.uniform(0.0, 2.0 * math.pi)
        lean = rand.uniform(0.10, 0.45)
        lean_dir = rand.uniform(0.0, 2.0 * math.pi)
        h = rand.uniform(0.16, 0.36)             # metres
        wroot = rand.uniform(0.008, 0.014)
        r = rand.uniform(0.0, 0.075)
        bx, by = math.cos(yaw) * r, math.sin(yaw) * r
        lx, ly = math.cos(lean_dir), math.sin(lean_dir)

        def pos(t):
            drift = math.sin(lean * t * 1.4) * h * t
            return (bx + lx * drift, by + ly * drift, h * t)

        g = rand.uniform(0.85, 1.2)
        c_root = (0.045 * g, 0.110 * g, 0.028 * g, 1.0)
        c_mid = (0.085 * g, 0.190 * g, 0.045 * g, 1.0)
        c_tip = (0.200 * g, 0.330 * g, 0.075 * g, 1.0)

        base_i = len(verts)
        for i, (t, wf) in enumerate(((0.0, 1.0), (0.55, 0.62))):
            p = pos(t)
            for v in ring(p, None, wroot * wf, wroot * 0.35 * wf):
                verts.append(v)
        tip = pos(1.0)
        verts.append(tip)
        tip_i = base_i + 6

        for k in range(3):
            a, b2 = base_i + k, base_i + (k + 1) % 3
            c, d = base_i + 3 + k, base_i + 3 + (k + 1) % 3
            faces.append((a, b2, d, c))
            vcols.append((c_root, c_root, c_mid, c_mid))
        for k in range(3):
            c, d = base_i + 3 + k, base_i + 3 + (k + 1) % 3
            faces.append((c, d, tip_i))
            vcols.append((c_mid, c_mid, c_tip))

    mesh = bpy.data.meshes.new("SM_GrassClump")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    attr = mesh.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    loop_i = 0
    for face, cols in zip(mesh.polygons, vcols):
        for li in range(face.loop_total):
            attr.data[loop_i].color = cols[li]
            loop_i += 1
    obj = bpy.data.objects.new("SM_GrassClump", mesh)
    bpy.context.collection.objects.link(obj)
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    out("SM_GrassClump: %d blades, %d tris, height<=0.36m" % (14, tris))
    return obj


def build_water_surface():
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=120, y_subdivisions=80, size=2)
    obj = bpy.context.active_object
    obj.name = "SM_WaterSurface"
    obj.data.name = "SM_WaterSurface"
    obj.scale = (48.0, 32.0, 1.0)              # -> 96 x 64 metres, centred
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    out("SM_WaterSurface: 96x64m, %d tris, centred pivot" % tris)
    return obj


def export_fbx(obj, name):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUT_DIR, name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"},
        apply_unit_scale=True, global_scale=1.0, bake_space_transform=True,
        use_mesh_modifiers=True, colors_type="SRGB", add_leaf_bones=False,
        path_mode="AUTO")
    out("exported %s.fbx (%.1f KB)" % (name, os.path.getsize(path) / 1024.0))


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    reset_scene()
    bake_set("asphalt", "T_CityAsphalt")
    bake_set("concrete", "T_CityConcrete")
    bake_set("grass", "T_CityGrassField")
    export_fbx(build_grass_clump(), "SM_GrassClump")
    export_fbx(build_water_surface(), "SM_WaterSurface")
    with open(os.path.join(OUT_DIR, "BlenderKitReport.txt"), "w") as fh:
        fh.write("\n".join(lines) + "\n")
    out("DONE")


main()
