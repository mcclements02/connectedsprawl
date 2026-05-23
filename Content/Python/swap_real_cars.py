"""Replace the kitbashed cube cars with the imported FBX vehicle meshes.

Picks a random imported car per City_DriveCar_ actor, computes a scale that
fits the Hull's ~470u length, and applies yaw=90 to correct Blender's
Y-forward export against UE's X-forward convention.
"""
import unreal
import random

random.seed(7)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

# Pool: 10 generics + the police interceptor. Skip the subway (way too big).
mesh_paths = [
    "/Game/Vehicles/Imported/Generic/Car_{}".format(i) for i in range(1, 11)
] + ["/Game/Vehicles/Imported/Police/police_interceptor"]

meshes = []
for p in mesh_paths:
    m = unreal.load_asset(p)
    if m and isinstance(m, unreal.StaticMesh):
        meshes.append(m)
unreal.log("[SwapCars] {} imported meshes available".format(len(meshes)))

if not meshes:
    raise RuntimeError("No imported car meshes found under /Game/Vehicles/Imported")

TARGET_LEN = 480.0   # match the ASprawlCar Hull's footprint
HULL_BOTTOM = -62.0  # local-space Z at the bottom of the Hull box

def assign(actor, mesh):
    bb = mesh.get_bounding_box()
    sz = bb.max - bb.min
    natural_len = max(sz.x, sz.y)
    scale = max(0.45, min(1.4, TARGET_LEN / max(natural_len, 1.0)))
    rel_z = HULL_BOTTOM - bb.min.z * scale
    actor.set_external_vehicle_mesh(
        mesh,
        unreal.Vector(0.0, 0.0, rel_z),
        unreal.Rotator(0.0, 90.0, 0.0),   # Blender Y-fwd → UE X-fwd
        unreal.Vector(scale, scale, scale),
    )

count = 0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("City_DriveCar_"):
        m = random.choice(meshes)
        assign(a, m)
        count += 1

unreal.log("[SwapCars] swapped {} cars to real imported meshes".format(count))
saved = les.save_current_level()
unreal.log("[SwapCars] level saved: {}. DONE".format(saved))
