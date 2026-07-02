"""RealKit part B: turn the neon greybox into a realistic daytime city.

  1. Re-parent the city's shared material instances onto the photoreal
     world-aligned masters (M_RealGround / M_RealFacade) with ambientCG sets.
  2. Strip the non-realistic Morales-era dressing (holo creatures, neon
     storefronts, billboard ads/pedestrians); keep graffiti (it reads real).
  3. Restore skeletal pedestrians hidden by the billboard pass.
  4. Daytime realistic lighting + post-process; remove duplicate light rigs.
  5. Scatter Poly Haven street props (lamps stay; hydrants, cans, benches,
     manholes, barriers, alley litter).

Run after realkit_import.py — NOTE: spawning actors needs a real RHI, do
NOT pass -nullrhi (EditorActorSubsystem.spawn_actor_from_class asserts in
viewport hit-proxy code under null RHI):
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended
"""
import random
import unreal

random.seed(42)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()

les.load_level("/Game/Maps/TestMap")

MAT_ROOT = "/Game/RealKit/Materials"
TEX_ROOT = "/Game/RealKit/Textures"
PROP_ROOT = "/Game/RealKit/Props"

ground_master = unreal.load_asset(MAT_ROOT + "/M_RealGround")
facade_master = unreal.load_asset(MAT_ROOT + "/M_RealFacade")
assert ground_master and facade_master, "run realkit_import.py first"


# ------------------------------------------------------------------
# helpers
# ------------------------------------------------------------------
def find_asset(name, class_name):
    for ad in ar.get_assets_by_class(
            unreal.TopLevelAssetPath("/Script/Engine", class_name), True):
        if str(ad.asset_name) == name:
            return str(ad.package_name)
    return None


def texture_set(set_id):
    """Return {kind: Texture2D} for an imported ambientCG set."""
    out = {}
    for ad in ar.get_assets_by_path(unreal.Name("{}/{}".format(TEX_ROOT, set_id)), recursive=True):
        n = str(ad.asset_name)
        for suffix, kind in (("_Color", "color"), ("_NormalGL", "normal"),
                             ("_Roughness", "rough"), ("_AmbientOcclusion", "ao")):
            if n.endswith(suffix):
                out[kind] = unreal.load_asset(str(ad.package_name))
    return out


def retarget_mic(mi_name, set_id, master, tile, rough_mul=1.0, tint=None):
    pkg = find_asset(mi_name, "MaterialInstanceConstant")
    if not pkg:
        unreal.log_warning("[Apply] MIC not found: {}".format(mi_name))
        return
    mi = unreal.load_asset(pkg)
    texs = texture_set(set_id)
    if "color" not in texs:
        unreal.log_warning("[Apply] texture set {} incomplete".format(set_id))
        return
    MEL.set_material_instance_parent(mi, master)
    MEL.set_material_instance_texture_parameter_value(mi, "AlbedoTex", texs["color"])
    if "normal" in texs:
        MEL.set_material_instance_texture_parameter_value(mi, "NormalTex", texs["normal"])
    if "rough" in texs:
        MEL.set_material_instance_texture_parameter_value(mi, "RoughTex", texs["rough"])
    if "ao" in texs:
        MEL.set_material_instance_texture_parameter_value(mi, "AOTex", texs["ao"])
    MEL.set_material_instance_scalar_parameter_value(mi, "TileSize", float(tile))
    MEL.set_material_instance_scalar_parameter_value(mi, "RoughnessMul", float(rough_mul))
    if tint:
        MEL.set_material_instance_vector_parameter_value(
            mi, "AlbedoTint", unreal.LinearColor(*tint))
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    unreal.log("[Apply] retargeted {} -> {}".format(mi_name, set_id))


# ------------------------------------------------------------------
# 1) retarget shared MICs to photoreal sets
# ------------------------------------------------------------------
G, F = ground_master, facade_master
RETARGETS = [
    ("MI_WetAsphalt_Road",      "Asphalt031",     G, 450, 1.0, None),
    ("MI_Asphalt",              "Asphalt031",     G, 450, 1.0, None),
    ("MI_WetConcrete_Sidewalk", "PavingStones142", G, 280, 1.0, None),
    ("MI_Sidewalk",             "PavingStones142", G, 280, 1.0, None),
    ("MI_Grass",                "Grass004",       G, 350, 1.0, None),
    ("MI_StdOpaque_Concrete",   "Concrete034",    G, 350, 1.0, None),
    ("MI_Sand",                 "Ground037",      G, 400, 1.0, None),
    ("MI_Metal",                "Metal049A",      F, 220, 1.0, None),
    ("MI_Facade_Tan",           "Bricks085",      F, 450, 1.0, None),
    ("MI_Facade_Pale",          "Plaster001",     F, 520, 1.0, (0.95, 0.93, 0.88, 1)),
    ("MI_Facade_Slate",         "Facade006",      F, 1500, 1.0, None),
    ("MI_Facade_Teal",          "Facade017",      F, 1400, 1.0, None),
    ("MI_Facade_Warm",          "Facade001",      F, 1300, 1.0, None),
    ("MI_Bldg_Tan",             "Plaster007",     F, 520, 1.0, None),
    ("MI_Bldg_Slate",           "Facade006",      F, 1500, 1.0, None),
    ("MI_Bldg_Brick",           "Bricks101",      F, 420, 1.0, None),
    ("MI_Bldg_Conc",            "Concrete046",    F, 420, 1.0, None),
    ("MI_Bldg_Glass",           "Facade017",      F, 1400, 1.0, None),
]
for row in RETARGETS:
    retarget_mic(*row)


