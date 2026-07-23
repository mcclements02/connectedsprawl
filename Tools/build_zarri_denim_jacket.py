"""Author a realistic denim trucker jacket for Zarri on a CC0 human base.

Pipeline (headless Blender, deterministic):
  1. Append the CC0 realistic male base (Blender Studio Human Base Meshes,
     GEO-body_male_realistic), strip multires, recenter, scale to Zarri's
     ~177 cm build. Saved as the canonical fitting figure.
  2. Extract the jacket shell straight from the body's OWN faces (torso band +
     hanging-arm tubes), push it off the skin, relax it so it drapes loose.
  3. Trucker detailing: fold-over collar, centre button placket with COPPER
     buttons, two flap chest pockets, buttoned cuffs, waistband hem.
  4. TOPSTITCHING: contrasting tan thread lines run the placket, yoke, collar,
     cuffs, hem and pocket flaps (a separate thread material).
  5. WRINKLES: a noise Displace adds soft fabric folds; the weave gives micro
     detail. Solidify + Subdivision for thickness.
  6. Denim WASH: deep-indigo twill with a cloud fade and roughness variation.
  7. Render front/back on the figure; export FBX; save the source .blend.

Writes only NEW files under BaseMeshes/ and Denim/ -- never the existing
Streetwear/Clothing cloth kits.

Run:
  /opt/homebrew/bin/blender --background --python Tools/build_zarri_denim_jacket.py
"""
import math
import os

import bpy
import bmesh
from mathutils import Vector

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRATCH = ("/private/tmp/claude-501/-Users-matthewx-code-ConnectedSprawl/"
           "dc4ad07d-766d-4cdc-a9e3-4ce18bf21fc6/scratchpad")
BUNDLE = os.path.join(SCRATCH, "hbm",
                      "human-base-meshes-bundle-v1.4.1",
                      "human_base_meshes_bundle.blend")

BASE_DIR = os.path.join(ROOT, "Content", "MetaHumans", "Source", "BaseMeshes")
DENIM_DIR = os.path.join(ROOT, "Content", "MetaHumans", "Source", "Denim")
EXPORT_DIR = os.path.join(ROOT, "Content", "Import", "Characters", "Denim")
BASE_BLEND = os.path.join(BASE_DIR, "HumanBaseMale.blend")
JACKET_BLEND = os.path.join(DENIM_DIR, "ConnectedSprawl_DenimJacket.blend")
FBX_PATH = os.path.join(EXPORT_DIR, "SM_ZarriDenimJacket.fbx")
REPORT_PATH = os.path.join(DENIM_DIR, "DenimJacketReport.txt")

TARGET_HEIGHT = 1.775
DENIM_GAP = 0.016
LOOSENESS = 0.030

lines = []


def out(text=""):
    lines.append(text)
    print("[Denim] " + text)


# ---------------------------------------------------------------- base ----
def load_base():
    bpy.ops.wm.read_homefile(use_empty=True)
    with bpy.data.libraries.load(BUNDLE, link=False) as (src, dst):
        dst.objects = ["GEO-body_male_realistic"]
    body = dst.objects[0]
    body.name = "ZarriBaseBody"
    bpy.context.collection.objects.link(body)
    bpy.context.view_layer.objects.active = body
    body.select_set(True)
    for m in list(body.modifiers):
        body.modifiers.remove(m)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    co = [v.co for v in body.data.vertices]
    cx = sum(c.x for c in co) / len(co)
    cy = sum(c.y for c in co) / len(co)
    zmin = min(c.z for c in co)
    zmax = max(c.z for c in co)
    scale = TARGET_HEIGHT / (zmax - zmin)
    for v in body.data.vertices:
        v.co = Vector(((v.co.x - cx) * scale,
                       (v.co.y - cy) * scale,
                       (v.co.z - zmin) * scale))
    body.data.update()
    out("base: recentred, scaled x%.3f -> height %.3fm verts=%d"
        % (scale, TARGET_HEIGHT, len(body.data.vertices)))
    return body


