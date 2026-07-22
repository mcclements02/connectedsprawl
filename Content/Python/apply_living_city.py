"""Apply the coherent living-city population to TestMap.

This focused, idempotent pass replaces lane-blocking display cars with one
recessed player car, a managed fleet of real moving traffic, legal recessed
parking bays, functional signals, and the recycled human crowd manager.
Run after import_characters.py and import_animated_vehicles.py.
"""
import math
import os
import random
import sys
import unreal


sys.dont_write_bytecode = True
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)
import vehicle_realism

random.seed(20260717)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2.0 + STEP / 2.0
LANE_OFFSET = 150.0
PARKING_OFFSET = 410.0
PARKED_TARGET = 36
TRAFFIC_TARGET = 14


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


def has_blocking_collision(actor):
    """Return whether an authored primitive can physically obstruct a car."""
    components = actor.get_components_by_class(unreal.PrimitiveComponent)
    if not components:
        return False
    for component in components:
        try:
            if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
                return True
        except Exception:
            # Conservative fallback for plugin components without the standard
            # collision accessor: their world bounds still protect the lane.
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


def blocking_actor_bounds():
    """World-space AABBs that a vehicle footprint must never overlap."""
    blockers = []
    for actor in eas.get_all_level_actors():
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if not overlaps_traffic_height(origin, extent):
            continue
        blockers.append((
            actor.get_actor_label(),
            origin.x - extent.x, origin.x + extent.x,
            origin.y - extent.y, origin.y + extent.y,
        ))
    return blockers


def vehicle_rect(x, y, yaw, clearance=35.0):
    """Conservative axis-aligned footprint for the 4.7m physics hull/body."""
    horizontal = abs(yaw % 180.0) < 1.0
    half_x = (250.0 if horizontal else 125.0) + clearance
    half_y = (125.0 if horizontal else 250.0) + clearance
    return x - half_x, x + half_x, y - half_y, y + half_y


def rects_overlap(first, second):
    return (first[1] > second[0] and first[0] < second[1]
            and first[3] > second[2] and first[2] < second[3])


def parking_spot_is_clear(spot, blockers, clearance=35.0):
    footprint = vehicle_rect(spot[0], spot[1], spot[2], clearance)
    return not any(rects_overlap(footprint, bounds[1:]) for bounds in blockers)


def _disable_actor_collision(actor):
    actor.modify()
    for component in actor.get_components_by_class(unreal.PrimitiveComponent):
        try:
            component.modify()
            component.set_collision_profile_name("NoCollision")
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        except Exception:
            pass


def clear_lane_foliage_collision():
    """Keep curb trees visible without letting broad canopy collision block lanes."""
    changed = 0
    foliage_words = ("tree", "foliage", "bush", "shrub")
    for actor in eas.get_all_level_actors():
        if not any(word in actor.get_actor_label().lower() for word in foliage_words):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (not overlaps_traffic_height(origin, extent)
                or not bounds_intrude_live_lane(origin, extent)):
            continue
        _disable_actor_collision(actor)
        changed += 1
    return changed


def clear_drive_over_surface_collision():
    """Let traffic cross flush utility covers without snagging their mesh hulls."""
    changed = 0
    surface_words = ("manhole", "utility", "drain", "grate", "roadcover")
    for actor in eas.get_all_level_actors():
        label = actor.get_actor_label().lower()
        if not any(word in label for word in surface_words):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (origin.z + extent.z > 65.0 or over_lake(origin.x, origin.y)
                or not bounds_intrude_live_lane(origin, extent)):
            continue
        _disable_actor_collision(actor)
        changed += 1
    return changed


def _move_actor_xy(actor, delta_x, delta_y):
    location = actor.get_actor_location()
    actor.modify()
    actor.set_actor_location(unreal.Vector(
        location.x + delta_x, location.y + delta_y, location.z),
        False, False)


def _is_storefront_label(label):
    return label.startswith(("City_DW_store_", "City_DW3_store_"))


def _is_facade_detail_label(label):
    return (_is_storefront_label(label)
            or label.startswith((
                "City_DW3_lvl2_", "City_DW3_trim_",
                "City_DW3_sign_", "City_DW3_fold_")))


