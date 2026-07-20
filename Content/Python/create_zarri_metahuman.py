"""Create and assemble Zarri from Unreal Engine 5.8 MetaHuman Core Data.

This pass is intentionally idempotent: it preserves an existing source character,
reapplies the approved mobile-safe look, and rebuilds the runtime Blueprint at
``/Game/MetaHumans/Zarri/BP_Zarri``.

The installed Core Data only ships a T-shirt/shorts outfit.  Those pieces are
colored as a neutral temporary outfit; the denim jacket, hoodie, cargo pants,
headphones, and cross-body strap remain project-owned wardrobe/accessory work.
"""

from __future__ import annotations

import unreal


SOURCE_PRESET = "/MetaHumanCharacter/Optional/Presets/Trey.Trey"
SOURCE_PACKAGE = "/Game/MetaHumans/Source"
SOURCE_NAME = "MHC_Zarri"
SOURCE_ASSET = f"{SOURCE_PACKAGE}/{SOURCE_NAME}.{SOURCE_NAME}"
RUNTIME_BLUEPRINT = "/Game/MetaHumans/Zarri/BP_Zarri.BP_Zarri_C"

HAIR = (
    "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/"
    "WI_Hair_S_AfroFade.WI_Hair_S_AfroFade"
)
EYEBROWS = (
    "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyebrows/"
    "WI_Eyebrows_M_FlatThick.WI_Eyebrows_M_FlatThick"
)
EYELASHES = (
    "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyelashes/"
    "WI_Eyelashes_S_Fine.WI_Eyelashes_S_Fine"
)
OUTFIT = (
    "/MetaHumanCharacter/Optional/Clothing/"
    "WI_DefaultGarment.WI_DefaultGarment"
)


def _log(message: str) -> None:
    unreal.log(f"[ZarriMetaHuman] {message}")


def _load_required(path: str):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required MetaHuman asset is missing: {path}")
    return asset


def _get_or_create_character():
    character = unreal.load_asset(SOURCE_ASSET)
    if character is not None:
        _log(f"Reusing source character {SOURCE_ASSET}")
        return character

    preset = _load_required(SOURCE_PRESET)
    character = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        SOURCE_NAME,
        SOURCE_PACKAGE,
        preset,
    )
    if character is None:
        raise RuntimeError(f"Could not duplicate {SOURCE_PRESET} to {SOURCE_ASSET}")
    _log(f"Created source character from Trey preset: {SOURCE_ASSET}")
    return character


def _select_wardrobe_item(preview_collection, slot_name: str, asset_path: str):
    item = _load_required(asset_path)
    item_key = preview_collection.try_add_item_from_wardrobe_item(
        slot_name=slot_name,
        wardrobe_item=item,
    )
    if item_key is None:
        raise RuntimeError(f"Could not add {asset_path} to MetaHuman slot {slot_name}")
    preview_collection.default_instance.set_single_slot_selection(
        slot_name=slot_name,
        item_key=item_key,
    )
    _log(f"Selected {slot_name}: {item.get_name()}")
    return item_key


def _set_parameter(parameters, name: str, *, float_value=None, color_value=None) -> bool:
    parameter = next((value for value in parameters if str(value.name) == name), None)
    if parameter is None:
        _log(f"Optional instance parameter not found: {name}")
        return False
    if float_value is not None:
        parameter.set_float(value=float_value)
    elif color_value is not None:
        parameter.set_color(value=color_value)
    else:
        raise ValueError("A float or color value is required")
    return True


def _apply_body(subsystem, character) -> None:
    constraints = subsystem.get_body_constraints(character)
    by_name = {
        str(constraint.name).lower().replace(" ", "_"): constraint
        for constraint in constraints
    }

    # The reference is a slim, athletic young man rather than a broad bodybuilder.
    targets_cm = {
        "height": 177.5,
        "chest_circumference": 92.0,
        "waist_circumference": 78.0,
    }
    changed = False
    for name, measurement in targets_cm.items():
        constraint = by_name.get(name)
        if constraint is None:
            _log(f"Body constraint unavailable in this preset: {name}")
            continue
        constraint.is_active = True
        constraint.target_measurement = measurement
        changed = True

    if changed:
        subsystem.set_body_constraints(character, list(by_name.values()))
        subsystem.commit_body_state(character)
        _log("Applied 177.5 cm slim-athletic body constraints")


