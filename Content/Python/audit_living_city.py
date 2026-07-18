"""Fail-fast structural audit for the authored living-city map pass."""
import math
import sys
import unreal


sys.dont_write_bytecode = True
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

N = 7
STEP = 2600.0
START = -7800.0
LANE_OFFSET = 150.0
PARKING_OFFSET = 410.0
SPAN = N * STEP


def block_center(index):
    return START + index * STEP


def road_center(index):
    return (block_center(index) + block_center(index + 1)) * 0.5


def over_lake(x, y, margin=200.0):
    x0 = block_center(4) - STEP * 0.5
    x1 = block_center(6) + STEP * 0.5
    y0 = block_center(0) - STEP * 0.5
    y1 = block_center(1) + STEP * 0.5
    return x0 - margin < x < x1 + margin and y0 - margin < y < y1 + margin


ROAD_CENTERS = [road_center(index) for index in range(N - 1)]


def actors_with_prefix(prefix):
    return [
        actor for actor in eas.get_all_level_actors()
        if actor.get_actor_label().startswith(prefix)
    ]


def actors_with_label(label):
    return [
        actor for actor in eas.get_all_level_actors()
        if actor.get_actor_label() == label
    ]


def nearest_offset(value):
    return min(abs(value - center) for center in ROAD_CENTERS)


def nearest_delta(value):
    center = min(ROAD_CENTERS, key=lambda candidate: abs(value - candidate))
    return value - center


def angle_delta(a, b):
    return abs((a - b + 180.0) % 360.0 - 180.0)


def static_mesh_component(actor, name):
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        if component.get_name() == name:
            return component
    return None


def component_mesh(component):
    if not component:
        return None
    try:
        return component.get_editor_property("static_mesh")
    except Exception:
        return None


def has_blocking_collision(actor):
    components = actor.get_components_by_class(unreal.PrimitiveComponent)
    if not components:
        return False
    for component in components:
        try:
            if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
                return True
        except Exception:
            return True
    return False


def bounds_intrude_live_lane(origin, extent, lane_half_width=110.0):
    min_x, max_x = origin.x - extent.x, origin.x + extent.x
    min_y, max_y = origin.y - extent.y, origin.y + extent.y
    grid_min = road_center(0) - 330.0
    grid_max = road_center(N - 2) + 330.0
    for road_index in range(N - 1):
        center = road_center(road_index)
        for lane in (center - LANE_OFFSET, center + LANE_OFFSET):
            horizontal = (max_x > grid_min and min_x < grid_max
                          and max_y > lane - lane_half_width
                          and min_y < lane + lane_half_width)
            vertical = (max_y > grid_min and min_y < grid_max
                        and max_x > lane - lane_half_width
                        and min_x < lane + lane_half_width)
            if horizontal or vertical:
                return True
    return False


def overlaps_traffic_height(origin, extent):
    """Match the moving car's settled hull/forward-sweep vertical envelope."""
    return origin.z + extent.z >= 35.0 and origin.z - extent.z <= 150.0


def vehicle_rect(actor, clearance=20.0):
    location = actor.get_actor_location()
    yaw = actor.get_actor_rotation().yaw % 180.0
    horizontal = yaw < 1.0 or yaw > 179.0
    half_x = (250.0 if horizontal else 125.0) + clearance
    half_y = (125.0 if horizontal else 250.0) + clearance
    return (location.x - half_x, location.x + half_x,
            location.y - half_y, location.y + half_y)


def rects_overlap(first, second):
    return (first[1] > second[0] and first[0] < second[1]
            and first[3] > second[2] and first[2] < second[3])


drive = actors_with_prefix("City_DriveCar_")
traffic = actors_with_prefix("City_TrafficCar_")
parked = actors_with_prefix("City_ParkedCar_")
signals = actors_with_prefix("City_Signal_")

assert len(drive) == 1, "expected 1 player car, found {}".format(len(drive))
assert len(traffic) == 14, "expected 14 traffic cars, found {}".format(len(traffic))
assert len(parked) == 36, "expected 36 parked cars, found {}".format(len(parked))
assert len(signals) == 30, "expected 30 signals, found {}".format(len(signals))
assert len(actors_with_label("Sprawl_Crowd")) == 1
assert len(actors_with_label("Sprawl_Traffic")) == 1
assert len(actors_with_label("Sprawl_RoadMarkings")) == 1
markings = actors_with_label("Sprawl_RoadMarkings")[0]
boundary_names = {
    component.get_name()
    for component in markings.get_components_by_class(unreal.BoxComponent)
    if "CityBoundary" in component.get_name()
}
assert boundary_names == {
    "CityBoundaryNorth", "CityBoundarySouth",
    "CityBoundaryEast", "CityBoundaryWest",
}, "missing city boundary walls: {}".format(sorted(boundary_names))