def torso_center(co, z, band):
    trunk = [c for c in co if abs(c.z - z) < band
             and math.hypot(c.x, c.y) < 0.24]
    if not trunk:
        return 0.0, 0.0
    return (sum(c.x for c in trunk) / len(trunk),
            sum(c.y for c in trunk) / len(trunk))


# ------------------------------------------- extract shell from the body ----
def extract_jacket_shell(body):
    zmax = max(v.co.z for v in body.data.vertices)
    z_hem = zmax * 0.55
    z_shoulder = zmax * 0.845
    z_wrist = zmax * 0.475
    torso_half = 0.245
    arm_min = 0.205

    dup = body.copy()
    dup.data = body.data.copy()
    dup.name = "ZarriDenimJacket"
    bpy.context.collection.objects.link(dup)
    dup.data.materials.clear()

    bm = bmesh.new()
    bm.from_mesh(dup.data)
    bm.faces.ensure_lookup_table()
    kill = []
    for f in bm.faces:
        c = f.calc_center_median()
        ax = abs(c.x)
        is_torso = (z_hem <= c.z <= z_shoulder) and ax < torso_half
        is_arm = (z_wrist <= c.z <= z_shoulder) and ax >= arm_min
        if not (is_torso or is_arm):
            kill.append(f)
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    bm.normal_update()
    for v in bm.verts:
        v.co = v.co + v.normal * LOOSENESS
    bm.to_mesh(dup.data)
    bm.free()
    dup.data.update()
    # Relax the SHELL ONLY (before any details are added) so muscle detail reads
    # as loose cloth. Details attached later keep their crisp volume.
    sm = dup.modifiers.new("relax", "SMOOTH")
    sm.factor = 0.9
    sm.iterations = 14
    bpy.context.view_layer.objects.active = dup
    bpy.ops.object.modifier_apply(modifier="relax")
    out("shell: %d faces from body, off-skin %.0fmm, relaxed"
        % (len(dup.data.polygons), LOOSENESS * 1000))
    return dup, z_hem, z_shoulder, z_wrist


# --------------------------------------------------------------- helpers ----
def add_box(name, center, size):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    o = bpy.context.active_object
    o.name = name
    o.scale = Vector(size) * 0.5
    bpy.ops.object.transform_apply(scale=True)
    return o


