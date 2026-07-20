"""Game-ready polish pass: see through car glass, dress Zarri properly.

1. Car glass: lighten /Game/CityArt/MI_CarGlass and retarget every glass-ish
   material slot on the imported vehicle kits to it, so seated drivers are
   visible instead of silhouettes behind near-opaque tint.
2. Zarri outfit: recolor the assembled MetaHuman garment instances from the
   authored near-black indigo to an intentional light-tee / dark-shorts
   streetwear read that works in daylight and at night.

Safe under -nullrhi (asset edits only, no actor spawning).

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject \
    -ExecutePythonScript="<abs path>/game_ready_polish.py" \
    -unattended -nosplash -nullrhi -stdout -DisablePlugins=Fab
"""
import unreal

MEL = unreal.MaterialEditingLibrary
ASSETS = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
REGISTRY = unreal.AssetRegistryHelpers.get_asset_registry()

GLASS_SLOT_TERMS = ("glass", "window", "windshield", "windscreen", "wind")
GLASS_TINT = unreal.LinearColor(0.52, 0.60, 0.68, 1.0)
GLASS_OPACITY = 0.30

SHIRT_MI = ("/Game/MetaHumans/Zarri/Clothing/"
            "MI_WI_DefaultGarment_None_5_M_DG_bodyShapeA_Shirt")
SHORT_MI = ("/Game/MetaHumans/Zarri/Clothing/"
            "MI_WI_DefaultGarment_None_5_M_DG_bodyShapeA_Short")
# Light heather tee over deep charcoal shorts.
SHIRT_PRIMARY = unreal.LinearColor(0.401, 0.412, 0.431, 1.0)
SHIRT_SECONDARY = unreal.LinearColor(0.242, 0.250, 0.266, 1.0)
SHORT_PRIMARY = unreal.LinearColor(0.026, 0.028, 0.033, 1.0)
SHORT_SECONDARY = unreal.LinearColor(0.085, 0.090, 0.102, 1.0)

changed_assets = []
_report_lines = []


def log(msg):
    # UE 5.8's pythonscript commandlet swallows LogPython output, so tee every
    # line into a sidecar report next to the project's Saved directory.
    unreal.log("[GameReadyPolish] " + msg)
    _report_lines.append(msg)


def flush_report():
    import os
    report_path = os.path.join(
        unreal.Paths.project_saved_dir(), "GameReadyPolishReport.txt")
    with open(report_path, "w") as handle:
        handle.write("\n".join(_report_lines) + "\n")


def base_material(mat):
    seen = 0
    while isinstance(mat, unreal.MaterialInstance) and seen < 8:
        mat = mat.get_editor_property("parent")
        seen += 1
    return mat if isinstance(mat, unreal.Material) else None


def param_names(mat_instance):
    base = base_material(mat_instance)
    if not base:
        return [], []
    scalars = [str(n) for n in MEL.get_scalar_parameter_names(base)]
    vectors = [str(n) for n in MEL.get_vector_parameter_names(base)]
    return scalars, vectors


def set_matching_vectors(mi, needles, color):
    _, vectors = param_names(mi)
    hits = []
    for name in vectors:
        lower = name.lower()
        if any(needle in lower for needle in needles):
            MEL.set_material_instance_vector_parameter_value(mi, name, color)
            hits.append(name)
    return hits


def set_matching_scalars(mi, needles, value):
    scalars, _ = param_names(mi)
    hits = []
    for name in scalars:
        lower = name.lower()
        if any(needle in lower for needle in needles):
            MEL.set_material_instance_scalar_parameter_value(mi, name, value)
            hits.append(name)
    return hits


def save(path):
    if ASSETS.save_asset(path, only_if_is_dirty=True):
        changed_assets.append(path)
        return True
    log("SAVE FAILED: " + path)
    return False


# ---------------------------------------------------------------- car glass
def ensure_translucent_glass_master():
    """The CityArt glass instance descends from an opaque master, so windows
    render as solid panels no matter how light the tint. Author a minimal
    translucent master and reparent MI_CarGlass onto it: every consumer (the
    kitbash cabin and all retargeted kit slots) becomes see-through at once."""
    master_path = "/Game/CityArt/M_CarGlassClear"
    existing = unreal.load_asset(master_path) \
        if unreal.EditorAssetLibrary.does_asset_exist(master_path) else None
    if isinstance(existing, unreal.Material):
        return existing

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    master = tools.create_asset(
        "M_CarGlassClear", "/Game/CityArt",
        unreal.Material, unreal.MaterialFactoryNew())
    if not master:
        log("FAILED to create M_CarGlassClear")
        return None
    master.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    master.set_editor_property("two_sided", True)

    color = MEL.create_material_expression(
        master, unreal.MaterialExpressionVectorParameter, -420, -80)
    color.set_editor_property("parameter_name", "BaseColor")
    color.set_editor_property(
        "default_value", unreal.LinearColor(0.52, 0.60, 0.68, 1.0))

    opacity = MEL.create_material_expression(
        master, unreal.MaterialExpressionScalarParameter, -420, 120)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", GLASS_OPACITY)

    roughness = MEL.create_material_expression(
        master, unreal.MaterialExpressionScalarParameter, -420, 240)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.12)

    metallic = MEL.create_material_expression(
        master, unreal.MaterialExpressionScalarParameter, -420, 360)
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", 0.0)

    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    MEL.recompile_material(master)
    save(master_path)
    log("Authored translucent master M_CarGlassClear")
    return master


