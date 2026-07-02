"""RealKit fix pass 3 — from the verified diagnosis:

Root cause of black shadows: SkyLight real_time_capture never completes in
short headless sessions -> zero ambient. Fix: deterministic captured-scene
skylight (movable) + recapture, pinned exposure (min==max snaps in one
frame), halved fog (it was masquerading as fill light), differentiated
facade tints (fixes both dark glass and the uniform-gray aerial), water
de-chromed, road markings de-glowed, props moved off the building line,
and an in-place M_RealFacade rebuild: roof-aware UV projection (roofs were
sampling a single smeared texel row) + normal flattening.

Run WITHOUT -nullrhi (spawns actors):
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended
"""
import random
import unreal

random.seed(43)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
atools = unreal.AssetToolsHelpers.get_asset_tools()

les.load_level("/Game/Maps/TestMap")

TEX_ROOT = "/Game/RealKit/Textures"
MAT_ROOT = "/Game/RealKit/Materials"


def find_mic(name):
    for ad in ar.get_assets_by_class(
            unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant"), True):
        if str(ad.asset_name) == name:
            return unreal.load_asset(str(ad.package_name))
    return None


def texture_set(set_id):
    out = {}
    for ad in ar.get_assets_by_path(unreal.Name("{}/{}".format(TEX_ROOT, set_id)), recursive=True):
        n = str(ad.asset_name)
        for suffix, kind in (("_Color", "color"), ("_NormalGL", "normal"),
                             ("_Roughness", "rough"), ("_AmbientOcclusion", "ao")):
            if n.endswith(suffix):
                out[kind] = unreal.load_asset(str(ad.package_name))
    return out


# ============================================================
# 1) New facade master (roof-aware UVs + soft normals), re-parent MICs.
#    NOT in-place: delete_all_material_expressions asserts (!IsRooted)
#    in 5.7 commandlet runs — build M_RealFacade2 fresh instead.
# ============================================================
if eal.does_asset_exist(MAT_ROOT + "/M_RealFacade2"):
    eal.delete_asset(MAT_ROOT + "/M_RealFacade2")
mat = atools.create_asset("M_RealFacade2", MAT_ROOT, unreal.Material,
                          unreal.MaterialFactoryNew())

wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1500, 0)
tile = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1500, 300)
tile.set_editor_property("parameter_name", "TileSize")
tile.set_editor_property("default_value", 400.0)


def mask(x, y, r, g, b):
    m = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x, y)
    m.set_editor_property("r", r)
    m.set_editor_property("g", g)
    m.set_editor_property("b", b)
    m.set_editor_property("a", False)
    return m


# facade projection: U = x+y, V = z
mx = mask(-1300, -120, True, False, False)
my = mask(-1300, -40, False, True, False)
mz = mask(-1300, 40, False, False, True)
for m in (mx, my, mz):
    MEL.connect_material_expressions(wp, "", m, "")
add_xy = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -1150, -80)
MEL.connect_material_expressions(mx, "", add_xy, "A")
MEL.connect_material_expressions(my, "", add_xy, "B")
append_f = MEL.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1020, -20)
MEL.connect_material_expressions(add_xy, "", append_f, "A")
MEL.connect_material_expressions(mz, "", append_f, "B")

# ground projection for roofs: U,V = x,y
m_rg = mask(-1300, 140, True, True, False)
MEL.connect_material_expressions(wp, "", m_rg, "")

# blend factor: |vertex normal z| remapped so walls=0, roofs=1
vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1300, 260)
vn_z = mask(-1170, 260, False, False, True)
MEL.connect_material_expressions(vn, "", vn_z, "")
vabs = MEL.create_material_expression(mat, unreal.MaterialExpressionAbs, -1060, 260)
MEL.connect_material_expressions(vn_z, "", vabs, "")
vsub = MEL.create_material_expression(mat, unreal.MaterialExpressionSubtract, -960, 260)
vsub.set_editor_property("const_b", 0.5)
MEL.connect_material_expressions(vabs, "", vsub, "A")
vmul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -860, 260)
vmul.set_editor_property("const_b", 4.0)
MEL.connect_material_expressions(vsub, "", vmul, "A")
vsat = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -760, 260)
MEL.connect_material_expressions(vmul, "", vsat, "")