def realign_storefront_modules():
    """Restore modular storefronts to the nearest building face."""
    actors = list(eas.get_all_level_actors())
    buildings = [
        actor.get_actor_bounds(False) for actor in actors
        if actor.get_actor_label().startswith("City_Bldg_")
    ]
    changed = 0
    for actor in actors:
        if not _is_storefront_label(actor.get_actor_label()):
            continue
        origin, _ = actor.get_actor_bounds(False)
        candidates = []
        for building_origin, building_extent in buildings:
            delta_x = origin.x - building_origin.x
            delta_y = origin.y - building_origin.y
            if abs(delta_y) <= building_extent.y + 250.0:
                sign_x = 1.0 if delta_x >= 0.0 else -1.0
                desired_x = (building_origin.x
                             + sign_x * (building_extent.x + 16.0))
                candidates.append((abs(origin.x - desired_x), desired_x, origin.y))
            if abs(delta_x) <= building_extent.x + 250.0:
                sign_y = 1.0 if delta_y >= 0.0 else -1.0
                desired_y = (building_origin.y
                             + sign_y * (building_extent.y + 16.0))
                candidates.append((abs(origin.y - desired_y), origin.x, desired_y))
        if not candidates:
            continue
        distance, desired_x, desired_y = min(candidates, key=lambda item: item[0])
        if distance <= 2.0 or distance > 220.0:
            continue
        _move_actor_xy(actor, desired_x - origin.x, desired_y - origin.y)
        changed += 1
    return changed


def realign_upper_facades():
    """Keep separate second-story facade meshes over their storefront base."""
    actors = list(eas.get_all_level_actors())
    storefronts = [
        actor.get_actor_bounds(False)[0] for actor in actors
        if _is_storefront_label(actor.get_actor_label())
    ]
    changed = 0
    for actor in actors:
        if not actor.get_actor_label().startswith(
                ("City_DW3_lvl2_", "City_DW3_trim_")):
            continue
        if not storefronts:
            break
        origin, _ = actor.get_actor_bounds(False)
        target = min(storefronts, key=lambda candidate: math.hypot(
            origin.x - candidate.x, origin.y - candidate.y))
        distance = math.hypot(origin.x - target.x, origin.y - target.y)
        if distance <= 2.0 or distance > 220.0:
            continue
        _move_actor_xy(actor, target.x - origin.x, target.y - origin.y)
        changed += 1
    return changed


def clear_lane_storefront_collision():
    """Use base-building collision when facade detail reaches a traffic lane."""
    changed = 0
    for actor in eas.get_all_level_actors():
        if not _is_facade_detail_label(actor.get_actor_label()):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (not overlaps_traffic_height(origin, extent)
                or over_lake(origin.x, origin.y)
                or not bounds_intrude_live_lane(origin, extent)):
            continue
        _disable_actor_collision(actor)
        changed += 1
    return changed


def clear_lane_street_dressing_collision():
    """Keep decorative sidewalk dressing visible without invisible lane walls."""
    changed = 0
    prefixes = ("City_R2_", "City_RPG_Junction_")
    for actor in eas.get_all_level_actors():
        if not actor.get_actor_label().startswith(prefixes):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (not overlaps_traffic_height(origin, extent)
                or over_lake(origin.x, origin.y)
                or not bounds_intrude_live_lane(origin, extent)):
            continue
        _disable_actor_collision(actor)
        changed += 1
    return changed


def center_and_clear_park_features():
    """Keep large park meshes centered without letting broad hulls reach roads."""
    changed = 0
    for actor in eas.get_all_level_actors():
        label = actor.get_actor_label()
        if label == "City_DW3_playground":
            origin, _ = actor.get_actor_bounds(False)
            target_x = block_center(2)
            target_y = block_center(2)
            if math.hypot(origin.x - target_x, origin.y - target_y) > 2.0:
                _move_actor_xy(actor, target_x - origin.x, target_y - origin.y)
        if label not in ("City_DW3_playground", "City_DW3_fountain"):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (overlaps_traffic_height(origin, extent)
                and bounds_intrude_live_lane(origin, extent)):
            _disable_actor_collision(actor)
            changed += 1
    return changed


def clear_background_collision():
    """Backdrop mountains are visual scenery and never gameplay collision."""
    changed = 0
    for actor in eas.get_all_level_actors():
        if not actor.get_actor_label().startswith("City_DW3_mountains_"):
            continue
        if has_blocking_collision(actor):
            _disable_actor_collision(actor)
            changed += 1
    return changed


