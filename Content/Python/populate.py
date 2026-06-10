"""Populate the city: fix streetlight colour, add parked cars, build a
waterfront (beach + dock + railing) at the lake, and place NPC pedestrians.
Idempotent: clears its own City_Car_/City_Water_/City_NPC_ actors.
"""
import unreal
import math
import random
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)

import vehicle_realism

random.seed(31)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ART = "/Game/CityArt"

CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_object(None, "/Engine/BasicShapes/Cylinder.Cylinder")

les.load_level("/Game/Maps/TestMap")

# ---- materials ----------------------------------------------------------
master = unreal.load_asset(ART + "/M_CityMaster")

def mi(name, rgb, rough=0.7, metal=0.0):
    p = ART + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        unreal.EditorAssetLibrary.delete_asset(p)
    m = atools.create_asset(name, ART, unreal.MaterialInstanceConstant,
                            unreal.MaterialInstanceConstantFactoryNew())
    m.set_editor_property("parent", master)
    MEL.set_material_instance_vector_parameter_value(
        m, "BaseColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    MEL.set_material_instance_scalar_parameter_value(m, "Roughness", rough)
    MEL.set_material_instance_scalar_parameter_value(m, "Metallic", metal)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

MI_METAL = mi("MI_Metal", (0.14, 0.14, 0.15), 0.35, 0.85)
MI_WOOD = mi("MI_Wood", (0.30, 0.19, 0.10), 0.85)
MI_SAND = mi("MI_Sand", (0.78, 0.70, 0.50), 0.92)
MI_TIRE = mi("MI_Tire", (0.045, 0.045, 0.05), 0.9)
MI_GLASSDK = mi("MI_CarGlass", (0.06, 0.08, 0.10), 0.15, 0.4)
MI_HEADLIGHT = mi("MI_CarHeadlight", (0.92, 0.86, 0.62), 0.10, 0.1)
MI_TAILLIGHT = mi("MI_CarTailLight", (0.85, 0.04, 0.03), 0.20, 0.2)
MI_PLATE = mi("MI_CarPlate", (0.82, 0.82, 0.76), 0.35, 0.0)
CAR_COLORS = [
    mi("MI_Car_Red", (0.42, 0.04, 0.04), 0.28, 0.5),
    mi("MI_Car_White", (0.72, 0.72, 0.72), 0.30, 0.4),
    mi("MI_Car_Blue", (0.05, 0.12, 0.34), 0.28, 0.5),
    mi("MI_Car_Black", (0.08, 0.085, 0.09), 0.25, 0.5),
    mi("MI_Car_Silver", (0.40, 0.42, 0.44), 0.25, 0.7),
    mi("MI_Car_Yellow", (0.85, 0.55, 0.05), 0.28, 0.5),
]
CAR_COLORS = vehicle_realism.load_vehicle_paint_materials(CAR_COLORS)
REAL_CAR_MESHES = vehicle_realism.load_vehicle_meshes()
unreal.log("[Populate] materials ready")
unreal.log("[Populate] real vehicle meshes available: {}".format(len(REAL_CAR_MESHES)))

# ---- helpers ------------------------------------------------------------
def mesh_actor(mesh, label, loc, scale, mat, yaw=0.0, roll=0.0):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc,
                                   unreal.Rotator(pitch=0.0, yaw=yaw, roll=roll))
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    smc = a.static_mesh_component
    smc.set_static_mesh(mesh)
    if mat:
        smc.set_material(0, mat)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    return a

def cube(label, loc, scale, mat, yaw=0.0):
    return mesh_actor(CUBE, label, loc, scale, mat, yaw)

def clear(prefix):
    n = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith(prefix):
            eas.destroy_actor(a)
            n += 1
    return n

# ---- grid params (match build_city.py) ----------------------------------
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2
def cx(i):
    return START + i * STEP
