"""Integrate the Downtown West Modular Pack into TestMap.

  1. Every SM_Streetlight actor gets the real DW streetlight mesh (and its
     leftover glow-sphere is removed).
  2. RealKit benches/hydrants swap to the DW game-ready versions.
  3. Downtown 3x3 blocks get storefront ground-floor modules + awnings on
     street-facing building faces — the GTA street feel.
  4. Parks get a plaza pass: hero trees, cafe tables with umbrellas and
     chairs, a food cart, flower pots.

Spawning actors => run WITHOUT -nullrhi:
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended
"""
import random
import unreal

random.seed(77)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

DW = "/Game/Downtown_West/Assets"
M = {
    "streetlight": DW + "/props/props_streetlight/SM_light_streetlight",
    "bench": DW + "/props/prop_bench_wood/SM_bench_wood_a",
    "hydrant": DW + "/props/prop_fire_hydrant/SM_fire_hydrant",
    "wall5": DW + "/building_b/SM_build_b_mod_lvl1_storefront_b_wall5m",
    "wall3": DW + "/building_b/SM_build_b_mod_lvl1_storefront_b_wall3m",
    "awning": DW + "/awnings/SM_building_awning_b",
    "tree": DW + "/foliage/tree_narrowleaf/SM_tree_narrowleaf_b",
    "table_umb": DW + "/props/prop_table/SM_prop_table_umbrella_a",
    "chair": DW + "/props/prop_table/SM_prop_dining_chair_a",
    "cart": DW + "/props/prop_hotdog_cart/SM_food_cart",
    "pot": DW + "/props/prop_pots/SM_pot_a",
    "recycle": DW + "/props/prop_recycle_bin/SM_prop_recycle_bin",
}
MESH = {}
for k, p in M.items():
    MESH[k] = unreal.load_asset(p)
    if not MESH[k]:
        unreal.log_warning("[DW] missing mesh {} -> {}".format(k, p))

SIDEWALK_Z = 14.0


def fit_to_ground(a, ground_z, want_x=None, want_y=None):
    """Self-correcting placement: shift actor so bounds bottom sits on
    ground_z (and optionally bounds center hits want_x/want_y)."""
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


# wipe any previous DW pass (idempotent)
wiped = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_DW_"):
        eas.destroy_actor(a)
        wiped += 1
unreal.log("[DW] wiped {} previous DW actors".format(wiped))

# ------------------------------------------------------------------
# 1) streetlights: swap mesh, drop glow spheres
# ------------------------------------------------------------------
lights, spheres = [], []
for a in eas.get_all_level_actors():
    smc = getattr(a, "static_mesh_component", None)
    if not smc or not smc.static_mesh:
        continue
    n = smc.static_mesh.get_name()
    if n == "SM_Streetlight":
        lights.append(a)
    elif n == "Sphere" and a.get_actor_label().startswith("City_Detail"):
        spheres.append(a)

swapped = 0
if MESH["streetlight"]:
    for a in lights:
        o, e = a.get_actor_bounds(False)
        ground_z = o.z - e.z
        smc = a.static_mesh_component
        smc.set_static_mesh(MESH["streetlight"])
        smc.set_editor_property("override_materials", [])
        a.set_actor_scale3d(unreal.Vector(1, 1, 1))
        fit_to_ground(a, max(ground_z, 0.0))
        swapped += 1

killed_glow = 0
for s in list(spheres):
    sl = s.get_actor_location()
    for a in lights:
        al = a.get_actor_location()
        if (sl.x - al.x) ** 2 + (sl.y - al.y) ** 2 < 250 * 250:
            eas.destroy_actor(s)
            killed_glow += 1
            break
unreal.log("[DW] streetlights swapped {} / glow spheres removed {}".format(swapped, killed_glow))

# ------------------------------------------------------------------
# 2) benches + hydrants -> DW versions
# ------------------------------------------------------------------
upgraded = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    smc = getattr(a, "static_mesh_component", None)
    if not smc:
        continue
    new_mesh = None
    if label.startswith("City_RealProp_") and ("bench" in label) and MESH["bench"]:
        new_mesh = MESH["bench"]
    elif label.startswith("City_RealProp_hydrant") and MESH["hydrant"]:
        new_mesh = MESH["hydrant"]
    if new_mesh:
        o, e = a.get_actor_bounds(False)
        smc.set_static_mesh(new_mesh)
        smc.set_editor_property("override_materials", [])
        fit_to_ground(a, SIDEWALK_Z)
        upgraded += 1
unreal.log("[DW] benches/hydrants upgraded: {}".format(upgraded))

# ------------------------------------------------------------------
# 3) storefront ground floors + awnings on downtown blocks
# ------------------------------------------------------------------
N, BLOCK, ROAD = 7, 2000.0, 600.0
STEP = BLOCK + ROAD
START = -N * STEP / 2 + STEP / 2


def cx(i):
    return START + i * STEP


downtown = set()
for gx in range(2, 5):
    for gy in range(2, 5):
        downtown.add((gx, gy))


def block_of(pos):
    gx = round((pos.x - START) / STEP)
    gy = round((pos.y - START) / STEP)
    return int(gx), int(gy)


