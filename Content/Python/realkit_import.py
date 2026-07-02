"""RealKit part A: import CC0 photoreal assets and build master materials.

Sources (downloaded by staging script into staging/realkit/):
  - ambientCG 2K PBR texture sets (CC0)  -> /Game/RealKit/Textures/<SetID>
  - Poly Haven glTF props (CC0)          -> /Game/RealKit/Props/<slug>

Builds two world-aligned master materials so the greybox cubes tile
photo textures without per-face UV work:
  - M_RealGround : projects along world XY (roads, sidewalks, grass)
  - M_RealFacade : projects (x+y, z) — correct for the axis-aligned block
                   buildings this city is made of

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended -nullrhi
"""
import glob
import os
import unreal

STAGING = "/Users/matthewx/code/ConnectedSprawl/staging/realkit"
TEX_ROOT = "/Game/RealKit/Textures"
MAT_ROOT = "/Game/RealKit/Materials"
PROP_ROOT = "/Game/RealKit/Props"

atools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

MAP_SUFFIXES = {
    "Color": "color",
    "NormalGL": "normal",
    "Roughness": "rough",
    "AmbientOcclusion": "ao",
    "Emission": "emissive",
    "Opacity": "opacity",
}


def import_file(filepath, dest_path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filepath)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    atools.import_asset_tasks([task])
    paths = list(task.get_editor_property("imported_object_paths"))
    return paths


def configure_texture(tex_path, kind):
    tex = unreal.load_asset(tex_path)
    if not tex:
        unreal.log_warning("[RealKit] missing texture {}".format(tex_path))
        return None
    if kind == "normal":
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("flip_green_channel", True)  # ambientCG ships OpenGL +Y
    elif kind in ("rough", "ao", "opacity"):
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", False)
    else:  # color / emissive stay sRGB
        tex.set_editor_property("srgb", True)
    eal.save_loaded_asset(tex)
    return tex


# ============================================================
# 1) ambientCG texture sets
# ============================================================
imported_sets = {}
for set_dir in sorted(glob.glob(os.path.join(STAGING, "textures", "*"))):
    set_id = os.path.basename(set_dir)
    dest = "{}/{}".format(TEX_ROOT, set_id)
    imported_sets[set_id] = {}
    for f in sorted(glob.glob(os.path.join(set_dir, "*.*"))):
        base = os.path.basename(f)
        if not base.lower().endswith((".jpg", ".png")):
            continue
        kind = None
        for suffix, k in MAP_SUFFIXES.items():
            if "_{}.".format(suffix) in base:
                kind = k
                break
        if not kind:
            continue
        asset_path = "{}/{}".format(dest, os.path.splitext(base)[0])
        if not eal.does_asset_exist(asset_path):
            import_file(f, dest)
        if configure_texture(asset_path, kind):
            imported_sets[set_id][kind] = asset_path
    unreal.log("[RealKit] set {}: {}".format(set_id, sorted(imported_sets[set_id])))