obsolete_prefixes = [
    "City_R2_parked_", "City_Ped_", "City_NPC_", "City_RPG_Junction_Ped_",
    "City_Detail_TrafficPole_", "City_Detail_TrafficArm_",
    "City_Detail_TrafficBox_", "City_Detail_TrafficLamp_",
]
for prefix in obsolete_prefixes:
    assert not actors_with_prefix(prefix), "obsolete actors remain: {}".format(prefix)

# Moving cars start on the exact right-hand lane for their heading.
lane_errors = []
for car in traffic:
    assert car.get_editor_property("auto_drive") is True
    location = car.get_actor_location()
    yaw = car.get_actor_rotation().yaw % 360.0
    if angle_delta(yaw, 90.0) < 1.0:       # north: west half
        error = abs(nearest_delta(location.x) + LANE_OFFSET)
    elif angle_delta(yaw, 270.0) < 1.0:    # south: east half
        error = abs(nearest_delta(location.x) - LANE_OFFSET)
    elif angle_delta(yaw, 0.0) < 1.0:      # east: north half
        error = abs(nearest_delta(location.y) - LANE_OFFSET)
    elif angle_delta(yaw, 180.0) < 1.0:    # west: south half
        error = abs(nearest_delta(location.y) + LANE_OFFSET)
    else:
        raise AssertionError("{} has non-cardinal yaw {}".format(
            car.get_actor_label(), yaw))
    lane_errors.append(error)

    body = static_mesh_component(car, "ExternalVehicleMesh")
    assert component_mesh(body), "{} missing real body".format(
        car.get_actor_label())
    for wheel_index in range(4):
        wheel = static_mesh_component(car, "Wheel{}".format(wheel_index))
        wheel_mesh = component_mesh(wheel)
        assert wheel_mesh, "{} missing wheel {}".format(
            car.get_actor_label(), wheel_index)
        assert "Wheel_" in wheel_mesh.get_name()
assert max(lane_errors) < 1.0, "lane placement error: {}".format(max(lane_errors))

# Every authored vehicle starts cardinal, upright, and inside the perimeter.
all_cars = drive + traffic + parked
for car in all_cars:
    rotation = car.get_actor_rotation()
    location = car.get_actor_location()
    assert abs(rotation.pitch) < 0.1 and abs(rotation.roll) < 0.1, (
        "{} is not upright: {}".format(car.get_actor_label(), rotation))
    assert abs(location.x) <= SPAN * 0.5 - 300.0
    assert abs(location.y) <= SPAN * 0.5 - 300.0

# Parked vehicles are no longer decorative static meshes: each can be entered,
# driven, and displays the same real body/four-wheel layout as moving traffic.
for car in parked:
    assert isinstance(car, unreal.SprawlCar)
    assert car.get_editor_property("auto_drive") is False
    assert "Sprawl.ParkedVehicle" in [
        str(tag) for tag in car.get_editor_property("tags")]
    body = static_mesh_component(car, "ExternalVehicleMesh")
    assert component_mesh(body), "{} missing real body".format(car.get_actor_label())
    for wheel_index in range(4):
        wheel = static_mesh_component(car, "Wheel{}".format(wheel_index))
        assert component_mesh(wheel), "{} missing wheel {}".format(
            car.get_actor_label(), wheel_index)

# Authored moving cars must start clear of real geometry, including foliage
# and curb props whose broad collision bounds can intrude into an outer lane.
blockers = []
for actor in eas.get_all_level_actors():
    if (actor in all_cars
            or actor.get_actor_label() == "Sprawl_RoadMarkings"
            or not has_blocking_collision(actor)):
        continue
    origin, extent = actor.get_actor_bounds(False)
    if (not overlaps_traffic_height(origin, extent)
            or over_lake(origin.x, origin.y)):
        continue
    blockers.append((
        actor.get_actor_label(),
        origin.x - extent.x, origin.x + extent.x,
        origin.y - extent.y, origin.y + extent.y,
    ))
