"""Re-space the city when the street layout changes.

Set OLD_ROAD / NEW_ROAD to the kerb-to-kerb widths you are moving between; the
block size never changes, so only the gaps between blocks open or close.

ANCHORING BY OWNERSHIP, NOT PROXIMITY
-------------------------------------
An earlier version mapped each actor to its *nearest* grid anchor. Anchors
alternate block-centre / road-centre every half step, so anything in the outer
~350cm of a block is nearer the road centre than its own block centre — and
that is exactly where kerbside trees, hydrants and shopfronts live. They
scaled with the road and ended up standing in the carriageway.

So ownership decides the anchor instead:

  * within BLOCK/2 of a block centre  -> block-owned. Keeps its exact offset
    from that block centre, so block interiors are preserved untouched.
  * otherwise                         -> road-owned (it is in the street).
    Its offset from the road centre is scaled by NEW_ROAD/OLD_ROAD, so
    markings stay proportional and bay positions track the new width.

Idempotent only in the sense that re-running with the same OLD/NEW pair is a
no-op; update the constants whenever the grid changes.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/expand_streets.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"

NUM_BLOCKS = 7
BLOCK = 2000.0
OLD_ROAD = 1900.0      # 2 lanes each way + bays
NEW_ROAD = 1200.0      # two-way: 1 lane each way + bays

OLD_STEP = BLOCK + OLD_ROAD
NEW_STEP = BLOCK + NEW_ROAD
OLD_SPAN = NUM_BLOCKS * OLD_STEP
NEW_SPAN = NUM_BLOCKS * NEW_STEP
HALF_BLOCK = BLOCK / 2.0
ROAD_SCALE = NEW_ROAD / OLD_ROAD

# Superseded by ASprawlRoadMarkings, which repaints from the grid constants.
LEGACY_MARKING_PREFIXES = (
    "City_Detail_VCenter_", "City_Detail_HCenter_",
    "City_Detail_VLane_", "City_Detail_HLane_",
    "City_Detail_VEdge_", "City_Detail_HEdge_",
)
GROUND_LABELS = ("City_Ground",)
# Rebuilt from scratch by their own modules after this pass.
REBUILT_PREFIXES = ("City_Surface_",)
SKIP_LABELS = ("Sprawl_RoadMarkings",)

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "ExpandStreetsReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def old_block_center(index):
    return -OLD_SPAN / 2.0 + OLD_STEP / 2.0 + index * OLD_STEP


def new_block_center(index):
    return -NEW_SPAN / 2.0 + NEW_STEP / 2.0 + index * NEW_STEP


def remap_axis(value):
    """Map one coordinate, choosing the anchor by ownership."""
    # Nearest block centre on this axis.
    index = round((value - old_block_center(0)) / OLD_STEP)
    index = max(0, min(NUM_BLOCKS - 1, index))
    block_offset = value - old_block_center(index)

    if abs(block_offset) <= HALF_BLOCK:
        # Block-owned: preserve the block's internal layout exactly.
        return new_block_center(index) + block_offset, "block"

    # Road-owned: sits between two blocks, so scale within the carriageway.
    lower = index if block_offset > 0 else index - 1
    lower = max(0, min(NUM_BLOCKS - 2, lower))
    old_road = (old_block_center(lower) + old_block_center(lower + 1)) / 2.0
    new_road = (new_block_center(lower) + new_block_center(lower + 1)) / 2.0
    return new_road + (value - old_road) * ROAD_SCALE, "road"


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    out("actors_total={}".format(len(actors)))
    out("road {:.0f} -> {:.0f}, step {:.0f} -> {:.0f}, span {:.0f} -> {:.0f}".format(
        OLD_ROAD, NEW_ROAD, OLD_STEP, NEW_STEP, OLD_SPAN, NEW_SPAN))

    # --- retire actors that other modules rebuild ---
    removed = 0
    for actor in list(actors):
        if not actor:
            continue
        label = actor.get_actor_label()
        if label.startswith(LEGACY_MARKING_PREFIXES) or label.startswith(REBUILT_PREFIXES):
            actor_subsystem.destroy_actor(actor)
            removed += 1
    out("legacy/rebuilt actors removed: {}".format(removed))

    # --- remap the rest ---
    block_owned = 0
    road_owned = 0
    grounds = 0
    max_shift = 0.0
    for actor in actor_subsystem.get_all_level_actors():
        if not actor:
            continue
        label = actor.get_actor_label()
        if label.startswith(SKIP_LABELS):
            continue

        if label.startswith(GROUND_LABELS):
            scale = actor.get_actor_scale3d()
            factor = NEW_STEP / OLD_STEP
            actor.set_actor_scale3d(
                unreal.Vector(scale.x * factor, scale.y * factor, scale.z))
            grounds += 1
            continue

        location = actor.get_actor_location()
        new_x, kind_x = remap_axis(location.x)
        new_y, kind_y = remap_axis(location.y)
        max_shift = max(max_shift, abs(new_x - location.x), abs(new_y - location.y))
        actor.set_actor_location(
            unreal.Vector(new_x, new_y, location.z), False, False)
        if kind_x == "block" and kind_y == "block":
            block_owned += 1
        else:
            road_owned += 1

    out("")
    out("block-owned actors (layout preserved): {}".format(block_owned))
    out("road-owned actors (scaled in the carriageway): {}".format(road_owned))
    out("ground planes rescaled: {}, max shift {:.0f}cm".format(grounds, max_shift))

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    out("")
    out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
