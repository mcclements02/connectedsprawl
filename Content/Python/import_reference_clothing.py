"""Import the Blender-authored gray shirt and dark pants as static meshes."""

from pathlib import Path

import unreal


DESTINATION = "/Game/Import/Characters/ReferenceClothing"
SOURCE_DIRECTORY = (
    Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    / "Import/Characters/ReferenceClothing"
)
PIECES = (
    "SM_ReferenceShirt_Torso",
    "SM_ReferenceShirt_Collar",
    "SM_ReferenceShirt_Hem",
    "SM_ReferenceShirt_UpperSleeve_L",
    "SM_ReferenceShirt_LowerSleeve_L",
    "SM_ReferenceShirt_Cuff_L",
    "SM_ReferenceShirt_UpperSleeve_R",
    "SM_ReferenceShirt_LowerSleeve_R",
    "SM_ReferenceShirt_Cuff_R",
    "SM_ReferencePants_Waist",
    "SM_ReferencePants_Waistband",
    "SM_ReferencePants_UpperLeg_L",
    "SM_ReferencePants_LowerLeg_L",
    "SM_ReferencePants_Cuff_L",
    "SM_ReferencePants_UpperLeg_R",
    "SM_ReferencePants_LowerLeg_R",
    "SM_ReferencePants_Cuff_R",
)


def import_piece(name: str) -> unreal.StaticMesh:
    source = SOURCE_DIRECTORY / f"{name}.fbx"
    if not source.is_file():
        raise RuntimeError(f"Reference clothing source is missing: {source}")

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = False
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.generate_lightmap_u_vs = False
    options.static_mesh_import_data.remove_degenerates = True

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION
    task.destination_name = name
    task.filename = str(source)
    task.options = options
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset = unreal.load_asset(f"{DESTINATION}/{name}")
    if not isinstance(asset, unreal.StaticMesh):
        imported = ", ".join(task.imported_object_paths)
        raise RuntimeError(f"Reference clothing import failed for {name}: [{imported}]")
    unreal.log(
        f"[ReferenceClothingImport] {name} vertices={asset.get_num_vertices(0)}")
    return asset


def import_all() -> None:
    imported = [import_piece(name) for name in PIECES]
    unreal.EditorAssetLibrary.save_loaded_assets(imported, only_if_is_dirty=False)
    unreal.log(
        f"[ReferenceClothingImport] PASS imported={len(imported)} "
        f"destination={DESTINATION}")


if __name__ == "__main__":
    import_all()
