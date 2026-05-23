"""Facade v2: a real window GRID (floors x columns), street-level storefronts,
rooftop caps, rooftop units, antennas, water tanks, and occasional billboards.
The grid uses the vertex normal to pick each face's horizontal axis, so windows
tile correctly on all four sides of any building.
"""
import unreal
import random

random.seed(23)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ART = "/Game/CityArt"
MP = unreal.MaterialProperty

# ============================================================
# 1) M_BuildingFacade v2 — window grid
# ============================================================
fp = ART + "/M_BuildingFacade"
if unreal.EditorAssetLibrary.does_asset_exist(fp):
    unreal.EditorAssetLibrary.delete_asset(fp)
mat = atools.create_asset("M_BuildingFacade", ART, unreal.Material, unreal.MaterialFactoryNew())

def E(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)
def K(val, x, y):
    c = E(unreal.MaterialExpressionConstant, x, y)
    c.set_editor_property("r", float(val))
    return c
def conn(a, ao, b, bi):
    MEL.connect_material_expressions(a, ao, b, bi)
def mask(src, x, y, r=False, g=False, b=False):
    m = E(unreal.MaterialExpressionComponentMask, x, y)
    m.set_editor_property("r", r); m.set_editor_property("g", g)
    m.set_editor_property("b", b); m.set_editor_property("a", False)
    conn(src, "", m, "")
    return m

wp = E(unreal.MaterialExpressionWorldPosition, -2100, 0)
nv = E(unreal.MaterialExpressionVertexNormalWS, -2100, 300)

wZ = mask(wp, -1900, -100, b=True)
wX = mask(wp, -1900, 40, r=True)
wY = mask(wp, -1900, 160, g=True)
nX = mask(nv, -1900, 320, r=True)

nXabs = E(unreal.MaterialExpressionAbs, -1750, 320)
conn(nX, "", nXabs, "")

# horizontal coord = lerp(worldX, worldY, abs(normal.x))
hcoord = E(unreal.MaterialExpressionLinearInterpolate, -1550, 120)
conn(wX, "", hcoord, "A")
conn(wY, "", hcoord, "B")
conn(nXabs, "", hcoord, "Alpha")

sharp = K(15.0, -1550, 520)
half = K(0.5, -1550, 600)

def band(coordExpr, spacing, halfwidth, x, y):
    """Return a 0/1 mask: 1 in the window middle of each repeating cell."""
    sp = K(spacing, x, y - 80)
    dv = E(unreal.MaterialExpressionDivide, x + 140, y)
    conn(coordExpr, "", dv, "A"); conn(sp, "", dv, "B")
    fr = E(unreal.MaterialExpressionFrac, x + 280, y)
    conn(dv, "", fr, "")
    # distance from cell centre
    sc = E(unreal.MaterialExpressionSubtract, x + 420, y)
    conn(fr, "", sc, "A"); conn(half, "", sc, "B")
    ab = E(unreal.MaterialExpressionAbs, x + 540, y)
    conn(sc, "", ab, "")
    hw = K(halfwidth, x + 420, y + 90)
    inv = E(unreal.MaterialExpressionSubtract, x + 660, y)
    conn(hw, "", inv, "A"); conn(ab, "", inv, "B")
    sh = E(unreal.MaterialExpressionMultiply, x + 800, y)
    conn(inv, "", sh, "A"); conn(sharp, "", sh, "B")
    cl = E(unreal.MaterialExpressionClamp, x + 940, y)
    cl.set_editor_property("min_default", 0.0)
    cl.set_editor_property("max_default", 1.0)
    conn(sh, "", cl, "")
    return cl

vBand = band(wZ, 265.0, 0.34, -1350, -100)      # floors
hBand = band(hcoord, 230.0, 0.36, -1350, 260)   # columns

winMask = E(unreal.MaterialExpressionMultiply, -250, 80)
conn(vBand, "", winMask, "A")
conn(hBand, "", winMask, "B")

