"""Kill the ground z-fighting and fix the sun angle.

- Delete the leftover original-TestMap 'Floor' cube that sits coplanar
  with City_Ground (causes whole-ground z-fighting speckle).
- Fix FixDirectionalLight rotation: earlier scripts passed Rotator args in
  the wrong order, leaving pitch=0 (sun on the horizon). Set a proper
  daytime elevation.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# 1) Delete stray coplanar Floor (and any other non-City_/non-Fix StaticMeshActor)
removed = []
for a in list(eas.get_all_level_actors()):
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    lbl = a.get_actor_label()
    if not lbl.startswith("City_"):
        eas.destroy_actor(a)
        removed.append(lbl)
unreal.log("[ZFix] removed stray StaticMeshActors: {}".format(removed))

# 2) Fix the sun: proper daytime elevation (pitch -48), shadows from the side
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "DirectionalLight":
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-48.0, yaw=-35.0), False)
        lc = a.light_component
        lc.set_intensity(12.0)
        lc.set_editor_property("shadow_bias", 0.7)
        unreal.log("[ZFix] sun '{}' set to pitch=-48".format(a.get_actor_label()))

saved = les.save_current_level()
unreal.log("[ZFix] level saved: {}".format(saved))
unreal.log("[ZFix] DONE")
