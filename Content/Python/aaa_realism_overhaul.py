"""AAA realism overhaul pass for The Connected Sprawl.

Wires the living-city C++ systems into TestMap and upgrades the look:
  1. Animated translucent water material on the lake
  2. M_SignalGlow (traffic signals) + MI_RoadPaint (lane markings) materials
  3. Tags streetlight bulbs so the environment controller can light them at dusk
  4. Ensures sun / sky light / sky atmosphere / clouds / height fog exist
  5. Spawns SprawlTrafficLight poles at every dry intersection
  6. Spawns the singleton world actors: environment controller (day/night),
     pedestrian crowd manager, procedural traffic manager, ground cover
     (instanced park grass), and road markings

Requires the ConnectedSprawl C++ module to be compiled first.

Run:
  UnrealEditor <project.uproject> -ExecutePythonScript="<abs path>" -unattended -nosplash
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

les.load_level("/Game/Maps/TestMap")

# Grid params (must match build_city.py and SprawlCityGridSubsystem)
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2


def cx(i):
    return START + i * STEP


def road(r):
    return (cx(r) + cx(r + 1)) / 2.0


LX0, LX1 = cx(4) - STEP / 2, cx(6) + STEP / 2
LY0, LY1 = cx(0) - STEP / 2, cx(1) + STEP / 2


def over_lake(x, y, margin=0.0):
    return (LX0 - margin < x < LX1 + margin and
            LY0 - margin < y < LY1 + margin)


MAT_DIR = "/Game/Materials"


def fresh_material(name):
    path = "{}/{}".format(MAT_DIR, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return atools.create_asset(name, MAT_DIR, unreal.Material,
                               unreal.MaterialFactoryNew())


# ============================================================
# 1) Animated lake water
# ============================================================
water = fresh_material("M_WaterSurface")
water.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
water.set_editor_property("translucency_lighting_mode",
                          unreal.TranslucencyLightingMode.TLM_SURFACE)

base = MEL.create_material_expression(
    water, unreal.MaterialExpressionConstant3Vector, -700, -250)
base.set_editor_property("constant", unreal.LinearColor(0.015, 0.07, 0.14, 1.0))
MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

rough = MEL.create_material_expression(
    water, unreal.MaterialExpressionConstant, -700, -100)
rough.set_editor_property("r", 0.04)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

spec = MEL.create_material_expression(
    water, unreal.MaterialExpressionConstant, -700, 0)
spec.set_editor_property("r", 1.0)
MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

# Two scrolling normal layers for moving ripples
noise_tex = unreal.load_asset("/Game/Textures/T_Noise01_N")
if noise_tex:
    add = MEL.create_material_expression(
        water, unreal.MaterialExpressionAdd, -250, 250)
    for idx, (tiling, sx, sy) in enumerate([(28.0, 0.016, 0.009),
                                            (17.0, -0.011, 0.014)]):
        tc = MEL.create_material_expression(
            water, unreal.MaterialExpressionTextureCoordinate, -950, 200 + idx * 220)
        tc.set_editor_property("u_tiling", tiling)
        tc.set_editor_property("v_tiling", tiling)
        pan = MEL.create_material_expression(
            water, unreal.MaterialExpressionPanner, -760, 200 + idx * 220)
        pan.set_editor_property("speed_x", sx)
        pan.set_editor_property("speed_y", sy)
        samp = MEL.create_material_expression(
            water, unreal.MaterialExpressionTextureSample, -550, 200 + idx * 220)
        samp.set_editor_property("texture", noise_tex)
        samp.set_editor_property("sampler_type",
                                 unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        MEL.connect_material_expressions(tc, "", pan, "Coordinate")
        MEL.connect_material_expressions(pan, "", samp, "UVs")
        MEL.connect_material_expressions(samp, "", add, "A" if idx == 0 else "B")
    MEL.connect_material_property(add, "", unreal.MaterialProperty.MP_NORMAL)

fade = MEL.create_material_expression(
    water, unreal.MaterialExpressionDepthFade, -450, -180)
fade.set_editor_property("opacity_default", 0.86)
fade.set_editor_property("fade_distance_default", 220.0)
MEL.connect_material_property(fade, "", unreal.MaterialProperty.MP_OPACITY)

MEL.recompile_material(water)
EAL.save_loaded_asset(water)

for a in eas.get_all_level_actors():
    if a.get_actor_label() == "City_Lake":
        a.static_mesh_component.set_material(0, water)
        unreal.log("[AAA] lake water material applied")

# ============================================================
# 2) Signal glow + road paint materials (loaded by C++ actors)
# ============================================================
glow = fresh_material("M_SignalGlow")
gcol = MEL.create_material_expression(
    glow, unreal.MaterialExpressionVectorParameter, -500, -100)
gcol.set_editor_property("parameter_name", "GlowColor")
gcol.set_editor_property("default_value", unreal.LinearColor(2.0, 0.06, 0.04, 1.0))
MEL.connect_material_property(gcol, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(gcol, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.recompile_material(glow)
EAL.save_loaded_asset(glow)

paint_path = "{}/MI_RoadPaint".format(MAT_DIR)
if EAL.does_asset_exist(paint_path):
    EAL.delete_asset(paint_path)
paint = atools.create_asset("MI_RoadPaint", MAT_DIR, unreal.Material,
                            unreal.MaterialFactoryNew())
pcol = MEL.create_material_expression(
    paint, unreal.MaterialExpressionConstant3Vector, -500, -100)
pcol.set_editor_property("constant", unreal.LinearColor(0.85, 0.85, 0.82, 1.0))
MEL.connect_material_property(pcol, "", unreal.MaterialProperty.MP_BASE_COLOR)
prough = MEL.create_material_expression(
    paint, unreal.MaterialExpressionConstant, -500, 80)
prough.set_editor_property("r", 0.55)
MEL.connect_material_property(prough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(paint)
EAL.save_loaded_asset(paint)
unreal.log("[AAA] signal + road paint materials built")

# ============================================================
# 3) Tag streetlight bulbs for the night cycle
# ============================================================
tagged = 0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_Detail_LampBulb_"):
        a.set_editor_property("tags", [unreal.Name("Streetlight")])
        tagged += 1
unreal.log("[AAA] tagged {} streetlight bulbs".format(tagged))

# ============================================================
# 4) Ensure the sky/lighting stack exists
# ============================================================
have = {a.get_class().get_name() for a in eas.get_all_level_actors()}
def ensure(cls_name, cls, loc):
    if cls_name not in have:
        actor = eas.spawn_actor_from_class(cls, loc)
        actor.set_actor_label("City_{}".format(cls_name))
        unreal.log("[AAA] spawned missing {}".format(cls_name))

ensure("DirectionalLight", unreal.DirectionalLight, unreal.Vector(0, 0, 5000))
ensure("SkyLight", unreal.SkyLight, unreal.Vector(0, 0, 4000))
ensure("SkyAtmosphere", unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
ensure("VolumetricCloud", unreal.VolumetricCloud, unreal.Vector(0, 0, 0))
ensure("ExponentialHeightFog", unreal.ExponentialHeightFog, unreal.Vector(0, 0, 200))

# The environment controller drives these every frame.
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    if cn in ("DirectionalLight", "SkyLight"):
        a.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    if cn == "SkyLight":
        a.light_component.set_editor_property("real_time_capture", True)

# ============================================================
# 5) Traffic lights at every dry intersection
# ============================================================
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_Signal_"):
        eas.destroy_actor(a)

signals = 0
CORNERS = [(1, 1), (-1, -1), (1, -1), (-1, 1)]
for ix in range(N - 1):
    for iy in range(N - 1):
        vx, hy = road(ix), road(iy)
        if over_lake(vx, hy, 100):
            continue
        # Put the pole on a sidewalk corner that isn't in the water.
        spot = None
        for sx, sy in CORNERS:
            px, py = vx + sx * 335, hy + sy * 335
            if not over_lake(px, py):
                spot = (px, py)
                break
        if spot is None:
            continue
        sig = eas.spawn_actor_from_class(
            unreal.SprawlTrafficLight,
            unreal.Vector(spot[0], spot[1], 14.0))
        sig.set_actor_label("City_Signal_{}_{}".format(ix, iy))
        sig.set_editor_property("intersection_x", ix)
        sig.set_editor_property("intersection_y", iy)
        signals += 1
unreal.log("[AAA] placed {} traffic lights".format(signals))

# ============================================================
# 6) Singleton world actors
# ============================================================
SINGLETONS = [
    ("Sprawl_Environment",  unreal.SprawlEnvironmentController),
    ("Sprawl_Crowd",        unreal.PedestrianCrowdManager),
    ("Sprawl_Traffic",      unreal.ProceduralTrafficManager),
    ("Sprawl_GroundCover",  unreal.SprawlGroundCover),
    ("Sprawl_RoadMarkings", unreal.SprawlRoadMarkings),
]
for label, _ in SINGLETONS:
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label() == label:
            eas.destroy_actor(a)
for label, cls in SINGLETONS:
    a = eas.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    a.set_actor_label(label)
unreal.log("[AAA] world actors spawned: {}".format(
    ", ".join(label for label, _ in SINGLETONS)))

saved = les.save_current_level()
unreal.log("[AAA] level saved: {}. DONE".format(saved))
