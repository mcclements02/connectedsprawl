"""Add volumetric clouds and a visible sun disk to TestMap."""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# --- Volumetric clouds ---
has_cloud = any(a.get_class().get_name() == "VolumetricCloud"
                for a in eas.get_all_level_actors())
if not has_cloud:
    cloud = eas.spawn_actor_from_class(unreal.VolumetricCloud,
                                       unreal.Vector(0, 0, 0))
    cloud.set_actor_label("City_Clouds")
    cc = cloud.get_component_by_class(unreal.VolumetricCloudComponent)
    if cc:
        cc.set_editor_property("layer_bottom_altitude", 5.0)   # km
        cc.set_editor_property("layer_height", 8.0)            # km
    unreal.log("[Sky] volumetric clouds added")
else:
    unreal.log("[Sky] clouds already present")

# --- Visible sun disk + sun-driven cloud lighting ---
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DirectionalLight":
        lc = a.light_component
        lc.set_editor_property("atmosphere_sun_light", True)
        # bigger angular size -> a clearly visible, photogenic sun disk
        lc.set_editor_property("light_source_angle", 2.0)
        try:
            lc.set_editor_property("cast_cloud_shadows", True)
            lc.set_editor_property("cloud_shadow_strength", 0.6)
        except Exception as e:
            unreal.log_warning("[Sky] cloud shadow props skipped: {}".format(e))
        unreal.log("[Sky] sun disk enabled on {}".format(a.get_actor_label()))
    if a.get_class().get_name() == "SkyLight":
        a.light_component.set_editor_property("real_time_capture", True)

saved = les.save_current_level()
unreal.log("[Sky] level saved: {}. DONE".format(saved))