def _belongs_to_building_detail_group(label, detail_id):
    suffix = "_{}".format(detail_id)
    if label == "City_Roof_Cap{}".format(suffix):
        return True
    if label == "City_RealProp_roofcap{}".format(suffix):
        return True
    if label.startswith("City_BuildingDetail_") and label.endswith(suffix):
        return True
    single_id_roof_prefixes = (
        "City_Roof_WaterTank_", "City_Roof_Antenna_",
        "City_Roof_BillboardFace_", "City_Roof_BillboardFrame_",
    )
    if any(label == prefix + detail_id for prefix in single_id_roof_prefixes):
        return True
    return (label.startswith("City_Roof_Unit_{}_".format(detail_id))
            or label.startswith("City_Roof_TankLeg_{}_".format(detail_id)))


def realign_recessed_building_details():
    """Keep facade and roof actors attached after a corner building moves."""
    actors = list(eas.get_all_level_actors())
    buildings = [
        (actor, *actor.get_actor_bounds(False)) for actor in actors
        if actor.get_actor_label().startswith("City_Bldg_")
    ]
    caps = [
        actor for actor in actors
        if actor.get_actor_label().startswith("City_Roof_Cap_")
    ]
    realigned = 0
    for cap in caps:
        detail_id = cap.get_actor_label().rsplit("_", 1)[-1]
        if not detail_id.isdigit() or not buildings:
            continue
        cap_origin, _ = cap.get_actor_bounds(False)
        _, building_origin, building_extent = min(
            buildings,
            key=lambda item: math.hypot(
                item[1].x - cap_origin.x, item[1].y - cap_origin.y))
        delta_x = building_origin.x - cap_origin.x
        delta_y = building_origin.y - cap_origin.y
        distance = math.hypot(delta_x, delta_y)
        if distance < 5.0 or distance > 320.0:
            continue

        for detail in actors:
            label = detail.get_actor_label()
            move_group_member = _belongs_to_building_detail_group(
                label, detail_id)
            if not move_group_member and label.startswith((
                    "City_DW_store_", "City_DW3_store_",
                    "City_DW_awning_", "City_DW3_awning_",
                    "City_DW3_lvl2_", "City_DW3_trim_",
                    "City_DW3_sign_", "City_DW3_fold_")):
                detail_origin, _ = detail.get_actor_bounds(False)
                move_group_member = (
                    abs(detail_origin.x - cap_origin.x) <= building_extent.x + 100.0
                    and abs(detail_origin.y - cap_origin.y) <= building_extent.y + 100.0)
            if move_group_member:
                _move_actor_xy(detail, delta_x, delta_y)
        realigned += 1
    return realigned


def recess_lane_intruding_buildings():
    """Move broad corner-building collision back inside its authored block."""
    changed = 0
    for actor in eas.get_all_level_actors():
        if not actor.get_actor_label().startswith("City_Bldg_"):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (not overlaps_traffic_height(origin, extent)
                or over_lake(origin.x, origin.y)
                or not bounds_intrude_live_lane(origin, extent)):
            continue
        target_x = block_center(round((origin.x - START) / STEP))
        target_y = block_center(round((origin.y - START) / STEP))
        inward_x = target_x - origin.x
        inward_y = target_y - origin.y
        length = math.hypot(inward_x, inward_y)
        if length < 1.0:
            continue
        _move_actor_xy(
            actor, inward_x / length * 160.0, inward_y / length * 160.0)
        changed += 1
    return changed


def recess_lane_intruding_curb_props():
    """Move solid street furniture fully behind the travel-lane envelope."""
    changed = 0
    curb_words = (
        "hydrant", "bench", "trash", "bin", "mailbox", "meter", "barrier",
        "recycle", "poster", "pots", "planter", "foodcart", "streetstand",
    )
    for actor in eas.get_all_level_actors():
        label = actor.get_actor_label().lower()
        if not any(word in label for word in curb_words):
            continue
        if not has_blocking_collision(actor):
            continue
        origin, extent = actor.get_actor_bounds(False)
        if (not overlaps_traffic_height(origin, extent)
                or over_lake(origin.x, origin.y)
                or not bounds_intrude_live_lane(origin, extent)):
            continue
        target_x = block_center(round((origin.x - START) / STEP))
        target_y = block_center(round((origin.y - START) / STEP))
        inward_x = target_x - origin.x
        inward_y = target_y - origin.y
        length = math.hypot(inward_x, inward_y)
        if length < 1.0:
            continue
        _move_actor_xy(
            actor, inward_x / length * 140.0, inward_y / length * 140.0)
        changed += 1
    return changed