def add_cylinder(name, center, radius, depth, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=radius, depth=depth,
                                        location=center, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    return o


def add_ring(name, center, radius, minor, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(location=center, rotation=rot,
                                     major_radius=radius, minor_radius=minor,
                                     major_segments=28, minor_segments=6)
    o = bpy.context.active_object
    o.name = name
    return o


def shrinkwrap_to(o, body, offset):
    sw = o.modifiers.new("fit", "SHRINKWRAP")
    sw.target = body
    sw.wrap_method = "NEAREST_SURFACEPOINT"
    sw.offset = offset
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.modifier_apply(modifier="fit")


def set_mat(o, mat):
    o.data.materials.clear()
    o.data.materials.append(mat)


# ----------------------------------------------------------- detail parts ----
def build_details(body, shell, z_hem, z_shoulder, z_wrist, denim, copper):
    co = [v.co for v in body.data.vertices]
    zmax = max(c.z for c in co)
    band = zmax * 0.02
    z_chest = (z_hem + z_shoulder) * 0.55
    cx, cy = torso_center(co, z_chest, band)
    front_y = min(c.y for c in co if abs(c.z - z_chest) < band
                  and math.hypot(c.x - cx, c.y - cy) < 0.24) - LOOSENESS
    fy = front_y - 0.004
    set_mat(shell, denim)
    denim_parts = [shell]
    copper_parts = []

    ncx, ncy = torso_center(co, z_shoulder * 0.99, band)
    collar = add_cylinder("Collar", (ncx, ncy, z_shoulder + 0.004), 0.15, 0.052)
    shrinkwrap_to(collar, body, DENIM_GAP + LOOSENESS + 0.006)
    denim_parts.append(collar)

    denim_parts.append(add_box("Placket",
                       (cx, fy, (z_hem + z_shoulder) * 0.5),
                       (0.038, 0.012, z_shoulder - z_hem)))
    for i in range(6):
        z = z_hem + (z_shoulder - z_hem) * (0.08 + 0.84 * i / 5.0)
        copper_parts.append(add_cylinder("Button_%d" % i,
                            (cx, fy - 0.001, z), 0.007, 0.003,
                            rot=(math.pi / 2, 0, 0)))

    for sgn in (-1, 1):
        px = cx + sgn * 0.078
        denim_parts.append(add_box("Pocket_%d" % sgn,
                           (px, fy, z_chest + 0.01),
                           (0.058, 0.010, 0.075)))
        denim_parts.append(add_box("PocketFlap_%d" % sgn,
                           (px, fy - 0.003, z_chest + 0.055),
                           (0.062, 0.012, 0.024)))

    for side in (-1, 1):
        wrist = Vector((side * 0.33, 0.02, z_wrist + 0.02))
        cuff = add_cylinder("Cuff_%d" % side,
                            (wrist.x, wrist.y, wrist.z), 0.044, 0.03)
        shrinkwrap_to(cuff, body, DENIM_GAP + LOOSENESS - 0.004)
        denim_parts.append(cuff)

    # No separate hem band -- the shell's own bottom edge is the hem, and a
    # floating cylinder band only detaches from it.

    for p in copper_parts:
        set_mat(p, copper)
    for p in denim_parts[1:]:
        set_mat(p, denim)
    out("details: collar, placket, 6 copper buttons, 2 pockets+studs, cuffs, hem")
    return denim_parts, copper_parts, (cx, cy, fy, z_chest, front_y)


# ------------------------------------------------------------- stitching ----
def build_stitching(body, z_hem, z_shoulder, z_wrist, anchor, thread):
    """Flat-front topstitch only -- curved collar/cuff/hem stitching needs a
    UV+texture pass and floats if done with shrinkwrapped rings."""
    cx, cy, fy, z_chest, front_y = anchor
    sy = fy - 0.003   # fine thread, just proud (details are not smoothed)
    d = 0.006         # stitch depth toward the viewer
    stitches = []

    # Iconic twin-needle placket topstitch flanking the buttons.
    for dx in (-0.030, 0.030):
        stitches.append(add_box("PlacketStitch_%.0f" % (dx * 1000),
                        (cx + dx, sy, (z_hem + z_shoulder) * 0.5),
                        (0.004, d, (z_shoulder - z_hem) * 0.94)))
    # Chest yoke seam across the front.
    z_yoke = z_shoulder * 0.88
    stitches.append(add_box("YokeStitch",
                    (cx, sy, z_yoke), (0.30, d, 0.004)))
    # Pocket flap bottom seams + pocket-mouth seams.
    for sgn in (-1, 1):
        px = cx + sgn * 0.078
        stitches.append(add_box("FlapStitch_%d" % sgn,
                        (px, sy, z_chest + 0.044), (0.060, d, 0.004)))
        stitches.append(add_box("PocketStitch_%d" % sgn,
                        (px, sy, z_chest - 0.024), (0.056, d, 0.004)))
    for s in stitches:
        set_mat(s, thread)
    out("stitching: twin-needle placket, yoke seam, pocket flap + mouth seams")
    return stitches


# ------------------------------------------------------------- assemble ----
def join(parts, z_floor):
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    jacket = bpy.context.active_object
    jacket.name = "SM_ZarriDenimJacket"

    # Kill stray faces that dropped below the hem line during extraction.
    bm = bmesh.new()
    bm.from_mesh(jacket.data)
    low = [f for f in bm.faces if f.calc_center_median().z < z_floor]
    if low:
        bmesh.ops.delete(bm, geom=low, context="FACES")
    bm.to_mesh(jacket.data)
    bm.free()
    jacket.data.update()

    # No relax here -- the shell was already relaxed, so details keep volume.
    # Gentle fabric wrinkles only.
    tex = bpy.data.textures.new("wrinkle", "CLOUDS")
    tex.noise_scale = 0.11
    disp = jacket.modifiers.new("wrinkle", "DISPLACE")
    disp.texture = tex
    disp.texture_coords = "GLOBAL"
    disp.strength = 0.004
    disp.mid_level = 0.5
    sol = jacket.modifiers.new("thickness", "SOLIDIFY")
    sol.thickness = 0.011
    sol.offset = 1.0
    sub = jacket.modifiers.new("smooth", "SUBSURF")
    sub.levels = 1
    sub.render_levels = 1
    bpy.ops.object.shade_smooth()
    return jacket


# ------------------------------------------------------------- materials ----
def denim_material():
    mat = bpy.data.materials.new("M_ZarriDenim")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out_node = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    tex = nt.nodes.new("ShaderNodeTexCoord")

    # Fine diagonal twill weave -> bump.
    wave = nt.nodes.new("ShaderNodeTexWave")
    wave.wave_type = "BANDS"
    wave.bands_direction = "DIAGONAL"
    wave.inputs["Scale"].default_value = 340.0
    wave.inputs["Distortion"].default_value = 1.0
    nt.links.new(tex.outputs["Object"], wave.inputs["Vector"])
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.22
    nt.links.new(wave.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])

    # Deep indigo with a cloud wash-fade.
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 6.0
    noise.inputs["Detail"].default_value = 4.0
    nt.links.new(tex.outputs["Object"], noise.inputs["Vector"])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (0.012, 0.028, 0.085, 1.0)
    ramp.color_ramp.elements[1].color = (0.048, 0.090, 0.190, 1.0)
    nt.links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])

    # Roughness varies with the weave for a matte-but-alive fabric.
    rramp = nt.nodes.new("ShaderNodeValToRGB")
    rramp.color_ramp.elements[0].color = (0.78, 0.78, 0.78, 1.0)
    rramp.color_ramp.elements[1].color = (0.90, 0.90, 0.90, 1.0)
    nt.links.new(wave.outputs["Fac"], rramp.inputs["Fac"])
    nt.links.new(rramp.outputs["Color"], bsdf.inputs["Roughness"])
    nt.links.new(bsdf.outputs["BSDF"], out_node.inputs["Surface"])
    return mat


