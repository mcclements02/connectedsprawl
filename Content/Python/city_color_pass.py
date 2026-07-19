"""Canonical final look pass for TestMap (idempotent, real RHI only).

Run this last, after city/RealKit authoring passes and a fresh editor build:
  UnrealEditor ConnectedSprawl.uproject /Game/Maps/TestMap \
    -ExecutePythonScript=.../city_color_pass.py -DisablePlugins=Fab -unattended

Owns balanced daylight, shadow lift, neutral color grading, the deterministic
facade/ground palette, environment-controller defaults, and streetlight tags.
"""
import unreal


MAP_PATH = "/Game/Maps/TestMap"
FACADE_MASTER_PATH = "/Game/RealKit/Materials/M_RealFacade2"

SUN_COLOR = unreal.LinearColor(1.0, 0.86, 0.70, 1.0)

PALETTE = {
    "/Game/CityArt/MI_Bldg_Brick": (0.45, 0.26, 0.20),
    "/Game/CityArt/MI_Bldg_Tan": (0.58, 0.47, 0.36),
    "/Game/CityArt/MI_Facade_Tan": (0.58, 0.47, 0.36),
    "/Game/CityArt/MI_Bldg_Slate": (0.48, 0.46, 0.43),
    "/Game/CityArt/MI_Bldg_Conc": (0.48, 0.46, 0.43),
    "/Game/CityArt/MI_Facade_Slate": (0.48, 0.46, 0.43),
    "/Game/CityArt/MI_Facade_Teal": (0.48, 0.46, 0.43),
    "/Game/CityArt/MI_Facade_Pale": (0.64, 0.58, 0.48),
    "/Game/CityArt/MI_Facade_Warm": (0.64, 0.58, 0.48),
    "/Game/CityArt/MI_Asphalt": (0.16, 0.16, 0.17),
    "/Game/CityArt/Materials/MI_WetAsphalt_Road": (0.16, 0.16, 0.17),
    "/Game/CityArt/MI_Sidewalk": (0.42, 0.42, 0.40),
    "/Game/CityArt/Materials/MI_WetConcrete_Sidewalk": (0.42, 0.42, 0.40),
}

ORPHAN_FACADES = {
    "/Game/CityArt/MI_Facade_Tan",
    "/Game/CityArt/MI_Facade_Pale",
    "/Game/CityArt/MI_Facade_Slate",
    "/Game/CityArt/MI_Facade_Teal",
    "/Game/CityArt/MI_Facade_Warm",
}

CORE_GRADE = {
    "auto_exposure_method": unreal.AutoExposureMethod.AEM_HISTOGRAM,
    "auto_exposure_bias": 0.70,
    "auto_exposure_min_brightness": 1.0,
    "auto_exposure_max_brightness": 1.0,
    "white_temp": 6200.0,
    "ambient_occlusion_intensity": 0.50,
    "ambient_occlusion_radius": 150.0,
    "ambient_occlusion_quality": 40.0,
    "vignette_intensity": 0.15,
    "color_saturation": unreal.Vector4(1.02, 1.02, 1.02, 1.0),
    "color_contrast": unreal.Vector4(1.03, 1.03, 1.03, 1.0),
    "bloom_intensity": 0.14,
    "bloom_threshold": 1.0,
    "lens_flare_intensity": 0.12,
    "lens_flare_threshold": 8.0,
    "blue_correction": 0.60,
    "motion_blur_amount": 0.0,
    "motion_blur_max": 0.0,
    "film_grain_intensity": 0.0,
}

CHANGE_REASONS = []

NEUTRAL_GRADE = {
    "color_gamma": unreal.Vector4(1.0, 1.0, 1.0, 1.0),
    "color_gain": unreal.Vector4(1.0, 1.0, 1.0, 1.0),
    "color_offset": unreal.Vector4(0.0, 0.0, 0.0, 0.0),
    "color_saturation_shadows": unreal.Vector4(1.0, 1.0, 1.0, 1.0),
    "color_offset_shadows": unreal.Vector4(0.0, 0.0, 0.0, 0.0),
    "color_saturation_highlights": unreal.Vector4(1.0, 1.0, 1.0, 1.0),
    "color_offset_highlights": unreal.Vector4(0.0, 0.0, 0.0, 0.0),
    "white_tint": 0.0,
    "scene_color_tint": unreal.LinearColor(1.06, 1.0, 0.94, 1.0),
    "scene_fringe_intensity": 0.0,
}


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

if not les.load_level(MAP_PATH):
    raise RuntimeError("[CityColor] could not load {}".format(MAP_PATH))


def one_actor(label):
    found = [a for a in eas.get_all_level_actors()
             if a.get_actor_label() == label]
    if len(found) != 1:
        raise RuntimeError("[CityColor] expected one {}, found {}".format(
            label, len(found)))
    return found[0]


