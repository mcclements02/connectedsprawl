"""Real trees + real buildings.

Buildings: a procedural ribbon-window facade material (M_BuildingFacade) driven
by world-Z, so every building cube becomes a multi-floor curtain-wall tower.
Trees: restore the imported HillTree_02 (reassign its 4 materials) and replace
every primitive lollipop tree with it.
"""
import unreal
import random

random.seed(11)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ART = "/Game/CityArt"


# ============================================================
# 1) M_BuildingFacade — world-Z ribbon-window material
# ============================================================
fpath = ART + "/M_BuildingFacade"
if unreal.EditorAssetLibrary.does_asset_exist(fpath):
    unreal.EditorAssetLibrary.delete_asset(fpath)
mat = atools.create_asset("M_BuildingFacade", ART, unreal.Material, unreal.MaterialFactoryNew())

def expr(cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)

concrete = expr(unreal.MaterialExpressionVectorParameter, -1500, -300)
concrete.set_editor_property("parameter_name", "ConcreteColor")
concrete.set_editor_property("default_value", unreal.LinearColor(0.38, 0.35, 0.31, 1.0))

glass = expr(unreal.MaterialExpressionVectorParameter, -1500, -120)
glass.set_editor_property("parameter_name", "GlassColor")
glass.set_editor_property("default_value", unreal.LinearColor(0.09, 0.14, 0.20, 1.0))

# world Z -> per-floor 0..1
wp = expr(unreal.MaterialExpressionWorldPosition, -1500, 250)
maskz = expr(unreal.MaterialExpressionComponentMask, -1300, 250)
maskz.set_editor_property("r", False)
maskz.set_editor_property("g", False)
maskz.set_editor_property("b", True)
maskz.set_editor_property("a", False)
MEL.connect_material_expressions(wp, "", maskz, "")

floorh = expr(unreal.MaterialExpressionConstant, -1300, 400)
floorh.set_editor_property("r", 320.0)
div = expr(unreal.MaterialExpressionDivide, -1120, 280)
MEL.connect_material_expressions(maskz, "", div, "A")
MEL.connect_material_expressions(floorh, "", div, "B")
frac = expr(unreal.MaterialExpressionFrac, -980, 280)
MEL.connect_material_expressions(div, "", frac, "")

# sharp window mask: saturate((frac - 0.32) * 14)
thresh = expr(unreal.MaterialExpressionConstant, -980, 430)
thresh.set_editor_property("r", 0.34)
sub = expr(unreal.MaterialExpressionSubtract, -840, 300)
MEL.connect_material_expressions(frac, "", sub, "A")
MEL.connect_material_expressions(thresh, "", sub, "B")
sharp = expr(unreal.MaterialExpressionConstant, -840, 450)
sharp.set_editor_property("r", 14.0)
mul = expr(unreal.MaterialExpressionMultiply, -700, 320)
MEL.connect_material_expressions(sub, "", mul, "A")
MEL.connect_material_expressions(sharp, "", mul, "B")
clamp = expr(unreal.MaterialExpressionClamp, -560, 320)
clamp.set_editor_property("min_default", 0.0)
clamp.set_editor_property("max_default", 1.0)
MEL.connect_material_expressions(mul, "", clamp, "")   # window mask 0/1

# BaseColor = lerp(concrete, glass, mask)
lerp_c = expr(unreal.MaterialExpressionLinearInterpolate, -340, -200)
MEL.connect_material_expressions(concrete, "", lerp_c, "A")
MEL.connect_material_expressions(glass, "", lerp_c, "B")
MEL.connect_material_expressions(clamp, "", lerp_c, "Alpha")
MEL.connect_material_property(lerp_c, "", unreal.MaterialProperty.MP_BASE_COLOR)

# Roughness = lerp(0.85 concrete, 0.18 glass, mask)
rc = expr(unreal.MaterialExpressionConstant, -560, 60)
rc.set_editor_property("r", 0.85)
rg = expr(unreal.MaterialExpressionConstant, -560, 140)
rg.set_editor_property("r", 0.18)
lerp_r = expr(unreal.MaterialExpressionLinearInterpolate, -340, 80)
MEL.connect_material_expressions(rc, "", lerp_r, "A")
MEL.connect_material_expressions(rg, "", lerp_r, "B")
MEL.connect_material_expressions(clamp, "", lerp_r, "Alpha")
MEL.connect_material_property(lerp_r, "", unreal.MaterialProperty.MP_ROUGHNESS)

