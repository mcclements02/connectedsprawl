"""Author the lightweight lit material used by runtime wardrobe accessories."""

import unreal


ASSET_PATH = "/Game/Materials/M_WardrobeAccessory"
ASSET_DIR = "/Game/Materials"
ASSET_NAME = "M_WardrobeAccessory"
VERTEX_SOURCE = (
    "/Engine/EngineDebugMaterials/VertexColorViewMode_ColorOnly."
    "VertexColorViewMode_ColorOnly")
VERTEX_PATH = "/Game/Materials/M_WardrobeVertexColor"


def ensure_wardrobe_material():
    editor_assets = unreal.EditorAssetLibrary
    if editor_assets.does_asset_exist(ASSET_PATH):
        material = editor_assets.load_asset(ASSET_PATH)
        if isinstance(material, unreal.Material):
            unreal.log("[WardrobeMaterial] already exists: {}".format(ASSET_PATH))
            return material
        raise RuntimeError("{} exists but is not a Material".format(ASSET_PATH))

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(
        ASSET_NAME, ASSET_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create {}".format(ASSET_PATH))

    editing = unreal.MaterialEditingLibrary
    base_color = editing.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -420, -80)
    base_color.set_editor_property("parameter_name", "BaseColor")
    base_color.set_editor_property(
        "default_value", unreal.LinearColor(0.18, 0.20, 0.24, 1.0))

    roughness = editing.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 80)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.72)

    specular = editing.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 220)
    specular.set_editor_property("parameter_name", "Specular")
    specular.set_editor_property("default_value", 0.28)

    editing.connect_material_property(
        base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    editing.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    editing.connect_material_property(
        specular, "", unreal.MaterialProperty.MP_SPECULAR)
    editing.recompile_material(material)
    editor_assets.save_loaded_asset(material)
    unreal.log("[WardrobeMaterial] created: {}".format(ASSET_PATH))
    return material


def ensure_vertex_color_material():
    editor_assets = unreal.EditorAssetLibrary
    if editor_assets.does_asset_exist(VERTEX_PATH):
        material = editor_assets.load_asset(VERTEX_PATH)
        if isinstance(material, unreal.Material):
            unreal.log("[WardrobeMaterial] already exists: {}".format(VERTEX_PATH))
            return material
        raise RuntimeError("{} exists but is not a Material".format(VERTEX_PATH))
    source = unreal.load_object(None, VERTEX_SOURCE)
    if not isinstance(source, unreal.Material):
        raise RuntimeError("Could not load {}".format(VERTEX_SOURCE))
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.duplicate_asset(
        "M_WardrobeVertexColor", ASSET_DIR, source)
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not duplicate {}".format(VERTEX_SOURCE))
    editor_assets.save_loaded_asset(material)
    unreal.log("[WardrobeMaterial] created: {}".format(VERTEX_PATH))
    return material


ensure_wardrobe_material()
ensure_vertex_color_material()