def values_match(current, target, tolerance=1.0e-4):
    if isinstance(target, float):
        return abs(float(current) - target) <= tolerance
    fields = None
    if isinstance(target, unreal.LinearColor):
        fields = ("r", "g", "b", "a")
    elif isinstance(target, unreal.Vector4):
        fields = ("x", "y", "z", "w")
    elif isinstance(target, unreal.Rotator):
        fields = ("pitch", "yaw", "roll")
    if fields:
        return all(abs(float(getattr(current, field))
                       - float(getattr(target, field))) <= tolerance
                   for field in fields)
    return current == target


def set_property_if_changed(target, name, value):
    current = target.get_editor_property(name)
    if values_match(current, value):
        return False
    CHANGE_REASONS.append("{}.{}".format(type(target).__name__, name))
    target.set_editor_property(name, value)
    return True


def set_override_if_changed(settings, name, value):
    changed = set_property_if_changed(settings, "override_" + name, True)
    return set_property_if_changed(settings, name, value) or changed


# Full preflight before changing any authored object.
sun = one_actor("FixDirectionalLight")
sky = one_actor("FixSkyLight")
fog = one_actor("FixHeightFog")
post = one_actor("FixPostProcess")

sun_comp = sun.light_component
sky_comp = sky.light_component
fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
if not sun_comp or not sky_comp or not fog_comp:
    raise RuntimeError("[CityColor] canonical light/fog component missing")

settings = post.settings
for name in list(CORE_GRADE) + list(NEUTRAL_GRADE):
    try:
        settings.get_editor_property("override_" + name)
        settings.get_editor_property(name)
    except Exception as exc:
        raise RuntimeError(
            "[CityColor] required post-process field {} unavailable ({})".format(
                name, exc))

controller_class = getattr(unreal, "SprawlEnvironmentController", None)
if controller_class is None:
    raise RuntimeError("[CityColor] rebuild ConnectedSprawlEditor first")
controller_cdo = unreal.get_default_object(controller_class)
for property_name in (
        "start_hour", "sun_yaw", "max_sun_elevation", "day_sun_intensity",
        "day_sun_color", "day_sky_intensity", "day_fog_density"):
    try:
        controller_cdo.get_editor_property(property_name)
    except Exception as exc:
        raise RuntimeError(
            "[CityColor] rebuild ConnectedSprawlEditor first; missing {} ({})".format(
                property_name, exc))

facade_master = eal.load_asset(FACADE_MASTER_PATH)
if not facade_master:
    raise RuntimeError("[CityColor] missing {}".format(FACADE_MASTER_PATH))
materials = {}
for path in PALETTE:
    material = eal.load_asset(path)
    if not material:
        raise RuntimeError("[CityColor] missing palette material {}".format(path))
    materials[path] = material

for path, material in materials.items():
    parameter_source = facade_master if path in ORPHAN_FACADES else material
    vector_names = {str(name) for name in mel.get_vector_parameter_names(
        parameter_source)}
    scalar_names = {str(name) for name in mel.get_scalar_parameter_names(
        parameter_source)}
    if "AlbedoTint" not in vector_names or "RoughnessMul" not in scalar_names:
        raise RuntimeError(
            "[CityColor] {} lacks AlbedoTint/RoughnessMul".format(path))

lamp_actors = [
    actor for actor in eas.get_all_level_actors()
    if actor.get_actor_label().startswith("City_Detail_StreetLight_")
]
if not lamp_actors:
    raise RuntimeError("[CityColor] no authored streetlights found")

controllers = [a for a in eas.get_all_level_actors()
               if a.get_class().get_name() == "SprawlEnvironmentController"]

map_changed = False


# Balanced daylight and lifted shadows.
target_rotation = unreal.Rotator(pitch=-32.0, yaw=-38.0, roll=0.0)
if not values_match(sun.get_actor_rotation(), target_rotation):
    CHANGE_REASONS.append("DirectionalLight.rotation")
    sun.set_actor_rotation(target_rotation, False)
    map_changed = True
map_changed = set_property_if_changed(
    sun_comp, "mobility", unreal.ComponentMobility.MOVABLE) or map_changed
map_changed = set_property_if_changed(sun_comp, "intensity", 5.2) or map_changed
# LightComponent stores this through FColor, so allow one 8-bit channel step
# when comparing the linear-color getter against the authored value.
if not values_match(sun_comp.get_light_color(), SUN_COLOR, tolerance=0.005):
    CHANGE_REASONS.append("DirectionalLight.light_color")
    sun_comp.set_light_color(SUN_COLOR)
    map_changed = True