vroads = [(cx(i) + cx(i + 1)) / 2.0 for i in range(N - 1)]
hroads = [(cx(j) + cx(j + 1)) / 2.0 for j in range(N - 1)]
LX0, LX1 = cx(4) - STEP / 2, cx(6) + STEP / 2
LY0, LY1 = cx(0) - STEP / 2, cx(1) + STEP / 2
def over_lake(x, y):
    return LX0 - 200 < x < LX1 + 200 and LY0 - 200 < y < LY1 + 200

# ============================================================
# 1) Streetlight colour fix
# ============================================================
ns = 0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_Detail_StreetLight_"):
        a.static_mesh_component.set_material(0, MI_METAL)
        ns += 1
unreal.log("[Populate] recoloured {} streetlights".format(ns))

# ============================================================
# 2) Parked cars
# ============================================================
clear("City_Car_")

def spawn_car(idx, x, y, yaw_deg):
    if REAL_CAR_MESHES:
        actor = vehicle_realism.spawn_real_parked_car(
            eas, idx, unreal.Vector(x, y, 0.0), yaw_deg, REAL_CAR_MESHES, CAR_COLORS)
        if actor:
            return actor

    color = vehicle_realism.paint_for_index(CAR_COLORS, idx) or random.choice(CAR_COLORS)
    yr = math.radians(yaw_deg)
    cos, sin = math.cos(yr), math.sin(yr)
    def w(lx, ly, z):
        return unreal.Vector(x + cos * lx - sin * ly, y + sin * lx + cos * ly, z)

    cube("City_Car_{}_body".format(idx), w(0, 0, 82),
         unreal.Vector(4.8, 1.96, 0.70), color, yaw_deg)
    cube("City_Car_{}_hood".format(idx), w(132, 0, 121),
         unreal.Vector(1.74, 1.84, 0.36), color, yaw_deg)
    cube("City_Car_{}_deck".format(idx), w(-150, 0, 124),
         unreal.Vector(1.36, 1.82, 0.38), color, yaw_deg)
    cube("City_Car_{}_front_fender".format(idx), w(142, 0, 83),
         unreal.Vector(1.55, 2.08, 0.62), color, yaw_deg)
    cube("City_Car_{}_rear_fender".format(idx), w(-151, 0, 84),
         unreal.Vector(1.48, 2.08, 0.62), color, yaw_deg)
    cube("City_Car_{}_cabin".format(idx), w(-24, 0, 172),
         unreal.Vector(2.18, 1.62, 0.72), MI_GLASSDK, yaw_deg)
    cube("City_Car_{}_roof".format(idx), w(-34, 0, 214),
         unreal.Vector(1.82, 1.48, 0.16), color, yaw_deg)

    cube("City_Car_{}_front_bumper".format(idx), w(254, 0, 84),
         unreal.Vector(0.22, 1.88, 0.30), MI_METAL, yaw_deg)
    cube("City_Car_{}_rear_bumper".format(idx), w(-254, 0, 84),
         unreal.Vector(0.22, 1.88, 0.30), MI_METAL, yaw_deg)
    for side in (-1, 1):
        cube("City_Car_{}_mirror_{}".format(idx, side), w(28, side * 117, 162),
             unreal.Vector(0.26, 0.10, 0.18), MI_METAL, yaw_deg)
        cube("City_Car_{}_headlight_{}".format(idx, side), w(259, side * 54, 109),
             unreal.Vector(0.08, 0.38, 0.18), MI_HEADLIGHT, yaw_deg)
        cube("City_Car_{}_taillight_{}".format(idx, side), w(-259, side * 55, 110),
             unreal.Vector(0.08, 0.32, 0.18), MI_TAILLIGHT, yaw_deg)
    cube("City_Car_{}_front_plate".format(idx), w(263, 0, 76),
         unreal.Vector(0.05, 0.56, 0.16), MI_PLATE, yaw_deg)
    cube("City_Car_{}_rear_plate".format(idx), w(-263, 0, 76),
         unreal.Vector(0.05, 0.56, 0.16), MI_PLATE, yaw_deg)

    for wi, (lx, ly) in enumerate([(150, 104), (150, -104), (-150, 104), (-150, -104)]):
        mesh_actor(CYL, "City_Car_{}_wheel_{}".format(idx, wi), w(lx, ly, 36),
                   unreal.Vector(0.82, 0.82, 0.34), MI_TIRE, yaw_deg, roll=90.0)