def fix_car_glass_master():
    path = "/Game/CityArt/MI_CarGlass"
    mi = unreal.load_asset(path)
    if not isinstance(mi, unreal.MaterialInstanceConstant):
        log("MI_CarGlass missing or not an instance: " + str(type(mi)))
        return None
    master = ensure_translucent_glass_master()
    if master:
        current_parent = mi.get_editor_property("parent")
        if not current_parent or current_parent.get_path_name() \
                != master.get_path_name():
            MEL.set_material_instance_parent(mi, master)
            log("Reparented MI_CarGlass onto M_CarGlassClear")
    scalars, vectors = param_names(mi)
    log("MI_CarGlass scalar params: " + ", ".join(scalars))
    log("MI_CarGlass vector params: " + ", ".join(vectors))
    tinted = set_matching_vectors(
        mi, ("color", "tint", "base"), GLASS_TINT)
    cleared = set_matching_scalars(mi, ("opacity",), GLASS_OPACITY)
    softened = set_matching_scalars(mi, ("roughness",), 0.12)
    dimmed = set_matching_scalars(mi, ("emissive", "metal"), 0.0)
    log("MI_CarGlass set vectors={} opacity={} roughness={} dimmed={}".format(
        tinted, cleared, softened, dimmed))
    save(path)
    return mi


def vehicle_mesh_paths():
    paths = []
    for folder in ("/Game/Vehicles", "/Game/RealKit"):
        if not unreal.EditorAssetLibrary.does_directory_exist(folder):
            continue
        filt = unreal.ARFilter(
            package_paths=[folder], recursive_paths=True,
            class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")])
        for data in REGISTRY.get_assets(filt) or []:
            name = str(data.asset_name).lower()
            object_path = "{}.{}".format(data.package_name, data.asset_name)
            if folder == "/Game/Vehicles" or any(
                    term in name for term in ("car", "van", "truck", "vehic")):
                paths.append(object_path)
    return sorted(set(paths))


def retarget_glass_slots(glass_mi):
    replaced_total = 0
    meshes_changed = 0
    for path in vehicle_mesh_paths():
        mesh = unreal.load_asset(path)
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        materials = list(mesh.get_editor_property("static_materials"))
        replaced_here = []
        for index, entry in enumerate(materials):
            slot = str(entry.get_editor_property("material_slot_name")).lower()
            current = entry.get_editor_property("material_interface")
            if not any(term in slot for term in GLASS_SLOT_TERMS):
                continue
            if current and current.get_path_name().startswith(
                    "/Game/CityArt/MI_CarGlass"):
                continue
            entry.set_editor_property("material_interface", glass_mi)
            materials[index] = entry
            replaced_here.append(slot)
        if replaced_here:
            mesh.set_editor_property("static_materials", materials)
            if save(path):
                meshes_changed += 1
                replaced_total += len(replaced_here)
                log("Glass slots -> MI_CarGlass on {}: {}".format(
                    path.split(".")[-1], replaced_here))
    log("Vehicle glass retarget: {} slots across {} meshes".format(
        replaced_total, meshes_changed))
    return replaced_total


# ------------------------------------------------------------- Zarri outfit
def recolor_outfit():
    for path, primary, secondary in (
            (SHIRT_MI, SHIRT_PRIMARY, SHIRT_SECONDARY),
            (SHORT_MI, SHORT_PRIMARY, SHORT_SECONDARY)):
        mi = unreal.load_asset(path)
        if not isinstance(mi, unreal.MaterialInstanceConstant):
            log("Outfit MI missing: " + path)
            continue
        scalars, vectors = param_names(mi)
        log("{} vector params: {}".format(path.split("/")[-1], ", ".join(vectors)))
        # Garment masters expose diffuse_color_1/2 tiers plus print colors.
        # Body tone goes on tier 1, trim/lining on tier 2, prints get muted
        # into the body tone so the outfit reads as clean solid streetwear.
        primary_hits = set_matching_vectors(
            mi, ("c_color", "diffuse_color_1"), primary)
        secondary_hits = set_matching_vectors(
            mi, ("diffuse_color_2",), secondary)
        print_hits = set_matching_vectors(mi, ("print",), primary)
        log("{} recolored primary={} secondary={} prints={}".format(
            path.split("/")[-1], primary_hits, secondary_hits, len(print_hits)))
        save(path)


def main():
    glass = fix_car_glass_master()
    if glass:
        retarget_glass_slots(glass)
    recolor_outfit()
    log("PASS changed {} assets:".format(len(changed_assets)))
    for path in changed_assets:
        log("  " + path)


try:
    main()
finally:
    flush_report()