# Metallic = mask * 0.55  (glass slightly metallic/reflective)
mm = expr(unreal.MaterialExpressionConstant, -560, 240)
mm.set_editor_property("r", 0.55)
met = expr(unreal.MaterialExpressionMultiply, -340, 240)
MEL.connect_material_expressions(clamp, "", met, "A")
MEL.connect_material_expressions(mm, "", met, "B")
MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)

# Emissive = glass * mask * 0.18  (faint window glow)
em1 = expr(unreal.MaterialExpressionMultiply, -340, 420)
MEL.connect_material_expressions(glass, "", em1, "A")
MEL.connect_material_expressions(clamp, "", em1, "B")
emk = expr(unreal.MaterialExpressionConstant, -340, 540)
emk.set_editor_property("r", 0.18)
em2 = expr(unreal.MaterialExpressionMultiply, -180, 440)
MEL.connect_material_expressions(em1, "", em2, "A")
MEL.connect_material_expressions(emk, "", em2, "B")
MEL.connect_material_property(em2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log("[Real] M_BuildingFacade built")

# facade instances with colour variety
def facade_mi(name, concrete_rgb, glass_rgb):
    p = ART + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        unreal.EditorAssetLibrary.delete_asset(p)
    mi = atools.create_asset(name, ART, unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", mat)
    MEL.set_material_instance_vector_parameter_value(
        mi, "ConcreteColor", unreal.LinearColor(concrete_rgb[0], concrete_rgb[1], concrete_rgb[2], 1.0))
    MEL.set_material_instance_vector_parameter_value(
        mi, "GlassColor", unreal.LinearColor(glass_rgb[0], glass_rgb[1], glass_rgb[2], 1.0))
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi

FACADES = [
    facade_mi("MI_Facade_Warm",  (0.42, 0.38, 0.32), (0.09, 0.14, 0.20)),
    facade_mi("MI_Facade_Pale",  (0.58, 0.57, 0.54), (0.12, 0.22, 0.26)),
    facade_mi("MI_Facade_Tan",   (0.48, 0.40, 0.30), (0.22, 0.15, 0.09)),
    facade_mi("MI_Facade_Slate", (0.28, 0.30, 0.35), (0.07, 0.11, 0.16)),
    facade_mi("MI_Facade_Teal",  (0.33, 0.37, 0.36), (0.10, 0.26, 0.28)),
]
unreal.log("[Real] {} facade instances built".format(len(FACADES)))


# ============================================================
# 2) Re-skin every building with a facade material
# ============================================================
les.load_level("/Game/Maps/TestMap")
nb = 0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_Bldg_"):
        a.static_mesh_component.set_material(0, random.choice(FACADES))
        nb += 1
unreal.log("[Real] reskinned {} buildings".format(nb))


# ============================================================
# 3) Real trees — HillTree_02 with materials reassigned
# ============================================================
tree_mesh = unreal.load_asset("/Game/SampleScene/Tree/HillTree_02")
tree_mats = [
    unreal.load_asset("/Game/SampleScene/Tree/Materials/M_HillTree_01_Branches"),
    unreal.load_asset("/Game/SampleScene/Tree/Materials/M_HillTree_01_Branches_2"),
    unreal.load_asset("/Game/SampleScene/Tree/Materials/M_HillTree_01_Fronds"),
    unreal.load_asset("/Game/SampleScene/Tree/Materials/M_HillTree_01_Leaves"),
]

# gather trunk positions from the old primitive trees, then delete them all
trunk_locs = []
for a in list(eas.get_all_level_actors()):
    lbl = a.get_actor_label()
    if "TreeTrunk" in lbl:
        loc = a.get_actor_location()
        trunk_locs.append((loc.x, loc.y))
        eas.destroy_actor(a)
    elif "TreeTop" in lbl:
        eas.destroy_actor(a)

made = 0
for i, (tx, ty) in enumerate(trunk_locs):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor,
                                   unreal.Vector(tx, ty, 14.0),
                                   unreal.Rotator(0, 0, random.uniform(0, 360)))
    a.set_actor_label("City_RealTree_{}".format(i))
    s = random.uniform(0.55, 0.85)
    a.set_actor_scale3d(unreal.Vector(s, s, s))
    smc = a.static_mesh_component
    smc.set_static_mesh(tree_mesh)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    for slot, m in enumerate(tree_mats):
        if m:
            smc.set_material(slot, m)
    made += 1
unreal.log("[Real] placed {} HillTree instances".format(made))

saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(ART, only_if_is_dirty=False)
unreal.log("[Real] level saved: {}. DONE".format(saved))
