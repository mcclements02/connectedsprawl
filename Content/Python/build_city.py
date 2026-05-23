"""Procedural greybox city for The Connected Sprawl.

Builds into /Game/Maps/TestMap:
  - Asphalt ground plane (road network shows between blocks)
  - 7x7 grid of city blocks with raised sidewalks
  - Varied, denser lot-based buildings (downtown = tall, midtown = shorter)
  - A lake in one corner with waterfront blocks
  - Park blocks with simple trees
  - Colored materials via an M_CityMaster master + MaterialInstanceConstants

Run:
  UnrealEditor <project.uproject> -ExecutePythonScript="<abs path>" -unattended -nosplash
"""
import unreal
import random

random.seed(2026)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_object(None, "/Engine/BasicShapes/Cylinder.Cylinder")
CONE = unreal.load_object(None, "/Engine/BasicShapes/Cone.Cone")
SPHERE = unreal.load_object(None, "/Engine/BasicShapes/Sphere.Sphere")

ART_DIR = "/Game/CityArt"


# ============================================================
# 1) Materials: one master + colored instances
# ============================================================
if unreal.EditorAssetLibrary.does_directory_exist(ART_DIR):
    unreal.EditorAssetLibrary.delete_directory(ART_DIR)

master = atools.create_asset("M_CityMaster", ART_DIR, unreal.Material,
                             unreal.MaterialFactoryNew())

col = MEL.create_material_expression(master, unreal.MaterialExpressionVectorParameter, -500, -100)
col.set_editor_property("parameter_name", "BaseColor")
col.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))

rough = MEL.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -500, 120)
rough.set_editor_property("parameter_name", "Roughness")
rough.set_editor_property("default_value", 0.85)

metal = MEL.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -500, 240)
metal.set_editor_property("parameter_name", "Metallic")
metal.set_editor_property("default_value", 0.0)

MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
MEL.recompile_material(master)
unreal.EditorAssetLibrary.save_loaded_asset(master)
unreal.log("[City] master material built + saved")


