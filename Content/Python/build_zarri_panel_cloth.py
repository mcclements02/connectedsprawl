"""Import Zarri's combined hoodie render mesh and build native Panel Cloth.

Run from Unreal Editor's Python console or with UnrealEditor-Cmd:
    -ExecutePythonScript=.../Content/Python/build_zarri_panel_cloth.py
"""

from pathlib import Path

import unreal


DESTINATION = "/Game/Import/Characters/Streetwear/PanelCloth"
RENDER_ASSET = f"{DESTINATION}/SM_ZarriHoodie_Render"
SIM_DESTINATION = "/Game/Import/Characters/Streetwear"
SIM_ASSET = f"{SIM_DESTINATION}/SM_RealCloth_Hoodie"
SOURCE_FBX = (
    Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    / "Import/Characters/Streetwear/SM_Streetwear_OversizedHoodie.fbx"
)
SIM_SOURCE_FBX = (
    Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    / "Import/Characters/Streetwear/SM_RealCloth_Hoodie.fbx"
)


def import_sim_mesh() -> unreal.StaticMesh:
    if not SIM_SOURCE_FBX.is_file():
        raise RuntimeError(f"Panel Cloth simulation source is missing: {SIM_SOURCE_FBX}")

    import_ui = unreal.FbxImportUI()
    import_ui.import_mesh = True
    import_ui.import_as_skeletal = False
    import_ui.import_materials = False
    import_ui.import_textures = False
    import_ui.static_mesh_import_data.combine_meshes = True
    import_ui.static_mesh_import_data.generate_lightmap_u_vs = False
    import_ui.static_mesh_import_data.remove_degenerates = True

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = SIM_DESTINATION
    task.destination_name = "SM_RealCloth_Hoodie"
    task.filename = str(SIM_SOURCE_FBX)
    task.options = import_ui
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    sim_mesh = unreal.load_asset(SIM_ASSET)
    if not isinstance(sim_mesh, unreal.StaticMesh):
        imported = ", ".join(task.imported_object_paths)
        raise RuntimeError(
            f"Panel Cloth simulation import failed; imported=[{imported}]"
        )
    unreal.log(f"[PanelClothAuthoring] Sim mesh ready: {sim_mesh.get_path_name()}")
    return sim_mesh


def import_render_mesh() -> unreal.StaticMesh:
    if not SOURCE_FBX.is_file():
        raise RuntimeError(f"Panel Cloth render source is missing: {SOURCE_FBX}")

    import_ui = unreal.FbxImportUI()
    import_ui.import_mesh = True
    import_ui.import_as_skeletal = False
    import_ui.import_materials = False
    import_ui.import_textures = False
    import_ui.static_mesh_import_data.combine_meshes = True
    import_ui.static_mesh_import_data.generate_lightmap_u_vs = False
    import_ui.static_mesh_import_data.remove_degenerates = True

    task = unreal.AssetImportTask()
    task.automated = True
    task.destination_path = DESTINATION
    task.destination_name = "SM_ZarriHoodie_Render"
    task.filename = str(SOURCE_FBX)
    task.options = import_ui
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    render_mesh = unreal.load_asset(RENDER_ASSET)
    if not isinstance(render_mesh, unreal.StaticMesh):
        imported = ", ".join(task.imported_object_paths)
        raise RuntimeError(
            f"Combined Panel Cloth render import failed; imported=[{imported}]"
        )
    unreal.log(f"[PanelClothAuthoring] Render mesh ready: {render_mesh.get_path_name()}")
    return render_mesh


def build_panel_cloth() -> None:
    sim_mesh = import_sim_mesh()
    render_mesh = import_render_mesh()
    result = unreal.SprawlPanelClothAuthoringLibrary.create_or_update_zarri_hoodie_asset()
    if isinstance(result, tuple):
        succeeded, message = result
    else:
        succeeded, message = bool(result), "See [PanelClothAuthoring] in the Unreal log"
    if not succeeded:
        raise RuntimeError(f"Panel Cloth build failed: {message}")
    unreal.EditorAssetLibrary.save_loaded_asset(sim_mesh, only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_loaded_asset(render_mesh, only_if_is_dirty=False)
    unreal.log(f"[PanelClothAuthoring] PASS {message}")


if __name__ == "__main__":
    build_panel_cloth()
