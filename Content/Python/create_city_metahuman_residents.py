"""Author and assemble the first project-owned MetaHuman resident roster.

The crowd runtime only accepts Optimized/Low Blueprint classes under
``/Game/MetaHumans``.  This idempotent editor pass creates two distinct source
characters from the UE 5.8 Core Data presets, applies project-owned body,
groom, and wardrobe decisions, requests joints-only rigs plus 2K textures, and
assembles:

* ``/Game/MetaHumans/Residents/Amina/BP_Amina``
* ``/Game/MetaHumans/Residents/Andre/BP_Andre``

The bundled presets are visual starting points, not narrative identities.  The
source assets are saved in the project so future face/wardrobe revisions remain
editable without changing the replicated runtime contract.
"""

from __future__ import annotations

from dataclasses import dataclass

import unreal


SOURCE_PACKAGE = "/Game/MetaHumans/Source"
RUNTIME_ROOT = "/Game/MetaHumans/Residents"
STAGING_ROOT = "/Game/MetaHumans/Residents_Staging"
BACKUP_ROOT = "/Game/MetaHumans/Residents_Backup"
COMMON_ROOT = "/Game/MetaHumans/Common"
OUTFIT = (
    "/MetaHumanCharacter/Optional/Clothing/"
    "WI_DefaultGarment.WI_DefaultGarment"
)
EYELASHES = (
    "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyelashes/"
    "WI_Eyelashes_S_Fine.WI_Eyelashes_S_Fine"
)


@dataclass(frozen=True)
class Resident:
    name: str
    preset: str
    hair: str
    eyebrows: str
    height_cm: float
    chest_cm: float
    waist_cm: float
    shirt: unreal.LinearColor
    shorts: unreal.LinearColor
    hair_melanin: float
    hair_redness: float

    @property
    def source_name(self) -> str:
        return f"MHC_{self.name}"

    @property
    def source_asset(self) -> str:
        return f"{SOURCE_PACKAGE}/{self.source_name}.{self.source_name}"

    @property
    def runtime_folder(self) -> str:
        return f"{RUNTIME_ROOT}/{self.name}"

    @property
    def runtime_class(self) -> str:
        return f"{self.runtime_folder}/BP_{self.name}.BP_{self.name}_C"

    @property
    def runtime_blueprint(self) -> str:
        return f"{self.runtime_folder}/BP_{self.name}.BP_{self.name}"

    @property
    def staging_folder(self) -> str:
        return f"{STAGING_ROOT}/{self.name}"

    @property
    def staging_class(self) -> str:
        return f"{self.staging_folder}/BP_{self.name}.BP_{self.name}_C"

    @property
    def backup_folder(self) -> str:
        return f"{BACKUP_ROOT}/{self.name}"


RESIDENTS = (
    Resident(
        name="Amina",
        preset="/MetaHumanCharacter/Optional/Presets/Asha.Asha",
        hair=(
            "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/"
            "WI_Hair_S_Cornrows.WI_Hair_S_Cornrows"
        ),
        eyebrows=(
            "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyebrows/"
            "WI_Eyebrows_M_SlightArch.WI_Eyebrows_M_SlightArch"
        ),
        height_cm=168.0,
        chest_cm=88.0,
        waist_cm=72.0,
        shirt=unreal.LinearColor(0.18, 0.025, 0.055, 1.0),
        shorts=unreal.LinearColor(0.025, 0.035, 0.075, 1.0),
        hair_melanin=0.98,
        hair_redness=0.03,
    ),
    Resident(
        name="Andre",
        preset="/MetaHumanCharacter/Optional/Presets/Omari.Omari",
        hair=(
            "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/"
            "WI_Hair_S_CurlyFade.WI_Hair_S_CurlyFade"
        ),
        eyebrows=(
            "/MetaHumanCharacter/Optional/Grooms/Bindings/Eyebrows/"
            "WI_Eyebrows_M_Natural.WI_Eyebrows_M_Natural"
        ),
        height_cm=183.0,
        chest_cm=101.0,
        waist_cm=86.0,
        shirt=unreal.LinearColor(0.025, 0.12, 0.065, 1.0),
        shorts=unreal.LinearColor(0.025, 0.028, 0.032, 1.0),
        hair_melanin=1.0,
        hair_redness=0.01,
    ),
)