uv_lerp = MEL.create_material_expression(
    mat, unreal.MaterialExpressionLinearInterpolate, -880, 20)
MEL.connect_material_expressions(append_f, "", uv_lerp, "A")
MEL.connect_material_expressions(m_rg, "", uv_lerp, "B")
MEL.connect_material_expressions(vsat, "", uv_lerp, "Alpha")

div = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -700, 60)
MEL.connect_material_expressions(uv_lerp, "", div, "A")
MEL.connect_material_expressions(tile, "", div, "B")


def tex_param(pname, y, sampler_type):
    node = MEL.create_material_expression(
        mat, unreal.MaterialExpressionTextureSampleParameter2D, -480, y)
    node.set_editor_property("parameter_name", pname)
    node.set_editor_property("sampler_type", sampler_type)
    MEL.connect_material_expressions(div, "", node, "UVs")
    return node


albedo = tex_param("AlbedoTex", -220, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
normal = tex_param("NormalTex", 20, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
rough = tex_param("RoughTex", 260, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
ao = tex_param("AOTex", 500, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)

defaults = texture_set("Concrete034")
for node, key in ((albedo, "color"), (normal, "normal"), (rough, "rough"), (ao, "ao")):
    if key in defaults:
        node.set_editor_property("texture", defaults[key])

tint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -480, -420)
tint.set_editor_property("parameter_name", "AlbedoTint")
tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
mul_tint = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, -220)
MEL.connect_material_expressions(albedo, "", mul_tint, "A")
MEL.connect_material_expressions(tint, "", mul_tint, "B")

# soft normals: lerp flat (0,0,1) toward map by NormalStrength
flat_n = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 100)
flat_n.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
n_str = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 180)
n_str.set_editor_property("parameter_name", "NormalStrength")
n_str.set_editor_property("default_value", 0.4)
n_lerp = MEL.create_material_expression(
    mat, unreal.MaterialExpressionLinearInterpolate, -140, 60)
MEL.connect_material_expressions(flat_n, "", n_lerp, "A")
MEL.connect_material_expressions(normal, "", n_lerp, "B")
MEL.connect_material_expressions(n_str, "", n_lerp, "Alpha")

rough_mul = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -480, 380)
rough_mul.set_editor_property("parameter_name", "RoughnessMul")
rough_mul.set_editor_property("default_value", 1.0)
mul_rough = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 300)
MEL.connect_material_expressions(rough, "", mul_rough, "A")
MEL.connect_material_expressions(rough_mul, "", mul_rough, "B")

