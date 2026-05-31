"""Replace the kitbashed parked cars (City_Car_*) with real imported FBX
car meshes at the same positions / orientations.
"""
import unreal
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)

import vehicle_realism
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

meshes = vehicle_realism.load_vehicle_meshes()
paint_mats = vehicle_realism.load_vehicle_paint_materials()
unreal.log("[ParkedCars] {} real meshes available".format(len(meshes)))
if not meshes:
    raise RuntimeError("No imported car meshes found.")

# Snapshot positions from each old "_body" piece, or from already-replaced
# City_Car_N static mesh actors so this script stays idempotent.
snapshots = []
seen = set()
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    real_match = re.match(r"^City_Car_(\d+)$", lbl)
    if real_match and real_match.group(1) not in seen:
        key = real_match.group(1)
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        snapshots.append((key, loc, rot.yaw - 90.0))
        seen.add(key)
        continue

    kitbash_match = re.match(r"^City_Car_(\d+)_body$", lbl)
    if kitbash_match and kitbash_match.group(1) not in seen:
        key = kitbash_match.group(1)
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        snapshots.append((key, loc, rot.yaw))
        seen.add(key)

# Clear ALL kitbash pieces.
removed = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_Car_"):
        eas.destroy_actor(a)
        removed += 1
unreal.log("[ParkedCars] cleared {} kitbash pieces, will place {} real cars".format(
    removed, len(snapshots)))

placed = 0
snapshots.sort(key=lambda s: int(s[0]))
for i, (_, loc, yaw) in enumerate(snapshots):
    vehicle_realism.spawn_real_parked_car(eas, i, loc, yaw, meshes, paint_mats)
    placed += 1
unreal.log("[ParkedCars] placed {} real parked cars with varied paint".format(placed))

saved = les.save_current_level()
unreal.log("[ParkedCars] level saved: {}. DONE".format(saved))