def _log(message: str) -> None:
    unreal.log(f"[CityMetaHumans] {message}")


def _load_required(path: str):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required MetaHuman asset is missing: {path}")
    return asset


def _get_or_create_source(spec: Resident):
    character = unreal.load_asset(spec.source_asset)
    if character is not None:
        _log(f"Reusing project source {spec.source_asset}")
        return character, False

    character = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        spec.source_name,
        SOURCE_PACKAGE,
        _load_required(spec.preset),
    )
    if character is None:
        raise RuntimeError(
            f"Could not duplicate {spec.preset} to {spec.source_asset}"
        )
    _log(f"Created {spec.name} from bundled preset {spec.preset}")
    return character, True


def _select_item(preview, slot_name: str, asset_path: str):
    item = _load_required(asset_path)
    key = preview.try_add_item_from_wardrobe_item(
        slot_name=slot_name,
        wardrobe_item=item,
    )
    if key is None:
        raise RuntimeError(f"Could not add {asset_path} to slot {slot_name}")
    preview.default_instance.set_single_slot_selection(
        slot_name=slot_name,
        item_key=key,
    )
    return key


def _set_parameter(parameters, name: str, *, number=None, color=None) -> bool:
    parameter = next((value for value in parameters if str(value.name) == name), None)
    if parameter is None:
        _log(f"Optional instance parameter not found: {name}")
        return False
    if number is not None:
        parameter.set_float(value=number)
    elif color is not None:
        parameter.set_color(value=color)
    else:
        raise ValueError("A number or color is required")
    return True


def _darken(color: unreal.LinearColor, amount: float) -> unreal.LinearColor:
    return unreal.LinearColor(
        color.r * amount,
        color.g * amount,
        color.b * amount,
        1.0,
    )


def _apply_mobile_preview(character) -> None:
    viewport = character.viewport_settings
    viewport.always_use_hair_cards = True
    viewport.level_of_detail = unreal.MetaHumanCharacterLOD.LOD3
    character.viewport_settings = viewport


def _apply_body(subsystem, character, spec: Resident) -> None:
    constraints = subsystem.get_body_constraints(character)
    by_name = {
        str(constraint.name).lower().replace(" ", "_"): constraint
        for constraint in constraints
    }
    targets = {
        "height": spec.height_cm,
        "chest_circumference": spec.chest_cm,
        "waist_circumference": spec.waist_cm,
    }
    changed = False
    for name, measurement in targets.items():
        constraint = by_name.get(name)
        if constraint is None:
            _log(f"{spec.name}: body constraint unavailable: {name}")
            continue
        constraint.is_active = True
        constraint.target_measurement = measurement
        changed = True
    if changed:
        subsystem.set_body_constraints(character, list(by_name.values()))
        subsystem.commit_body_state(character)
    _log(
        f"{spec.name}: body {spec.height_cm:.0f} cm, "
        f"chest {spec.chest_cm:.0f}, waist {spec.waist_cm:.0f}"
    )