# ------------------------------------------------------------------
# 2) strip non-realistic dressing
# ------------------------------------------------------------------
def any_material_contains(a, needle):
    smc = getattr(a, "static_mesh_component", None)
    if not smc:
        return False
    for i in range(smc.get_num_materials()):
        m = smc.get_material(i)
        if m and needle.lower() in m.get_name().lower():
            return True
    return False


killed = {"creature": 0, "neon": 0, "billboard": 0, "human": 0, "lightdup": 0}
for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    cls = a.get_class().get_name()
    if label.startswith("City_Creature_"):
        eas.destroy_actor(a); killed["creature"] += 1
    elif label.startswith("City_Morales_") and not any_material_contains(a, "graffiti"):
        eas.destroy_actor(a); killed["neon"] += 1
    elif label.startswith("City_Ped_") and cls == "StaticMeshActor":
        eas.destroy_actor(a); killed["billboard"] += 1
    elif label.startswith("City_Human_"):
        eas.destroy_actor(a); killed["human"] += 1
    elif label in ("SkyAtmosphere", "SkyLight", "PostProcessVolume", "PlayerStart"):
        # duplicates of the Fix* rig
        eas.destroy_actor(a); killed["lightdup"] += 1
unreal.log("[Apply] removed: {}".format(killed))

# restore the skeletal pedestrians the billboard pass hid
restored = 0
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "SprawlPedestrian":
        mc = a.get_component_by_class(unreal.SkeletalMeshComponent)
        if mc:
            mc.set_visibility(True)
            restored += 1
unreal.log("[Apply] pedestrian meshes restored: {}".format(restored))


# ------------------------------------------------------------------
# 3) daytime lighting + post
# ------------------------------------------------------------------
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label == "FixDirectionalLight":
        a.set_actor_rotation(unreal.Rotator(0.0, -35.0, -42.0), False)
        c = a.light_component
        c.set_editor_property("intensity", 10.0)
        c.set_editor_property("light_color", unreal.Color(255, 244, 229, 255))
        c.set_editor_property("atmosphere_sun_light", True)
        c.set_editor_property("cast_shadows", True)
    elif label == "FixSkyLight":
        c = a.light_component
        c.set_editor_property("intensity", 1.2)
        c.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_CAPTURED_SCENE)
    elif label == "FixHeightFog":
        c = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if c:
            c.set_editor_property("fog_density", 0.012)
            c.set_editor_property("fog_height_falloff", 0.18)
            c.set_editor_property("start_distance", 2500.0)
    elif label == "FixPostProcess":
        s = a.settings
        s.set_editor_property("override_auto_exposure_bias", True)
        s.set_editor_property("auto_exposure_bias", 0.8)
        s.set_editor_property("override_auto_exposure_min_brightness", True)
        s.set_editor_property("auto_exposure_min_brightness", 0.45)
        s.set_editor_property("override_auto_exposure_max_brightness", True)
        s.set_editor_property("auto_exposure_max_brightness", 1.9)
        s.set_editor_property("override_bloom_intensity", True)
        s.set_editor_property("bloom_intensity", 0.35)
        s.set_editor_property("override_ambient_occlusion_intensity", True)
        s.set_editor_property("ambient_occlusion_intensity", 0.55)
        s.set_editor_property("override_ambient_occlusion_radius", True)
        s.set_editor_property("ambient_occlusion_radius", 180.0)
        s.set_editor_property("override_vignette_intensity", True)
        s.set_editor_property("vignette_intensity", 0.3)
        s.set_editor_property("override_white_temp", True)
        s.set_editor_property("white_temp", 6600.0)
        a.settings = s
unreal.log("[Apply] daytime lighting configured")


