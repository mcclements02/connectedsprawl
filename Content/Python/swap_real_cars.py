"""Replace the kitbashed cube cars with the imported FBX vehicle meshes.

Cycles imported cars and paint per City_DriveCar_ actor, computes a scale that
fits the Hull's ~470u length, and applies yaw=90 to correct Blender's
Y-forward export against UE's X-forward convention.
"""
import unreal
import os
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
unreal.log("[SwapCars] {} imported meshes available".format(len(meshes)))

if not meshes:
    raise RuntimeError("No imported car meshes found under /Game/Vehicles/Imported")

count = 0
cars = [a for a in eas.get_all_level_actors()
        if a.get_actor_label().startswith("City_DriveCar_")]
cars.sort(key=lambda a: a.get_actor_label())
for count, a in enumerate(cars):
    vehicle_realism.configure_drivable_car(a, count, meshes, paint_mats)
count = len(cars)

unreal.log("[SwapCars] swapped {} cars to real imported meshes with varied paint".format(count))
saved = les.save_current_level()
unreal.log("[SwapCars] level saved: {}. DONE".format(saved))