def _apply_wardrobe(subsystem, character, spec: Resident) -> None:
    preview = subsystem.get_preview_collection(character)
    if preview is None:
        raise RuntimeError(f"{spec.name}: MetaHuman preview collection is unavailable")

    hair_key = _select_item(preview, "Hair", spec.hair)
    _select_item(preview, "Eyebrows", spec.eyebrows)
    _select_item(preview, "Eyelashes", EYELASHES)
    outfit_key = _select_item(preview, "Outfits", OUTFIT)

    empty_key = unreal.MetaHumanPaletteItemKey()
    for slot in ("Beard", "Mustache", "Peachfuzz"):
        preview.default_instance.set_single_slot_selection(
            slot_name=slot,
            item_key=empty_key,
        )
    subsystem.on_edit_preview_collection(character)
    subsystem.assemble_for_preview(character=character)

    hair_parameters = preview.default_instance.get_instance_parameters(
        item_path=unreal.MetaHumanPaletteItemPath(item_key=hair_key)
    )
    _set_parameter(hair_parameters, "Melanin", number=spec.hair_melanin)
    _set_parameter(hair_parameters, "Redness", number=spec.hair_redness)
    _set_parameter(hair_parameters, "Whiteness", number=0.0)
    _set_parameter(hair_parameters, "Lightness", number=0.0)
    _set_parameter(hair_parameters, "Roughness", number=0.82)
    _set_parameter(
        hair_parameters,
        "DyeColor",
        color=unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
    )

    outfit_parameters = preview.default_instance.get_instance_parameters(
        item_path=unreal.MetaHumanPaletteItemPath(item_key=outfit_key)
    )
    required_colors = {
        "PrimaryColorShirt": spec.shirt,
        "SecondaryColorShirt": _darken(spec.shirt, 0.65),
        "PrimaryColorShort": spec.shorts,
        "SecondaryColorShort": _darken(spec.shorts, 0.65),
    }
    for name, color in required_colors.items():
        if not _set_parameter(outfit_parameters, name, color=color):
            raise RuntimeError(f"{spec.name}: required outfit parameter missing: {name}")
    subsystem.on_edit_preview_collection(character)
    _log(f"{spec.name}: selected unique groom and wardrobe palette")


def _request_cloud_data(subsystem, character, spec: Resident) -> None:
    _log(f"{spec.name}: requesting joints-only rig (blocking)")
    rig = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    rig.blocking = True
    rig.report_progress = True
    rig.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
    subsystem.request_auto_rigging(character, rig)

    _log(f"{spec.name}: requesting 2K texture sources (blocking)")
    textures = unreal.MetaHumanCharacterTextureRequestParams()
    textures.blocking = True
    textures.report_progress = True
    subsystem.request_texture_sources(character, textures)


def _replace_runtime(subsystem, character, spec: Resident) -> None:
    # The MetaHuman baker is expensive and may be interrupted by an editor or
    # GPU failure. Build into a disposable staging folder first; never delete a
    # known-good resident until the complete staged Blueprint has loaded.
    if unreal.EditorAssetLibrary.does_directory_exist(spec.staging_folder):
        if not unreal.EditorAssetLibrary.delete_directory(spec.staging_folder):
            raise RuntimeError(
                f"Could not clear generated staging folder: {spec.staging_folder}"
            )

    if not subsystem.can_build_meta_human(character, True):
        raise RuntimeError(f"{spec.name}: source is not ready for assembly")
    build = unreal.MetaHumanCharacterEditorBuildParameters()
    build.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    build.pipeline_quality = unreal.MetaHumanQualityLevel.LOW
    build.animation_system_name = "AnimBP"
    build.absolute_build_path = STAGING_ROOT
    build.name_override = spec.name
    build.common_folder_path = COMMON_ROOT
    build.enable_wardrobe_item_validation = True
    subsystem.build_meta_human(character=character, params=build)

    if unreal.load_class(None, spec.staging_class) is None:
        raise RuntimeError(f"Assembly did not create {spec.staging_class}")
    if not unreal.EditorAssetLibrary.save_directory(
        spec.staging_folder, only_if_is_dirty=False, recursive=True
    ):
        raise RuntimeError(f"Could not save staged assembly: {spec.staging_folder}")

    if unreal.EditorAssetLibrary.does_directory_exist(spec.backup_folder):
        if not unreal.EditorAssetLibrary.delete_directory(spec.backup_folder):
            raise RuntimeError(
                f"Could not clear stale generated backup: {spec.backup_folder}"
            )

    moved_existing = False
    if unreal.EditorAssetLibrary.does_directory_exist(spec.runtime_folder):
        moved_existing = unreal.EditorAssetLibrary.rename_directory(
            spec.runtime_folder, spec.backup_folder
        )
        if not moved_existing:
            raise RuntimeError(
                f"Could not preserve current runtime: {spec.runtime_folder}"
            )

    if not unreal.EditorAssetLibrary.rename_directory(
        spec.staging_folder, spec.runtime_folder
    ):
        if moved_existing:
            unreal.EditorAssetLibrary.rename_directory(
                spec.backup_folder, spec.runtime_folder
            )
        raise RuntimeError(
            f"Could not promote staged assembly: {spec.staging_folder}"
        )

    if unreal.load_class(None, spec.runtime_class) is None:
        if moved_existing:
            unreal.EditorAssetLibrary.rename_directory(
                spec.runtime_folder, spec.staging_folder
            )
            unreal.EditorAssetLibrary.rename_directory(
                spec.backup_folder, spec.runtime_folder
            )
        raise RuntimeError(f"Promoted assembly did not load: {spec.runtime_class}")

    if not unreal.EditorAssetLibrary.save_directory(
        spec.runtime_folder, only_if_is_dirty=False, recursive=True
    ):
        raise RuntimeError(f"Could not save runtime assembly: {spec.runtime_folder}")
    if moved_existing:
        unreal.EditorAssetLibrary.delete_directory(spec.backup_folder)
    _log(f"{spec.name}: verified {spec.runtime_class}")