storefronts = 0
awnings = 0
for a in list(eas.get_all_level_actors()):
    if not a.get_actor_label().startswith("City_Bldg"):
        continue
    loc = a.get_actor_location()
    gxy = block_of(loc)
    if gxy not in downtown:
        continue
    bx, by = cx(gxy[0]), cx(gxy[1])
    o, e = a.get_actor_bounds(False)
    # four candidate faces: (axis, sign)
    for axis, sign in (("x", 1), ("x", -1), ("y", 1), ("y", -1)):
        if axis == "x":
            face_pos = o.x + sign * e.x
            street_edge = bx + (1000 if sign > 0 else -1000)
            length = e.y * 2
        else:
            face_pos = o.y + sign * e.y
            street_edge = by + (1000 if sign > 0 else -1000)
            length = e.x * 2
        if abs(face_pos - street_edge) > 430:
            continue  # face not on the street side
        if length < 320:
            continue
        # tile 5m modules, fill one 3m if room
        n5 = int(length // 510)
        used = n5 * 510
        fill3 = MESH["wall3"] and (length - used) >= 320
        mods = [(MESH["wall5"], 510)] * n5 + ([(MESH["wall3"], 310)] if fill3 else [])
        if not mods or not MESH["wall5"]:
            continue
        total = sum(w for _, w in mods)
        along0 = -total / 2.0
        # module front (+X local) must point AWAY from the building face
        if axis == "x":
            yaw = 0.0 if sign > 0 else 180.0
        else:
            yaw = 90.0 if sign > 0 else 270.0
        out = 16.0 * sign  # push slightly out of the face
        cursor = along0
        for i, (mesh, w) in enumerate(mods):
            center_along = cursor + w / 2.0
            cursor += w
            if axis == "x":
                px, py = face_pos + out, o.y + center_along
            else:
                px, py = o.x + center_along, face_pos + out
            sa = spawn_mesh(mesh, "City_DW_store_{}_{}".format(storefronts, i),
                            px, py, SIDEWALK_Z + 5, yaw)
            fit_to_ground(sa, SIDEWALK_Z, want_x=px, want_y=py)
            storefronts += 1
            # awning above every other module
            if MESH["awning"] and i % 2 == 0:
                if axis == "x":
                    ax_, ay_ = face_pos + sign * 55.0, py
                else:
                    ax_, ay_ = px, face_pos + sign * 55.0
                aw = spawn_mesh(MESH["awning"], "City_DW_awning_{}".format(awnings),
                                ax_, ay_, SIDEWALK_Z + 300, yaw)
                # awning hangs at top of storefront, not on ground
                oo, ee = aw.get_actor_bounds(False)
                al = aw.get_actor_location()
                aw.set_actor_location(unreal.Vector(
                    al.x + (ax_ - oo.x), al.y + (ay_ - oo.y),
                    al.z + (SIDEWALK_Z + 395 - (oo.z - ee.z))), False, False)
                awnings += 1
unreal.log("[DW] storefront modules {} awnings {}".format(storefronts, awnings))

# ------------------------------------------------------------------
# 4) park plazas
# ------------------------------------------------------------------
PARKS = [(1, 5), (2, 2), (5, 5)]
plaza = 0
for pi, (gx, gy) in enumerate(PARKS):
    bx, by = cx(gx), cx(gy)
    if MESH["tree"]:
        for t in range(2):
            spawn_mesh(MESH["tree"], "City_DW_tree_{}_{}".format(pi, t),
                       bx + random.uniform(-500, 500), by + random.uniform(-500, 500),
                       SIDEWALK_Z + 5, random.uniform(0, 360), ground=SIDEWALK_Z)
            plaza += 1
    for s in range(2):
        tx = bx + random.uniform(-650, 650)
        ty = by + random.uniform(-650, 650)
        if MESH["table_umb"]:
            spawn_mesh(MESH["table_umb"], "City_DW_tableumb_{}_{}".format(pi, s),
                       tx, ty, SIDEWALK_Z + 5, random.uniform(0, 360), ground=SIDEWALK_Z)
            plaza += 1
        if MESH["chair"]:
            for c in range(2):
                ang = random.uniform(0, 6.28)
                spawn_mesh(MESH["chair"], "City_DW_chair_{}_{}_{}".format(pi, s, c),
                           tx + 130 * (1 if c else -1), ty + random.uniform(-60, 60),
                           SIDEWALK_Z + 5, random.uniform(0, 360), ground=SIDEWALK_Z)
                plaza += 1
    if MESH["pot"]:
        for p in range(3):
            spawn_mesh(MESH["pot"], "City_DW_pot_{}_{}".format(pi, p),
                       bx + random.uniform(-800, 800), by + random.uniform(-800, 800),
                       SIDEWALK_Z + 5, 0, ground=SIDEWALK_Z)
            plaza += 1

if MESH["cart"]:
    bx, by = cx(2), cx(2)
    spawn_mesh(MESH["cart"], "City_DW_foodcart", bx + 700, by - 700,
               SIDEWALK_Z + 5, 45, ground=SIDEWALK_Z)
    plaza += 1
unreal.log("[DW] plaza items placed: {}".format(plaza))

saved = les.save_current_level()
unreal.log("[DW] level saved: {} — DONE".format(saved))
