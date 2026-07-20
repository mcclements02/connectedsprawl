"""Put kerbside objects back on their block after the street widening.

`expand_streets.py` mapped each actor to its *nearest* grid anchor. Anchors
alternate block-centre / road-centre every half step, so for anything in the
outer ~350cm of a block the road centre was nearer than the block centre — and
that band is exactly where kerbside trees, hydrants, benches, planters and
shopfronts live. Those objects scaled with the road instead of staying with
their block, which left them standing in the widened carriageway.

The mapping is invertible, so the correction is exact rather than a guess:

    old block half-width      = 1000     (blocks did not change size)
    old road half-width       =  300     (the whole old gap)
    old anchor spacing        = 1300     -> block objects sat 300..650 from
                                           the road centre after remapping
    new anchor spacing        = 1950
    required outward shift    = 1950 - 1300 = 650

So any object now sitting 280..700cm from a road centre on an axis was
block-owned and is moved 650cm further out on that axis, which lands it back
at its original distance from its block centre. Genuine road furniture
(manholes, painted markings) sits within 280cm and is left alone; parked cars
sit in the 825cm bays and are likewise untouched.

Idempotent: once corrected, nothing remains in the 280..700 band.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/fix_kerbside_anchoring.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"

# HISTORICAL ONE-SHOT. This repaired the 600 -> 1900 expansion, which used
# proximity anchoring. expand_streets.py now anchors by ownership, so the
# damage cannot recur and this correction is not needed. Its 280-700cm band
# would also straddle the current 1200cm carriageway and shove legitimate
# road furniture onto the pavement, so it refuses to run unless enabled.
ENABLED = False

NUM_BLOCKS = 7
BLOCK = 2000.0
ROAD = 1200.0        # two-way: 1 lane each way + parking bays
STEP = BLOCK + ROAD
SPAN = NUM_BLOCKS * STEP
NUM_ROADS = NUM_BLOCKS - 1

OLD_ANCHOR_HALF = (BLOCK + 600.0) / 2.0     # 1300
NEW_ANCHOR_HALF = STEP / 2.0                # 1950
OUTWARD_SHIFT = NEW_ANCHOR_HALF - OLD_ANCHOR_HALF   # 650

# Band occupied by mis-anchored block objects (old road half .. old block edge).
BAND_MIN = 280.0
BAND_MAX = 700.0

# Never move these: the ground plane, the marking actor, managers.
SKIP_PREFIXES = ("City_Ground", "Sprawl_RoadMarkings", "City_ParkedCar_")

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "KerbsideAnchoringReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def block_center(index):
    return -SPAN / 2.0 + STEP / 2.0 + index * STEP


def road_center(index):
    return (block_center(index) + block_center(index + 1)) / 2.0


def road_centers():
    return [road_center(i) for i in range(NUM_ROADS)]


def corrected(value, centers):
    """Shift a coordinate out of the carriageway and back onto its block."""
    for center in centers:
        offset = value - center
        distance = abs(offset)
        if BAND_MIN < distance <= BAND_MAX:
            direction = 1.0 if offset > 0 else -1.0
            return value + direction * OUTWARD_SHIFT, True
    return value, False


def main():
    if not ENABLED:
        out("disabled: historical one-shot for the 600->1900 expansion")
        return
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()
    centers = road_centers()

    out("actors_total={}".format(len(actors)))
    out("band {:.0f}..{:.0f}cm from a road centre -> shifted {:.0f}cm outward".format(
        BAND_MIN, BAND_MAX, OUTWARD_SHIFT))

    moved = 0
    by_prefix = {}
    for actor in actors:
        if not actor:
            continue
        label = actor.get_actor_label()
        if label.startswith(SKIP_PREFIXES):
            continue

        location = actor.get_actor_location()
        new_x, fixed_x = corrected(location.x, centers)
        new_y, fixed_y = corrected(location.y, centers)
        if not fixed_x and not fixed_y:
            continue

        actor.set_actor_location(
            unreal.Vector(new_x, new_y, location.z), False, False)
        moved += 1
        prefix = "_".join(label.split("_")[:3])
        by_prefix[prefix] = by_prefix.get(prefix, 0) + 1

    out("")
    out("objects returned to their block: {}".format(moved))
    for prefix, count in sorted(by_prefix.items(), key=lambda kv: -kv[1])[:25]:
        out("  {:6d}  {}".format(count, prefix))

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    out("")
    out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
