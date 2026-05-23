"""Populate /Game/Maps/TestMap with lighting, sky, player start, and a floor.

Idempotent: actors are tagged with a 'Fix*' label and skipped if already present.
Run with:
  UnrealEditor <project.uproject> -ExecutePythonScript="<abs path to this file>" -unattended -nosplash
"""

import unreal

LEVEL_PATH = "/Game/Maps/TestMap"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

unreal.log("[FixTestMap] loading {}".format(LEVEL_PATH))
les.load_level(LEVEL_PATH)


def has_label(label):
    return any(a.get_actor_label() == label for a in eas.get_all_level_actors())


def spawn(cls, label, location, rotation=None):
    if rotation is None:
        rotation = unreal.Rotator(0, 0, 0)
    if has_label(label):
        unreal.log("[FixTestMap] {} exists, skipping".format(label))
        return None
    actor = eas.spawn_actor_from_class(cls, location, rotation)
    actor.set_actor_label(label)
    unreal.log("[FixTestMap] spawned {}".format(label))
    return actor


dl = spawn(
    unreal.DirectionalLight, "FixDirectionalLight",
    unreal.Vector(0, 0, 1000), unreal.Rotator(-50, 0, 30),
)
if dl:
    dl.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    dl.light_component.set_intensity(10.0)

sl = spawn(unreal.SkyLight, "FixSkyLight", unreal.Vector(0, 0, 300))
if sl:
    sl.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sl.light_component.set_editor_property("real_time_capture", True)
    sl.light_component.set_intensity(1.0)

spawn(unreal.SkyAtmosphere, "FixSkyAtmosphere", unreal.Vector(0, 0, 0))
spawn(unreal.ExponentialHeightFog, "FixHeightFog", unreal.Vector(0, 0, 0))
spawn(unreal.PlayerStart, "FixPlayerStart", unreal.Vector(0, 0, 200))

if not has_label("FixFloor"):
    floor = eas.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(0, 0, 0),
        unreal.Rotator(0, 0, 0),
    )
    floor.set_actor_label("FixFloor")
    floor.set_actor_scale3d(unreal.Vector(50, 50, 1))
    mesh = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")
    floor.static_mesh_component.set_static_mesh(mesh)
    floor.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    unreal.log("[FixTestMap] spawned FixFloor")
else:
    unreal.log("[FixTestMap] FixFloor exists, skipping")

unreal.log("[FixTestMap] saving level")
saved = les.save_current_level()
unreal.log("[FixTestMap] save_current_level returned: {}".format(saved))
unreal.log("[FixTestMap] DONE")