def clear_prefixes(prefixes):
    removed = 0
    for actor in list(eas.get_all_level_actors()):
        if any(actor.get_actor_label().startswith(prefix) for prefix in prefixes):
            eas.destroy_actor(actor)
            removed += 1
    return removed


def clear_exact(labels):
    labels = set(labels)
    removed = 0
    for actor in list(eas.get_all_level_actors()):
        if actor.get_actor_label() in labels:
            eas.destroy_actor(actor)
            removed += 1
    return removed


starts = [
    actor for actor in eas.get_all_level_actors()
    if isinstance(actor, unreal.PlayerStart)
]
if not starts:
    raise RuntimeError("TestMap has no PlayerStart")
player_start = starts[0].get_actor_location()

removed = clear_prefixes([
    "City_DriveCar_", "City_TrafficCar_", "City_Car_", "City_ParkedCar_",
    "City_R2_parked_",
    "City_Ped_", "City_NPC_", "City_RPG_Junction_Ped_", "City_Signal_",
    "City_Detail_TrafficPole_", "City_Detail_TrafficArm_",
    "City_Detail_TrafficBox_", "City_Detail_TrafficLamp_",
])
clear_exact(["Sprawl_Crowd", "Sprawl_Traffic", "Sprawl_RoadMarkings"])
unreal.log("[LivingCity] removed {} obsolete/blocking actors".format(removed))
unreal.log("[LivingCity] cleared collision on {} lane-intruding foliage actors".format(
    clear_lane_foliage_collision()))
unreal.log("[LivingCity] cleared collision on {} drive-over surface props".format(
    clear_drive_over_surface_collision()))
realigned_details = realign_recessed_building_details()
recessed_buildings = recess_lane_intruding_buildings()
realigned_details += realign_recessed_building_details()
unreal.log("[LivingCity] recessed {} lane-intruding corner buildings and "
           "realigned {} detail groups".format(
               recessed_buildings, realigned_details))
realigned_storefronts = realign_storefront_modules()
realigned_upper_facades = realign_upper_facades()
unreal.log("[LivingCity] realigned {} storefront modules and {} upper facades; "
           "cleared {} lane-facing facade collisions".format(
               realigned_storefronts, realigned_upper_facades,
               clear_lane_storefront_collision()))
unreal.log("[LivingCity] cleared collision on {} lane-facing street dressings".format(
    clear_lane_street_dressing_collision()))
unreal.log("[LivingCity] cleared collision on {} oversized park features and {} "
           "background mountains".format(
               center_and_clear_park_features(), clear_background_collision()))
unreal.log("[LivingCity] recessed {} solid curb props".format(
    recess_lane_intruding_curb_props()))

combined_meshes = vehicle_realism.load_vehicle_meshes()
paint_materials = vehicle_realism.load_vehicle_paint_materials()
if not combined_meshes:
    raise RuntimeError("Imported combined vehicle meshes are missing")

# Capture the cleaned-up authored geometry before adding any vehicles. Parking
# is accepted only when the entire car footprint clears buildings and props.
world_blockers = blocking_actor_bounds()


def parking_candidates():
    candidates = []
    for gx in range(N):
        for gy in range(N):
            bx = block_center(gx)
            by = block_center(gy)
            if over_lake(bx, by, 0.0):
                continue

            # Each tuple is (axis, side, road index). The center sits 410cm
            # from the road center, recessed 110cm into the block-side bay.
            edges = []
            if gx < N - 1:
                edges.append(("vertical", +1, gx))
            if gx > 0:
                edges.append(("vertical", -1, gx - 1))
            if gy < N - 1:
                edges.append(("horizontal", +1, gy))
            if gy > 0:
                edges.append(("horizontal", -1, gy - 1))

            for edge_index, (axis, side, road_index) in enumerate(edges):
                # Inspect every block face, then let the full-footprint tests
                # below choose a sparse, non-overlapping legal subset.
                for along in (-470.0, 470.0):
                    if axis == "vertical":
                        x = road_center(road_index) - side * PARKING_OFFSET
                        y = by + along
                        yaw = 90.0 if (gx + gy + edge_index) % 2 == 0 else 270.0
                    else:
                        x = bx + along
                        y = road_center(road_index) - side * PARKING_OFFSET
                        yaw = 0.0 if (gx + gy + edge_index) % 2 == 0 else 180.0
                    if (abs(x) <= SPAN * 0.5 - 300.0
                            and abs(y) <= SPAN * 0.5 - 300.0
                            and not over_lake(x, y, 50.0)):
                        candidates.append((x, y, yaw, axis, road_index))
    return candidates


