"""Replace the kitbashed parked cars (City_Car_*) with real imported FBX
car meshes at the same positions / orientations.
"""
import unreal
import random

random.seed(13)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

mesh_paths = [
    "/Game/Vehicles/Imported/Generic/Car_{}".format(i) for i in range(1, 11)
] + ["/Game/Vehicles/Imported/Police/police_interceptor"]
meshes = [m for m in (unreal.load_asset(p) for p in mesh_paths)
          if m and isinstance(m, unreal.StaticMesh)]
unreal.log("[ParkedCars] {} real meshes available".format(len(meshes)))
if not meshes:
    raise RuntimeError("No imported car meshes found.")

# Snapshot positions from each "_body" piece (one per kitbash car).
snapshots = []
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if lbl.startswith("City_Car_") and lbl.endswith("_body"):
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        snapshots.append((loc, rot.yaw))

# Clear ALL kitbash pieces.
removed = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_Car_"):
        eas.destroy_actor(a)
        removed += 1
unreal.log("[ParkedCars] cleared {} kitbash pieces, will place {} real cars".format(
    removed, len(snapshots)))

TARGET_LEN = 470.0

def place(idx, loc, car_yaw):
    mesh = random.choice(meshes)
    bb = mesh.get_bounding_box()
    sz = bb.max - bb.min
    natural_len = max(sz.x, sz.y, 1.0)
    scale = max(0.45, min(1.4, TARGET_LEN / natural_len))
    # Drop the actor so the mesh bottom sits at z ≈ 8 (just above the road).
    z = 8.0 - bb.min.z * scale
    actor_loc = unreal.Vector(loc.x, loc.y, z)
    # FBX cars are Y-forward; add yaw=90 so the car's long axis aligns with our chosen yaw.
    rot = unreal.Rotator(0.0, car_yaw + 90.0, 0.0)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, actor_loc, rot)
    a.set_actor_label("City_Car_{}".format(idx))
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    smc = a.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    return a

placed = 0
for i, (loc, yaw) in enumerate(snapshots):
    place(i, loc, yaw)
    placed += 1
unreal.log("[ParkedCars] placed {} real parked cars".format(placed))

saved = les.save_current_level()
unreal.log("[ParkedCars] level saved: {}. DONE".format(saved))