def _apply_skin_and_makeup(subsystem, character) -> None:
    # MetaHuman's U/V skin control maps lightness/redness.  These values keep
    # Trey's underlying complexion detail while matching the warm medium-deep
    # cheek tone in the approved Zarri reference.
    skin_settings = character.skin_settings
    skin_settings.skin.u = 0.82
    skin_settings.skin.v = 0.48
    skin_settings.skin.roughness = 0.82
    skin_settings.freckles.density = 0.0
    skin_settings.freckles.strength = 0.0
    character.preview_material_type = unreal.MetaHumanCharacterSkinPreviewMaterial.EDITABLE
    subsystem.commit_skin_settings(character=character, skin_settings=skin_settings)

    makeup = character.makeup_settings
    makeup.foundation.apply_foundation = False
    makeup.blush.intensity = 0.0
    makeup.eyes.opacity = 0.0
    makeup.lips.opacity = 0.0
    subsystem.commit_makeup_settings(character=character, makeup_settings=makeup)
    _log("Applied warm medium-deep skin with clean youthful complexion")


def _apply_mobile_viewport(character) -> None:
    viewport = character.viewport_settings
    viewport.always_use_hair_cards = True
    viewport.level_of_detail = unreal.MetaHumanCharacterLOD.LOD3
    character.viewport_settings = viewport
    _log("Enabled hair-card preview and LOD3 mobile inspection")


def _apply_wardrobe(subsystem, character) -> None:
    preview = subsystem.get_preview_collection(character)
    if preview is None:
        raise RuntimeError("MetaHuman preview collection is unavailable")

    hair_key = _select_wardrobe_item(preview, "Hair", HAIR)
    _select_wardrobe_item(preview, "Eyebrows", EYEBROWS)
    _select_wardrobe_item(preview, "Eyelashes", EYELASHES)
    outfit_key = _select_wardrobe_item(preview, "Outfits", OUTFIT)

    empty_key = unreal.MetaHumanPaletteItemKey()
    for slot in ("Beard", "Mustache", "Peachfuzz"):
        preview.default_instance.set_single_slot_selection(
            slot_name=slot,
            item_key=empty_key,
        )
    subsystem.on_edit_preview_collection(character)
    _log("Removed beard, mustache, and peach fuzz")

    subsystem.assemble_for_preview(character=character)

    hair_parameters = preview.default_instance.get_instance_parameters(
        item_path=unreal.MetaHumanPaletteItemPath(item_key=hair_key)
    )
    _set_parameter(hair_parameters, "Melanin", float_value=1.0)
    _set_parameter(hair_parameters, "Redness", float_value=0.05)
    _set_parameter(hair_parameters, "Whiteness", float_value=0.0)
    _set_parameter(hair_parameters, "Lightness", float_value=0.0)
    _set_parameter(hair_parameters, "Roughness", float_value=0.85)
    _set_parameter(
        hair_parameters,
        "DyeColor",
        color_value=unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
    )

    outfit_parameters = preview.default_instance.get_instance_parameters(
        item_path=unreal.MetaHumanPaletteItemPath(item_key=outfit_key)
    )
    outfit_colors = {
        "PrimaryColorShirt": unreal.LinearColor(0.025, 0.05, 0.12, 1.0),
        "SecondaryColorShirt": unreal.LinearColor(0.07, 0.085, 0.12, 1.0),
        "PrimaryColorShort": unreal.LinearColor(0.012, 0.016, 0.025, 1.0),
        "SecondaryColorShort": unreal.LinearColor(0.035, 0.04, 0.05, 1.0),
    }
    for parameter_name, color in outfit_colors.items():
        if not _set_parameter(
            outfit_parameters,
            parameter_name,
            color_value=color,
        ):
            raise RuntimeError(f"Required outfit parameter is missing: {parameter_name}")

    subsystem.on_edit_preview_collection(character)
    _log("Applied black Afro Fade and dark indigo/charcoal temporary outfit")


