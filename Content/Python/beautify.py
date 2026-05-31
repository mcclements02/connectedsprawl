"""Cinematic polish pass: tuned post-process, warm sun, dramatic clouds,
subtle atmospheric fog. Plays well with Lumen + TAA enabled in
Config/DefaultEngine.ini.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")


def try_set(struct, name, value):
    """Set an override + value pair; ignore properties that don't exist on this UE build."""
    try:
        struct.set_editor_property("override_" + name, True)
        struct.set_editor_property(name, value)
    except Exception:
        pass


def try_setprop(obj, name, value):
    try:
        obj.set_editor_property(name, value)
    except Exception:
        pass


# ============================================================
# Post-process volume — cinematic look
# ============================================================
ppv = None
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FixPostProcess":
        ppv = a
        break
if ppv is None:
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_actor_label("FixPostProcess")
ppv.set_editor_property("unbound", True)
ppv.set_editor_property("priority", 200.0)

s = ppv.settings

# Exposure — tight histogram window for filmic look
try_set(s, "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
try_set(s, "auto_exposure_bias", 0.9)
try_set(s, "auto_exposure_min_brightness", 0.4)
try_set(s, "auto_exposure_max_brightness", 2.0)
try_set(s, "auto_exposure_speed_up", 8.0)
try_set(s, "auto_exposure_speed_down", 5.0)
try_set(s, "histogram_log_min", -8.0)
try_set(s, "histogram_log_max", 4.0)

# Bloom + lens flares — sunlight catches metal/glass
try_set(s, "bloom_method", unreal.BloomMethod.BM_SOG)
try_set(s, "bloom_intensity", 0.50)
try_set(s, "bloom_threshold", 1.0)
try_set(s, "lens_flare_intensity", 0.7)
try_set(s, "lens_flare_bokeh_size", 3.0)
try_set(s, "lens_flare_threshold", 6.0)

# Ambient occlusion — grounds objects in shadows
try_set(s, "ambient_occlusion_intensity", 0.90)
try_set(s, "ambient_occlusion_radius", 110.0)
try_set(s, "ambient_occlusion_quality", 100.0)

# Tone mapping
try_set(s, "tone_curve_amount", 1.0)
try_set(s, "expand_gamut", 1.0)
try_set(s, "blue_correction", 0.65)
try_set(s, "scene_color_tint", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

# Colour grading — warm shadows, cooler highlights (GTA-ish)
try_set(s, "white_temp", 6300.0)
try_set(s, "white_tint", 0.0)
try_set(s, "color_saturation", unreal.Vector4(1.14, 1.14, 1.14, 1.0))
try_set(s, "color_contrast",   unreal.Vector4(1.10, 1.10, 1.10, 1.0))
try_set(s, "color_gamma",      unreal.Vector4(1.0, 1.0, 1.0, 1.0))
try_set(s, "color_gain",       unreal.Vector4(1.0, 1.0, 1.0, 1.0))
try_set(s, "color_offset",     unreal.Vector4(0.0, 0.0, 0.0, 0.0))

# Shadow/highlight separation
try_set(s, "color_saturation_shadows",    unreal.Vector4(1.05, 1.05, 1.10, 1.0))
try_set(s, "color_offset_shadows",        unreal.Vector4(0.010, 0.005, -0.008, 0.0))
try_set(s, "color_saturation_highlights", unreal.Vector4(1.05, 1.05, 1.00, 1.0))
try_set(s, "color_offset_highlights",     unreal.Vector4(-0.008, 0.0, 0.012, 0.0))

# Final-image polish
try_set(s, "vignette_intensity", 0.45)
try_set(s, "film_grain_intensity", 0.08)
try_set(s, "chromatic_aberration_start_offset", 0.4)
try_set(s, "scene_fringe_intensity", 0.4)
try_set(s, "screen_percentage", 100.0)

# Motion blur (subtle, cinematic)
try_set(s, "motion_blur_amount", 0.35)
try_set(s, "motion_blur_max", 5.0)

# Lumen quality knobs (the method itself is set project-wide in DefaultEngine.ini).
try_set(s, "lumen_scene_lighting_quality", 1.5)
try_set(s, "lumen_scene_detail", 1.3)
try_set(s, "lumen_reflection_quality", 1.3)

ppv.settings = s
unreal.log("[Beautify] post-process tuned")


# ============================================================
# Directional Light — warm golden sun with visible disk
# ============================================================
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DirectionalLight":
        lc = a.light_component
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-38.0, yaw=-50.0), False)
        lc.set_light_color(unreal.LinearColor(1.0, 0.93, 0.78, 1.0))
        lc.set_intensity(8.0)
        try_setprop(lc, "use_temperature", True)
        try_setprop(lc, "temperature", 5800.0)
        try_setprop(lc, "light_source_angle", 3.0)
        try_setprop(lc, "atmosphere_sun_light", True)
        try_setprop(lc, "shadow_bias", 0.5)
        try_setprop(lc, "shadow_slope_bias", 0.5)
        try_setprop(lc, "cast_cloud_shadows", True)
        try_setprop(lc, "cloud_shadow_strength", 0.55)
        try_setprop(lc, "dynamic_shadow_distance_movable_light", 25000.0)
        try_setprop(lc, "dynamic_shadow_cascades", 4)
        try_setprop(lc, "cascade_distribution_exponent", 3.0)
        unreal.log("[Beautify] sun tuned (warm, pitch -38, disk angle 3°)")


# ============================================================
# Sky Light — real-time capture, slight cool tint
# ============================================================
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "SkyLight":
        lc = a.light_component
        try_setprop(lc, "real_time_capture", True)
        try_setprop(lc, "intensity", 1.8)
        try_setprop(lc, "light_color", unreal.LinearColor(0.92, 0.96, 1.0, 1.0))


# ============================================================
# Volumetric Cloud — dramatic, thick, slow-drifting
# ============================================================
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "VolumetricCloud":
        cc = a.get_component_by_class(unreal.VolumetricCloudComponent)
        if cc:
            try_setprop(cc, "layer_bottom_altitude", 4.0)
            try_setprop(cc, "layer_height", 9.0)
            try_setprop(cc, "tracing_start_max_distance", 350.0)
            try_setprop(cc, "tracing_max_distance", 80.0)
            try_setprop(cc, "planet_radius", 6378.0)
            try_setprop(cc, "ground_albedo", unreal.LinearColor(0.4, 0.4, 0.4, 1.0))
            try_setprop(cc, "sky_light_cloud_bottom_occlusion", 0.5)
        unreal.log("[Beautify] clouds tuned")


# ============================================================
# Exponential Height Fog — subtle atmospheric depth
# ============================================================
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "ExponentialHeightFog":
        fc = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fc:
            try_setprop(fc, "fog_density", 0.0007)
            try_setprop(fc, "fog_height_falloff", 0.18)
            try_setprop(fc, "fog_inscattering_luminance", unreal.LinearColor(0.55, 0.62, 0.74, 1.0))
            try_setprop(fc, "start_distance", 1500.0)
            try_setprop(fc, "fog_max_opacity", 0.85)
            try_setprop(fc, "volumetric_fog", True)
            try_setprop(fc, "volumetric_fog_distance", 12000.0)
            try_setprop(fc, "volumetric_fog_scattering_distribution", 0.4)
        unreal.log("[Beautify] fog tuned (subtle depth + volumetric)")


saved = les.save_current_level()
unreal.log("[Beautify] level saved: {}. DONE".format(saved))