concrete = E(unreal.MaterialExpressionVectorParameter, -250, -260)
concrete.set_editor_property("parameter_name", "ConcreteColor")
concrete.set_editor_property("default_value", unreal.LinearColor(0.40, 0.37, 0.32, 1.0))
glass = E(unreal.MaterialExpressionVectorParameter, -250, -120)
glass.set_editor_property("parameter_name", "GlassColor")
glass.set_editor_property("default_value", unreal.LinearColor(0.07, 0.13, 0.20, 1.0))

baseC = E(unreal.MaterialExpressionLinearInterpolate, 100, -200)
conn(concrete, "", baseC, "A"); conn(glass, "", baseC, "B"); conn(winMask, "", baseC, "Alpha")
MEL.connect_material_property(baseC, "", MP.MP_BASE_COLOR)

rC = K(0.86, 100, 40); rG = K(0.14, 100, 110)
rough = E(unreal.MaterialExpressionLinearInterpolate, 260, 70)
conn(rC, "", rough, "A"); conn(rG, "", rough, "B"); conn(winMask, "", rough, "Alpha")
MEL.connect_material_property(rough, "", MP.MP_ROUGHNESS)

mK = K(0.6, 100, 200)
metal = E(unreal.MaterialExpressionMultiply, 260, 200)
conn(winMask, "", metal, "A"); conn(mK, "", metal, "B")
MEL.connect_material_property(metal, "", MP.MP_METALLIC)

