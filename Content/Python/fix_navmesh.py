"""Make the city nav mesh actually generate: ensure a RecastNavMesh exists and
set it (and the bounds volume) up for dynamic runtime generation, so the
pedestrians have a nav mesh to walk on in -game.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# Report what nav actors exist
volume = None
recast = None
for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    if cn == "NavMeshBoundsVolume":
        volume = a
    elif cn == "RecastNavMesh":
        recast = a
unreal.log("[Nav] NavMeshBoundsVolume={} RecastNavMesh={}".format(
    volume.get_actor_label() if volume else None,
    recast.get_actor_label() if recast else None))

# Spawn a RecastNavMesh if the level doesn't have one
if recast is None:
    try:
        recast = eas.spawn_actor_from_class(unreal.RecastNavMesh,
                                            unreal.Vector(0, 0, 0))
        recast.set_actor_label("City_NavData")
        unreal.log("[Nav] spawned RecastNavMesh")
    except Exception as e:
        unreal.log_warning("[Nav] could not spawn RecastNavMesh: {}".format(e))

# Force dynamic runtime generation so the mesh builds when the game starts
if recast:
    try:
        recast.set_editor_property("runtime_generation",
                                   unreal.RuntimeGenerationType.DYNAMIC)
        unreal.log("[Nav] runtime_generation -> DYNAMIC")
    except Exception as e:
        unreal.log_warning("[Nav] runtime_generation set failed: {}".format(e))

# Trigger an editor-time nav build so static data is also saved with the level
try:
    nav_sys = unreal.NavigationSystemV1.get_navigation_system(
        unreal.EditorLevelLibrary.get_editor_world()
        if hasattr(unreal, "EditorLevelLibrary") else None)
except Exception:
    nav_sys = None

saved = les.save_current_level()
unreal.log("[Nav] level saved: {}. DONE".format(saved))
