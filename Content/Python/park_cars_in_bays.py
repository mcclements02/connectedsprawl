"""Put every parked car in a marked parallel-parking bay, clear of obstacles.

Parked cars were scattered at arbitrary points inside the blocks — on the
pavement, in planters, with trees growing through them — because the old
`ParkingOffset` (410cm) sat outside the old 300cm road half-width, i.e. past
the kerb. With the widened street each side now carries a real 250cm bay
inboard of the kerb, so cars can be parked on the street where they belong.

Each car is assigned a bay slot and checked against every tree, prop, street
light and building footprint, plus the cars already placed, so nothing ends up
intersecting anything. Cars that cannot be given a clear bay are reported
rather than dumped somewhere invalid.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/park_cars_in_bays.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"

# Must match USprawlCityGridSubsystem.
NUM_BLOCKS = 7
BLOCK = 2000.0
LANE_WIDTH = 350.0
LANES_PER_DIRECTION = 1
PARKING_BAY_WIDTH = 250.0
ROAD = 2.0 * (LANES_PER_DIRECTION * LANE_WIDTH + PARKING_BAY_WIDTH)  # 1200
STEP = BLOCK + ROAD
SPAN = NUM_BLOCKS * STEP
NUM_ROADS = NUM_BLOCKS - 1
PARKING_OFFSET = LANES_PER_DIRECTION * LANE_WIDTH + PARKING_BAY_WIDTH / 2.0  # 475
BAY_LENGTH = 700.0
INTERSECTION_HALF = ROAD / 2.0

CAR_HALF_LENGTH = 250.0
CAR_HALF_WIDTH = 110.0
PARK_Z = 120.0          # dropped onto the road by physics at BeginPlay
CLEARANCE = 40.0        # breathing room around each parked car

LAKE_GX, LAKE_GY = (4, 5, 6), (0, 1)

# Anything solid enough that a car must not share space with it.
OBSTACLE_PREFIXES = (
    "City_DW_tree", "City_DW3_tree", "City_RealProp_", "City_Detail_StreetLight",
    "City_DW_store", "City_DW3_store", "City_DW_chair", "City_DW_tableumb",
    "City_DW_foodcart", "City_R2_recycle", "City_R2_pots", "City_DW3_playground",
    "City_Detail_Curb", "City_BuildingDetail_",
)

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "ParkingReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def block_center(index):
    return -SPAN / 2.0 + STEP / 2.0 + index * STEP


def road_center(index):
    return (block_center(index) + block_center(index + 1)) / 2.0


def over_lake(x, y, margin=200.0):
    lx0 = block_center(4) - STEP / 2.0
    lx1 = block_center(6) + STEP / 2.0
    ly0 = block_center(0) - STEP / 2.0
    ly1 = block_center(1) + STEP / 2.0
    return (lx0 - margin < x < lx1 + margin) and (ly0 - margin < y < ly1 + margin)


def near_intersection(along):
    for index in range(NUM_ROADS):
        if abs(along - road_center(index)) < INTERSECTION_HALF + 260.0:
            return True
    return False


def boxes_overlap(a, b):
    return not (a[2] <= b[0] or b[2] <= a[0] or a[3] <= b[1] or b[3] <= a[1])


def build_slots():
    """Every legal bay: (x, y, yaw, vertical_road)."""
    slots = []
    low = road_center(0) - STEP / 2.0
    high = road_center(NUM_ROADS - 1) + STEP / 2.0
    for index in range(NUM_ROADS):
        center = road_center(index)
        for side in (-1.0, 1.0):
            lateral = center + side * PARKING_OFFSET
            along = low
            while along < high:
                if not near_intersection(along):
                    # Vertical road: bay runs along Y. Right-hand traffic puts
                    # the -X side northbound (yaw 90) and +X southbound (270).
                    if not over_lake(lateral, along):
                        slots.append((lateral, along,
                                      90.0 if side < 0 else 270.0, True))
                    # Horizontal road: +Y side is eastbound (yaw 0).
                    if not over_lake(along, lateral):
                        slots.append((along, lateral,
                                      0.0 if side > 0 else 180.0, False))
                along += BAY_LENGTH
    return slots


def car_box(x, y, vertical):
    half_x = CAR_HALF_WIDTH if vertical else CAR_HALF_LENGTH
    half_y = CAR_HALF_LENGTH if vertical else CAR_HALF_WIDTH
    half_x += CLEARANCE
    half_y += CLEARANCE
    return (x - half_x, y - half_y, x + half_x, y + half_y)


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    obstacles = []
    parked_cars = []
    for actor in actors:
        if not actor:
            continue
        label = actor.get_actor_label()
        if label.startswith("City_ParkedCar_"):
            parked_cars.append(actor)
            continue
        if not label.startswith(OBSTACLE_PREFIXES):
            continue
        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        if extent.x <= 0.0 and extent.y <= 0.0:
            continue
        obstacles.append((origin.x - extent.x, origin.y - extent.y,
                          origin.x + extent.x, origin.y + extent.y))

    slots = build_slots()
    out("parked_cars={} candidate_bays={} obstacles={}".format(
        len(parked_cars), len(slots), len(obstacles)))

    # Deterministic spread: walk the bay list at a stride so cars land along
    # different streets instead of bunching into the first road.
    stride = max(1, len(slots) // max(1, len(parked_cars)))
    placed_boxes = []
    placed = 0
    unplaced = []

    for car_index, car in enumerate(parked_cars):
        chosen = None
        for attempt in range(len(slots)):
            slot = slots[(car_index * stride + attempt) % len(slots)]
            x, y, yaw, vertical = slot
            box = car_box(x, y, vertical)
            if any(boxes_overlap(box, other) for other in obstacles):
                continue
            if any(boxes_overlap(box, other) for other in placed_boxes):
                continue
            chosen = slot
            placed_boxes.append(box)
            break

        if chosen is None:
            unplaced.append(car.get_actor_label())
            continue

        x, y, yaw, vertical = chosen
        car.set_actor_location(unreal.Vector(x, y, PARK_Z), False, False)
        car.set_actor_rotation(
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), False)
        placed += 1

    out("parked in bays: {}".format(placed))
    if unplaced:
        out("no clear bay found for {}: {}".format(
            len(unplaced), ", ".join(unplaced[:10])))
    else:
        out("every parked car has a clear bay (no tree, prop or car overlap)")

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    out("")
    out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
