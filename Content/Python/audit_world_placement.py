"""Diagnostic: find level actors that are tilted, sunken, floating, or sitting
at character height where they can intersect a walking pedestrian.

Read-only. Writes Saved/WorldPlacementReport.txt because the UE 5.8
pythonscript commandlet swallows LogPython output.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/audit_world_placement.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"
TILT_TOLERANCE_DEG = 1.0
# A pedestrian capsule spans roughly 0..184 cm; anything thin and flat sitting
# inside that band on a walkable surface will visually cut through a character.
CHARACTER_LOW = 6.0
CHARACTER_HIGH = 190.0
FLAT_RATIO = 0.12   # thickness vs. footprint that reads as "a painted marking"

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "WorldPlacementReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def load_level():
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsystem.load_level(MAP_PATH)


def actor_bounds(actor):
    try:
        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        return origin, extent
    except Exception:
        return None, None


def main():
    load_level()
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()
    out("actors_total={}".format(len(actors)))

    by_class = {}
    tilted = []
    flat_in_character_band = []
    sunken = []
    floating = []

    for actor in actors:
        if not actor:
            continue
        class_name = actor.get_class().get_name()
        by_class[class_name] = by_class.get(class_name, 0) + 1

        label = actor.get_actor_label()
        rotation = actor.get_actor_rotation()
        location = actor.get_actor_location()
        roll = abs(unreal.MathLibrary.normalize_axis(rotation.roll))
        pitch = abs(unreal.MathLibrary.normalize_axis(rotation.pitch))

        if roll > TILT_TOLERANCE_DEG or pitch > TILT_TOLERANCE_DEG:
            tilted.append((label, class_name, roll, pitch, rotation.yaw, location))

        origin, extent = actor_bounds(actor)
        if origin is None or extent is None:
            continue
        if extent.x <= 0.0 and extent.y <= 0.0 and extent.z <= 0.0:
            continue

        bottom = origin.z - extent.z
        top = origin.z + extent.z
        footprint = max(extent.x, extent.y)
        thickness = extent.z

        # Thin, flat and hovering inside the walking band: paint or signage that
        # will slice through a pedestrian rather than lie on the ground.
        if (footprint > 20.0 and thickness < max(4.0, footprint * FLAT_RATIO)
                and bottom > CHARACTER_LOW and bottom < CHARACTER_HIGH):
            flat_in_character_band.append(
                (label, class_name, bottom, top, footprint, thickness, location))

        if top < -40.0:
            sunken.append((label, class_name, top, location))
        # Props (not buildings/lights) resting well off the ground.
        if bottom > 260.0 and footprint < 600.0 and thickness < 400.0:
            floating.append((label, class_name, bottom, footprint, location))

    out("")
    out("=== class histogram (top 30) ===")
    for name, count in sorted(by_class.items(), key=lambda kv: -kv[1])[:30]:
        out("{:6d}  {}".format(count, name))

    out("")
    out("=== tilted actors (|roll| or |pitch| > {} deg): {} ===".format(
        TILT_TOLERANCE_DEG, len(tilted)))
    for label, class_name, roll, pitch, yaw, location in tilted[:60]:
        out("  {:<44} {:<26} roll={:7.2f} pitch={:7.2f} yaw={:7.2f} "
            "loc=({:.0f},{:.0f},{:.0f})".format(
                label, class_name, roll, pitch, yaw,
                location.x, location.y, location.z))

    out("")
    out("=== flat props inside the walking band: {} ===".format(
        len(flat_in_character_band)))
    for entry in flat_in_character_band[:60]:
        label, class_name, bottom, top, footprint, thickness, location = entry
        out("  {:<44} {:<26} bottom_z={:7.1f} top_z={:7.1f} footprint={:6.0f} "
            "thickness={:5.1f} loc=({:.0f},{:.0f},{:.0f})".format(
                label, class_name, bottom, top, footprint, thickness,
                location.x, location.y, location.z))

    out("")
    out("=== sunken actors (entirely below z=-40): {} ===".format(len(sunken)))
    for label, class_name, top, location in sunken[:40]:
        out("  {:<44} {:<26} top_z={:8.1f} loc=({:.0f},{:.0f},{:.0f})".format(
            label, class_name, top, location.x, location.y, location.z))

    out("")
    out("=== floating props (bottom_z > 260): {} ===".format(len(floating)))
    for label, class_name, bottom, footprint, location in floating[:40]:
        out("  {:<44} {:<26} bottom_z={:8.1f} footprint={:6.0f} "
            "loc=({:.0f},{:.0f},{:.0f})".format(
                label, class_name, bottom, footprint,
                location.x, location.y, location.z))


try:
    main()
finally:
    flush()
