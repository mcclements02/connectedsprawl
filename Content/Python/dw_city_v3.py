"""Downtown West city pass v3 — full-city street life + hero facades.

Builds on dw_integrate (storefronts on the 3x3 core) and dw_realism2:
  1. Storefront ground floors + awnings on the ring of blocks around the
     core (gx,gy in 1..5, skipping core/parks/lake) with kit variety.
  2. Hero second story on the core: lvl2 window modules + roofline trim
     stacked directly on every existing City_DW_store module.
  3. Hanging store signs + sidewalk foldout signs along storefront rows.
  4. Street trees along sidewalk bands of every buildable block.
  5. Park centerpieces: fountain plaza (5,5), playground (2,2).
  6. Background mountain vista beyond the lake edge.

Spawns actors => run WITHOUT -nullrhi (and with -DisablePlugins=Fab):
  UnrealEditor-Cmd <uproject> -ExecutePythonScript="<this file>" -unattended
"""
import random
import unreal

random.seed(133)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

N, BLOCK, ROAD = 7, 2000.0, 600.0
STEP = BLOCK + ROAD
START = -N * STEP / 2 + STEP / 2
LAKE = {(gx, gy) for gx in (4, 5, 6) for gy in (0, 1)}
PARKS = {(1, 5), (2, 2), (5, 5)}
CORE = {(gx, gy) for gx in (2, 3, 4) for gy in (2, 3, 4)}
RING = {(gx, gy) for gx in range(1, 6) for gy in range(1, 6)} - CORE - PARKS - LAKE
SIDEWALK_Z = 14.0

DW = "/Game/Downtown_West/Assets"
PATHS = {
    "wall5_b": DW + "/building_b/SM_build_b_mod_lvl1_storefront_b_wall5m",
    "wall3_b": DW + "/building_b/SM_build_b_mod_lvl1_storefront_b_wall3m",
    "wall5_a": DW + "/building_b/SM_build_b_mod_lvl1_storefront_a_5m",
    "wall35_a": DW + "/building_b/SM_build_b_mod_lvl1_storefront_a_3_5m",
    "lvl2_wide": DW + "/building_b/SM_build_b_mod_lvl2_widewindow",
    "lvl2_double": DW + "/building_b/SM_build_b_mod_lvl2_doublewindow",
    "lvl2_single": DW + "/building_b/SM_build_b_mod_lvl2_singlewindow",
    "top_trim": DW + "/building_b/SM_build_b_mod_top_trim",
    "awning": DW + "/awnings/SM_building_awning_b",
    "tree_a": DW + "/foliage/tree_narrowleaf/SM_tree_narrowleaf_a",
    "tree_b": DW + "/foliage/tree_narrowleaf/SM_tree_narrowleaf_b",
    "tree_oak": DW + "/foliage/tree_oak_a/SM_tree_oak_a",
    "fountain": DW + "/props/prop_fountain/SM_prop_fountain_full",
    "playground": DW + "/playground/SM_playground",
    "mountains": DW + "/background_mountain/SM_background_mountains",
}
SIGN_DIR = DW + "/props/prop_store_signs"

MESH = {}
for k, p in PATHS.items():
    MESH[k] = unreal.load_asset(p)
    if not MESH[k]:
        unreal.log_warning("[DW3] missing {} -> {}".format(k, p))

eal = unreal.EditorAssetLibrary
HANG_SIGNS, FOLDOUT_SIGNS = [], []
for path in eal.list_assets(SIGN_DIR, recursive=False):
    name = path.split("/")[-1].split(".")[0]
    if not name.startswith("SM_signs_sign"):
        continue
    m = eal.load_asset(path)
    if not m:
        continue
    (FOLDOUT_SIGNS if "foldout" in name else HANG_SIGNS).append(m)
unreal.log("[DW3] signs loaded: {} hanging, {} foldout".format(
    len(HANG_SIGNS), len(FOLDOUT_SIGNS)))


def cx(i):
    return START + i * STEP


def block_of(pos):
    return int(round((pos.x - START) / STEP)), int(round((pos.y - START) / STEP))


def mesh_size(mesh):
    b = mesh.get_bounds()
    return b.box_extent * 2.0


def fit_to_ground(a, ground_z, want_x=None, want_y=None):
    o, e = a.get_actor_bounds(False)
    loc = a.get_actor_location()
    dx = (want_x - o.x) if want_x is not None else 0.0
    dy = (want_y - o.y) if want_y is not None else 0.0
    dz = ground_z - (o.z - e.z)
    a.set_actor_location(unreal.Vector(loc.x + dx, loc.y + dy, loc.z + dz), False, False)


def spawn_mesh(mesh, label, x, y, z, yaw=0.0, ground=None):
    a = eas.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, yaw, 0))
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(mesh)
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    if ground is not None:
        fit_to_ground(a, ground)
    return a


# idempotent wipe of this pass
wiped = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_DW3_"):
        eas.destroy_actor(a)
        wiped += 1
unreal.log("[DW3] wiped {} previous actors".format(wiped))

all_actors = eas.get_all_level_actors()

