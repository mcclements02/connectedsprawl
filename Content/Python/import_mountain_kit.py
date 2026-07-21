"""Import the Blender mountain ridges and build their vertex-colour material.

Consumes Content/Import/CityKit/SM_MountainRidge{A,B}.fbx (from
Tools/build_mountain_kit.py) into /Game/Import/CityKit and creates
M_MountainRock — VertexColor -> BaseColor with high constant roughness, zero
textures. No actors are spawned, so -nullrhi is safe. Proof of work is
Saved/MountainKitImportReport.txt (UE 5.8 swallows python stdout).

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/import_mountain_kit.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

SRC = os.path.join(unreal.Paths.project_content_dir(), "Import", "CityKit")
DEST = "/Game/Import/CityKit"

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "MountainKitImportReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def import_mesh(fbx_name):
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_materials = False
    ui.import_textures = False
    ui.import_animations = False
    ui.import_as_skeletal = False
    ui.static_mesh_import_data.combine_meshes = True
    ui.static_mesh_import_data.generate_lightmap_u_vs = False
    ui.static_mesh_import_data.remove_degenerates = True
    ui.static_mesh_import_data.vertex_color_import_option = (
        unreal.VertexColorImportOption.REPLACE)
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, fbx_name + ".fbx")
    task.destination_path = DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = ui
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(DEST + "/" + fbx_name)
    if not mesh:
        out("FAILED mesh import: " + fbx_name)
        return None
    ext = mesh.get_bounds().box_extent
    out("mesh %s ok extent=(%.0f, %.0f, %.0f)cm tris_lod0=%d" % (
        fbx_name, ext.x, ext.y, ext.z, mesh.get_num_triangles(0)))
    return mesh


def make_mountain_material():
    path = DEST + "/M_MountainRock"
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
    else:
        mat = tools.create_asset("M_MountainRock", DEST, unreal.Material,
                                 unreal.MaterialFactoryNew())
    vcol = mel.create_material_expression(
        mat, unreal.MaterialExpressionVertexColor, -400, 0)
    rough = mel.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -400, 220)
    rough.set_editor_property("r", 0.95)
    mel.connect_material_property(vcol, "",
                                  unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "",
                                  unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    out("material M_MountainRock (vertex-colour, textureless) ok")
    return mat


def main():
    import_mesh("SM_MountainRidgeA")
    import_mesh("SM_MountainRidgeB")
    make_mountain_material()
    out("")
    out("PASS")


try:
    main()
except Exception as exc:  # noqa: BLE001 - report every failure to the sidecar
    out("EXCEPTION: %r" % exc)
finally:
    flush()
