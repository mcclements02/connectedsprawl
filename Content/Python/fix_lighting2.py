"""Fix the washed-out look: cut the fog haze, tame exposure, warm it up,
and turn the PlayerStart to face down the street instead of into a wall.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# --- 1. Fog: drop to a whisper ---
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "ExponentialHeightFog":
        fc = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fc:
            fc.set_editor_property("fog_density", 0.0009)
            fc.set_editor_property("fog_height_falloff", 0.2)
            fc.set_editor_property("fog_inscattering_luminance",
                                   unreal.LinearColor(0.70, 0.71, 0.66, 1.0))
            fc.set_editor_property("start_distance", 2000.0)
        unreal.log("[Fix2] fog reduced to a whisper")

# --- 2. Post-process: tame exposure ---
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FixPostProcess":
        s = a.settings
        def setp(n, v):
            s.set_editor_property("override_" + n, True)
            s.set_editor_property(n, v)
        setp("auto_exposure_bias", -0.2)
        setp("auto_exposure_min_brightness", 0.5)
        setp("auto_exposure_max_brightness", 1.3)
        setp("bloom_intensity", 0.35)
        setp("white_temp", 6500.0)
        setp("color_saturation", unreal.Vector4(1.14, 1.14, 1.14, 1.0))
        setp("color_contrast", unreal.Vector4(1.08, 1.08, 1.09, 1.0))
        a.settings = s
        unreal.log("[Fix2] exposure tamed")

# --- 3. PlayerStart faces down the street ---
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "PlayerStart":
        loc = a.get_actor_location()
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), False)
        unreal.log("[Fix2] PlayerStart turned to face down-street")

saved = les.save_current_level()
unreal.log("[Fix2] level saved: {}. DONE".format(saved))