# ============================================================
# 2) Master materials (world-aligned projection)
# ============================================================
def build_master(name, facade_mode):
    path = "{}/{}".format(MAT_ROOT, name)
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = atools.create_asset(name, MAT_ROOT, unreal.Material,
                              unreal.MaterialFactoryNew())

    wp = MEL.create_material_expression(
        mat, unreal.MaterialExpressionWorldPosition, -1250, 0)

    tile = MEL.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, -1250, 260)
    tile.set_editor_property("parameter_name", "TileSize")
    tile.set_editor_property("default_value", 400.0)

    if facade_mode:
        # facades of axis-aligned boxes: U = x + y (one term constant per
        # face), V = z. Cheap box projection with no triplanar cost.
        mask_x = MEL.create_material_expression(
            mat, unreal.MaterialExpressionComponentMask, -1050, -80)
        mask_x.set_editor_property("r", True)
        mask_x.set_editor_property("g", False)
        mask_x.set_editor_property("b", False)
        mask_x.set_editor_property("a", False)
        mask_y = MEL.create_material_expression(
            mat, unreal.MaterialExpressionComponentMask, -1050, 20)
        mask_y.set_editor_property("r", False)
        mask_y.set_editor_property("g", True)
        mask_y.set_editor_property("b", False)
        mask_y.set_editor_property("a", False)
        mask_z = MEL.create_material_expression(
            mat, unreal.MaterialExpressionComponentMask, -1050, 120)
        mask_z.set_editor_property("r", False)
        mask_z.set_editor_property("g", False)
        mask_z.set_editor_property("b", True)
        mask_z.set_editor_property("a", False)
        MEL.connect_material_expressions(wp, "", mask_x, "")
        MEL.connect_material_expressions(wp, "", mask_y, "")
        MEL.connect_material_expressions(wp, "", mask_z, "")
        add_xy = MEL.create_material_expression(
            mat, unreal.MaterialExpressionAdd, -900, -30)
        MEL.connect_material_expressions(mask_x, "", add_xy, "A")
        MEL.connect_material_expressions(mask_y, "", add_xy, "B")
        append = MEL.create_material_expression(
            mat, unreal.MaterialExpressionAppendVector, -780, 40)
        MEL.connect_material_expressions(add_xy, "", append, "A")
        MEL.connect_material_expressions(mask_z, "", append, "B")
        uv_src = append
    else:
        mask_xy = MEL.create_material_expression(
            mat, unreal.MaterialExpressionComponentMask, -1050, 0)
        mask_xy.set_editor_property("r", True)
        mask_xy.set_editor_property("g", True)
        mask_xy.set_editor_property("b", False)
        mask_xy.set_editor_property("a", False)
        MEL.connect_material_expressions(wp, "", mask_xy, "")
        uv_src = mask_xy

    div = MEL.create_material_expression(
        mat, unreal.MaterialExpressionDivide, -650, 60)
    MEL.connect_material_expressions(uv_src, "", div, "A")
    MEL.connect_material_expressions(tile, "", div, "B")

    def tex_param(pname, y, sampler_type):
        node = MEL.create_material_expression(
            mat, unreal.MaterialExpressionTextureSampleParameter2D, -420, y)
        node.set_editor_property("parameter_name", pname)
        node.set_editor_property("sampler_type", sampler_type)
        MEL.connect_material_expressions(div, "", node, "UVs")
        return node

    albedo = tex_param("AlbedoTex", -220, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    normal = tex_param("NormalTex", 20, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    rough = tex_param("RoughTex", 260, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    ao = tex_param("AOTex", 500, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)

    # sensible defaults so the params compile before MIC overrides land
    defaults = imported_sets.get("Concrete034") or next(iter(imported_sets.values()), {})
    for node, key in ((albedo, "color"), (normal, "normal"), (rough, "rough"), (ao, "ao")):
        if key in defaults:
            node.set_editor_property("texture", unreal.load_asset(defaults[key]))

    tint = MEL.create_material_expression(
        mat, unreal.MaterialExpressionVectorParameter, -420, -420)
    tint.set_editor_property("parameter_name", "AlbedoTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul_tint = MEL.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -160, -220)
    MEL.connect_material_expressions(albedo, "", mul_tint, "A")
    MEL.connect_material_expressions(tint, "", mul_tint, "B")

    rough_mul = MEL.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, -420, 380)
    rough_mul.set_editor_property("parameter_name", "RoughnessMul")
    rough_mul.set_editor_property("default_value", 1.0)
    mul_rough = MEL.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -160, 300)
    MEL.connect_material_expressions(rough, "", mul_rough, "A")
    MEL.connect_material_expressions(rough_mul, "", mul_rough, "B")

    MEL.connect_material_property(mul_tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(mul_rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    MEL.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log("[RealKit] master {} built".format(name))
    return mat


build_master("M_RealGround", facade_mode=False)
build_master("M_RealFacade", facade_mode=True)


# ============================================================
# 3) Poly Haven props (glTF via Interchange)
# ============================================================
for prop_dir in sorted(glob.glob(os.path.join(STAGING, "props", "*"))):
    slug = os.path.basename(prop_dir)
    gltfs = glob.glob(os.path.join(prop_dir, "*.gltf"))
    if not gltfs:
        unreal.log_warning("[RealKit] no gltf for {}".format(slug))
        continue
    dest = "{}/{}".format(PROP_ROOT, slug)
    if eal.does_directory_exist(dest) and eal.list_assets(dest, recursive=False):
        unreal.log("[RealKit] prop {} already imported".format(slug))
        continue
    paths = import_file(gltfs[0], dest)
    unreal.log("[RealKit] prop {}: {} objects".format(slug, len(paths)))

# report triangle counts so the apply script can budget
ar = unreal.AssetRegistryHelpers.get_asset_registry()
report = []
for ad in ar.get_assets_by_path(unreal.Name(PROP_ROOT), recursive=True):
    if str(ad.asset_class_path.asset_name) != "StaticMesh":
        continue
    sm = unreal.load_asset(str(ad.package_name))
    if sm:
        tris = sm.get_num_triangles(0)
        report.append("{} tris={}".format(ad.package_name, tris))
for line in sorted(report):
    unreal.log("[RealKit][tris] {}".format(line))

unreal.log("[RealKit] import phase DONE")