def _request_cloud_data(subsystem, character) -> None:
    _log("Requesting joints-only MetaHuman rig (blocking)")
    rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    rig.blocking = True
    rig.report_progress = True
    rig.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
    subsystem.request_auto_rigging(character, rig)

    _log("Requesting 2K source textures (blocking)")
    textures = unreal.MetaHumanCharacterTextureRequestParams()
    textures.blocking = True
    textures.report_progress = True
    subsystem.request_texture_sources(character, textures)


def _clear_runtime_assembly() -> None:
    """Remove the previous generated output so stale duplicates never accumulate."""

    runtime_folder = "/Game/MetaHumans/Zarri"
    if not unreal.EditorAssetLibrary.does_directory_exist(runtime_folder):
        return
    if not unreal.EditorAssetLibrary.delete_directory(runtime_folder):
        raise RuntimeError(f"Could not replace existing assembly: {runtime_folder}")
    _log("Removed the prior runtime assembly before rebuilding")


def _assemble(subsystem, character) -> None:
    if not subsystem.can_build_meta_human(character, True):
        raise RuntimeError("Zarri is not ready for assembly; see preceding MetaHuman log error")

    build = unreal.MetaHumanCharacterEditorBuildParameters()
    build.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    build.pipeline_quality = unreal.MetaHumanQualityLevel.LOW
    build.animation_system_name = "AnimBP"
    build.absolute_build_path = "/Game/MetaHumans"
    build.name_override = "Zarri"
    build.common_folder_path = "/Game/MetaHumans/Common"
    build.enable_wardrobe_item_validation = True

    _log("Assembling Optimized Low runtime assets")
    subsystem.build_meta_human(character=character, params=build)

    runtime_class = unreal.load_class(None, RUNTIME_BLUEPRINT)
    if runtime_class is None:
        raise RuntimeError(f"Assembly did not create {RUNTIME_BLUEPRINT}")
    _log(f"Verified runtime Blueprint: {RUNTIME_BLUEPRINT}")


def _tint_assembled_materials() -> None:
    """Lock the assembled hair to Zarri's black reference palette."""

    hair_materials = [
        path
        for path in unreal.EditorAssetLibrary.list_assets(
            "/Game/MetaHumans/Zarri/Grooms",
            recursive=True,
            include_folder=False,
        )
        if "MI_WI_Hair_S_AfroFade_" in path
    ]
    if not hair_materials:
        raise RuntimeError("Assembled Zarri hair materials were not found")
    for asset_path in hair_materials:
        material = _load_required(asset_path)
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material,
            "hairMelanin",
            1.0,
        )
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material,
            "hairRedness",
            0.0,
        )
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material,
            "LightAmount",
            0.0,
        )
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material,
            "HairRoughness",
            0.85,
        )
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            material,
            "hairDye",
            unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
        )
        unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

    _log("Applied assembled black-hair material overrides")


def main() -> None:
    _log("Starting Zarri MetaHuman pass")
    character = _get_or_create_character()
    subsystem = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)

    if not subsystem.try_add_object_to_edit(character):
        raise RuntimeError("Zarri MetaHuman is already open for editing; close it and rerun")

    try:
        _apply_mobile_viewport(character)
        _apply_skin_and_makeup(subsystem, character)
        _apply_body(subsystem, character)
        _apply_wardrobe(subsystem, character)
        unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
        _request_cloud_data(subsystem, character)
        unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
        _clear_runtime_assembly()
        _assemble(subsystem, character)
        _tint_assembled_materials()
        unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    finally:
        if subsystem.is_object_added_for_editing(character):
            subsystem.remove_object_to_edit(character)

    _log("PASS: Zarri MetaHuman source and runtime Blueprint are saved")


if __name__ == "__main__":
    main()
