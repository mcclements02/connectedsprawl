"""Import the downloaded car FBX files into /Game/Vehicles/Imported.
Also imports the police-car texture set and the subway car.
Probes bounding boxes so we know how to scale them when used as ASprawlCar bodies.
"""
import unreal
import os
import glob

DEST = "/Game/Vehicles/Imported"
STAGING = "/Users/matthewx/code/ConnectedSprawl/staging"

# Gather FBX files we want to import.
generic_fbx = sorted(glob.glob(os.path.join(STAGING, "cars", "*.fbx")))
subway_fbx = "/Users/matthewx/code/ConnectedSprawl/sm_subwaycar.fbx"
police_fbx = os.path.join(STAGING, "police_interceptor.fbx")

# Police car textures (the .png/.jpg set)
police_tex_dir = os.path.join(STAGING, "police", "textures")


def fbx_import_task(filepath, dest_path):
    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", True)
    opts.set_editor_property("import_textures", True)
    opts.set_editor_property("import_materials", True)
    opts.set_editor_property("import_as_skeletal", False)
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm_opts = opts.static_mesh_import_data
    sm_opts.set_editor_property("combine_meshes", True)
    sm_opts.set_editor_property("generate_lightmap_u_vs", True)
    sm_opts.set_editor_property("auto_generate_collision", True)
    # Center pivot at bounds bottom so wheels land on the ground cleanly.
    # (We'll re-anchor in code; just leave defaults.)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filepath)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("options", opts)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    return task


def tex_import_task(filepath, dest_path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filepath)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    return task


tasks = []
for f in generic_fbx:
    tasks.append(fbx_import_task(f, DEST + "/Generic"))
if os.path.exists(subway_fbx):
    tasks.append(fbx_import_task(subway_fbx, DEST + "/Subway"))
if os.path.exists(police_fbx):
    tasks.append(fbx_import_task(police_fbx, DEST + "/Police"))
for f in sorted(glob.glob(os.path.join(police_tex_dir, "*"))):
    tasks.append(tex_import_task(f, DEST + "/Police/Textures"))

unreal.log("[ImportVeh] {} import tasks".format(len(tasks)))
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

# Probe what we ended up with — list mesh names and their bounding-box extents.
imported = unreal.EditorAssetLibrary.list_assets(DEST, recursive=True)
unreal.log("[ImportVeh] {} assets total under {}".format(len(imported), DEST))
for path in imported:
    a = unreal.load_asset(path.rstrip("'"))
    if isinstance(a, unreal.StaticMesh):
        bb = a.get_bounding_box()
        sz = bb.max - bb.min
        unreal.log("[ImportVeh] MESH {:35s} size=({:.0f},{:.0f},{:.0f})".format(
            a.get_name(), sz.x, sz.y, sz.z))

unreal.log("[ImportVeh] DONE")
