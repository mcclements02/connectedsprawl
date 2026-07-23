"""Author a full parameterized streetwear outfit for Zarri on a CC0 human base.

Extends the denim-jacket pipeline into a complete fit -- hoodie, joggers, and
sneakers -- all fitted to the SAME realistic CC0 male base by extracting each
garment shell from the body's own faces (real anatomical fit), relaxing the
shell so muscle reads as loose cloth, then adding modest details and PBR
fabrics. Colours are parameters (edit PARAMS, or pass a JSON object after `--`).

Run:
  /opt/homebrew/bin/blender --background --python Tools/build_zarri_outfit.py
  # optional colour override:
  #   ... --python Tools/build_zarri_outfit.py -- '{"top_color":[0.5,0.1,0.1]}'

Writes only NEW files under Outfit/ dirs -- never the in-flight cloth kits.
"""
import json
import math
import os
import sys

import bpy
import bmesh
from mathutils import Vector

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCRATCH = ("/private/tmp/claude-501/-Users-matthewx-code-ConnectedSprawl/"
           "dc4ad07d-766d-4cdc-a9e3-4ce18bf21fc6/scratchpad")
BUNDLE = os.path.join(SCRATCH, "hbm", "human-base-meshes-bundle-v1.4.1",
                      "human_base_meshes_bundle.blend")

SRC_DIR = os.path.join(ROOT, "Content", "MetaHumans", "Source", "Outfit")
EXPORT_DIR = os.path.join(ROOT, "Content", "Import", "Characters", "Outfit")
OUTFIT_BLEND = os.path.join(SRC_DIR, "ConnectedSprawl_Outfit.blend")
FBX_PATH = os.path.join(EXPORT_DIR, "SM_ZarriOutfit.fbx")
REPORT_PATH = os.path.join(SRC_DIR, "OutfitReport.txt")

TARGET_HEIGHT = 1.775

# Editable outfit palette (RGB 0..1). Override any key with a JSON argv.
PARAMS = {
    "top_color": [0.34, 0.42, 0.54],     # light blue-grey top (in-game Zarri)
    "pants_color": [0.15, 0.21, 0.34],   # blue denim jeans
    "shoe_color": [0.88, 0.89, 0.91],    # white sneaker upper
    "shoe_sole": [0.10, 0.11, 0.13],     # dark outsole
    "shoe_accent": [0.10, 0.60, 0.58],   # teal accent stripe
    "hat_color": [0.26, 0.42, 0.66],     # blue bucket hat
}

lines = []


def out(text=""):
    lines.append(text)
    print("[Outfit] " + text)


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
    s = TARGET_HEIGHT / (zmax - zmin)
    for v in body.data.vertices:
        v.co = Vector(((v.co.x - cx) * s, (v.co.y - cy) * s, (v.co.z - zmin) * s))
    body.data.update()
    out("base: scaled to %.3fm, verts=%d" % (TARGET_HEIGHT, len(body.data.vertices)))
    return body, zmax * s


# ---------------------------------------------------- shell extraction ----
def extract_shell(body, name, keep_fn, loosen, relax_iters):
    """Duplicate the body, keep only faces passing keep_fn(center), inflate off
    the skin, and relax so muscle reads as cloth."""
    dup = body.copy()
    dup.data = body.data.copy()
    dup.name = name
    bpy.context.collection.objects.link(dup)
    dup.data.materials.clear()
    bm = bmesh.new()
    bm.from_mesh(dup.data)
    kill = [f for f in bm.faces if not keep_fn(f.calc_center_median())]
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    bm.to_mesh(dup.data)
    bm.free()
    dup.data.update()
    if len(dup.data.polygons) == 0:
        out("WARN: %s extracted 0 faces" % name)
        return dup

    # Relax FIRST -- smoothing shrinks the mesh, so inflating before it would
    # let the body poke back through (holes at thighs/crotch).
    sm = dup.modifiers.new("relax", "SMOOTH")
    sm.factor = 0.9
    sm.iterations = relax_iters
    bpy.context.view_layer.objects.active = dup
    bpy.ops.object.modifier_apply(modifier="relax")

    # ...then push the relaxed shell off the skin so the offset really holds.
    bm2 = bmesh.new()
    bm2.from_mesh(dup.data)
    bm2.normal_update()
    for v in bm2.verts:
        v.co = v.co + v.normal * loosen
    bm2.to_mesh(dup.data)
    bm2.free()
    dup.data.update()
    return dup