parking_pool = [
    spot for spot in parking_candidates()
    if parking_spot_is_clear(spot, world_blockers)
]
parking_pool.sort(key=lambda spot: math.hypot(
    spot[0] - player_start.x, spot[1] - player_start.y))
player_spots = [
    spot for spot in parking_pool
    if parking_spot_is_clear(spot, world_blockers, 120.0)
]
if not player_spots:
    raise RuntimeError("No parking bay has a safe player-side walkway")
player_spot = player_spots[0]
parking = [player_spot]
for spot in parking_pool:
    if spot == player_spot:
        continue
    footprint = vehicle_rect(spot[0], spot[1], spot[2])
    if any(rects_overlap(footprint, vehicle_rect(
            selected[0], selected[1], selected[2])) for selected in parking):
        continue
    parking.append(spot)
    if len(parking) >= PARKED_TARGET + 1:
        break
if len(parking) < PARKED_TARGET + 1:
    raise RuntimeError("Only found {} of {} building-clear parking bays".format(
        len(parking), PARKED_TARGET + 1))

# The closest extra-clear bay is Zarri's stationary, enterable car. It is no
# longer abandoned in a live lane, and the rest of the fleet is real traffic.
parking.pop(0)
player_car = eas.spawn_actor_from_class(
    unreal.SprawlCar,
    unreal.Vector(player_spot[0], player_spot[1], 100.0),
    unreal.Rotator(roll=0.0, pitch=0.0, yaw=player_spot[2]),
)
player_car.set_actor_label("City_DriveCar_0")
player_car.set_editor_property("auto_drive", False)
player_car.set_editor_property("tags", ["Sprawl.PlayerVehicle"])
if not vehicle_realism.configure_animated_car(player_car, 0, paint_materials):
    vehicle_realism.configure_drivable_car(
        player_car, 0, combined_meshes, paint_materials)

# Put Zarri on the protected block side of the selected car. The larger
# player-bay clearance above guarantees room for the character capsule here.
start_distance = 180.0
if player_spot[3] == "vertical":
    road_axis = road_center(player_spot[4])
    away = 1.0 if player_spot[0] > road_axis else -1.0
    new_player_start = unreal.Vector(
        player_spot[0] + away * start_distance,
        player_spot[1], player_start.z)
else:
    road_axis = road_center(player_spot[4])
    away = 1.0 if player_spot[1] > road_axis else -1.0
    new_player_start = unreal.Vector(
        player_spot[0], player_spot[1] + away * start_distance,
        player_start.z)
# Face along the sidewalk. Facing directly toward the car put the third-person
# boom into the building behind Zarri and made the opening view a wall.
start_yaw = player_spot[2]
starts[0].set_actor_location(new_player_start, False, False)
starts[0].set_actor_rotation(
    unreal.Rotator(roll=0.0, pitch=0.0, yaw=start_yaw), False)
player_start = new_player_start

# Every parked car is a real ASprawlCar: stationary until entered, with the
# same animated wheels and driving controls as Zarri's closest vehicle.
parked = 0
for index, spot in enumerate(parking):
    if parked >= PARKED_TARGET:
        break
    actor = eas.spawn_actor_from_class(
        unreal.SprawlCar,
        unreal.Vector(spot[0], spot[1], 100.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=spot[2]))
    if actor:
        actor.set_actor_label("City_ParkedCar_{}".format(parked))
        actor.set_editor_property("auto_drive", False)
        actor.set_editor_property("tags", ["Sprawl.ParkedVehicle"])
        if not vehicle_realism.configure_animated_car(
                actor, parked + 20, paint_materials):
            vehicle_realism.configure_drivable_car(
                actor, parked + 20, combined_meshes, paint_materials)
        parked += 1


# Keep the authored-world blockers captured before vehicle spawning, then add
# the known physics-hull footprints explicitly. Actor bounds also include the
# new non-colliding seated-driver skeletal mesh, which must not make a valid
# lane look blocked during this deterministic authoring pass.
blockers = list(world_blockers)
for label, spot in [("City_DriveCar_0", player_spot)] + [
        ("City_ParkedCar_{}".format(index), spot)
        for index, spot in enumerate(parking)]:
    blockers.append((label,) + vehicle_rect(spot[0], spot[1], spot[2]))