MEL.connect_material_property(mul_tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(n_lerp, "", unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(mul_rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
MEL.recompile_material(mat)
eal.save_loaded_asset(mat)
unreal.log("[Fix3] M_RealFacade2 built (roof UVs + soft normals)")

# re-parent every MIC from the old facade master to the new one
reparented = 0
for ad in ar.get_assets_by_class(
        unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant"), True):
    mi = unreal.load_asset(str(ad.package_name))
    p = mi.get_editor_property("parent") if mi else None
    if p and p.get_name() == "M_RealFacade":
        MEL.set_material_instance_parent(mi, mat)
        MEL.update_material_instance(mi)
        eal.save_loaded_asset(mi)
        reparented += 1
unreal.log("[Fix3] re-parented {} MICs to M_RealFacade2".format(reparented))

# ============================================================
# 2) Differentiated facade tints + retunes
# ============================================================
def set_tint(name, rgba):
    mi = find_mic(name)
    if mi:
        MEL.set_material_instance_vector_parameter_value(
            mi, "AlbedoTint", unreal.LinearColor(*rgba))
        MEL.update_material_instance(mi)
        eal.save_loaded_asset(mi)
        unreal.log("[Fix3] tint {} = {}".format(name, rgba))


set_tint("MI_Bldg_Brick", (1.0, 0.62, 0.5, 1))
set_tint("MI_Facade_Tan", (1.05, 0.9, 0.72, 1))
set_tint("MI_Facade_Warm", (1.0, 0.85, 0.72, 1))
set_tint("MI_Facade_Slate", (0.75, 0.78, 0.85, 1))
set_tint("MI_Facade_Pale", (1.0, 0.95, 0.85, 1))
set_tint("MI_Bldg_Tan", (1.0, 0.93, 0.8, 1))
set_tint("MI_Facade_Teal", (0.8, 0.9, 1.0, 1))
set_tint("MI_Bldg_Glass", (1.7, 1.7, 1.8, 1))
set_tint("MI_Asphalt", (1.6, 1.6, 1.6, 1))
set_tint("MI_WetAsphalt_Road", (1.6, 1.6, 1.6, 1))

# MI_Bldg_Slate: dark Facade006 -> brighter concrete
mi = find_mic("MI_Bldg_Slate")
if mi:
    texs = texture_set("Concrete046")
    for pname, kind in (("AlbedoTex", "color"), ("NormalTex", "normal"),
                        ("RoughTex", "rough"), ("AOTex", "ao")):
        if kind in texs:
            MEL.set_material_instance_texture_parameter_value(mi, pname, texs[kind])
    MEL.set_material_instance_scalar_parameter_value(mi, "TileSize", 420.0)
    MEL.set_material_instance_vector_parameter_value(
        mi, "AlbedoTint", unreal.LinearColor(1.2, 1.2, 1.2, 1))
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi)

# water: kill the chrome mirror
mi = find_mic("MI_Water")
if mi:
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.15)
    MEL.set_material_instance_vector_parameter_value(
        mi, "BaseColor", unreal.LinearColor(0.012, 0.05, 0.065, 1))
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    unreal.log("[Fix3] water de-chromed")

# lamp bulbs slightly darker
mi = find_mic("MI_LampDay")
if mi:
    MEL.set_material_instance_vector_parameter_value(
        mi, "BaseColor", unreal.LinearColor(0.55, 0.55, 0.5, 1))
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.5)
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi)

