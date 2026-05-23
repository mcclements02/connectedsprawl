"""Second pass on TestMap: fix multi-directional-light warning and dark scene.

- Delete all DirectionalLights except FixDirectionalLight (or, if none of ours,
  keep the brightest one and rename it).
- Crank the survivor's intensity, mark it as Atmosphere Sun Light, set Movable.
- Bump SkyLight intensity.
- Add an unbounded PostProcessVolume with manual exposure compensation bumped
  high enough to overcome `r.DefaultFeature.AutoExposure=False`.
"""

import unreal

LEVEL_PATH = "/Game/Maps/TestMap"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

unreal.log("[FixLighting] loading {}".format(LEVEL_PATH))
les.load_level(LEVEL_PATH)


def all_actors_of(cls):
    out = []
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == cls.__name__ or a.__class__ is cls:
            out.append(a)
    return out


# 1) Resolve DirectionalLight conflict
dls = []
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DirectionalLight":
        dls.append(a)
unreal.log("[FixLighting] found {} DirectionalLight actors".format(len(dls)))

survivor = None
for a in dls:
    if a.get_actor_label() == "FixDirectionalLight":
        survivor = a
        break
if survivor is None and dls:
    survivor = dls[0]
    survivor.set_actor_label("FixDirectionalLight")

for a in dls:
    if a is not survivor:
        unreal.log("[FixLighting] destroying duplicate DL '{}'".format(a.get_actor_label()))
        eas.destroy_actor(a)

if survivor is None:
    survivor = eas.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0, 0, 1000),
        unreal.Rotator(-50, 0, 30),
    )
    survivor.set_actor_label("FixDirectionalLight")

lc = survivor.light_component
lc.set_mobility(unreal.ComponentMobility.MOVABLE)
lc.set_intensity(10.0)
lc.set_editor_property("atmosphere_sun_light", True)
survivor.set_actor_rotation(unreal.Rotator(-50, 0, 30), False)
unreal.log("[FixLighting] survivor DL configured")


# 2) Boost SkyLight
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "SkyLight":
        a.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        a.light_component.set_editor_property("real_time_capture", True)
        a.light_component.set_intensity(2.0)
        unreal.log("[FixLighting] SkyLight '{}' boosted".format(a.get_actor_label()))


# 3) Add unbounded PostProcessVolume to force auto-exposure ON for this level
ppv = None
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FixPostProcess":
        ppv = a
        break
if ppv is None:
    ppv = eas.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(0, 0, 0),
        unreal.Rotator(0, 0, 0),
    )
    ppv.set_actor_label("FixPostProcess")
    unreal.log("[FixLighting] spawned FixPostProcess")
ppv.set_editor_property("unbound", True)
ppv.set_editor_property("priority", 100.0)

settings = ppv.settings
settings.set_editor_property("override_auto_exposure_method", True)
settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
settings.set_editor_property("override_auto_exposure_bias", True)
settings.set_editor_property("auto_exposure_bias", 4.0)
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", 0.03)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_max_brightness", 8.0)
ppv.settings = settings


unreal.log("[FixLighting] saving level")
saved = les.save_current_level()
unreal.log("[FixLighting] save_current_level returned: {}".format(saved))
unreal.log("[FixLighting] DONE")