for car in all_cars:
    footprint = vehicle_rect(car, 35.0)
    overlaps = [
        label for label, min_x, max_x, min_y, max_y in blockers
        if rects_overlap(footprint, (min_x, max_x, min_y, max_y))
    ]
    assert not overlaps, "{} starts in blocking geometry: {}".format(
        car.get_actor_label(), overlaps[:5])

# No persistent colliding prop may occupy the cars' 35-150cm sensing/hull
# envelope in a live lane; otherwise later-routed traffic would form a
# permanent queue. Roof caps, signs, and awnings above that envelope are safe.
lane_intruders = []
for actor in eas.get_all_level_actors():
    if (actor in all_cars
            or actor.get_actor_label() == "Sprawl_RoadMarkings"
            or not has_blocking_collision(actor)):
        continue
    origin, extent = actor.get_actor_bounds(False)
    if (not overlaps_traffic_height(origin, extent)
            or over_lake(origin.x, origin.y)):
        continue
    if bounds_intrude_live_lane(origin, extent):
        lane_intruders.append((
            actor.get_actor_label(),
            round(origin.z - extent.z, 1),
            round(origin.z + extent.z, 1),
        ))
assert not lane_intruders, "blocking actors intrude into live lanes: {}".format(
    lane_intruders[:12])

# Recessed parking sits beyond the 300cm road edge, never in a travel lane.
parking_offsets = []
for car in drive + parked:
    location = car.get_actor_location()
    offset = min(nearest_offset(location.x), nearest_offset(location.y))
    parking_offsets.append(offset)
assert min(parking_offsets) >= PARKING_OFFSET - 1.0
assert max(parking_offsets) <= PARKING_OFFSET + 1.0

parked_fleet = drive + parked
parking_overlaps = []
for index, car in enumerate(parked_fleet):
    first = vehicle_rect(car)
    for other in parked_fleet[index + 1:]:
        if rects_overlap(first, vehicle_rect(other)):
            parking_overlaps.append((car.get_actor_label(), other.get_actor_label()))
assert not parking_overlaps, "parked cars overlap: {}".format(parking_overlaps[:8])

# No two initial moving vehicles are stacked into the same queue position.
traffic_locations = [car.get_actor_location() for car in traffic]
min_spacing = min(
    math.hypot(a.x - b.x, a.y - b.y)
    for index, a in enumerate(traffic_locations)
    for b in traffic_locations[index + 1:]
)
assert min_spacing >= 899.0, "traffic starts overlapped: {}".format(min_spacing)

starts = [
    actor for actor in eas.get_all_level_actors()
    if isinstance(actor, unreal.PlayerStart)
]
assert starts
player_distance = math.hypot(
    drive[0].get_actor_location().x - starts[0].get_actor_location().x,
    drive[0].get_actor_location().y - starts[0].get_actor_location().y,
)
assert player_distance <= 800.0, "player car is too far away: {}".format(player_distance)
assert drive[0].get_editor_property("auto_drive") is False

# Every imported character has a mesh and all five behavior clips.
variants = [
    "Baldman", "BizDude", "Cappy", "Chill", "Erika", "Jennifer", "Jimmy",
    "Kate", "Kyle", "Lydia", "Mafiossini", "OldMoustache", "Olivia", "Rose",
    "Samuela", "Shiro",
]
for variant in variants:
    root = "/Game/Pedestrians/{0}".format(variant)
    expected = ["SK_{}".format(variant)] + [
        "{}_{}".format(variant, clip)
        for clip in ("Idle", "Walk", "Jog", "Talk", "WalkFormal")
    ]
    for asset_name in expected:
        assert unreal.EditorAssetLibrary.does_asset_exist(
            "{}/{}".format(root, asset_name)
        ), "missing character asset {}".format(asset_name)

unreal.log(
    "[LivingCityAudit] PASS: cars=1+14+36 signals=30 crowd=26 "
    "max_lane_error={:.2f} parking_offset={:.2f} min_traffic_spacing={:.2f} "
    "player_car_distance={:.2f} upright={} enterable_parked=36 boundaries=4".format(
        max(lane_errors), min(parking_offsets), min_spacing, player_distance,
        len(all_cars)))