for name, value in {
        "use_temperature": False,
        "atmosphere_sun_light": True,
        "cast_shadows": True}.items():
    map_changed = set_property_if_changed(sun_comp, name, value) or map_changed

for name, value in {
        "mobility": unreal.ComponentMobility.MOVABLE,
        "source_type": unreal.SkyLightSourceType.SLS_CAPTURED_SCENE,
        "real_time_capture": False,
        "intensity": 1.85}.items():
    map_changed = set_property_if_changed(sky_comp, name, value) or map_changed

for name, value in {
        "fog_density": 0.006,
        "fog_height_falloff": 0.16,
        "start_distance": 1200.0}.items():
    map_changed = set_property_if_changed(fog_comp, name, value) or map_changed

map_changed = set_property_if_changed(post, "unbound", True) or map_changed
map_changed = set_property_if_changed(post, "priority", 200.0) or map_changed
post_changed = False
for name, value in {**CORE_GRADE, **NEUTRAL_GRADE}.items():
    post_changed = set_override_if_changed(settings, name, value) or post_changed
if post_changed:
    post.settings = settings
    map_changed = True


# Deterministic, physically readable material palette.
changed_materials = []
for path, rgb in PALETTE.items():
    material = materials[path]
    material_changed = False
    if path in ORPHAN_FACADES and material.get_editor_property("parent") != facade_master:
        mel.set_material_instance_parent(material, facade_master)
        material_changed = True
    target_tint = unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)
    current_tint = mel.get_material_instance_vector_parameter_value(
        material, "AlbedoTint")
    if not values_match(current_tint, target_tint):
        mel.set_material_instance_vector_parameter_value(
            material, "AlbedoTint", target_tint)
        material_changed = True
    current_roughness = mel.get_material_instance_scalar_parameter_value(
        material, "RoughnessMul")
    if not values_match(current_roughness, 0.90):
        mel.set_material_instance_scalar_parameter_value(
            material, "RoughnessMul", 0.90)
        material_changed = True
    if not values_match(mel.get_material_instance_vector_parameter_value(
            material, "AlbedoTint"), target_tint):
        raise RuntimeError("[CityColor] AlbedoTint readback failed for {}".format(path))
    if not values_match(mel.get_material_instance_scalar_parameter_value(
            material, "RoughnessMul"), 0.90):
        raise RuntimeError("[CityColor] RoughnessMul readback failed for {}".format(path))
    if material_changed:
        changed_materials.append((path, material))


# Exactly one runtime controller, synchronized with the authored starting look.
if controllers:
    controller = controllers[0]
    for duplicate in controllers[1:]:
        CHANGE_REASONS.append("SprawlEnvironmentController.duplicate")
        eas.destroy_actor(duplicate)
        map_changed = True
else:
    CHANGE_REASONS.append("SprawlEnvironmentController.missing")
    controller = eas.spawn_actor_from_class(
        controller_class, unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(pitch=0.0, yaw=0.0, roll=0.0))
    map_changed = True
if not controller:
    raise RuntimeError("[CityColor] could not create SprawlEnvironmentController")
if controller.get_actor_label() != "Sprawl_Environment":
    CHANGE_REASONS.append("SprawlEnvironmentController.label")
    controller.set_actor_label("Sprawl_Environment")
    map_changed = True
for name, value in {
        "start_hour": 8.071530,
        "sun_yaw": -38.0,
        "max_sun_elevation": 62.0,
        "day_sun_intensity": 5.2,
        "day_sun_color": SUN_COLOR,
        "day_sky_intensity": 1.85,
        "day_fog_density": 0.006}.items():
    map_changed = set_property_if_changed(controller, name, value) or map_changed


# Preserve any existing actor tags while enabling the runtime pooled lamps.
tagged_lamps = 0
for actor in lamp_actors:
    tags = list(actor.get_editor_property("tags"))
    if not any(str(tag) == "Streetlight" for tag in tags):
        CHANGE_REASONS.append("Streetlight.tag")
        tags.append(unreal.Name("Streetlight"))
        actor.set_editor_property("tags", tags)
        map_changed = True
    tagged_lamps += 1

for path, material in changed_materials:
    if not eal.save_loaded_asset(material):
        raise RuntimeError("[CityColor] failed to save {}".format(path))
if map_changed:
    sky_comp.recapture_sky()
    if not les.save_current_level():
        raise RuntimeError("[CityColor] TestMap save failed")

unreal.log_warning(
    "[CityColor] DONE sun=5.2 sky=1.85 fog=0.006 exposure=0.70 "
    "materials={} changed_materials={} streetlights={} controllers=1 "
    "map_changed={} change_reasons={}".format(
        len(PALETTE), len(changed_materials), tagged_lamps, map_changed,
        ",".join(CHANGE_REASONS) if CHANGE_REASONS else "none"))