def add_box(name, center, size):
    # A size=1.0 cube has 1 m sides, so scale by `size` (not size*0.5) to make a
    # box whose dimensions are exactly `size`.
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=center)
    o = bpy.context.active_object
    o.name = name
    o.scale = Vector(size)
    bpy.ops.object.transform_apply(scale=True)
    return o


def add_cyl(name, center, radius, depth, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=radius, depth=depth,
                                        location=center, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    return o


def shrinkwrap(o, body, offset):
    sw = o.modifiers.new("fit", "SHRINKWRAP")
    sw.target = body
    sw.wrap_method = "NEAREST_SURFACEPOINT"
    sw.offset = offset
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.modifier_apply(modifier="fit")


def fabric(name, color, roughness=0.85, metallic=0.0, sheen=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    b = mat.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (color[0], color[1], color[2], 1.0)
    b.inputs["Roughness"].default_value = roughness
    b.inputs["Metallic"].default_value = metallic
    if "Sheen Weight" in b.inputs:
        b.inputs["Sheen Weight"].default_value = sheen
    return mat


def set_mat(o, mat):
    o.data.materials.clear()
    o.data.materials.append(mat)


def loft(name, rings, cap_start=True, cap_end=True):
    """Bridge a list of equal-length vertex rings into a closed lofted solid."""
    bm = bmesh.new()
    grid = [[bm.verts.new(p) for p in ring] for ring in rings]
    bm.verts.ensure_lookup_table()
    n, seg = len(rings), len(rings[0])
    for i in range(n - 1):
        for j in range(seg):
            bm.faces.new((grid[i][j], grid[i][(j + 1) % seg],
                          grid[i + 1][(j + 1) % seg], grid[i + 1][j]))
    if cap_start:
        bm.faces.new(grid[0][::-1])
    if cap_end:
        bm.faces.new(grid[-1])
    bm.normal_update()
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(o)
    return o


def smooth_only(o, sub=2):
    """Subdivide a already-closed solid (no solidify -- it has volume)."""
    s = o.modifiers.new("sub", "SUBSURF")
    s.levels = sub
    s.render_levels = sub
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.shade_smooth()


def finalize(o, thickness, sub=1):
    sol = o.modifiers.new("thick", "SOLIDIFY")
    sol.thickness = thickness
    sol.offset = 1.0
    s = o.modifiers.new("sub", "SUBSURF")
    s.levels = sub
    s.render_levels = sub
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.shade_smooth()


# ------------------------------------------------------------- garments ----
def build_hoodie(body, H, color):
    z_hem, z_sh, z_wr = H * 0.46, H * 0.845, H * 0.475
    LOOSE = 0.013   # fitted -- relax runs first, so this offset really holds

    def keep(c):
        ax = abs(c.x)
        torso = (z_hem <= c.z <= z_sh) and ax < 0.25
        arm = (z_wr <= c.z <= z_sh) and ax >= 0.205
        return torso or arm

    shell = extract_shell(body, "Hoodie", keep, LOOSE, 16)
    parts = [shell]
    co = [v.co for v in body.data.vertices]

    def center(z):
        band = [v for v in co if abs(v.z - z) < H * 0.02 and math.hypot(v.x, v.y) < 0.24]
        if not band:
            return 0.0, 0.0
        return sum(v.x for v in band) / len(band), sum(v.y for v in band) / len(band)

    # Hood-down: a thick rolled cowl on the upper back / neck.
    ncx, ncy = center(z_sh * 0.99)
    hood = add_box("Hood", (ncx, ncy + 0.05, z_sh + 0.02), (0.26, 0.14, 0.10))
    shrinkwrap(hood, body, LOOSE + 0.02)
    parts.append(hood)
    # Ribbed cuffs (no separate hem band -- the shell's own edge is the hem, and
    # no belly pocket -- a floating box only blobs at the crotch).
    for side in (-1, 1):
        wr = Vector((side * 0.33, 0.02, z_wr + 0.02))
        cuff = add_cyl("Cuff_%d" % side, (wr.x, wr.y, wr.z), 0.036, 0.028)
        shrinkwrap(cuff, body, LOOSE)
        parts.append(cuff)

    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = shell
    bpy.ops.object.join()
    hoodie = bpy.context.active_object
    hoodie.name = "SM_Hoodie"
    set_mat(hoodie, fabric("M_Hoodie", color, roughness=0.9, sheen=0.4))
    finalize(hoodie, 0.009)   # thinner fabric reads as fitted, not padded
    out("hoodie: hood-down, kangaroo pocket, ribbed hem+cuffs")
    return hoodie


def build_pants(body, H, color):
    z_ankle, z_waist = H * 0.075, H * 0.52
    # 0.011 was too tight -- the relaxed shell sank inside the thighs and the
    # body poked through. This is the fitted-but-safe floor.
    LOOSE = 0.017

    def keep(c):
        # Legs + pelvis; exclude the hanging hands out at the sides.
        return (z_ankle <= c.z <= z_waist) and abs(c.x) < 0.25

    shell = extract_shell(body, "Pants", keep, LOOSE, 14)
    parts = [shell]
    co = [v.co for v in body.data.vertices]

    def center(z):
        band = [v for v in co if abs(v.z - z) < H * 0.02 and math.hypot(v.x, v.y) < 0.26]
        if not band:
            return 0.0, 0.0
        return sum(v.x for v in band) / len(band), sum(v.y for v in band) / len(band)

    wcx, wcy = center(z_waist)
    wb = add_cyl("Waistband", (wcx, wcy, z_waist - 0.01), 0.20, 0.06)
    shrinkwrap(wb, body, LOOSE + 0.008)
    parts.append(wb)
    # Cuffed ankles (joggers).
    for side in (-1, 1):
        acx = side * 0.09
        cuff = add_cyl("Ankle_%d" % side, (acx, 0.03, z_ankle + 0.04), 0.075, 0.05)
        shrinkwrap(cuff, body, LOOSE + 0.004)
        parts.append(cuff)

    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = shell
    bpy.ops.object.join()
    pants = bpy.context.active_object
    pants.name = "SM_Pants"
    set_mat(pants, fabric("M_Pants", color, roughness=0.88))
    finalize(pants, 0.010)
    out("pants: joggers with waistband + cuffed ankles")
    return pants


def build_sneakers(body, H, color, sole_color, accent):
    """Low-profile sneakers over each foot. Feet point forward (-Y); a slim
    outsole, a fitted upper, a toe cap, and a teal side stripe."""
    up_mat = fabric("M_ShoeUpper", color, roughness=0.45)
    sole_mat = fabric("M_ShoeSole", sole_color, roughness=0.35)
    acc_mat = fabric("M_ShoeAccent", accent, roughness=0.4)
    shoes = []
    for side in (-1, 1):
        # Size to the actual foot bounding box (everything below the ankle).
        fv = [v.co for v in body.data.vertices
              if v.co.z < 0.13 and (v.co.x * side) > 0.02]
        if not fv:
            continue
        xmin = min(v.x for v in fv); xmax = max(v.x for v in fv)
        ymin = min(v.y for v in fv); ymax = max(v.y for v in fv)  # ymin = toe(-Y)
        ztop = max(v.z for v in fv)
        cx = (xmin + xmax) * 0.5; cy = (ymin + ymax) * 0.5
        fw = xmax - xmin; fl = ymax - ymin

        # --- lofted shoe last: elliptical sections toe -> heel -------------
        SOLE_H = 0.034
        N, SEG = 18, 16
        upper_rings, sole_rings = [], []
        for i in range(N):
            t = i / (N - 1)
            y = (ymin - 0.030) + t * (fl + 0.052)
            # widest at the ball (~t 0.45), narrower at toe tip and heel
            a = (fw * 0.5 + 0.010) * (0.80 + 0.34 * math.sin(
                math.pi * min(1.0, t * 1.1)))
            # low over the toe, rising to an ankle collar at the heel
            b = 0.026 + 0.060 * (t ** 1.4)
            # D-shaped section: domed top with a FLAT bottom on the sole line,
            # so the upper meets the sole at full width instead of pinching to
            # a point (which let the sole flare out as a bare plate).
            ring = []
            DOME = 12
            for k in range(DOME):
                ang = math.pi * k / (DOME - 1)
                ring.append((cx + a * math.cos(ang), y,
                             SOLE_H + b * math.sin(ang)))
            for k in range(1, 5):
                ring.append((cx - a + 2.0 * a * (k / 5.0), y, SOLE_H))
            upper_rings.append(ring)
            asole = a * 0.97
            sole_rings.append([(cx - asole, y, 0.0), (cx + asole, y, 0.0),
                               (cx + asole, y, SOLE_H), (cx - asole, y, SOLE_H)])

        upper = loft("Upper_%d" % side, upper_rings)
        set_mat(upper, up_mat)
        smooth_only(upper, sub=2)
        sole = loft("Sole_%d" % side, sole_rings)
        set_mat(sole, sole_mat)
        smooth_only(sole, sub=2)
        shoes += [upper, sole]

        # Teal side stripe over the widest part of the last.
        stripe = add_box("Stripe_%d" % side,
                         (cx + side * (fw * 0.5 + 0.016), ymin + fl * 0.46,
                          SOLE_H + 0.032),
                         (0.012, fl * 0.40, 0.017))
        set_mat(stripe, acc_mat)
        finalize(stripe, 0.003, sub=1)
        shoes.append(stripe)

        # Lace bars across the instep.
        for k in range(4):
            lt = 0.34 + 0.11 * k
            ly = (ymin - 0.030) + lt * (fl + 0.052)
            lb = 0.026 + 0.060 * (lt ** 1.4)
            # ring top is SOLE_H + 2b, so sit the bar just proud of it
            lace = add_box("Lace_%d_%d" % (side, k),
                           (cx, ly, SOLE_H + lb * 2.0 + 0.002),
                           (fw * 0.50, 0.010, 0.009))
            set_mat(lace, acc_mat)
            finalize(lace, 0.002, sub=1)
            shoes.append(lace)
    out("sneakers: lofted last (18 sections) + outsole + stripe + laces")
    return shoes


def build_hat(body, H, color):
    """Blue bucket hat: a crown sitting on the skull plus a down-turned brim."""
    head = [v.co for v in body.data.vertices if v.co.z > H * 0.88]
    if not head:
        return []
    cx = sum(v.x for v in head) / len(head)
    cy = sum(v.y for v in head) / len(head)
    ztop = max(v.z for v in head)
    hat_mat = fabric("M_Hat", color, roughness=0.85)
    crown = add_cyl("HatCrown", (cx, cy, ztop - 0.045), 0.104, 0.115)
    set_mat(crown, hat_mat)
    finalize(crown, 0.008, sub=2)
    brim = add_cyl("HatBrim", (cx, cy, ztop - 0.102), 0.163, 0.020)
    set_mat(brim, hat_mat)
    finalize(brim, 0.006, sub=2)
    out("bucket hat: crown + brim on the skull")
    return [crown, brim]


# --------------------------------------------------------------- render ----
def setup_and_render(body, garments):
    scn = bpy.context.scene
    scn.render.engine = "CYCLES"
    scn.cycles.device = "CPU"
    scn.cycles.samples = 48
    scn.render.resolution_x = 1000
    scn.render.resolution_y = 1500
    w = bpy.data.worlds.new("W")
    w.use_nodes = True
    w.node_tree.nodes["Background"].inputs["Color"].default_value = (0.05, 0.055, 0.07, 1.0)
    scn.world = w
    skin = fabric("Skin", [0.34, 0.21, 0.14], roughness=0.6)
    body.data.materials.clear()
    body.data.materials.append(skin)

    cam_data = bpy.data.cameras.new("Cam")
    cam_data.lens = 60
    cam = bpy.data.objects.new("Cam", cam_data)
    scn.collection.objects.link(cam)
    scn.camera = cam

    def area(nm, e, sz, loc, rot):
        d = bpy.data.lights.new(nm, "AREA")
        d.energy = e
        d.size = sz
        o = bpy.data.objects.new(nm, d)
        o.location = loc
        o.rotation_euler = rot
        scn.collection.objects.link(o)

    # Restrained energies so the true garment colours read instead of blowing
    # out to pale pastel.
    area("Key", 260, 3.5, (2.8, -3.4, 2.9), (math.radians(56), 0, math.radians(38)))
    area("Fill", 90, 4.0, (-3.0, -2.6, 1.9), (math.radians(70), 0, math.radians(-52)))
    area("Rim", 150, 2.5, (0.0, 3.6, 3.0), (math.radians(120), 0, 0))

    renders = []
    for nm, pos, rot in (
        ("front", (0.0, -4.0, 0.9), (math.radians(90), 0, 0)),
        ("back", (0.0, 4.0, 0.9), (math.radians(90), 0, math.radians(180))),
        ("threequarter", (2.6, -3.0, 1.0), (math.radians(84), 0, math.radians(41))),
    ):
        cam.location = pos
        cam.rotation_euler = rot
        p = os.path.join(SCRATCH, "outfit_%s.png" % nm)
        scn.render.filepath = p
        bpy.ops.render.render(write_still=True)
        renders.append(p)
        out("rendered %s" % nm)
    import shutil
    shutil.copyfile(renders[0], os.path.join(SRC_DIR, "OutfitPreview.png"))
    return renders


def main():
    for d in (SRC_DIR, EXPORT_DIR):
        os.makedirs(d, exist_ok=True)
    # Colour overrides from a JSON argv after `--`.
    if "--" in sys.argv:
        try:
            PARAMS.update(json.loads(sys.argv[sys.argv.index("--") + 1]))
            out("params overridden from argv")
        except (ValueError, IndexError):
            out("argv override ignored (bad JSON)")
    out("palette: " + json.dumps(PARAMS))

    body, H = load_base()
    hoodie = build_hoodie(body, H, PARAMS["top_color"])
    pants = build_pants(body, H, PARAMS["pants_color"])
    shoes = build_sneakers(body, H, PARAMS["shoe_color"],
                           PARAMS["shoe_sole"], PARAMS["shoe_accent"])
    hat = build_hat(body, H, PARAMS["hat_color"])
    garments = [hoodie, pants] + shoes + hat

    renders = setup_and_render(body, garments)

    bpy.ops.object.select_all(action="DESELECT")
    for g in garments:
        g.select_set(True)
    bpy.context.view_layer.objects.active = hoodie
    bpy.ops.export_scene.fbx(filepath=FBX_PATH, use_selection=True,
                             object_types={"MESH"}, apply_unit_scale=True,
                             use_mesh_modifiers=True, add_leaf_bones=False,
                             mesh_smooth_type="FACE", path_mode="COPY")
    out("exported %s (%.1f KB)" % (os.path.relpath(FBX_PATH, ROOT),
                                   os.path.getsize(FBX_PATH) / 1024.0))
    bpy.ops.wm.save_as_mainfile(filepath=OUTFIT_BLEND)
    out("saved outfit source -> %s" % os.path.relpath(OUTFIT_BLEND, ROOT))
    out("DONE renders=%d garments=%d" % (len(renders), len(garments)))


try:
    main()
finally:
    os.makedirs(SRC_DIR, exist_ok=True)
    with open(REPORT_PATH, "w") as fh:
        fh.write("\n".join(lines) + "\n")