# road markings: matte paint instead of glow-yellow
city_master = unreal.load_asset("/Game/CityArt/M_CityMaster")
line_day_path = MAT_ROOT + "/MI_RoadLineDay"
if not eal.does_asset_exist(line_day_path):
    line_day = atools.create_asset("MI_RoadLineDay", MAT_ROOT,
                                   unreal.MaterialInstanceConstant,
                                   unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(line_day, city_master)
    MEL.set_material_instance_vector_parameter_value(
        line_day, "BaseColor", unreal.LinearColor(0.42, 0.33, 0.05, 1))
    MEL.set_material_instance_scalar_parameter_value(line_day, "Roughness", 0.9)
    MEL.update_material_instance(line_day)
    eal.save_loaded_asset(line_day)
else:
    line_day = unreal.load_asset(line_day_path)

swapped = 0
for a in eas.get_all_level_actors():
    smc = getattr(a, "static_mesh_component", None)
    if not smc:
        continue
    for i in range(smc.get_num_materials()):
        m = smc.get_material(i)
        if m and m.get_name() == "M_RoadLine":
            smc.set_material(i, line_day)
            swapped += 1
unreal.log("[Fix3] road-line slots swapped: {}".format(swapped))

# ============================================================
# 3) Lighting: deterministic skylight + pinned exposure + less fog
# ============================================================
skylight_comp = None
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label == "FixSkyLight":
        c = a.light_component
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_editor_property("real_time_capture", False)
        c.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_CAPTURED_SCENE)
        c.set_editor_property("intensity", 3.0)
        c.set_editor_property("lower_hemisphere_is_black", False)
        c.set_editor_property("cast_shadows", False)
        skylight_comp = c
    elif label == "FixDirectionalLight":
        c = a.light_component
        c.set_editor_property("intensity", 10.0)
        c.set_editor_property("contact_shadow_length", 0.02)
        c.set_editor_property("contact_shadow_length_in_ws", False)
        c.set_editor_property("dynamic_shadow_distance_movable_light", 22000.0)
        c.set_editor_property("cascade_distribution_exponent", 2.5)
        c.set_editor_property("shadow_bias", 0.35)
        c.set_editor_property("shadow_slope_bias", 0.35)
    elif label == "FixHeightFog":
        c = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if c:
            c.set_editor_property("fog_density", 0.006)
    elif label == "FixPostProcess":
        s = a.settings
        s.set_editor_property("override_auto_exposure_min_brightness", True)
        s.set_editor_property("auto_exposure_min_brightness", 1.0)
        s.set_editor_property("override_auto_exposure_max_brightness", True)
        s.set_editor_property("auto_exposure_max_brightness", 1.0)
        s.set_editor_property("override_auto_exposure_bias", True)
        s.set_editor_property("auto_exposure_bias", 0.0)
        s.set_editor_property("override_ambient_occlusion_intensity", True)
        s.set_editor_property("ambient_occlusion_intensity", 0.40)
        a.settings = s
unreal.log("[Fix3] lighting: captured-scene skylight, pinned exposure, fog halved")

