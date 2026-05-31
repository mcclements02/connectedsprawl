"""Wire up the live world: a nav-mesh volume over the city, walking
pedestrians (ASprawlPedestrian), and drivable cars (ASprawlCar).
"""
import unreal
import random
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)

import vehicle_realism

random.seed(44)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

les.load_level("/Game/Maps/TestMap")

# grid params (match build_city.py)
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2
def cx(i):
    return START + i * STEP
LX0, LX1 = cx(4) - STEP / 2, cx(6) + STEP / 2
LY0, LY1 = cx(0) - STEP / 2, cx(1) + STEP / 2
def over_lake(x, y):
    return LX0 - 200 < x < LX1 + 200 and LY0 - 200 < y < LY1 + 200


def clear(prefix):
    n = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith(prefix):
            eas.destroy_actor(a)
            n += 1
    return n

# ============================================================
# 1) Nav-mesh bounds volume over the whole city
# ============================================================
clear("City_Nav")
nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume,
                                 unreal.Vector(0, 0, 350))
nav.set_actor_label("City_NavVolume")
# default volume brush is 200u; scale to blanket the city + headroom
nav.set_actor_scale3d(unreal.Vector(SPAN / 200.0 + 4, SPAN / 200.0 + 4, 9.0))
unreal.log("[World] nav volume placed")

# ============================================================
# 2) Walking pedestrians
# ============================================================
clear("City_NPC_")      # remove the old idle mannequins
clear("City_Ped_")
peds = 0
for gx in range(N):
    for gy in range(N):
        bx, by = cx(gx), cx(gy)
        if over_lake(bx, by):
            continue
        for _ in range(random.choice([0, 1, 1, 2])):
            px = bx + random.uniform(-BLOCK / 2 + 150, BLOCK / 2 - 150)
            py = by + random.uniform(-BLOCK / 2 + 150, BLOCK / 2 - 150)
            a = eas.spawn_actor_from_class(
                unreal.SprawlPedestrian,
                unreal.Vector(px, py, 120),
                unreal.Rotator(0, 0, random.uniform(0, 360)))
            a.set_actor_label("City_Ped_{}".format(peds))
            peds += 1
unreal.log("[World] spawned {} walking pedestrians".format(peds))

# ============================================================
# 3) Drivable cars
# ============================================================
clear("City_DriveCar_")
car_mats = vehicle_realism.load_vehicle_paint_materials()
real_car_meshes = vehicle_realism.load_vehicle_meshes()
unreal.log("[World] real vehicle meshes available: {}".format(len(real_car_meshes)))

# spots: a couple right by the player spawn, the rest scattered on roads
spots = [
    (cx(2) / 2.0 + cx(3) / 2.0 + 180, 700, 90),   # next to PlayerStart
    (cx(2) / 2.0 + cx(3) / 2.0 - 180, -500, 270),
]
vroads = [(cx(i) + cx(i + 1)) / 2.0 for i in range(N - 1)]
hroads = [(cx(j) + cx(j + 1)) / 2.0 for j in range(N - 1)]
for _ in range(8):
    if random.random() < 0.5:
        x = random.choice(vroads) + random.uniform(-150, 150)
        y = cx(random.randint(1, N - 1)) + random.uniform(-300, 300)
        yaw = random.choice([90, 270])
    else:
        x = cx(random.randint(1, N - 1)) + random.uniform(-300, 300)
        y = random.choice(hroads) + random.uniform(-150, 150)
        yaw = random.choice([0, 180])
    if not over_lake(x, y):
        spots.append((x, y, yaw))

cars = 0
real_cars = 0
for (x, y, yaw) in spots:
    car = eas.spawn_actor_from_class(unreal.SprawlCar,
                                     unreal.Vector(x, y, 170),
                                     unreal.Rotator(0, 0, yaw))
    car.set_actor_label("City_DriveCar_{}".format(cars))
    if vehicle_realism.configure_drivable_car(car, cars, real_car_meshes, car_mats):
        real_cars += 1
    cars += 1
unreal.log("[World] spawned {} drivable cars".format(cars))
unreal.log("[World] applied real meshes and varied paint to {} drivable cars".format(real_cars))

saved = les.save_current_level()
unreal.log("[World] level saved: {}. DONE".format(saved))
