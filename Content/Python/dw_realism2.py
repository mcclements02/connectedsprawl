"""Realism pass v2: the street-level details that sell a real city.

  1. Parked cars along curbs (the single biggest GTA-city realism win).
  2. Crosswalk decals at intersections + grime decals at building bases.
  3. Remove awnings that clip building corners (fix from screenshot review).
  4. Roof-cap color variety (aerial view was too uniform).
  5. Extra sidewalk dressing: recycle bins, poster stands, lightpost pots.

Spawns actors => run WITHOUT -nullrhi:
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended
"""
import random
import unreal

random.seed(99)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
atools = unreal.AssetToolsHelpers.get_asset_tools()
ar = unreal.AssetRegistryHelpers.get_asset_registry()

les.load_level("/Game/Maps/TestMap")

N, BLOCK, ROAD = 7, 2000.0, 600.0
STEP = BLOCK + ROAD
START = -N * STEP / 2 + STEP / 2
LAKE_GX, LAKE_GY = {4, 5, 6}, {0, 1}
PARKS = {(1, 5), (2, 2), (5, 5)}
SIDEWALK_Z = 14.0


def cx(i):
    return START + i * STEP


def in_lake(x, y):
    gx = round((x - START) / STEP)
    gy = round((y - START) / STEP)
    return gx in LAKE_GX and gy in LAKE_GY


# idempotent wipe
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_R2_"):
        eas.destroy_actor(a)


def fit_ground(a, ground_z):
    o, e = a.get_actor_bounds(False)
    loc = a.get_actor_location()
    a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z + (ground_z - (o.z - e.z))), False, False)


def spawn(mesh, label, x, y, z, yaw=0.0, ground=None, mat=None):
    a = eas.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, yaw, 0))
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(mesh)
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    if mat:
        a.static_mesh_component.set_material(0, mat)
    if ground is not None:
        fit_ground(a, ground)
    return a


