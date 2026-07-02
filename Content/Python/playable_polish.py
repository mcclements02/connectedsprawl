"""Playability polish: make sure the loop 'spawn -> walk -> enter car -> drive'
works from the default PlayerStart.

  - Finds the nearest drivable City_DriveCar_* to the player start; if it is
    beyond walking distance, parks one on the road right next to spawn.
  - Parked player car must not wander off: bAutoDrive = False.
  - Logs (does not change) anything else load-bearing for the loop.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended -nullrhi
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# player start
starts = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
if not starts:
    raise RuntimeError("no PlayerStart in level")
spawn_loc = starts[0].get_actor_location()
unreal.log("[Playable] player start at {}".format(spawn_loc))

cars = [a for a in eas.get_all_level_actors()
        if a.get_actor_label().startswith("City_DriveCar_")]
unreal.log("[Playable] {} drivable cars in level".format(len(cars)))


def dist2(a, b):
    return (a.x - b.x) ** 2 + (a.y - b.y) ** 2


cars.sort(key=lambda c: dist2(c.get_actor_location(), spawn_loc))
if cars:
    nearest = cars[0]
    d = dist2(nearest.get_actor_location(), spawn_loc) ** 0.5
    unreal.log("[Playable] nearest car {} at {:.0f}cm".format(
        nearest.get_actor_label(), d))
    if d > 1200.0:
        # park it on the road just south of spawn, facing down the street
        nearest.set_actor_location(
            unreal.Vector(spawn_loc.x, spawn_loc.y + 450.0, 60.0), False, False)
        nearest.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
        unreal.log("[Playable] moved {} next to spawn".format(
            nearest.get_actor_label()))
    try:
        nearest.set_editor_property("b_auto_drive", False)
        unreal.log("[Playable] player car auto-drive off")
    except Exception as e:
        unreal.log_warning("[Playable] could not clear auto-drive: {}".format(e))
else:
    unreal.log_warning("[Playable] NO drivable cars found!")

# sanity log: interact radius source-of-truth is C++ (550uu)
# make sure nothing occupies the spawn point itself
hits = [a for a in eas.get_all_level_actors()
        if a.get_actor_label().startswith("City_")
        and dist2(a.get_actor_location(), spawn_loc) < 90000
        and a.get_class().get_name() == "StaticMeshActor"]
for a in hits:
    unreal.log("[Playable] near-spawn actor: {} at {}".format(
        a.get_actor_label(), a.get_actor_location()))

# what is City_Clouds? (log only)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "City_Clouds":
        smc = getattr(a, "static_mesh_component", None)
        if smc:
            unreal.log("[Playable] City_Clouds mesh={} scale={} loc={}".format(
                smc.static_mesh.get_name() if smc.static_mesh else None,
                a.get_actor_scale3d(), a.get_actor_location()))

saved = les.save_current_level()
unreal.log("[Playable] level saved: {} — DONE".format(saved))