def make_mi(name, rgb, roughness=0.85, metallic=0.0):
    mi = atools.create_asset(name, ART_DIR, unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", master)
    MEL.set_material_instance_vector_parameter_value(
        mi, "BaseColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", roughness)
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", metallic)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi


MI = {
    "asphalt":  make_mi("MI_Asphalt",  (0.035, 0.035, 0.040), 0.92),
    "sidewalk": make_mi("MI_Sidewalk", (0.34, 0.34, 0.36), 0.80),
    "grass":    make_mi("MI_Grass",    (0.10, 0.26, 0.09), 0.85),
    "water":    make_mi("MI_Water",    (0.02, 0.10, 0.22), 0.06, 0.30),
    "trunk":    make_mi("MI_Trunk",    (0.16, 0.10, 0.05), 0.90),
    "leaves":   make_mi("MI_Leaves",   (0.07, 0.24, 0.07), 0.85),
}
BLDG_MATS = [
    make_mi("MI_Bldg_Tan",   (0.40, 0.34, 0.27), 0.80),
    make_mi("MI_Bldg_Slate", (0.22, 0.26, 0.32), 0.70),
    make_mi("MI_Bldg_Brick", (0.34, 0.16, 0.13), 0.85),
    make_mi("MI_Bldg_Conc",  (0.52, 0.52, 0.50), 0.78),
    make_mi("MI_Bldg_Glass", (0.16, 0.30, 0.42), 0.18, 0.85),
]
unreal.log("[City] {} material instances built".format(len(MI) + len(BLDG_MATS)))


# ============================================================
# 2) Helpers
# ============================================================
def spawn(mesh, label, loc, scale, mat, rot=None):
    if rot is None:
        rot = unreal.Rotator(0, 0, 0)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    smc = a.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_material(0, mat)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    return a


# ============================================================
# 3) Load level, clear old city
# ============================================================
les.load_level("/Game/Maps/TestMap")

killed = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_"):
        eas.destroy_actor(a)
        killed += 1
unreal.log("[City] cleared {} old City_* actors".format(killed))


# ============================================================
# 4) Grid layout
# ============================================================
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD          # 2600
SPAN = N * STEP              # 18200
START = -SPAN / 2 + STEP / 2  # block center for index 0

def cx(i):
    return START + i * STEP

# Asphalt ground covering whole city
spawn(CUBE, "City_Ground", unreal.Vector(0, 0, -50),
      unreal.Vector(SPAN / 100.0 + 4, SPAN / 100.0 + 4, 1.0), MI["asphalt"])

# Lake region: gx in {4,5,6}, gy in {0,1}
LAKE_GX = {4, 5, 6}
LAKE_GY = {0, 1}
PARKS = {(1, 5), (2, 2), (5, 5)}


def is_lake(gx, gy):
    return gx in LAKE_GX and gy in LAKE_GY


# One big water slab over the lake bounding box
lx0 = cx(4) - STEP / 2
lx1 = cx(6) + STEP / 2
ly0 = cx(0) - STEP / 2
ly1 = cx(1) + STEP / 2
spawn(CUBE, "City_Lake",
      unreal.Vector((lx0 + lx1) / 2, (ly0 + ly1) / 2, 4),
      unreal.Vector((lx1 - lx0) / 100.0, (ly1 - ly0) / 100.0, 0.12),
      MI["water"])

bldg_count = 0
tree_count = 0
alley_count = 0

for gx in range(N):
    for gy in range(N):
        if is_lake(gx, gy):
            continue
        bx, by = cx(gx), cx(gy)
        is_park = (gx, gy) in PARKS
        # downtown core = central 3x3
        downtown = (2 <= gx <= 4) and (2 <= gy <= 4)

        # Block surface: sidewalk (grey) or grass (park)
        surf_mat = MI["grass"] if is_park else MI["sidewalk"]
        spawn(CUBE, "City_Surf_{}_{}".format(gx, gy),
              unreal.Vector(bx, by, 7),
              unreal.Vector(BLOCK / 100.0, BLOCK / 100.0, 0.14), surf_mat)

        if is_park:
            # scatter simple trees: cylinder trunk + sphere canopy
            for t in range(random.randint(6, 11)):
                tx = bx + random.uniform(-BLOCK / 2 + 220, BLOCK / 2 - 220)
                ty = by + random.uniform(-BLOCK / 2 + 220, BLOCK / 2 - 220)
                th = random.uniform(280, 440)            # trunk height
                spawn(CYL, "City_TreeTrunk_{}_{}_{}".format(gx, gy, t),
                      unreal.Vector(tx, ty, 14 + th / 2),
                      unreal.Vector(0.45, 0.45, th / 100.0), MI["trunk"])
                fr = random.uniform(170, 250)            # canopy radius
                spawn(SPHERE, "City_TreeTop_{}_{}_{}".format(gx, gy, t),
                      unreal.Vector(tx, ty, 14 + th + fr * 0.30),
                      unreal.Vector(fr / 50.0, fr / 50.0, fr / 58.0),
                      MI["leaves"])
                tree_count += 1
            continue

        # Buildings on the block. Four loose lots prevent overlaps and create
        # denser street walls without needing bespoke meshes.
        lots = [
            (-BLOCK * 0.26, -BLOCK * 0.26),
            ( BLOCK * 0.26, -BLOCK * 0.26),
            (-BLOCK * 0.26,  BLOCK * 0.26),
            ( BLOCK * 0.26,  BLOCK * 0.26),
        ]
        random.shuffle(lots)
        n_bldgs = random.choice([2, 3, 3]) if downtown else random.choice([2, 3, 3, 4])

        if random.random() < (0.55 if downtown else 0.38):
            if random.random() < 0.5:
                spawn(CUBE, "City_Alley_NS_{}_{}".format(gx, gy),
                      unreal.Vector(bx + random.uniform(-160, 160), by, 10),
                      unreal.Vector(1.7, BLOCK / 100.0, 0.08), MI["asphalt"])
            else:
                spawn(CUBE, "City_Alley_EW_{}_{}".format(gx, gy),
                      unreal.Vector(bx, by + random.uniform(-160, 160), 10),
                      unreal.Vector(BLOCK / 100.0, 1.7, 0.08), MI["asphalt"])
            alley_count += 1

        for b in range(n_bldgs):
            lot_x, lot_y = lots[b]
            if downtown:
                bh = random.uniform(2600, 6200)
                bw = random.uniform(620, 940)
                bd = random.uniform(620, 940)
            else:
                bh = random.uniform(650, 2600)
                bw = random.uniform(430, 720)
                bd = random.uniform(430, 720)
            ox = lot_x + random.uniform(-120, 120)
            oy = lot_y + random.uniform(-120, 120)
            mat = random.choice(BLDG_MATS)
            spawn(CUBE, "City_Bldg_{}_{}_{}".format(gx, gy, b),
                  unreal.Vector(bx + ox, by + oy, 14 + bh / 2),
                  unreal.Vector(bw / 100.0, bd / 100.0, bh / 100.0), mat)
            bldg_count += 1

unreal.log("[City] placed {} buildings, {} trees, {} service alleys".format(
    bldg_count, tree_count, alley_count))


# ============================================================
# 5) PlayerStart on a downtown road
# ============================================================
road_x = (cx(2) + cx(3)) / 2.0   # vertical road
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "PlayerStart":
        a.set_actor_location(unreal.Vector(road_x, cx(3), 300), False, False)
        a.set_actor_rotation(unreal.Rotator(0, 0, 0), False)

# ============================================================
# 6) Save
# ============================================================
saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(ART_DIR, only_if_is_dirty=False)
unreal.log("[City] level saved: {}, CityArt saved".format(saved))
unreal.log("[City] DONE")