# ------------------------------------------------------------------
# 1) parked cars along curbs
# ------------------------------------------------------------------
car_meshes = []
for ad in ar.get_assets_by_class(
        unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh"), True):
    n = str(ad.asset_name)
    if n.startswith("Car_") and "/Game/Vehicles" in str(ad.package_name):
        m = unreal.load_asset(str(ad.package_name))
        if m:
            car_meshes.append(m)
unreal.log("[R2] car meshes found: {}".format(len(car_meshes)))

parked = 0
if car_meshes:
    # roads run between blocks; curb lanes sit just off the block edge
    for gi in range(N):          # block row/col index
        for axis in ("x", "y"):
            for side in (1, -1):
                # park along each block's street edge, 2-3 cars per edge
                for gj in range(N):
                    bx, by = cx(gi), cx(gj)
                    if in_lake(bx, by) or (gi, gj) in PARKS and axis == "x":
                        continue
                    if random.random() < 0.45:
                        continue
                    n_here = random.randint(1, 3)
                    for k in range(n_here):
                        along = random.uniform(-BLOCK / 2 + 250, BLOCK / 2 - 250)
                        lane = BLOCK / 2 + 130  # just off the sidewalk, on the road
                        if axis == "x":
                            px, py = bx + side * lane, by + along
                            yaw = 90 + (0 if side > 0 else 180)
                        else:
                            px, py = bx + along, by + side * lane
                            yaw = 0 + (0 if side > 0 else 180)
                        if in_lake(px, py):
                            continue
                        m = random.choice(car_meshes)
                        a = spawn(m, "City_R2_parked_{}".format(parked),
                                  px, py, 30, yaw + random.uniform(-2, 2), ground=1.0)
                        parked += 1
                        if parked >= 70:
                            break
                    if parked >= 70:
                        break
                if parked >= 70:
                    break
unreal.log("[R2] parked cars: {}".format(parked))

# ------------------------------------------------------------------
# 2) crosswalk + grime decals
# ------------------------------------------------------------------
def find_mi(name):
    for ad in ar.get_assets_by_class(
            unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant"), True):
        if str(ad.asset_name) == name:
            return unreal.load_asset(str(ad.package_name))
    return None


cross_mat = find_mi("MI_Decal_Road_Crosswalk")
grunge_mat = find_mi("MI_Decal_Grunge_b")

decals = 0
if cross_mat:
    for gi in range(N - 1):
        for gj in range(N - 1):
            ix, iy = cx(gi) + STEP / 2, cx(gj) + STEP / 2
            if in_lake(ix, iy):
                continue
            # two crosswalks per intersection (N and E arms)
            for (dx, dy, yaw) in ((0, -STEP / 2 + ROAD / 2 + 90, 90), (-STEP / 2 + ROAD / 2 + 90, 0, 0)):
                d = eas.spawn_actor_from_class(
                    unreal.DecalActor, unreal.Vector(ix + dx, iy + dy, 5),
                    unreal.Rotator(0, yaw, -90))
                d.set_actor_label("City_R2_cross_{}".format(decals))
                d.decal.set_decal_material(cross_mat)
                # DecalSize is read-only from python; scale the actor instead
                # (default decal box is 128x256x256)
                d.set_actor_scale3d(unreal.Vector(60 / 128.0, (ROAD / 2 + 40) / 256.0, 220 / 256.0))
                decals += 1
unreal.log("[R2] crosswalk decals: {}".format(decals))

grime = 0
if grunge_mat:
    for a in list(eas.get_all_level_actors()):
        if not a.get_actor_label().startswith("City_Bldg"):
            continue
        if random.random() < 0.6:
            continue
        o, e = a.get_actor_bounds(False)
        axis, sign = random.choice([("x", 1), ("x", -1), ("y", 1), ("y", -1)])
        if axis == "x":
            px, py, yaw = o.x + sign * (e.x + 5), o.y + random.uniform(-e.y * 0.5, e.y * 0.5), 0 if sign > 0 else 180
        else:
            px, py, yaw = o.x + random.uniform(-e.x * 0.5, e.x * 0.5), o.y + sign * (e.y + 5), 90 if sign > 0 else 270
        d = eas.spawn_actor_from_class(
            unreal.DecalActor, unreal.Vector(px, py, SIDEWALK_Z + 160), unreal.Rotator(0, yaw, 0))
        d.set_actor_label("City_R2_grime_{}".format(grime))
        d.decal.set_decal_material(grunge_mat)
        d.set_actor_scale3d(unreal.Vector(60 / 128.0, 170 / 256.0, 260 / 256.0))
        grime += 1
        if grime >= 60:
            break
unreal.log("[R2] grime decals: {}".format(grime))

# ------------------------------------------------------------------
# 3) remove corner-clipping awnings
# ------------------------------------------------------------------
bldg_bounds = []
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_Bldg"):
        bldg_bounds.append(a.get_actor_bounds(False))

removed_awn = 0
for a in list(eas.get_all_level_actors()):
    if not a.get_actor_label().startswith("City_DW_awning"):
        continue
    o, e = a.get_actor_bounds(False)
    # an awning is bad if it overlaps a building corner region on two axes
    for (bo, be) in bldg_bounds:
        ox = abs(o.x - bo.x) < be.x + 40
        oy = abs(o.y - bo.y) < be.y + 40
        near_cx = abs(abs(o.x - bo.x) - be.x) < 90
        near_cy = abs(abs(o.y - bo.y) - be.y) < 90
        if ox and oy and near_cx and near_cy:
            eas.destroy_actor(a)
            removed_awn += 1
            break
unreal.log("[R2] corner awnings removed: {}".format(removed_awn))

# ------------------------------------------------------------------
# 4) roof-cap variety
# ------------------------------------------------------------------
ground_master = unreal.load_asset("/Game/RealKit/Materials/M_RealGround")
def texset(set_id):
    out = {}
    for ad in ar.get_assets_by_path(unreal.Name("/Game/RealKit/Textures/" + set_id), recursive=True):
        n = str(ad.asset_name)
        for suf, k in (("_Color", "color"), ("_NormalGL", "normal"), ("_Roughness", "rough"), ("_AmbientOcclusion", "ao")):
            if n.endswith(suf):
                out[k] = unreal.load_asset(str(ad.package_name))
    return out


roof_variants = []
for name, set_id, tint in (("MI_Roof_Dark", "Asphalt031", (0.4, 0.4, 0.42, 1)),
                           ("MI_Roof_Brown", "Ground037", (0.75, 0.65, 0.55, 1)),
                           ("MI_Roof_Pale", "Concrete046", (1.0, 1.0, 1.0, 1))):
    path = "/Game/RealKit/Materials/" + name
    if eal.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = atools.create_asset(name, "/Game/RealKit/Materials",
                                 unreal.MaterialInstanceConstant,
                                 unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, ground_master)
        ts = texset(set_id)
        for pn, k in (("AlbedoTex", "color"), ("NormalTex", "normal"), ("RoughTex", "rough"), ("AOTex", "ao")):
            if k in ts:
                MEL.set_material_instance_texture_parameter_value(mi, pn, ts[k])
        MEL.set_material_instance_scalar_parameter_value(mi, "TileSize", 450.0)
        MEL.set_material_instance_vector_parameter_value(mi, "AlbedoTint", unreal.LinearColor(*tint))
        MEL.update_material_instance(mi)
        eal.save_loaded_asset(mi)
    roof_variants.append(mi)

retinted = 0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_RealProp_roofcap"):
        a.static_mesh_component.set_material(0, random.choice(roof_variants))
        retinted += 1
unreal.log("[R2] roof caps retinted: {}".format(retinted))

# ------------------------------------------------------------------
# 5) extra dressing: recycle bins, poster stands, lightpost pots
# ------------------------------------------------------------------
DW = "/Game/Downtown_West/Assets"
extra_meshes = {
    "recycle": unreal.load_asset(DW + "/props/prop_recycle_bin/SM_prop_recycle_bin"),
    "poster": unreal.load_asset(DW + "/props/prop_poster_stands/SM_poster_parts_a"),
    "pots": unreal.load_asset(DW + "/props/props_light_post/SM_lightpost_lightpost_plant_pots"),
}
extra = 0
for gi in range(N):
    for gj in range(N):
        bx, by = cx(gi), cx(gj)
        if in_lake(bx, by) or (gi, gj) in PARKS:
            continue
        edge = BLOCK / 2 - 55
        for key, prob in (("recycle", 0.5), ("poster", 0.3), ("pots", 0.4)):
            m = extra_meshes.get(key)
            if m and random.random() < prob:
                s = random.choice([(edge, random.uniform(-700, 700)), (-edge, random.uniform(-700, 700)),
                                   (random.uniform(-700, 700), edge), (random.uniform(-700, 700), -edge)])
                spawn(m, "City_R2_{}_{}".format(key, extra), bx + s[0], by + s[1],
                      SIDEWALK_Z + 5, random.uniform(0, 360), ground=SIDEWALK_Z)
                extra += 1
unreal.log("[R2] extra dressing: {}".format(extra))

saved = les.save_current_level()
unreal.log("[R2] level saved: {} — DONE".format(saved))
