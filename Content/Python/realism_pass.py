"""Realism pass: lighting + post-processing overhaul, textured ground/road
surfaces from the Building kit, and real streetlight meshes.

Run:
  UnrealEditor <project.uproject> -ExecutePythonScript="<abs path>" -unattended -nosplash
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# ============================================================
# 1) LIGHTING
# ============================================================
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    if cn == "DirectionalLight":
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-42.0, yaw=-52.0), False)
        lc = a.light_component
        lc.set_intensity(9.0)
        lc.set_light_color(unreal.LinearColor(1.0, 0.945, 0.84, 1.0))
        lc.set_editor_property("shadow_bias", 0.6)
        unreal.log("[Realism] sun tuned (warm, pitch -42)")
    if cn == "SkyLight":
        lc = a.light_component
        lc.set_intensity(1.4)
        lc.set_editor_property("real_time_capture", True)

# Exponential height fog for atmospheric depth
has_fog = any(x.get_class().get_name() == "ExponentialHeightFog"
              for x in eas.get_all_level_actors())
if not has_fog:
    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog,
                                     unreal.Vector(0, 0, 200))
    fog.set_actor_label("City_Fog")
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fc:
        fc.set_editor_property("fog_density", 0.012)
        fc.set_editor_property("fog_height_falloff", 0.08)
        fc.set_editor_property("fog_inscattering_luminance",
                               unreal.LinearColor(0.45, 0.55, 0.72, 1.0))
    unreal.log("[Realism] height fog added")

# Post-process volume
ppv = None
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FixPostProcess":
        ppv = a
if ppv is None:
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_actor_label("FixPostProcess")
ppv.set_editor_property("unbound", True)
ppv.set_editor_property("priority", 100.0)

s = ppv.settings
def setp(name, value):
    s.set_editor_property("override_" + name, True)
    s.set_editor_property(name, value)

# Exposure — auto, framed so it's bright but not blown
setp("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
setp("auto_exposure_bias", 1.0)
setp("auto_exposure_min_brightness", 0.35)
setp("auto_exposure_max_brightness", 2.2)
setp("auto_exposure_speed_up", 6.0)
setp("auto_exposure_speed_down", 6.0)
# Bloom
setp("bloom_intensity", 0.45)
# Ambient occlusion — grounds objects
setp("ambient_occlusion_intensity", 0.7)
setp("ambient_occlusion_radius", 90.0)
# Colour grading — gentle filmic warmth + contrast
setp("white_temp", 6200.0)
setp("color_contrast", unreal.Vector4(1.06, 1.06, 1.07, 1.0))
setp("color_saturation", unreal.Vector4(1.10, 1.10, 1.10, 1.0))
setp("color_gamma", unreal.Vector4(1.0, 1.0, 1.0, 1.0))
# Vignette + a hint of grain
setp("vignette_intensity", 0.38)
setp("film_grain_intensity", 0.10)
ppv.settings = s
unreal.log("[Realism] post-process volume configured")

# ============================================================
# 2) TEXTURED SURFACES from the Building kit
# ============================================================
MI_ROAD = unreal.load_asset("/Game/Materials/MI_Road")
MI_CONCRETE = unreal.load_asset("/Game/Materials/MI_StdOpaque_Concrete")
MI_GREEN = unreal.load_asset("/Game/Materials/MI_Green")

reskinned = 0
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.static_mesh_component
    if lbl == "City_Ground" and MI_ROAD:
        smc.set_material(0, MI_ROAD); reskinned += 1
    elif lbl.startswith("City_Surf_") and MI_CONCRETE:
        # park surfaces (grass) keep their green; sidewalks -> concrete
        cur = smc.get_material(0)
        if cur and "Grass" in cur.get_name() and MI_GREEN:
            smc.set_material(0, MI_GREEN)
        else:
            smc.set_material(0, MI_CONCRETE)
        reskinned += 1
unreal.log("[Realism] reskinned {} surface actors".format(reskinned))

# ============================================================
# 3) REAL STREETLIGHT MESHES
# ============================================================
STREETLIGHT = unreal.load_object(None, "/Game/Geometry/SM_StreetLight.SM_StreetLight")
M_LAMP = unreal.load_asset("/Game/CityArt/M_LampGlow")
MI_DARK = unreal.load_asset("/Game/CityArt/MI_Asphalt")

# collect existing primitive streetlight locations, then replace
pole_locs = []
for a in list(eas.get_all_level_actors()):
    lbl = a.get_actor_label()
    if lbl.startswith("City_Detail_Pole_"):
        pole_locs.append(a.get_actor_location())
        eas.destroy_actor(a)
    elif lbl.startswith("City_Detail_Lamp_"):
        eas.destroy_actor(a)

made = 0
if STREETLIGHT:
    for i, loc in enumerate(pole_locs):
        base = unreal.Vector(loc.x, loc.y, 14.0)
        sl = eas.spawn_actor_from_class(unreal.StaticMeshActor, base,
                                        unreal.Rotator(0, 0, 0))
        sl.set_actor_label("City_Detail_StreetLight_{}".format(i))
        smc = sl.static_mesh_component
        smc.set_static_mesh(STREETLIGHT)
        smc.set_mobility(unreal.ComponentMobility.STATIC)
        if MI_DARK:
            smc.set_material(0, MI_DARK)
        if M_LAMP:
            smc.set_material(1, M_LAMP)
        # emissive lamp blob near the top of the 422-tall pole
        lamp = eas.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(loc.x, loc.y, 14 + 410),
            unreal.Rotator(0, 0, 0))
        lamp.set_actor_label("City_Detail_LampBulb_{}".format(i))
        lc = lamp.static_mesh_component
        lc.set_static_mesh(unreal.load_object(None, "/Engine/BasicShapes/Sphere.Sphere"))
        lc.set_world_scale3d(unreal.Vector(0.45, 0.45, 0.45))
        lc.set_mobility(unreal.ComponentMobility.STATIC)
        if M_LAMP:
            lc.set_material(0, M_LAMP)
        made += 1
unreal.log("[Realism] placed {} real streetlights".format(made))

saved = les.save_current_level()
unreal.log("[Realism] level saved: {}. DONE".format(saved))
