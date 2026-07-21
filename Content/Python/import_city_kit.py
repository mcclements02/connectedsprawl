"""Import the Blender-authored city kit and build its materials.

Consumes Content/Import/CityKit/ (from Tools/build_city_kit.py):
  * 9 PNGs  -> /Game/Import/CityKit/Textures (normal maps TC_Normalmap,
               roughness linear masks)
  * 2 FBX   -> /Game/Import/CityKit (SM_GrassClump with vertex colours,
               SM_WaterSurface centred + finely tessellated)
  * MIs     -> MI_AsphaltWorn / MI_ConcreteWorn / MI_GrassField parented to
               the world-projected M_RealGround master, and a new M_GrassBlade
               material driven by the clump's vertex colours.

No actors are spawned, so -nullrhi is safe. UE 5.8 swallows python stdout:
the proof of work is Saved/CityKitImportReport.txt.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/import_city_kit.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

SRC = os.path.join(unreal.Paths.project_content_dir(), "Import", "CityKit")
DEST = "/Game/Import/CityKit"
TEX_DEST = DEST + "/Textures"
GROUND_MASTER = "/Game/RealKit/Materials/M_RealGround"

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "CityKitImportReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def import_texture(png_name):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, "Textures", png_name + ".png")
    task.destination_path = TEX_DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = unreal.load_asset(TEX_DEST + "/" + png_name)
    if not tex:
        out("FAILED texture import: " + png_name)
        return None
    if png_name.endswith("_NormalGL"):
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif png_name.endswith("_Roughness"):
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    out("texture %s ok (srgb=%s)" % (png_name, tex.get_editor_property("srgb")))
    return tex


def import_mesh(fbx_name, vertex_colors):
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_materials = False
    ui.import_textures = False
    ui.import_animations = False
    ui.import_as_skeletal = False
    ui.static_mesh_import_data.combine_meshes = True
    ui.static_mesh_import_data.generate_lightmap_u_vs = False
    ui.static_mesh_import_data.remove_degenerates = True
    if vertex_colors:
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
    bounds = mesh.get_bounds()
    ext = bounds.box_extent
    out("mesh %s ok extent=(%.0f, %.0f, %.0f)cm tris_lod0=%d" % (
        fbx_name, ext.x, ext.y, ext.z,
        mesh.get_num_triangles(0)))
    return mesh


def make_ground_mi(name, albedo, normal, rough):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    parent = unreal.load_asset(GROUND_MASTER)
    if not parent:
        out("FAILED: ground master missing: " + GROUND_MASTER)
        return None
    mel = unreal.MaterialEditingLibrary
    existing = unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + name)
    if existing:
        mi = unreal.load_asset(DEST + "/" + name)
    else:
        mi = tools.create_asset(name, DEST,
                                unreal.MaterialInstanceConstant,
                                unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for pname, tex in (("AlbedoTex", albedo), ("NormalTex", normal),
                       ("RoughTex", rough)):
        if tex:
            mel.set_material_instance_texture_parameter_value(mi, pname, tex)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    out("material %s -> parent M_RealGround" % name)
    return mi


def make_grass_blade_material():
    """Opaque material whose base colour is the clump's baked vertex ramp."""
    path = DEST + "/M_GrassBlade"
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mat = unreal.load_asset(path)
    else:
        mat = tools.create_asset("M_GrassBlade", DEST, unreal.Material,
                                 unreal.MaterialFactoryNew())
    vcol = mel.create_material_expression(
        mat, unreal.MaterialExpressionVertexColor, -400, 0)
    rough = mel.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -400, 220)
    rough.set_editor_property("r", 0.85)
    mel.connect_material_property(vcol, "",
                                  unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "",
                                  unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    out("material M_GrassBlade (vertex-colour ramp) ok")
    return mat


def main():
    # Live sanity: which texture params does the ground master actually expose?
    parent = unreal.load_asset(GROUND_MASTER)
    if parent:
        names = list(unreal.MaterialEditingLibrary
                     .get_texture_parameter_names(parent))
        out("M_RealGround texture params: %s" % ", ".join(
            str(n) for n in names))

    sets = {}
    for prefix in ("T_CityAsphalt", "T_CityConcrete", "T_CityGrassField"):
        sets[prefix] = tuple(
            import_texture(prefix + suffix)
            for suffix in ("_Color", "_NormalGL", "_Roughness"))

    import_mesh("SM_GrassClump", vertex_colors=True)
    import_mesh("SM_WaterSurface", vertex_colors=False)

    make_ground_mi("MI_AsphaltWorn", *sets["T_CityAsphalt"])
    make_ground_mi("MI_ConcreteWorn", *sets["T_CityConcrete"])
    make_ground_mi("MI_GrassField", *sets["T_CityGrassField"])
    make_grass_blade_material()

    out("")
    out("PASS")


try:
    main()
except Exception as exc:  # noqa: BLE001 - report every failure to the sidecar
    out("EXCEPTION: %r" % exc)
finally:
    flush()