def thread_material():
    mat = bpy.data.materials.new("M_DenimThread")
    mat.use_nodes = True
    b = mat.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.52, 0.38, 0.16, 1.0)  # tan thread
    b.inputs["Roughness"].default_value = 0.5
    return mat


def copper_material():
    mat = bpy.data.materials.new("M_DenimCopper")
    mat.use_nodes = True
    b = mat.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (0.46, 0.26, 0.13, 1.0)
    b.inputs["Metallic"].default_value = 0.9
    b.inputs["Roughness"].default_value = 0.38
    return mat


# --------------------------------------------------------------- render ----
def setup_and_render(body, jacket):
    scn = bpy.context.scene
    scn.render.engine = "CYCLES"
    scn.cycles.device = "CPU"
    scn.cycles.samples = 56
    scn.render.resolution_x = 1000
    scn.render.resolution_y = 1400
    world = bpy.data.worlds.new("W")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (
        0.045, 0.05, 0.065, 1.0)
    scn.world = world

    skin = bpy.data.materials.new("Skin")
    skin.use_nodes = True
    sb = skin.node_tree.nodes["Principled BSDF"]
    sb.inputs["Base Color"].default_value = (0.34, 0.21, 0.14, 1.0)
    sb.inputs["Roughness"].default_value = 0.6
    body.data.materials.clear()
    body.data.materials.append(skin)

    cam_data = bpy.data.cameras.new("Cam")
    cam_data.lens = 72
    cam = bpy.data.objects.new("Cam", cam_data)
    scn.collection.objects.link(cam)
    scn.camera = cam

    def area(name, energy, size, loc, rot):
        d = bpy.data.lights.new(name, "AREA")
        d.energy = energy
        d.size = size
        o = bpy.data.objects.new(name, d)
        o.location = loc
        o.rotation_euler = rot
        scn.collection.objects.link(o)

    area("Key", 320, 3.0, (2.6, -3.2, 2.7),
         (math.radians(58), 0, math.radians(38)))
    area("Fill", 120, 3.5, (-2.8, -2.4, 1.8),
         (math.radians(70), 0, math.radians(-52)))
    area("Rim", 240, 2.0, (0.0, 3.4, 2.8), (math.radians(120), 0, 0))

    renders = []
    for name, pos, rot in (
        ("front", (0.0, -3.3, 1.15), (math.radians(90), 0, 0)),
        ("back", (0.0, 3.3, 1.15), (math.radians(90), 0, math.radians(180))),
        ("detail", (1.5, -1.9, 1.32), (math.radians(80), 0, math.radians(40))),
    ):
        cam.location = pos
        cam.rotation_euler = rot
        if name == "detail":
            cam_data.lens = 110
        path = os.path.join(SCRATCH, "denim_%s.png" % name)
        scn.render.filepath = path
        bpy.ops.render.render(write_still=True)
        renders.append(path)
        out("rendered %s" % path)
    import shutil
    shutil.copyfile(renders[0], os.path.join(DENIM_DIR, "DenimJacketPreview.png"))
    return renders


