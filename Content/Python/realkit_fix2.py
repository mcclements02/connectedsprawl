"""RealKit fix pass 2, from screenshot review:
  - stronger skylight fill + higher sun + brighter exposure (shadowed
    facades were reading near-black without GI)
  - more facade color variety (red/tan brick mix) + denser window tiling
  - kill the daytime neon: every M_LampGlow slot becomes a neutral lamp
    material (the big storefront light-bars glowed white at noon)

Run (no actor spawning, but keep RHI for consistency):
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
atools = unreal.AssetToolsHelpers.get_asset_tools()

les.load_level("/Game/Maps/TestMap")

TEX_ROOT = "/Game/RealKit/Textures"
facade_master = unreal.load_asset("/Game/RealKit/Materials/M_RealFacade")


def find_asset(name, class_name):
    for ad in ar.get_assets_by_class(
            unreal.TopLevelAssetPath("/Script/Engine", class_name), True):
        if str(ad.asset_name) == name:
            return str(ad.package_name)
    return None


def texture_set(set_id):
    out = {}
    for ad in ar.get_assets_by_path(unreal.Name("{}/{}".format(TEX_ROOT, set_id)), recursive=True):
        n = str(ad.asset_name)
        for suffix, kind in (("_Color", "color"), ("_NormalGL", "normal"),
                             ("_Roughness", "rough"), ("_AmbientOcclusion", "ao")):
            if n.endswith(suffix):
                out[kind] = unreal.load_asset(str(ad.package_name))
    return out


def retune(mi_name, set_id, tile, tint=None):
    pkg = find_asset(mi_name, "MaterialInstanceConstant")
    if not pkg:
        unreal.log_warning("[Fix2] MIC missing: " + mi_name)
        return
    mi = unreal.load_asset(pkg)
    texs = texture_set(set_id)
    MEL.set_material_instance_parent(mi, facade_master)
    for pname, kind in (("AlbedoTex", "color"), ("NormalTex", "normal"),
                        ("RoughTex", "rough"), ("AOTex", "ao")):
        if kind in texs:
            MEL.set_material_instance_texture_parameter_value(mi, pname, texs[kind])
    MEL.set_material_instance_scalar_parameter_value(mi, "TileSize", float(tile))
    MEL.set_material_instance_vector_parameter_value(
        mi, "AlbedoTint",
        unreal.LinearColor(*(tint or (1.15, 1.15, 1.15, 1.0))))
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    unreal.log("[Fix2] retuned {} -> {} tile={}".format(mi_name, set_id, tile))


# facade variety: two brick tones, plaster, one classic and one glass facade
retune("MI_Facade_Warm",  "Facade001", 900,  (1.25, 1.2, 1.15, 1))
retune("MI_Facade_Slate", "Bricks101", 380,  (1.15, 1.15, 1.15, 1))
retune("MI_Facade_Teal",  "Facade017", 1000, (1.2, 1.2, 1.25, 1))
retune("MI_Facade_Tan",   "Bricks085", 380,  (1.2, 1.18, 1.1, 1))
retune("MI_Facade_Pale",  "Plaster001", 480, (1.25, 1.22, 1.15, 1))
retune("MI_Bldg_Brick",   "Bricks101", 380,  (1.15, 1.15, 1.15, 1))
retune("MI_Bldg_Tan",     "Plaster007", 480, (1.2, 1.18, 1.12, 1))

# ------------------------------------------------------------------
# daytime lamp material replaces every M_LampGlow slot
# ------------------------------------------------------------------
city_master = unreal.load_asset("/Game/CityArt/M_CityMaster")
lamp_day_path = "/Game/RealKit/Materials/MI_LampDay"
if not eal.does_asset_exist(lamp_day_path):
    lamp_day = atools.create_asset(
        "MI_LampDay", "/Game/RealKit/Materials",
        unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(lamp_day, city_master)
    MEL.set_material_instance_vector_parameter_value(
        lamp_day, "BaseColor", unreal.LinearColor(0.9, 0.9, 0.85, 1.0))
    MEL.set_material_instance_scalar_parameter_value(lamp_day, "Roughness", 0.35)
    MEL.update_material_instance(lamp_day)
    eal.save_loaded_asset(lamp_day)
else:
    lamp_day = unreal.load_asset(lamp_day_path)

swapped = 0
for a in eas.get_all_level_actors():
    smc = getattr(a, "static_mesh_component", None)
    if not smc:
        continue
    for i in range(smc.get_num_materials()):
        m = smc.get_material(i)
        if m and m.get_name() == "M_LampGlow":
            smc.set_material(i, lamp_day)
            swapped += 1
unreal.log("[Fix2] lamp-glow slots neutralized: {}".format(swapped))

# ------------------------------------------------------------------
# lighting: more fill, higher sun, brighter exposure
# ------------------------------------------------------------------
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if label == "FixDirectionalLight":
        a.set_actor_rotation(
            unreal.Rotator(roll=0.0, pitch=-50.0, yaw=-40.0), False)
        c = a.light_component
        c.set_editor_property("intensity", 12.0)
    elif label == "FixSkyLight":
        c = a.light_component
        c.set_editor_property("intensity", 3.0)
        c.set_editor_property("lower_hemisphere_is_black", False)
        try:
            c.recapture_sky()
        except Exception:
            pass
    elif label == "FixPostProcess":
        s = a.settings
        s.set_editor_property("override_auto_exposure_bias", True)
        s.set_editor_property("auto_exposure_bias", 1.1)
        s.set_editor_property("override_auto_exposure_min_brightness", True)
        s.set_editor_property("auto_exposure_min_brightness", 0.6)
        s.set_editor_property("override_auto_exposure_max_brightness", True)
        s.set_editor_property("auto_exposure_max_brightness", 2.0)
        a.settings = s
unreal.log("[Fix2] lighting boosted")

saved = les.save_current_level()
unreal.log("[Fix2] level saved: {} — DONE".format(saved))
