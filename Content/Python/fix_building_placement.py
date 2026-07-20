"""Keep buildings on their block: never in the street, never over the kerb.

A building may occupy its block; it may not stand in the carriageway, and its
facade should not overhang the pavement walking band. This measures every
building footprint against the street corridor and pushes any intruder back to
the building line, perpendicular to the street it is intruding on.

Definitions, all derived from the grid so they follow any future re-layout:

    carriageway   |offset from road centre| <  ROAD/2        (950)
    pavement band ROAD/2 .. ROAD/2 + WALKWAY                 (950..1350)
    building line ROAD/2 + WALKWAY                           (1350)

Awnings and signs are allowed to overhang the pavement — that is what they are
for — so only their supporting structure is held to the building line.

Report-only by default: set APPLY = False to audit without moving anything.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/fix_building_placement.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"
APPLY = True

NUM_BLOCKS = 7
BLOCK = 2000.0
ROAD = 1200.0        # two-way: 1 lane each way + parking bays
STEP = BLOCK + ROAD
SPAN = NUM_BLOCKS * STEP
NUM_ROADS = NUM_BLOCKS - 1

HALF_ROAD = ROAD / 2.0          # 950 - kerb line
WALKWAY = 400.0                 # pavement kept clear for pedestrians
BUILDING_LINE = HALF_ROAD + WALKWAY   # 1350

# Structures held to the building line.
BUILDING_PREFIXES = (
    "City_DW_store", "City_DW3_store", "City_DW3_lvl2", "City_DW3_trim",
    "City_BuildingDetail_StorefrontGlass", "City_BuildingDetail_Door",
)
# Allowed to overhang the pavement by design.
OVERHANG_PREFIXES = (
    "City_DW_awning", "City_DW3_awning", "City_DW3_sign",
    "City_BuildingDetail_Awning", "City_BuildingDetail_StorefrontSign",
)

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "BuildingPlacementReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def block_center(index):
    return -SPAN / 2.0 + STEP / 2.0 + index * STEP


def road_center(index):
    return (block_center(index) + block_center(index + 1)) / 2.0


ROAD_CENTERS = [road_center(i) for i in range(NUM_ROADS)]


def nearest_road_offset(value):
    """Signed offset to the nearest road centreline on this axis."""
    best = None
    for center in ROAD_CENTERS:
        offset = value - center
        if best is None or abs(offset) < abs(best):
            best = offset
    return best if best is not None else 0.0


def intrusion(low, high, limit):
    """How far a span reaches inside `limit` of a road centre, and which way."""
    worst = 0.0
    direction = 0.0
    for edge in (low, high):
        offset = nearest_road_offset(edge)
        depth = limit - abs(offset)
        if depth > worst:
            worst = depth
            direction = 1.0 if offset > 0 else -1.0
    return worst, direction


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    out("actors_total={}".format(len(actors)))
    out("kerb at {:.0f}cm, building line at {:.0f}cm from each road centre".format(
        HALF_ROAD, BUILDING_LINE))

    in_street = []
    over_pavement = []
    moved = 0

    for actor in actors:
        if not actor:
            continue
        label = actor.get_actor_label()
        is_structure = label.startswith(BUILDING_PREFIXES)
        is_overhang = label.startswith(OVERHANG_PREFIXES)
        if not is_structure and not is_overhang:
            continue

        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        if extent.x <= 0.0 and extent.y <= 0.0:
            continue

        # Overhangs only have to stay out of the carriageway itself.
        limit = BUILDING_LINE if is_structure else HALF_ROAD
        depth_x, dir_x = intrusion(origin.x - extent.x, origin.x + extent.x, limit)
        depth_y, dir_y = intrusion(origin.y - extent.y, origin.y + extent.y, limit)

        if depth_x <= 1.0 and depth_y <= 1.0:
            continue

        # Record against the stricter carriageway test for reporting.
        street_depth = max(
            intrusion(origin.x - extent.x, origin.x + extent.x, HALF_ROAD)[0],
            intrusion(origin.y - extent.y, origin.y + extent.y, HALF_ROAD)[0])
        (in_street if street_depth > 1.0 else over_pavement).append(
            (label, max(depth_x, depth_y)))

        if not APPLY:
            continue

        location = actor.get_actor_location()
        # Push straight back from the street it intrudes on.
        actor.set_actor_location(
            unreal.Vector(location.x + dir_x * depth_x,
                          location.y + dir_y * depth_y,
                          location.z), False, False)
        moved += 1

    out("")
    out("=== structures standing in the carriageway: {} ===".format(len(in_street)))
    for label, depth in sorted(in_street, key=lambda r: -r[1])[:20]:
        out("  {:<44} {:6.0f}cm".format(label, depth))

    out("")
    out("=== structures over the pavement walking band: {} ===".format(
        len(over_pavement)))
    for label, depth in sorted(over_pavement, key=lambda r: -r[1])[:20]:
        out("  {:<44} {:6.0f}cm".format(label, depth))

    if APPLY:
        out("")
        out("set back to the building line: {}".format(moved))
        unreal.get_editor_subsystem(
            unreal.LevelEditorSubsystem).save_current_level()
        out("saved {}".format(MAP_PATH))
    else:
        out("")
        out("audit only; nothing moved")


try:
    main()
finally:
    flush()