cars = 0
for x in vroads:
    for j in range(N):
        y = cx(j) + random.uniform(-400, 400)
        px = x + (ROAD / 2 - 110) * random.choice([-1, 1])
        if over_lake(px, y) or random.random() > 0.55:
            continue
        spawn_car(cars, px, y, random.choice([90, 270]))
        cars += 1
for y in hroads:
    for i in range(N):
        x = cx(i) + random.uniform(-400, 400)
        py = y + (ROAD / 2 - 110) * random.choice([-1, 1])
        if over_lake(x, py) or random.random() > 0.45:
            continue
        spawn_car(cars, x, py, random.choice([0, 180]))
        cars += 1
unreal.log("[Populate] placed {} parked cars".format(cars))

# ============================================================
# 3) Waterfront — beach, dock, railing
# ============================================================
clear("City_Water_")
# beach strip along the city-side edge of the lake (y = LY1)
beach_cx = (LX0 + LX1) / 2.0
beach_w = (LX1 - LX0)
cube("City_Water_Beach", unreal.Vector(beach_cx, LY1 + 220, 8),
     unreal.Vector(beach_w / 100.0, 5.2, 0.12), MI_SAND)
# wooden dock extending into the lake
dock_x = beach_cx
dock_y0 = LY1 + 100
for k in range(8):
    dy = dock_y0 - k * 320
    cube("City_Water_DockPlank_{}".format(k),
         unreal.Vector(dock_x, dy, 18),
         unreal.Vector(3.4, 3.2, 0.14), MI_WOOD)
    for sx in (-150, 150):
        cube("City_Water_DockPost_{}_{}".format(k, sx),
             unreal.Vector(dock_x + sx, dy, -20),
             unreal.Vector(0.28, 0.28, 0.9), MI_WOOD)
# railing posts along the beach edge
rail_n = 22
for k in range(rail_n):
    rx = LX0 + (k + 0.5) * (beach_w / rail_n)
    if abs(rx - dock_x) < 260:
        continue
    cube("City_Water_RailPost_{}".format(k),
         unreal.Vector(rx, LY1 + 60, 70), unreal.Vector(0.16, 0.16, 1.3), MI_METAL)
cube("City_Water_RailBar", unreal.Vector(beach_cx, LY1 + 60, 128),
     unreal.Vector(beach_w / 100.0, 0.14, 0.14), MI_METAL)
unreal.log("[Populate] waterfront built")

# ============================================================
# 4) NPC pedestrians (use live Sprawl pedestrians so imported avatars win)
# ============================================================
clear("City_NPC_")
npc = 0
for gx in range(N):
    for gy in range(N):
        bx, by = cx(gx), cx(gy)
        if over_lake(bx, by):
            continue
        if random.random() > 0.62:
            continue
        # place near a block edge facing a road
        edge = random.choice([(0, BLOCK / 2 - 120), (0, -(BLOCK / 2 - 120)),
                               (BLOCK / 2 - 120, 0), (-(BLOCK / 2 - 120), 0)])
        loc = unreal.Vector(bx + edge[0] + random.uniform(-200, 200),
                            by + edge[1] + random.uniform(-200, 200), 14)
        a = eas.spawn_actor_from_class(unreal.SprawlPedestrian, loc,
                                       unreal.Rotator(0, 0, random.uniform(0, 360)))
        a.set_actor_label("City_NPC_{}".format(npc))
        npc += 1
unreal.log("[Populate] placed {} live NPC pedestrians".format(npc))

saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(ART, only_if_is_dirty=False)
unreal.log("[Populate] level saved: {}. DONE".format(saved))