# ------------------------------------------------------------------
# 4) street props
# ------------------------------------------------------------------
def prop_mesh(slug):
    # multi-part glTFs import as several meshes (body, chain, lid...);
    # the main body is the one with the largest bounding volume
    best, best_vol = None, -1.0
    for ad in ar.get_assets_by_path(unreal.Name("{}/{}".format(PROP_ROOT, slug)), recursive=True):
        if str(ad.asset_class_path.asset_name) != "StaticMesh":
            continue
        sm = unreal.load_asset(str(ad.package_name))
        if sm:
            e = sm.get_bounds().box_extent
            vol = max(e.x, 1.0) * max(e.y, 1.0) * max(e.z, 1.0)
            if vol > best_vol:
                best, best_vol = sm, vol
    if best:
        bs = best.get_editor_property("body_setup")
        if bs:
            bs.set_editor_property(
                "collision_trace_flag",
                unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            eal.save_loaded_asset(best)
    return best


PROPS = {s: prop_mesh(s) for s in [
    "fire_hydrant", "metal_trash_can", "painted_wooden_bench",
    "water_manhole_cover", "utility_box_02", "concrete_road_barrier",
    "trashbag", "old_tyre"]}
for k, v in PROPS.items():
    unreal.log("[Apply] prop {}: {}".format(k, v.get_name() if v else "MISSING"))

# wipe any previous RealProp pass so this stays idempotent
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_RealProp_"):
        eas.destroy_actor(a)


def place(mesh, label, x, y, z, yaw=None, scale=1.0):
    if not mesh:
        return None
    a = eas.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, z),
        unreal.Rotator(0, yaw if yaw is not None else random.uniform(0, 360), 0))
    a.set_actor_label("City_RealProp_" + label)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    a.static_mesh_component.set_static_mesh(mesh)
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return a


# city grid constants from build_city.py
N, BLOCK, ROAD = 7, 2000.0, 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2
LAKE_GX, LAKE_GY = {4, 5, 6}, {0, 1}
PARKS = {(1, 5), (2, 2), (5, 5)}
SIDEWALK_Z = 14.0
ROAD_Z = 1.5


def cx(i):
    return START + i * STEP


placed = 0
for gx in range(N):
    for gy in range(N):
        if gx in LAKE_GX and gy in LAKE_GY:
            continue
        bx, by = cx(gx), cx(gy)
        edge = BLOCK / 2 - 110
        if (gx, gy) in PARKS:
            for i in range(3):
                placed += bool(place(PROPS["painted_wooden_bench"],
                                     "bench_{}_{}_{}".format(gx, gy, i),
                                     bx + random.uniform(-600, 600),
                                     by + random.uniform(-600, 600), SIDEWALK_Z))
            continue
        # one hydrant per block, on a random street edge
        side = random.choice([(edge, random.uniform(-edge, edge)),
                              (-edge, random.uniform(-edge, edge)),
                              (random.uniform(-edge, edge), edge),
                              (random.uniform(-edge, edge), -edge)])
        placed += bool(place(PROPS["fire_hydrant"],
                             "hydrant_{}_{}".format(gx, gy),
                             bx + side[0], by + side[1], SIDEWALK_Z))
        # trash can near one corner
        cxs, cys = random.choice([(1, 1), (1, -1), (-1, 1), (-1, -1)])
        placed += bool(place(PROPS["metal_trash_can"],
                             "trash_{}_{}".format(gx, gy),
                             bx + cxs * edge, by + cys * edge, SIDEWALK_Z))
        # utility box occasionally
        if random.random() < 0.35:
            placed += bool(place(PROPS["utility_box_02"],
                                 "utility_{}_{}".format(gx, gy),
                                 bx - side[0], by - side[1], SIDEWALK_Z,
                                 yaw=random.choice([0, 90, 180, 270])))
        # sidewalk bench occasionally
        if random.random() < 0.30:
            placed += bool(place(PROPS["painted_wooden_bench"],
                                 "swbench_{}_{}".format(gx, gy),
                                 bx + random.uniform(-edge, edge), by - edge,
                                 SIDEWALK_Z, yaw=0))

# manhole covers at road intersections
for gx in range(N - 1):
    for gy in range(N - 1):
        if gx in LAKE_GX and gy in LAKE_GY:
            continue
        placed += bool(place(PROPS["water_manhole_cover"],
                             "manhole_{}_{}".format(gx, gy),
                             cx(gx) + STEP / 2 + random.uniform(-120, 120),
                             cx(gy) + STEP / 2 + random.uniform(-120, 120),
                             ROAD_Z))

# concrete barriers along the lake edge
lx = cx(4) - STEP / 2 - ROAD / 2
for i in range(8):
    placed += bool(place(PROPS["concrete_road_barrier"], "barrier_{}".format(i),
                         lx, cx(0) - STEP / 2 + i * 700, ROAD_Z, yaw=90))

# alley litter
for i in range(18):
    gx, gy = random.randint(0, N - 1), random.randint(0, N - 1)
    if gx in LAKE_GX and gy in LAKE_GY or (gx, gy) in PARKS:
        continue
    mesh = PROPS["trashbag"] if i % 2 else PROPS["old_tyre"]
    placed += bool(place(mesh, "litter_{}".format(i),
                         cx(gx) + random.uniform(-700, 700),
                         cx(gy) + random.uniform(-700, 700), SIDEWALK_Z))

unreal.log("[Apply] props placed: {}".format(placed))

saved = les.save_current_level()
unreal.log("[Apply] level saved: {} — DONE".format(saved))
