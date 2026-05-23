"""Delete the leftover nav-mesh actors so the screen no longer shows
'NAVMESH NEEDS TO BE REBUILT'. The pedestrians do their own wandering
in C++ and don't need a nav mesh."""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

killed = []
for a in list(eas.get_all_level_actors()):
    cn = a.get_class().get_name()
    if cn in ("NavMeshBoundsVolume", "RecastNavMesh", "RecastNavMesh-Default", "NavigationData"):
        killed.append(a.get_actor_label() or cn)
        eas.destroy_actor(a)
    elif a.get_actor_label().startswith("City_Nav"):
        killed.append(a.get_actor_label())
        eas.destroy_actor(a)

unreal.log("[StripNav] removed: {}".format(killed))
les.save_current_level()
unreal.log("[StripNav] DONE")