def _author_one(subsystem, spec: Resident) -> None:
    if unreal.EditorAssetLibrary.does_asset_exist(spec.runtime_blueprint):
        _log(f"{spec.name}: verified existing runtime; no rebuild needed")
        return

    character, created = _get_or_create_source(spec)
    if not subsystem.try_add_object_to_edit(character):
        raise RuntimeError(f"{spec.name} is already open for MetaHuman editing")
    try:
        # Resume safely after an editor/material-bake interruption.  A source
        # with complete cloud data can assemble directly; re-committing the
        # same body would invalidate that expensive completed rig request.
        if not created and subsystem.can_build_meta_human(character, True):
            _log(
                f"{spec.name}: resuming from saved rig/texture source data "
                "and refreshing wardrobe"
            )
            _apply_mobile_preview(character)
            _apply_wardrobe(subsystem, character, spec)
            unreal.EditorAssetLibrary.save_loaded_asset(
                character, only_if_is_dirty=False
            )
            if not subsystem.can_build_meta_human(character, True):
                _request_cloud_data(subsystem, character, spec)
                unreal.EditorAssetLibrary.save_loaded_asset(
                    character, only_if_is_dirty=False
                )
            _replace_runtime(subsystem, character, spec)
            return

        _apply_mobile_preview(character)
        _apply_body(subsystem, character, spec)
        _apply_wardrobe(subsystem, character, spec)
        unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
        _request_cloud_data(subsystem, character, spec)
        unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
        _replace_runtime(subsystem, character, spec)
        unreal.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False)
    finally:
        if subsystem.is_object_added_for_editing(character):
            subsystem.remove_object_to_edit(character)


def main() -> None:
    _log("Starting two-person city MetaHuman authoring pass")
    subsystem = unreal.get_editor_subsystem(
        unreal.MetaHumanCharacterEditorSubsystem
    )
    for spec in RESIDENTS:
        _author_one(subsystem, spec)
    # Save only assets owned by this pass. Interactive editor sessions may have
    # unrelated dirty maps or content that must remain entirely user-controlled.
    unreal.EditorAssetLibrary.save_directory(
        COMMON_ROOT, only_if_is_dirty=True, recursive=True
    )
    _log("PASS: Amina and Andre source/runtime assets are saved")


if __name__ == "__main__":
    main()