# ============================================================
# 4) Roof caps (roofs read as tarred asphalt from aerial)
# ============================================================
rooftar_path = MAT_ROOT + "/MI_RoofTar"
if not eal.does_asset_exist(rooftar_path):
    rooftar = atools.create_asset("MI_RoofTar", MAT_ROOT,
                                  unreal.MaterialInstanceConstant,
                                  unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(rooftar, unreal.load_asset(MAT_ROOT + "/M_RealGround"))
    texs = texture_set("Asphalt031")
    for pname, kind in (("AlbedoTex", "color"), ("NormalTex", "normal"),
                        ("RoughTex", "rough"), ("AOTex", "ao")):
        if kind in texs:
            MEL.set_material_instance_texture_parameter_value(rooftar, pname, texs[kind])
    MEL.set_material_instance_scalar_parameter_value(rooftar, "TileSize", 500.0)
    MEL.set_material_instance_vector_parameter_value(
        rooftar, "AlbedoTint", unreal.LinearColor(0.55, 0.54, 0.52, 1))
    MEL.update_material_instance(rooftar)
    eal.save_loaded_asset(rooftar)
else:
    rooftar = unreal.load_asset(rooftar_path)

CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")

# ============================================================
# 5) Re-place props with corrected offsets + roof caps
# ============================================================
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_RealProp_"):
        eas.destroy_actor(a)

PROP_ROOT = "/Game/RealKit/Props"


def prop_mesh(slug):
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
    return best


PROPS = {s: prop_mesh(s) for s in [
    "fire_hydrant", "metal_trash_can", "painted_wooden_bench",
    "water_manhole_cover", "utility_box_02", "concrete_road_barrier",
    "trashbag", "old_tyre"]}


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


N, BLOCK, ROAD = 7, 2000.0, 600.0
STEP = BLOCK + ROAD
START = -N * STEP / 2 + STEP / 2
LAKE_GX, LAKE_GY = {4, 5, 6}, {0, 1}
PARKS = {(1, 5), (2, 2), (5, 5)}
SIDEWALK_Z, ROAD_Z = 14.0, 1.5


def cx(i):
    return START + i * STEP


placed = 0
edge = BLOCK / 2 - 55  # curb strip: clears buildings (~870) and curb (1000)
for gx in range(N):
    for gy in range(N):
        if gx in LAKE_GX and gy in LAKE_GY:
            continue
        bx, by = cx(gx), cx(gy)
        if (gx, gy) in PARKS:
            for i in range(3):
                placed += bool(place(PROPS["painted_wooden_bench"],
                                     "bench_{}_{}_{}".format(gx, gy, i),
                                     bx + random.uniform(-600, 600),
                                     by + random.uniform(-600, 600), SIDEWALK_Z))
            continue
        side = random.choice([(edge, random.uniform(-700, 700)),
                              (-edge, random.uniform(-700, 700)),
                              (random.uniform(-700, 700), edge),
                              (random.uniform(-700, 700), -edge)])
        placed += bool(place(PROPS["fire_hydrant"], "hydrant_{}_{}".format(gx, gy),
                             bx + side[0], by + side[1], SIDEWALK_Z))
        csx, csy = random.choice([(1, 1), (1, -1), (-1, 1), (-1, -1)])
        placed += bool(place(PROPS["metal_trash_can"], "trash_{}_{}".format(gx, gy),
                             bx + csx * edge, by + csy * edge, SIDEWALK_Z))
        if random.random() < 0.35:
            placed += bool(place(PROPS["utility_box_02"], "utility_{}_{}".format(gx, gy),
                                 bx - side[0], by - side[1], SIDEWALK_Z,
                                 yaw=random.choice([0, 90, 180, 270])))
        if random.random() < 0.30:
            placed += bool(place(PROPS["painted_wooden_bench"], "swbench_{}_{}".format(gx, gy),
                                 bx + random.uniform(-600, 600), by - edge,
                                 SIDEWALK_Z, yaw=0))

for gx in range(N - 1):
    for gy in range(N - 1):
        if gx in LAKE_GX and gy in LAKE_GY:
            continue
        placed += bool(place(PROPS["water_manhole_cover"], "manhole_{}_{}".format(gx, gy),
                             cx(gx) + STEP / 2 + random.uniform(-120, 120),
                             cx(gy) + STEP / 2 + random.uniform(-120, 120), ROAD_Z))

lx = cx(4) - STEP / 2 - ROAD / 2
for i in range(8):
    placed += bool(place(PROPS["concrete_road_barrier"], "barrier_{}".format(i),
                         lx, cx(0) - STEP / 2 + i * 700, ROAD_Z, yaw=90))

for i in range(18):
    gx, gy = random.randint(0, N - 1), random.randint(0, N - 1)
    if gx in LAKE_GX and gy in LAKE_GY or (gx, gy) in PARKS:
        continue
    mesh = PROPS["trashbag"] if i % 2 else PROPS["old_tyre"]
    placed += bool(place(mesh, "litter_{}".format(i),
                         cx(gx) + random.uniform(-700, 700),
                         cx(gy) + random.uniform(-700, 700), SIDEWALK_Z))

# roof caps on every building
caps = 0
for a in list(eas.get_all_level_actors()):
    if not a.get_actor_label().startswith("City_Bldg"):
        continue
    o, e = a.get_actor_bounds(False)
    cap = eas.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(o.x, o.y, o.z + e.z + 4.0), unreal.Rotator(0, 0, 0))
    cap.set_actor_label("City_RealProp_roofcap_{}".format(caps))
    cap.set_actor_scale3d(unreal.Vector(
        e.x * 2 / 100.0 * 0.98, e.y * 2 / 100.0 * 0.98, 0.08))
    cap.static_mesh_component.set_static_mesh(CUBE)
    cap.static_mesh_component.set_material(0, rooftar)
    cap.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    caps += 1

unreal.log("[Fix3] props placed: {} roof caps: {}".format(placed, caps))

# final: recapture skylight after all scene changes, then save
if skylight_comp:
    skylight_comp.recapture_sky()
saved = les.save_current_level()
unreal.log("[Fix3] level saved: {} — DONE".format(saved))