def traffic_spot_is_clear(x, y, yaw):
    horizontal = abs((yaw % 180.0)) < 1.0
    half_x = 330.0 if horizontal else 140.0
    half_y = 140.0 if horizontal else 330.0
    for _, min_x, max_x, min_y, max_y in blockers:
        if (x + half_x > min_x and x - half_x < max_x
                and y + half_y > min_y and y - half_y < max_y):
            return False
    return True


def traffic_candidates():
    spots = []
    # Segment centers are well away from crosswalks and stop lines. Heading
    # chooses the exact right-hand lane via the authoritative ±150 convention.
    for road_index in range(N - 1):
        for segment in range(1, N - 1):
            along = block_center(segment)
            if (road_index + segment) % 2 == 0:
                heading = "north"
                x = road_center(road_index) - LANE_OFFSET
                y = along
                yaw = 90.0
            else:
                heading = "south"
                x = road_center(road_index) + LANE_OFFSET
                y = along
                yaw = 270.0
            if not over_lake(x, y):
                spots.append((x, y, yaw, heading, road_index))

            if (road_index * 3 + segment) % 2 == 0:
                heading = "east"
                x = along
                y = road_center(road_index) + LANE_OFFSET
                yaw = 0.0
            else:
                heading = "west"
                x = along
                y = road_center(road_index) - LANE_OFFSET
                yaw = 180.0
            if not over_lake(x, y):
                spots.append((x, y, yaw, heading, road_index))
    random.shuffle(spots)
    return spots


traffic = 0
occupied = [(player_spot[0], player_spot[1])]
for spot in traffic_candidates():
    if traffic >= TRAFFIC_TARGET:
        break
    x, y, yaw, _, _ = spot
    if math.hypot(x - player_start.x, y - player_start.y) < 1400.0:
        continue
    if not traffic_spot_is_clear(x, y, yaw):
        continue
    if any(math.hypot(x - ox, y - oy) < 900.0 for ox, oy in occupied):
        continue
    car = eas.spawn_actor_from_class(
        unreal.SprawlCar, unreal.Vector(x, y, 175.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    car.set_actor_label("City_TrafficCar_{}".format(traffic))
    car.set_editor_property("auto_drive", True)
    if not vehicle_realism.configure_animated_car(car, traffic + 1, paint_materials):
        vehicle_realism.configure_drivable_car(
            car, traffic + 1, combined_meshes, paint_materials)
    occupied.append((x, y))
    traffic += 1
if traffic != TRAFFIC_TARGET:
    raise RuntimeError("Only placed {} of {} traffic cars".format(
        traffic, TRAFFIC_TARGET))

# Functional signals share the exact phase function and reservation authority
# used by the cars. Replace decorative permanently-lit assemblies everywhere.
signals = 0
for ix in range(N - 1):
    for iy in range(N - 1):
        center_x = road_center(ix)
        center_y = road_center(iy)
        if over_lake(center_x, center_y, 100.0):
            continue
        signal = eas.spawn_actor_from_class(
            unreal.SprawlTrafficLight,
            # The actor owns four sidewalk-corner poles around this centre.
            unreal.Vector(center_x, center_y, 14.0))
        signal.set_actor_label("City_Signal_{}_{}".format(ix, iy))
        signal.set_editor_property("intersection_x", ix)
        signal.set_editor_property("intersection_y", iy)
        signals += 1

crowd = eas.spawn_actor_from_class(
    unreal.PedestrianCrowdManager, unreal.Vector(0.0, 0.0, 0.0))
crowd.set_actor_label("Sprawl_Crowd")
crowd.set_editor_property("target_count", 26)

traffic_manager = eas.spawn_actor_from_class(
    unreal.ProceduralTrafficManager, unreal.Vector(0.0, 0.0, 0.0))
traffic_manager.set_actor_label("Sprawl_Traffic")
traffic_manager.set_editor_property("target_active_count", TRAFFIC_TARGET)

markings = eas.spawn_actor_from_class(
    unreal.SprawlRoadMarkings, unreal.Vector(0.0, 0.0, 0.0))
markings.set_actor_label("Sprawl_RoadMarkings")

saved = les.save_current_level()
if not saved:
    raise RuntimeError("TestMap failed to save")

unreal.log(
    "[LivingCity] DONE: 1 legal player car, {} animated traffic, {} "
    "building-clear enterable parked cars, {} functional signals, "
    "26 recycled humans".format(
        traffic, parked, signals))
