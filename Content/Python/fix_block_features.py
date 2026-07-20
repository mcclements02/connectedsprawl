"""Keep park and plaza features inside their block, square to the grid.

A park deck was found lying diagonally across a junction. Two causes, both
structural rather than cosmetic:

  1. SIZE. `City_DW3_playground` measures 2030 x 2658cm against a 2000cm
     block. A feature larger than its block cannot sit in it, so it spills
     into the carriageway no matter where it is centred.
  2. ANGLE. Features were authored at arbitrary yaw. The city is an
     axis-aligned grid, so anything off-axis presents diagonal edges that cut
     across the kerb line even when it would otherwise fit.

So each feature is squared to the grid (yaw snapped to the nearest 90 deg),
scaled down until its footprint fits the block interior, and recentred inside
it. The interior is the block minus the sidewalk ring, which is the area a
feature is actually allowed to occupy:

    interior half-width = BlockSize/2 - SIDEWALK_WIDTH = 600

Report-only when APPLY is False. Idempotent: a contained feature is skipped.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/fix_block_features.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"
APPLY = True

NUM_BLOCKS = 7
BLOCK = 2000.0
ROAD = 1200.0
STEP = BLOCK + ROAD
SPAN = NUM_BLOCKS * STEP

SIDEWALK_WIDTH = 400.0
INTERIOR_HALF = BLOCK / 2.0 - SIDEWALK_WIDTH   # 600
MARGIN = 20.0

# Block features: things that furnish a block's interior. Buildings, kerbside
# props and road furniture are owned by their own modules.
FEATURE_PREFIXES = (
    "City_DW3_playground", "City_DW_playground",
    "City_DW3_park", "City_DW_park",
    "City_DW3_deck", "City_DW_deck", "City_R2_deck",
    "City_DW3_plaza", "City_DW_plaza",
    # NOT City_RPG_Junction: despite reading like a plaza, these belong at
    # road junctions. Including them here recentred 25 of them onto block
    # centres before that was caught — verify a prefix before adding it.
)

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "BlockFeaturesReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def block_center(index):
    return -SPAN / 2.0 + STEP / 2.0 + index * STEP


def nearest_block_center(value):
    index = round((value - block_center(0)) / STEP)
    index = max(0, min(NUM_BLOCKS - 1, index))
    return block_center(index)


def snapped_yaw(yaw):
    return round(unreal.MathLibrary.normalize_axis(yaw) / 90.0) * 90.0


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()

    out("actors_total={}".format(len(actors)))
    out("block interior: {:.0f}cm half-width (block {:.0f} minus {:.0f} sidewalk)".format(
        INTERIOR_HALF, BLOCK / 2.0, SIDEWALK_WIDTH))

    examined = 0
    squared = 0
    resized = 0
    recentred = 0

    for actor in actors:
        if not actor or not actor.get_actor_label().startswith(FEATURE_PREFIXES):
            continue
        examined += 1
        label = actor.get_actor_label()

        rotation = actor.get_actor_rotation()
        target_yaw = snapped_yaw(rotation.yaw)
        needs_square = (abs(unreal.MathLibrary.normalize_axis(rotation.yaw - target_yaw)) > 0.5
                        or abs(unreal.MathLibrary.normalize_axis(rotation.roll)) > 0.5
                        or abs(unreal.MathLibrary.normalize_axis(rotation.pitch)) > 0.5)
        if needs_square and APPLY:
            actor.set_actor_rotation(
                unreal.Rotator(roll=0.0, pitch=0.0, yaw=target_yaw), False)
        if needs_square:
            squared += 1

        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        widest = max(extent.x, extent.y)
        if widest <= 0.0:
            continue

        allowed = INTERIOR_HALF - MARGIN
        if widest > allowed:
            # Too big for any block: shrink until the footprint fits.
            factor = allowed / widest
            scale = actor.get_actor_scale3d()
            if APPLY:
                actor.set_actor_scale3d(unreal.Vector(
                    scale.x * factor, scale.y * factor, scale.z * factor))
            resized += 1
            out("  {:<36} {:.0f}cm wide -> scaled x{:.2f}".format(
                label, widest * 2.0, factor))
            origin, extent = actor.get_actor_bounds(only_colliding_components=False)

        # Recentre so the whole footprint sits inside the block interior.
        location = actor.get_actor_location()
        target = unreal.Vector(nearest_block_center(location.x),
                               nearest_block_center(location.y), location.z)
        drift = max(abs(location.x - target.x), abs(location.y - target.y))
        if drift > 1.0:
            if APPLY:
                # Preserve the offset between the actor pivot and its bounds.
                actor.set_actor_location(unreal.Vector(
                    target.x + (location.x - origin.x),
                    target.y + (location.y - origin.y),
                    location.z), False, False)
            recentred += 1

    out("")
    out("features examined: {}".format(examined))
    out("  squared to the grid: {}".format(squared))
    out("  scaled to fit the block: {}".format(resized))
    out("  recentred on their block: {}".format(recentred))

    if APPLY:
        unreal.get_editor_subsystem(
            unreal.LevelEditorSubsystem).save_current_level()
        out("")
        out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