em1 = E(unreal.MaterialExpressionMultiply, 100, 340)
conn(glass, "", em1, "A"); conn(winMask, "", em1, "B")
emK = K(0.30, 100, 440)
em2 = E(unreal.MaterialExpressionMultiply, 260, 360)
conn(em1, "", em2, "A"); conn(emK, "", em2, "B")
MEL.connect_material_property(em2, "", MP.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log("[FacadeV2] material built")

# ============================================================
# 2) Facade instances
# ============================================================
def facade(name, c, g):
    p = ART + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        unreal.EditorAssetLibrary.delete_asset(p)
    mi = atools.create_asset(name, ART, unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", mat)
    MEL.set_material_instance_vector_parameter_value(mi, "ConcreteColor",
        unreal.LinearColor(c[0], c[1], c[2], 1.0))
    MEL.set_material_instance_vector_parameter_value(mi, "GlassColor",
        unreal.LinearColor(g[0], g[1], g[2], 1.0))
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi

FACADES = [
    facade("MI_Facade_Warm",  (0.44, 0.40, 0.33), (0.06, 0.12, 0.19)),
    facade("MI_Facade_Pale",  (0.60, 0.59, 0.56), (0.10, 0.20, 0.26)),
    facade("MI_Facade_Tan",   (0.50, 0.42, 0.31), (0.20, 0.13, 0.07)),
    facade("MI_Facade_Slate", (0.26, 0.29, 0.34), (0.05, 0.09, 0.15)),
    facade("MI_Facade_Teal",  (0.34, 0.39, 0.38), (0.07, 0.22, 0.25)),
]
unreal.log("[FacadeV2] {} instances".format(len(FACADES)))

# ============================================================
# 3) Re-skin buildings + add rooftops
# ============================================================
les.load_level("/Game/Maps/TestMap")
CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")
CYL = unreal.load_object(None, "/Engine/BasicShapes/Cylinder.Cylinder")
ROOF_MAT = unreal.load_asset("/Game/CityArt/MI_Asphalt")
UNIT_MAT = unreal.load_asset("/Game/Materials/MI_StdOpaque_Concrete")
GLASS_MAT = unreal.load_asset("/Game/CityArt/MI_CarGlass")
METAL_MAT = unreal.load_asset("/Game/CityArt/MI_Metal")
LAMP_MAT = unreal.load_asset("/Game/CityArt/M_LampGlow")
SIGN_MATS = [
    unreal.load_asset("/Game/CityArt/MI_Car_Red"),
    unreal.load_asset("/Game/CityArt/MI_Car_Yellow"),
    unreal.load_asset("/Game/CityArt/MI_Car_Blue"),
    LAMP_MAT,
]
AWNING_MATS = [
    unreal.load_asset("/Game/CityArt/MI_Car_Red"),
    unreal.load_asset("/Game/CityArt/MI_Car_Blue"),
    unreal.load_asset("/Game/CityArt/MI_Facade_Teal"),
    unreal.load_asset("/Game/CityArt/MI_Facade_Warm"),
]

# clear old building detail
for a in list(eas.get_all_level_actors()):
    lbl = a.get_actor_label()
    if lbl.startswith("City_Roof_") or lbl.startswith("City_BuildingDetail_"):
        eas.destroy_actor(a)

def spawn_mesh(mesh, label, loc, scale, mat, yaw=0.0, roll=0.0):
    a = eas.spawn_actor_from_class(
        unreal.StaticMeshActor,
        loc,
        unreal.Rotator(pitch=0.0, yaw=yaw, roll=roll))
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    a.static_mesh_component.set_static_mesh(mesh)
    if mat:
        a.static_mesh_component.set_material(0, mat)
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return a

def spawn_cube(label, loc, scale, mat, yaw=0.0):
    return spawn_mesh(CUBE, label, loc, scale, mat, yaw)

def real_mat(options, fallback):
    live = [m for m in options if m]
    return random.choice(live) if live else fallback

def add_storefront(detail_id, loc, w, d):
    side = random.choice(["N", "S", "E", "W"])
    sign_mat = real_mat(SIGN_MATS, LAMP_MAT or UNIT_MAT)
    awning_mat = real_mat(AWNING_MATS, UNIT_MAT)
    glass_mat = GLASS_MAT or FACADES[0]
    metal_mat = METAL_MAT or ROOF_MAT or UNIT_MAT

    if side in ("N", "S"):
        sy = 1 if side == "N" else -1
        edge_y = loc.y + sy * (d / 2 + 12)
        face_w = min(w * 0.72, 760)
        spawn_cube("City_BuildingDetail_StorefrontGlass_{}".format(detail_id),
                   unreal.Vector(loc.x, edge_y, 105),
                   unreal.Vector(face_w / 100.0, 0.08, 0.76), glass_mat)
        spawn_cube("City_BuildingDetail_StorefrontSign_{}".format(detail_id),
                   unreal.Vector(loc.x, edge_y + sy * 6, 205),
                   unreal.Vector(face_w / 100.0, 0.08, 0.34), sign_mat)
        spawn_cube("City_BuildingDetail_Awning_{}".format(detail_id),
                   unreal.Vector(loc.x, edge_y + sy * 26, 160),
                   unreal.Vector(face_w / 100.0, 0.34, 0.10), awning_mat)
        spawn_cube("City_BuildingDetail_Door_{}".format(detail_id),
                   unreal.Vector(loc.x - face_w * 0.28, edge_y + sy * 9, 82),
                   unreal.Vector(0.44, 0.07, 0.88), metal_mat)
    else:
        sx = 1 if side == "E" else -1
        edge_x = loc.x + sx * (w / 2 + 12)
        face_d = min(d * 0.72, 760)
        spawn_cube("City_BuildingDetail_StorefrontGlass_{}".format(detail_id),
                   unreal.Vector(edge_x, loc.y, 105),
                   unreal.Vector(0.08, face_d / 100.0, 0.76), glass_mat)
        spawn_cube("City_BuildingDetail_StorefrontSign_{}".format(detail_id),
                   unreal.Vector(edge_x + sx * 6, loc.y, 205),
                   unreal.Vector(0.08, face_d / 100.0, 0.34), sign_mat)
        spawn_cube("City_BuildingDetail_Awning_{}".format(detail_id),
                   unreal.Vector(edge_x + sx * 26, loc.y, 160),
                   unreal.Vector(0.34, face_d / 100.0, 0.10), awning_mat)
        spawn_cube("City_BuildingDetail_Door_{}".format(detail_id),
                   unreal.Vector(edge_x + sx * 9, loc.y - face_d * 0.28, 82),
                   unreal.Vector(0.07, 0.44, 0.88), metal_mat)

def add_roof_detail(detail_id, loc, w, d, top):
    metal_mat = METAL_MAT or UNIT_MAT
    if CYL and random.random() < 0.32 and w > 520 and d > 520:
        tank_x = loc.x + random.uniform(-w * 0.22, w * 0.22)
        tank_y = loc.y + random.uniform(-d * 0.22, d * 0.22)
        spawn_mesh(CYL, "City_Roof_WaterTank_{}".format(detail_id),
                   unreal.Vector(tank_x, tank_y, top + 130),
                   unreal.Vector(0.92, 0.92, 1.28), metal_mat)
        for n, (ox, oy) in enumerate([(-52, -52), (-52, 52), (52, -52), (52, 52)]):
            spawn_cube("City_Roof_TankLeg_{}_{}".format(detail_id, n),
                       unreal.Vector(tank_x + ox, tank_y + oy, top + 48),
                       unreal.Vector(0.10, 0.10, 0.96), metal_mat)

    if random.random() < 0.42 and top > 1500:
        spawn_mesh(CYL, "City_Roof_Antenna_{}".format(detail_id),
                   unreal.Vector(loc.x + random.uniform(-w * 0.24, w * 0.24),
                                 loc.y + random.uniform(-d * 0.24, d * 0.24),
                                 top + 215),
                   unreal.Vector(0.06, 0.06, 3.5), metal_mat)

    if random.random() < 0.22 and w > 560:
        sign_mat = real_mat(SIGN_MATS, LAMP_MAT or UNIT_MAT)
        edge_y = loc.y + d / 2 - 44
        spawn_cube("City_Roof_BillboardFace_{}".format(detail_id),
                   unreal.Vector(loc.x, edge_y, top + 250),
                   unreal.Vector(min(w * 0.52, 520) / 100.0, 0.10, 1.34), sign_mat)
        spawn_cube("City_Roof_BillboardFrame_{}".format(detail_id),
                   unreal.Vector(loc.x, edge_y - 10, top + 165),
                   unreal.Vector(min(w * 0.58, 580) / 100.0, 0.08, 0.10), metal_mat)

nb = nr = nd = 0
for a in list(eas.get_all_level_actors()):
    if not a.get_actor_label().startswith("City_Bldg_"):
        continue
    a.static_mesh_component.set_material(0, random.choice(FACADES))
    nb += 1
    loc = a.get_actor_location()
    sc = a.get_actor_scale3d()
    w, d, h = sc.x * 100, sc.y * 100, sc.z * 100
    top = loc.z + h / 2.0
    # parapet cap — slightly overhanging dark slab
    spawn_cube("City_Roof_Cap_{}".format(nr),
               unreal.Vector(loc.x, loc.y, top + 18),
               unreal.Vector(sc.x * 1.05, sc.y * 1.05, 0.36), ROOF_MAT)
    # 1-3 rooftop HVAC units
    for u in range(random.randint(1, 3)):
        uw = random.uniform(120, min(w, d) * 0.4 + 120)
        uh = random.uniform(90, 240)
        ox = random.uniform(-w * 0.25, w * 0.25)
        oy = random.uniform(-d * 0.25, d * 0.25)
        spawn_cube("City_Roof_Unit_{}_{}".format(nr, u),
                   unreal.Vector(loc.x + ox, loc.y + oy, top + 36 + uh / 2),
                   unreal.Vector(uw / 100.0, uw / 100.0, uh / 100.0), UNIT_MAT)
    add_storefront(nr, loc, w, d)
    add_roof_detail(nr, loc, w, d, top)
    nd += 1
    nr += 1

unreal.log("[FacadeV2] reskinned {} buildings, {} rooftops, {} street-level detail sets".format(nb, nr, nd))

saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(ART, only_if_is_dirty=False)
unreal.log("[FacadeV2] saved: {}. DONE".format(saved))
