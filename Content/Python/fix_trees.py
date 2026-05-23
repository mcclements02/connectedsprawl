"""Fix the HillTree: build clean leaf + bark materials that sample the real
imported textures directly (the original materials' texture refs broke on
import), and reassign them to every City_RealTree_ actor.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
ART = "/Game/CityArt"

TEX = "/Game/SampleScene/Tree/Textures/"
atlas = unreal.load_asset(TEX + "T_HillTree_01_Atlas")
atlas_n = unreal.load_asset(TEX + "T_HillTree_01_Atlas_N")
bark = unreal.load_asset(TEX + "T_Craghead_Oak_Tile_01_D")
bark_n = unreal.load_asset(TEX + "T_Craghead_Oak_Tile_01_N")
unreal.log("[Trees] textures: atlas={} bark={}".format(bool(atlas), bool(bark)))


def fresh(name):
    p = ART + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        unreal.EditorAssetLibrary.delete_asset(p)
    return atools.create_asset(name, ART, unreal.Material, unreal.MaterialFactoryNew())


# --- leaf material: masked, two-sided, samples the foliage atlas ---
leaf = fresh("M_TreeLeaf")
leaf.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
leaf.set_editor_property("two_sided", True)

ts = MEL.create_material_expression(leaf, unreal.MaterialExpressionTextureSample, -700, -100)
ts.set_editor_property("texture", atlas)
# brighten / greenify the albedo a touch
tint = MEL.create_material_expression(leaf, unreal.MaterialExpressionConstant3Vector, -700, 150)
tint.set_editor_property("constant", unreal.LinearColor(1.25, 1.5, 0.9, 1.0))
mulc = MEL.create_material_expression(leaf, unreal.MaterialExpressionMultiply, -480, 0)
MEL.connect_material_expressions(ts, "RGB", mulc, "A")
MEL.connect_material_expressions(tint, "", mulc, "B")
MEL.connect_material_property(mulc, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(ts, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
if atlas_n:
    tsn = MEL.create_material_expression(leaf, unreal.MaterialExpressionTextureSample, -700, 320)
    tsn.set_editor_property("texture", atlas_n)
    tsn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_property(tsn, "RGB", unreal.MaterialProperty.MP_NORMAL)
rough = MEL.create_material_expression(leaf, unreal.MaterialExpressionConstant, -480, 200)
rough.set_editor_property("r", 0.7)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(leaf)
unreal.EditorAssetLibrary.save_loaded_asset(leaf)
unreal.log("[Trees] M_TreeLeaf built")

# --- bark material: opaque, samples the oak bark texture ---
barkmat = fresh("M_TreeBark")
tsb = MEL.create_material_expression(barkmat, unreal.MaterialExpressionTextureSample, -700, -50)
tsb.set_editor_property("texture", bark)
MEL.connect_material_property(tsb, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
if bark_n:
    tsbn = MEL.create_material_expression(barkmat, unreal.MaterialExpressionTextureSample, -700, 250)
    tsbn.set_editor_property("texture", bark_n)
    tsbn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_property(tsbn, "RGB", unreal.MaterialProperty.MP_NORMAL)
rb = MEL.create_material_expression(barkmat, unreal.MaterialExpressionConstant, -480, 150)
rb.set_editor_property("r", 0.9)
MEL.connect_material_property(rb, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(barkmat)
unreal.EditorAssetLibrary.save_loaded_asset(barkmat)
unreal.log("[Trees] M_TreeBark built")

# --- reassign on every real tree: slots 0,1 = bark; 2,3 = leaf ---
les.load_level("/Game/Maps/TestMap")
n = 0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_RealTree_"):
        smc = a.static_mesh_component
        smc.set_material(0, barkmat)
        smc.set_material(1, barkmat)
        smc.set_material(2, leaf)
        smc.set_material(3, leaf)
        n += 1
unreal.log("[Trees] reskinned {} trees".format(n))

saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(ART, only_if_is_dirty=False)
unreal.log("[Trees] level saved: {}. DONE".format(saved))
