"""RealKit fix 4 — make both masters compile on Metal (nullrhi-safe, no spawns).

Diagnosis (ledger item "RealKit sampler warnings"): M_RealFacade2 and
M_RealGround fail to compile for SF_METAL_SM5 — their AOTex parameter
(SAMPLERTYPE_LINEAR_COLOR) was left on the engine's sRGB DefaultTexture
because the Concrete034 default set ships no _AmbientOcclusion map. A
LinearColor sampler bound to an sRGB texture is a hard error on Metal, so
the whole material falls back to WorldGridMaterial in game.

Fix: rebuild both masters fresh (editing nodes in place is not scriptable)
with complete default texture sets — Bricks085 for the facade, Ground037
for the ground, both of which ship all four maps — then refresh every MIC
parented to them. Parameter names are unchanged, so MIC overrides survive.
"""
import unreal

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
atools = unreal.AssetToolsHelpers.get_asset_tools()

TEX_ROOT = "/Game/RealKit/Textures"
MAT_ROOT = "/Game/RealKit/Materials"


def texture_set(set_id):
    out = {}
    for ad in ar.get_assets_by_path(unreal.Name("{}/{}".format(TEX_ROOT, set_id)), recursive=True):
        n = str(ad.asset_name)
        for suffix, kind in (("_Color", "color"), ("_NormalGL", "normal"),
                             ("_Roughness", "rough"), ("_AmbientOcclusion", "ao")):
            if n.endswith(suffix):
                out[kind] = unreal.load_asset(str(ad.package_name))
    return out


def build_master(name, facade_mode, defaults):
    path = "{}/{}".format(MAT_ROOT, name)
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = atools.create_asset(name, MAT_ROOT, unreal.Material,
                              unreal.MaterialFactoryNew())

    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1500, 0)
    tile = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1500, 300)
    tile.set_editor_property("parameter_name", "TileSize")
    tile.set_editor_property("default_value", 400.0)

    def mask(x, y, r, g, b):
        m = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x, y)
        m.set_editor_property("r", r)
        m.set_editor_property("g", g)
        m.set_editor_property("b", b)
        m.set_editor_property("a", False)
        return m

    if facade_mode:
        # facade projection: U = x+y, V = z; roofs blend to ground projection
        mx = mask(-1300, -120, True, False, False)
        my = mask(-1300, -40, False, True, False)
        mz = mask(-1300, 40, False, False, True)
        for m in (mx, my, mz):
            MEL.connect_material_expressions(wp, "", m, "")
        add_xy = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -1150, -80)
        MEL.connect_material_expressions(mx, "", add_xy, "A")
        MEL.connect_material_expressions(my, "", add_xy, "B")
        append_f = MEL.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -1020, -20)
        MEL.connect_material_expressions(add_xy, "", append_f, "A")
        MEL.connect_material_expressions(mz, "", append_f, "B")

        m_rg = mask(-1300, 140, True, True, False)
        MEL.connect_material_expressions(wp, "", m_rg, "")

        vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1300, 260)
        vn_z = mask(-1170, 260, False, False, True)
        MEL.connect_material_expressions(vn, "", vn_z, "")
        vabs = MEL.create_material_expression(mat, unreal.MaterialExpressionAbs, -1060, 260)
        MEL.connect_material_expressions(vn_z, "", vabs, "")
        vsub = MEL.create_material_expression(mat, unreal.MaterialExpressionSubtract, -960, 260)
        vsub.set_editor_property("const_b", 0.5)
        MEL.connect_material_expressions(vabs, "", vsub, "A")
        vmul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -860, 260)
        vmul.set_editor_property("const_b", 4.0)
        MEL.connect_material_expressions(vsub, "", vmul, "A")
        vsat = MEL.create_material_expression(mat, unreal.MaterialExpressionSaturate, -760, 260)
        MEL.connect_material_expressions(vmul, "", vsat, "")

        uv_lerp = MEL.create_material_expression(
            mat, unreal.MaterialExpressionLinearInterpolate, -880, 20)
        MEL.connect_material_expressions(append_f, "", uv_lerp, "A")
        MEL.connect_material_expressions(m_rg, "", uv_lerp, "B")
        MEL.connect_material_expressions(vsat, "", uv_lerp, "Alpha")
        uv_src = uv_lerp
    else:
        mask_xy = mask(-1050, 0, True, True, False)
        MEL.connect_material_expressions(wp, "", mask_xy, "")
        uv_src = mask_xy

    div = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -650, 60)
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

    missing = [k for k in ("color", "normal", "rough", "ao") if k not in defaults]
    if missing:
        unreal.log_error("[Fix4] default set incomplete ({}) — aborting {}".format(missing, name))
        return None
    for node, key in ((albedo, "color"), (normal, "normal"), (rough, "rough"), (ao, "ao")):
        node.set_editor_property("texture", defaults[key])

    tint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -420, -420)
    tint.set_editor_property("parameter_name", "AlbedoTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul_tint = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -160, -220)
    MEL.connect_material_expressions(albedo, "", mul_tint, "A")
    MEL.connect_material_expressions(tint, "", mul_tint, "B")

    # soft normals: lerp flat (0,0,1) toward map by NormalStrength
    flat_n = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 100)
    flat_n.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
    n_str = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -300, 180)
    n_str.set_editor_property("parameter_name", "NormalStrength")
    n_str.set_editor_property("default_value", 0.4 if facade_mode else 1.0)
    n_lerp = MEL.create_material_expression(
        mat, unreal.MaterialExpressionLinearInterpolate, -140, 60)
    MEL.connect_material_expressions(flat_n, "", n_lerp, "A")
    MEL.connect_material_expressions(normal, "", n_lerp, "B")
    MEL.connect_material_expressions(n_str, "", n_lerp, "Alpha")

    rough_mul = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -420, 380)
    rough_mul.set_editor_property("parameter_name", "RoughnessMul")
    rough_mul.set_editor_property("default_value", 1.0)
    mul_rough = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -160, 300)
    MEL.connect_material_expressions(rough, "", mul_rough, "A")
    MEL.connect_material_expressions(rough_mul, "", mul_rough, "B")

    MEL.connect_material_property(mul_tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(n_lerp, "", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(mul_rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    MEL.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log("[Fix4] rebuilt {} with complete defaults".format(name))
    return mat


# record dependent MICs BEFORE deleting masters (delete orphans their parent ref)
dependents = {"M_RealFacade2": [], "M_RealGround": []}
for ad in ar.get_assets_by_class(
        unreal.TopLevelAssetPath("/Script/Engine", "MaterialInstanceConstant"), True):
    mi = unreal.load_asset(str(ad.package_name))
    p = mi.get_editor_property("parent") if mi else None
    if p and p.get_name() in dependents:
        dependents[p.get_name()].append(str(ad.package_name))

facade = build_master("M_RealFacade2", True, texture_set("Bricks085"))
ground = build_master("M_RealGround", False, texture_set("Ground037"))

refreshed = 0
for master, paths in (("M_RealFacade2", dependents["M_RealFacade2"]),
                      ("M_RealGround", dependents["M_RealGround"])):
    new_parent = facade if master == "M_RealFacade2" else ground
    if not new_parent:
        continue
    for pkg in paths:
        mi = unreal.load_asset(pkg)
        if not mi:
            continue
        MEL.set_material_instance_parent(mi, new_parent)
        MEL.update_material_instance(mi)
        eal.save_loaded_asset(mi)
        refreshed += 1

unreal.log_warning("[Fix4] done — masters rebuilt, {} MICs refreshed".format(refreshed))