def main():
    for d in (BASE_DIR, DENIM_DIR, EXPORT_DIR):
        os.makedirs(d, exist_ok=True)

    body = load_base()
    bpy.ops.wm.save_as_mainfile(filepath=BASE_BLEND, copy=True)
    out("saved base figure -> %s" % os.path.relpath(BASE_BLEND, ROOT))

    denim = denim_material()
    thread = thread_material()
    copper = copper_material()

    shell, z_hem, z_shoulder, z_wrist = extract_jacket_shell(body)
    denim_parts, copper_parts, anchor = build_details(
        body, shell, z_hem, z_shoulder, z_wrist, denim, copper)
    stitches = build_stitching(body, z_hem, z_shoulder, z_wrist, anchor, thread)

    jacket = join(denim_parts + copper_parts + stitches, z_hem - 0.03)
    out("jacket assembled: %d faces, %d material slots"
        % (len(jacket.data.polygons), len(jacket.data.materials)))

    renders = setup_and_render(body, jacket)

    bpy.ops.object.select_all(action="DESELECT")
    jacket.select_set(True)
    bpy.context.view_layer.objects.active = jacket
    bpy.ops.export_scene.fbx(filepath=FBX_PATH, use_selection=True,
                             object_types={"MESH"}, apply_unit_scale=True,
                             use_mesh_modifiers=True, add_leaf_bones=False,
                             path_mode="COPY", embed_textures=False)
    out("exported %s (%.1f KB)" % (os.path.relpath(FBX_PATH, ROOT),
                                   os.path.getsize(FBX_PATH) / 1024.0))
    bpy.ops.wm.save_as_mainfile(filepath=JACKET_BLEND)
    out("saved jacket source -> %s" % os.path.relpath(JACKET_BLEND, ROOT))
    out("DONE renders=%d" % len(renders))


try:
    main()
finally:
    os.makedirs(DENIM_DIR, exist_ok=True)
    with open(REPORT_PATH, "w") as fh:
        fh.write("\n".join(lines) + "\n")
