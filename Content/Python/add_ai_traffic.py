"""Spawn self-driving AI cars that wander the road grid.

Each car gets bAutoDrive=true and a real imported mesh + paint. Cars are
placed on road centerlines aligned with the road's axis, well clear of
the player spawn so they don't immediately collide with the player.
"""
import unreal
import random
import math
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)
import vehicle_realism

random.seed(77)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

# Grid params
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2
def cx(i):
    return START + i * STEP
vroads = [(cx(i) + cx(i + 1)) / 2.0 for i in range(N - 1)]
hroads = [(cx(j) + cx(j + 1)) / 2.0 for j in range(N - 1)]
LX0, LX1 = cx(4) - STEP / 2, cx(6) + STEP / 2
LY0, LY1 = cx(0) - STEP / 2, cx(1) + STEP / 2
def over_lake(x, y):
    return LX0 - 250 < x < LX1 + 250 and LY0 - 250 < y < LY1 + 250

# Find PlayerStart to keep AI traffic clear of the spawn.
SPAWN = (0.0, 0.0)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "PlayerStart":
        L = a.get_actor_location()
        SPAWN = (L.x, L.y)
        break
unreal.log("[Traffic] PlayerStart at {}".format(SPAWN))

# Clear old AI traffic.
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_TrafficCar_"):
        eas.destroy_actor(a)

meshes = vehicle_realism.load_vehicle_meshes()
paints = vehicle_realism.load_vehicle_paint_materials()
if not meshes:
    raise RuntimeError("No imported car meshes — run import_vehicles first")

# Reserve every existing drivable car before choosing traffic positions. The
# two generators are often run independently, so checking only this script's
# cars can spawn overlapping physics hulls and launch one of them upside down.
placed = []
for actor in eas.get_all_level_actors():
    if actor.get_actor_label().startswith("City_DriveCar_"):
        location = actor.get_actor_location()
        placed.append((location.x, location.y))
unreal.log("[Traffic] reserved {} existing drivable-car positions".format(len(placed)))
MIN_GAP = 900.0
SPAWN_CLEAR = 1400.0
TARGET = 12

def far_enough(x, y):
    for (px, py) in placed:
        if math.hypot(x - px, y - py) < MIN_GAP:
            return False
    return True

attempts = 0
spawned = 0
while spawned < TARGET and attempts < 600:
    attempts += 1
    if random.random() < 0.5:
        # On a north/south road
        rx = random.choice(vroads)
        heading_north = random.choice([False, True])
        x = rx - 150.0 if heading_north else rx + 150.0
        y = cx(random.randint(0, N - 1)) + random.uniform(-450, 450)
        yaw = 90.0 if heading_north else 270.0
    else:
        ry = random.choice(hroads)
        heading_east = random.choice([False, True])
        y = ry + 150.0 if heading_east else ry - 150.0
        x = cx(random.randint(0, N - 1)) + random.uniform(-450, 450)
        yaw = 0.0 if heading_east else 180.0
    if over_lake(x, y):
        continue
    if math.hypot(x - SPAWN[0], y - SPAWN[1]) < SPAWN_CLEAR:
        continue
    if not far_enough(x, y):
        continue

    car = eas.spawn_actor_from_class(
        unreal.SprawlCar,
        unreal.Vector(x, y, 180.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    car.set_actor_label("City_TrafficCar_{}".format(spawned))
    car.set_editor_property("auto_drive", True)
    if not vehicle_realism.configure_animated_car(car, spawned + 100, paints):
        vehicle_realism.configure_drivable_car(car, spawned + 100, meshes, paints)

    placed.append((x, y))
    spawned += 1

# Always drop one nearby AI car ~1100 units down the street from spawn so
# the player can immediately see it driving off.
nearest_v = min(vroads, key=lambda value: abs(value - SPAWN[0]))
nearest_h = min(hroads, key=lambda value: abs(value - SPAWN[1]))
nearby_spots = [
    (nearest_v - 150.0, SPAWN[1] + 1100.0, 90.0),
    (nearest_v + 150.0, SPAWN[1] - 1100.0, 270.0),
    (SPAWN[0] + 1100.0, nearest_h + 150.0, 0.0),
    (SPAWN[0] - 1100.0, nearest_h - 150.0, 180.0),
]
for nearby_x, nearby_y, nearby_yaw in nearby_spots:
    if over_lake(nearby_x, nearby_y) or not far_enough(nearby_x, nearby_y):
        continue
    car = eas.spawn_actor_from_class(
        unreal.SprawlCar,
        unreal.Vector(nearby_x, nearby_y, 180.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=nearby_yaw))
    car.set_actor_label("City_TrafficCar_{}".format(spawned))
    car.set_editor_property("auto_drive", True)
    if not vehicle_realism.configure_animated_car(car, spawned + 100, paints):
        vehicle_realism.configure_drivable_car(car, spawned + 100, meshes, paints)
    placed.append((nearby_x, nearby_y))
    spawned += 1
    break
else:
    unreal.log_warning("[Traffic] no collision-free nearby traffic position was available")

unreal.log("[Traffic] spawned {} AI traffic cars".format(spawned))
saved = les.save_current_level()
unreal.log("[Traffic] level saved: {}. DONE".format(saved))
