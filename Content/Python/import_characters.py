"""Import only the 16 authored human avatars and their locomotion clips.

Unlike import_artwork.py this focused pass never touches TestMap or street
props. It is safe to rerun when character source FBXs change.
"""
import os
import unreal


CONTENT_DIR = unreal.SystemLibrary.get_project_content_directory()
SOURCE_DIR = os.path.join(CONTENT_DIR, "Import", "Pedestrians")
DEST_ROOT = "/Game/Pedestrians"
ANIMATION_SUFFIXES = ["Idle", "Walk", "Jog", "Talk", "WalkFormal"]

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def import_character(fbx_path):
    variant = os.path.splitext(os.path.basename(fbx_path))[0]
    destination = "{}/{}".format(DEST_ROOT, variant)
    if eal.does_directory_exist(destination):
        eal.delete_directory(destination)

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.import_animations = True
    options.import_materials = True
    options.import_textures = True
    options.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
    options.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
    options.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = destination
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = options
    asset_tools.import_asset_tasks([task])

    for path in list(eal.list_assets(destination, recursive=True)):
        asset = eal.load_asset(path)
        if not asset:
            continue
        source_name = path.rsplit("/", 1)[-1].split(".", 1)[0]
        target_name = None
        if isinstance(asset, unreal.SkeletalMesh):
            target_name = "SK_{}".format(variant)
        elif isinstance(asset, unreal.AnimSequence):
            for suffix in ANIMATION_SUFFIXES:
                if source_name.endswith("A_" + suffix):
                    target_name = "{}_{}".format(variant, suffix)
                    break
        if target_name and source_name != target_name:
            eal.rename_asset(path, "{}/{}".format(destination, target_name))

    eal.save_directory(destination, only_if_is_dirty=False)
    expected = ["{}/SK_{}".format(destination, variant)] + [
        "{}/{}_{}".format(destination, variant, suffix)
        for suffix in ANIMATION_SUFFIXES
    ]
    missing = [path for path in expected if not eal.does_asset_exist(path)]
    if missing:
        raise RuntimeError("{} import missing {}".format(variant, ", ".join(missing)))
    unreal.log("[Characters] imported {} with five locomotion clips".format(variant))


fbx_files = sorted(
    os.path.join(SOURCE_DIR, name)
    for name in os.listdir(SOURCE_DIR)
    if name.lower().endswith(".fbx")
)
if not fbx_files:
    raise RuntimeError("No avatar FBXs found under {}".format(SOURCE_DIR))

for source_path in fbx_files:
    import_character(source_path)

unreal.log("[Characters] DONE: {} real human variants ready".format(len(fbx_files)))
