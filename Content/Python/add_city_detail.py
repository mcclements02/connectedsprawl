"""Add street-level city detail to TestMap: lane markings, crosswalks,
curbs, traffic lights, streetlights, and trees lining the roads.
Idempotent: clears City_Detail_*.

Run:
  UnrealEditor <project.uproject> -ExecutePythonScript="<abs path>" -unattended -nosplash
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ART = "/Game/CityArt"

CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_object(None, "/Engine/BasicShapes/Cylinder.Cylinder")
SPHERE = unreal.load_object(None, "/Engine/BasicShapes/Sphere.Sphere")

MI_TRUNK = unreal.load_asset(ART + "/MI_Trunk")
MI_LEAVES = unreal.load_asset(ART + "/MI_Leaves")
MI_ASPHALT = unreal.load_asset(ART + "/MI_Asphalt")


# ---- standalone emissive materials --------------------------------------
def make_mat(name, base, emissive):
    path = ART + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    m = atools.create_asset(name, ART, unreal.Material, unreal.MaterialFactoryNew())
    b = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, -50)
    b.set_editor_property("constant", unreal.LinearColor(base[0], base[1], base[2], 1.0))
    MEL.connect_material_property(b, "", unreal.MaterialProperty.MP_BASE_COLOR)
    e = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -400, 180)
    e.set_editor_property("constant", unreal.LinearColor(emissive[0], emissive[1], emissive[2], 1.0))
    MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m


M_ROADLINE = make_mat("M_RoadLine", (0.55, 0.45, 0.05), (0.25, 0.20, 0.02))
M_ROADWHITE = make_mat("M_RoadWhite", (0.78, 0.78, 0.72), (0.04, 0.04, 0.035))
M_CURB = make_mat("M_CurbConcrete", (0.47, 0.46, 0.43), (0.0, 0.0, 0.0))
M_LAMP = make_mat("M_LampGlow", (1.0, 0.92, 0.72), (7.0, 5.0, 2.4))
M_SIGNAL_RED = make_mat("M_SignalRed", (0.85, 0.05, 0.03), (4.0, 0.08, 0.04))
M_SIGNAL_AMBER = make_mat("M_SignalAmber", (0.95, 0.55, 0.06), (3.2, 1.0, 0.05))
M_SIGNAL_GREEN = make_mat("M_SignalGreen", (0.05, 0.82, 0.20), (0.05, 3.2, 0.20))
unreal.log("[Detail] materials ready")


# ---- helpers ------------------------------------------------------------
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


# ---- grid params (must match build_city.py) -----------------------------
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2

def cx(i):
    return START + i * STEP

vroads = [(cx(i) + cx(i + 1)) / 2.0 for i in range(N - 1)]   # x of N-1 roads
hroads = [(cx(j) + cx(j + 1)) / 2.0 for j in range(N - 1)]   # y of N-1 roads

# lake bounding box (matches build_city.py)
LX0, LX1 = cx(4) - STEP / 2, cx(6) + STEP / 2
LY0, LY1 = cx(0) - STEP / 2, cx(1) + STEP / 2

def over_lake(x, y):
    return LX0 < x < LX1 and LY0 < y < LY1


def road_points(start, stop, step):
    v = start
    while v <= stop:
        yield v
        v += step


# ---- clear previous detail ---------------------------------------------
n = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_Detail_"):
        eas.destroy_actor(a)
        n += 1
les.load_level("/Game/Maps/TestMap")
unreal.log("[Detail] cleared {} prior detail actors".format(n))

half = SPAN / 2.0
lines = lights = trees = curbs = crosswalks = signals = 0

# ---- curbs around every block ------------------------------------------
for gx in range(N):
    for gy in range(N):
        if over_lake(cx(gx), cx(gy)):
            continue
        bx, by = cx(gx), cx(gy)
        z = 18.0
        curbs += 1
        spawn(CUBE, "City_Detail_Curb_N_{}_{}".format(gx, gy),
              unreal.Vector(bx, by + BLOCK / 2 + 18, z),
              unreal.Vector(BLOCK / 100.0, 0.18, 0.18), M_CURB)
        curbs += 1
        spawn(CUBE, "City_Detail_Curb_S_{}_{}".format(gx, gy),
              unreal.Vector(bx, by - BLOCK / 2 - 18, z),
              unreal.Vector(BLOCK / 100.0, 0.18, 0.18), M_CURB)
        curbs += 1
        spawn(CUBE, "City_Detail_Curb_E_{}_{}".format(gx, gy),
              unreal.Vector(bx + BLOCK / 2 + 18, by, z),
              unreal.Vector(0.18, BLOCK / 100.0, 0.18), M_CURB)
        curbs += 1
        spawn(CUBE, "City_Detail_Curb_W_{}_{}".format(gx, gy),
              unreal.Vector(bx - BLOCK / 2 - 18, by, z),
              unreal.Vector(0.18, BLOCK / 100.0, 0.18), M_CURB)

# ---- lane markings ------------------------------------------------------
DASH_LEN = 360.0
DASH_STEP = 760.0
LANE_OFFSET = 150.0
EDGE_OFFSET = ROAD / 2 - 70.0

for x in vroads:
    for y in road_points(-half + 360, half - 360, DASH_STEP):
        if over_lake(x, y):
            continue
        spawn(CUBE, "City_Detail_VCenter_{:.0f}_{:.0f}".format(x, y),
              unreal.Vector(x, y, 4), unreal.Vector(0.20, DASH_LEN / 100.0, 0.035), M_ROADLINE)
        lines += 1
        for ox in (-LANE_OFFSET, LANE_OFFSET):
            spawn(CUBE, "City_Detail_VLane_{:.0f}_{:.0f}_{:.0f}".format(x, y, ox),
                  unreal.Vector(x + ox, y, 4), unreal.Vector(0.13, 260 / 100.0, 0.035), M_ROADWHITE)
            lines += 1
        for ox in (-EDGE_OFFSET, EDGE_OFFSET):
            spawn(CUBE, "City_Detail_VEdge_{:.0f}_{:.0f}_{:.0f}".format(x, y, ox),
                  unreal.Vector(x + ox, y, 4), unreal.Vector(0.12, 560 / 100.0, 0.035), M_ROADWHITE)
            lines += 1

for y in hroads:
    for x in road_points(-half + 360, half - 360, DASH_STEP):
        if over_lake(x, y):
            continue
        spawn(CUBE, "City_Detail_HCenter_{:.0f}_{:.0f}".format(x, y),
              unreal.Vector(x, y, 4), unreal.Vector(DASH_LEN / 100.0, 0.20, 0.035), M_ROADLINE)
        lines += 1
        for oy in (-LANE_OFFSET, LANE_OFFSET):
            spawn(CUBE, "City_Detail_HLane_{:.0f}_{:.0f}_{:.0f}".format(x, y, oy),
                  unreal.Vector(x, y + oy, 4), unreal.Vector(260 / 100.0, 0.13, 0.035), M_ROADWHITE)
            lines += 1
        for oy in (-EDGE_OFFSET, EDGE_OFFSET):
            spawn(CUBE, "City_Detail_HEdge_{:.0f}_{:.0f}_{:.0f}".format(x, y, oy),
                  unreal.Vector(x, y + oy, 4), unreal.Vector(560 / 100.0, 0.12, 0.035), M_ROADWHITE)
            lines += 1

# ---- zebra crosswalks + traffic lights ---------------------------------
for ix, x in enumerate(vroads):
    for iy, y in enumerate(hroads):
        if over_lake(x, y):
            continue
        for side in (-1, 1):
            yy = y + side * (ROAD / 2 - 105)
            for k in range(-2, 3):
                spawn(CUBE, "City_Detail_Crosswalk_EW_{}_{}_{}_{}".format(ix, iy, side, k),
                      unreal.Vector(x, yy + k * 46, 5),
                      unreal.Vector((ROAD - 120) / 100.0, 0.16, 0.035), M_ROADWHITE)
                crosswalks += 1
            xx = x + side * (ROAD / 2 - 105)
            for k in range(-2, 3):
                spawn(CUBE, "City_Detail_Crosswalk_NS_{}_{}_{}_{}".format(ix, iy, side, k),
                      unreal.Vector(xx + k * 46, y, 5),
                      unreal.Vector(0.16, (ROAD - 120) / 100.0, 0.035), M_ROADWHITE)
                crosswalks += 1

        if (ix + iy) % 2 == 1:
            continue
        px = x - ROAD / 2 + 62
        py = y - ROAD / 2 + 62
        spawn(CYL, "City_Detail_TrafficPole_{}_{}".format(ix, iy),
              unreal.Vector(px, py, 14 + 265),
              unreal.Vector(0.12, 0.12, 5.3), MI_ASPHALT)
        spawn(CUBE, "City_Detail_TrafficArm_{}_{}".format(ix, iy),
              unreal.Vector(px + 195, py, 14 + 510),
              unreal.Vector(3.9, 0.10, 0.10), MI_ASPHALT)
        spawn(CUBE, "City_Detail_TrafficBox_{}_{}".format(ix, iy),
              unreal.Vector(px + 380, py, 14 + 435),
              unreal.Vector(0.24, 0.16, 0.82), MI_ASPHALT)
        for li, (mat, zoff) in enumerate([(M_SIGNAL_RED, 70), (M_SIGNAL_AMBER, 0), (M_SIGNAL_GREEN, -70)]):
            spawn(CUBE, "City_Detail_TrafficLamp_{}_{}_{}".format(ix, iy, li),
                  unreal.Vector(px + 392, py - 9, 14 + 435 + zoff),
                  unreal.Vector(0.08, 0.08, 0.12), mat)
        signals += 1

# ---- streetlights along roads ------------------------------------------
POLE_H = 720.0
for x in vroads:
    for j in range(N):
        y = cx(j)
        for side in (-1, 1):
            px = x + side * (ROAD / 2 + 50)
            if over_lake(px, y):
                continue
            spawn(CYL, "City_Detail_Pole_{:.0f}_{:.0f}".format(px, y),
                  unreal.Vector(px, y, 14 + POLE_H / 2),
                  unreal.Vector(0.14, 0.14, POLE_H / 100.0), MI_ASPHALT)
            spawn(CUBE, "City_Detail_Lamp_{:.0f}_{:.0f}".format(px, y),
                  unreal.Vector(px, y, 14 + POLE_H + 20),
                  unreal.Vector(0.7, 0.7, 0.35), M_LAMP)
            lights += 1

# ---- street trees along roads ------------------------------------------
for y in hroads:
    for i in range(N):
        x = cx(i)
        for side in (-1, 1):
            ty = y + side * (ROAD / 2 + 60)
            if over_lake(x, ty):
                continue
            spawn(CYL, "City_Detail_TreeTrunk_{:.0f}_{:.0f}".format(x, ty),
                  unreal.Vector(x, ty, 14 + 170),
                  unreal.Vector(0.45, 0.45, 3.4), MI_TRUNK)
            spawn(SPHERE, "City_Detail_TreeTop_{:.0f}_{:.0f}".format(x, ty),
                  unreal.Vector(x, ty, 14 + 340 + 60),
                  unreal.Vector(4.2, 4.2, 3.8), MI_LEAVES)
            trees += 1

unreal.log("[Detail] {} curbs, {} road markings, {} crosswalk bars, {} traffic lights, {} streetlights, {} street trees".format(
    curbs, lines, crosswalks, signals, lights, trees))

saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(ART, only_if_is_dirty=False)
unreal.log("[Detail] level saved: {}. DONE".format(saved))