# occupancy grid of existing actor XY (trees/signs must not clip props/cars)
occupied = []
for a in all_actors:
    smc = getattr(a, "static_mesh_component", None)
    if smc and smc.static_mesh:
        loc = a.get_actor_location()
        occupied.append((loc.x, loc.y))


def is_free(x, y, radius=230.0):
    r2 = radius * radius
    for ox, oy in occupied:
        if (x - ox) ** 2 + (y - oy) ** 2 < r2:
            return False
    return True


def claim(x, y):
    occupied.append((x, y))


# ------------------------------------------------------------------
# 1) ring storefronts + awnings (same face math as dw_integrate)
# ------------------------------------------------------------------
storefronts = 0
awnings = 0
store_rows = []   # (axis, sign, yaw, face_pos, along_center, width, module_top)
for a in list(all_actors):
    if not a.get_actor_label().startswith("City_Bldg"):
        continue
    gxy = block_of(a.get_actor_location())
    if gxy not in RING:
        continue
    bx, by = cx(gxy[0]), cx(gxy[1])
    o, e = a.get_actor_bounds(False)
    kit5 = MESH["wall5_a"] if (gxy[0] + gxy[1]) % 2 else MESH["wall5_b"]
    kit3 = MESH["wall35_a"] if (gxy[0] + gxy[1]) % 2 else MESH["wall3_b"]
    for axis, sign in (("x", 1), ("x", -1), ("y", 1), ("y", -1)):
        if axis == "x":
            face_pos = o.x + sign * e.x
            street_edge = bx + (1000 if sign > 0 else -1000)
            length = e.y * 2
        else:
            face_pos = o.y + sign * e.y
            street_edge = by + (1000 if sign > 0 else -1000)
            length = e.x * 2
        if abs(face_pos - street_edge) > 430 or length < 320:
            continue
        n5 = int(length // 510)
        used = n5 * 510
        fill3 = kit3 and (length - used) >= 320
        mods = [(kit5, 510)] * n5 + ([(kit3, 310)] if fill3 else [])
        if not mods or not kit5:
            continue
        total = sum(w for _, w in mods)
        yaw = (0.0 if sign > 0 else 180.0) if axis == "x" else (90.0 if sign > 0 else 270.0)
        out = 16.0 * sign
        cursor = -total / 2.0
        for i, (mesh, w) in enumerate(mods):
            center_along = cursor + w / 2.0
            cursor += w
            if axis == "x":
                px, py = face_pos + out, o.y + center_along
            else:
                px, py = o.x + center_along, face_pos + out
            sa = spawn_mesh(mesh, "City_DW3_store_{}_{}".format(storefronts, i),
                            px, py, SIDEWALK_Z + 5, yaw)
            fit_to_ground(sa, SIDEWALK_Z, want_x=px, want_y=py)
            claim(px, py)
            so, se = sa.get_actor_bounds(False)
            store_rows.append((axis, sign, yaw, face_pos, center_along, w,
                               so.z + se.z, px, py))
            storefronts += 1
            if MESH["awning"] and i % 2 == 0:
                if axis == "x":
                    ax_, ay_ = face_pos + sign * 55.0, py
                else:
                    ax_, ay_ = px, face_pos + sign * 55.0
                aw = spawn_mesh(MESH["awning"], "City_DW3_awning_{}".format(awnings),
                                ax_, ay_, SIDEWALK_Z + 300, yaw)
                oo, ee = aw.get_actor_bounds(False)
                al = aw.get_actor_location()
                aw.set_actor_location(unreal.Vector(
                    al.x + (ax_ - oo.x), al.y + (ay_ - oo.y),
                    al.z + (SIDEWALK_Z + 395 - (oo.z - ee.z))), False, False)
                awnings += 1
unreal.log("[DW3] ring storefronts {} awnings {}".format(storefronts, awnings))

# ------------------------------------------------------------------
# 2) hero second story + roofline trim over EVERY existing storefront
#    (core pass modules are labeled City_DW_store_*, ring pass City_DW3_store_*)
# ------------------------------------------------------------------
lvl2 = 0
for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    if not (label.startswith("City_DW_store_") or label.startswith("City_DW3_store_")):
        continue
    smc = getattr(a, "static_mesh_component", None)
    if not smc or not smc.static_mesh:
        continue
    o, e = a.get_actor_bounds(False)
    yaw = a.get_actor_rotation().yaw
    width = max(e.x, e.y) * 2.0
    if width > 420 and MESH["lvl2_wide"]:
        upper = MESH["lvl2_wide"]
    elif MESH["lvl2_double"]:
        upper = MESH["lvl2_double"]
    else:
        continue
    top = o.z + e.z
    ua = spawn_mesh(upper, "City_DW3_lvl2_{}".format(lvl2), o.x, o.y, top + 5, yaw)
    uo, ue = ua.get_actor_bounds(False)
    ul = ua.get_actor_location()
    ua.set_actor_location(unreal.Vector(
        ul.x + (o.x - uo.x), ul.y + (o.y - uo.y),
        ul.z + (top - (uo.z - ue.z))), False, False)
    if MESH["top_trim"]:
        uo, ue = ua.get_actor_bounds(False)
        t_top = uo.z + ue.z
        ta = spawn_mesh(MESH["top_trim"], "City_DW3_trim_{}".format(lvl2),
                        o.x, o.y, t_top + 5, yaw)
        to, te = ta.get_actor_bounds(False)
        tl = ta.get_actor_location()
        ta.set_actor_location(unreal.Vector(
            tl.x + (o.x - to.x), tl.y + (o.y - to.y),
            tl.z + (t_top - (to.z - te.z))), False, False)
    lvl2 += 1
unreal.log("[DW3] second stories added: {}".format(lvl2))

# ------------------------------------------------------------------
# 3) store signs: hang on every 3rd storefront, foldouts on sidewalk
# ------------------------------------------------------------------
signs = 0
folds = 0
for idx, (axis, sign, yaw, face_pos, along, w, mod_top, px, py) in enumerate(store_rows):
    if HANG_SIGNS and idx % 3 == 1:
        m = random.choice(HANG_SIGNS)
        if axis == "x":
            sx, sy = face_pos + sign * 70.0, py
        else:
            sx, sy = px, face_pos + sign * 70.0
        sa = spawn_mesh(m, "City_DW3_sign_{}".format(signs), sx, sy, mod_top - 60, yaw)
        oo, ee = sa.get_actor_bounds(False)
        al = sa.get_actor_location()
        sa.set_actor_location(unreal.Vector(
            al.x + (sx - oo.x), al.y + (sy - oo.y),
            al.z + (mod_top - 130 - (oo.z - ee.z))), False, False)
        signs += 1
    if FOLDOUT_SIGNS and idx % 5 == 2:
        if axis == "x":
            fx, fy = face_pos + sign * 190.0, py + random.uniform(-80, 80)
        else:
            fx, fy = px + random.uniform(-80, 80), face_pos + sign * 190.0
        if is_free(fx, fy, 150):
            spawn_mesh(random.choice(FOLDOUT_SIGNS), "City_DW3_fold_{}".format(folds),
                       fx, fy, SIDEWALK_Z + 5, random.uniform(0, 360), ground=SIDEWALK_Z)
            claim(fx, fy)
            folds += 1
unreal.log("[DW3] hanging signs {} foldouts {}".format(signs, folds))

# ------------------------------------------------------------------
# 4) street trees along sidewalk bands of every buildable block
# ------------------------------------------------------------------
TREES = [m for m in (MESH["tree_a"], MESH["tree_b"], MESH["tree_oak"]) if m]
trees = 0
if TREES:
    for gx in range(N):
        for gy in range(N):
            if (gx, gy) in LAKE:
                continue
            bx, by = cx(gx), cx(gy)
            band = 1000 - 130  # sidewalk band inset from block edge
            spots = []
            for t in (-600, 0, 600):
                spots.append((bx + t, by - band))
                spots.append((bx + t, by + band))
                spots.append((bx - band, by + t))
                spots.append((bx + band, by + t))
            random.shuffle(spots)
            placed = 0
            for sx, sy in spots:
                if placed >= 5:
                    break
                if not is_free(sx, sy, 260):
                    continue
                spawn_mesh(random.choice(TREES),
                           "City_DW3_tree_{}".format(trees),
                           sx, sy, SIDEWALK_Z + 5, random.uniform(0, 360),
                           ground=SIDEWALK_Z)
                claim(sx, sy)
                trees += 1
                placed += 1
unreal.log("[DW3] street trees planted: {}".format(trees))

# ------------------------------------------------------------------
# 5) park centerpieces
# ------------------------------------------------------------------
if MESH["fountain"]:
    fx, fy = cx(5), cx(5)
    spawn_mesh(MESH["fountain"], "City_DW3_fountain", fx, fy,
               SIDEWALK_Z + 5, 0, ground=SIDEWALK_Z)
    unreal.log("[DW3] fountain placed at park (5,5)")
if MESH["playground"]:
    px_, py_ = cx(2), cx(2)
    spawn_mesh(MESH["playground"], "City_DW3_playground", px_ + 300, py_ + 300,
               SIDEWALK_Z + 5, 30, ground=SIDEWALK_Z)
    unreal.log("[DW3] playground placed at park (2,2)")

# ------------------------------------------------------------------
# 6) background mountain vista beyond the lake (lake sits at +X, -Y)
# ------------------------------------------------------------------
if MESH["mountains"]:
    size = mesh_size(MESH["mountains"])
    scale = max(1.0, 24000.0 / max(size.x, 1.0))
    for i, (mx, my, myaw) in enumerate([
            (cx(5), START - STEP * 3.2, 0.0),
            (START - STEP * 3.4, cx(1), 90.0)]):
        ma = eas.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(mx, my, 0), unreal.Rotator(0, myaw, 0))
        ma.set_actor_label("City_DW3_mountains_{}".format(i))
        ma.static_mesh_component.set_static_mesh(MESH["mountains"])
        ma.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
        ma.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        fit_to_ground(ma, -50.0)
    unreal.log("[DW3] mountain vista placed (scale {:.1f})".format(scale))

saved = les.save_current_level()
unreal.log("[DW3] level saved: {}".format(saved))
