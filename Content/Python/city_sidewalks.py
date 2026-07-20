"""Lay a proper sidewalk ring around every block.

`city_surfaces.py` gives each block a single surface — pavement, or grass on a
park block. That leaves a park meeting the carriageway with no pavement at
all: grass runs straight to the kerb and there is nowhere to walk.

Real blocks have a pavement ring at their perimeter regardless of what fills
the middle. This lays that ring — four strips per block, inboard of the kerb —
sitting just proud of the block surface so it reads as the walking surface
over grass or paving alike.

Idempotent: previous rings are removed before new ones are laid.

NOTE: spawns actors, so run WITHOUT -nullrhi.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/city_sidewalks.py" -unattended -nosplash -stdout \
    -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"

# Must match USprawlCityGridSubsystem.
NUM_BLOCKS = 7
BLOCK = 2000.0
ROAD = 1200.0
STEP = BLOCK + ROAD
SPAN = NUM_BLOCKS * STEP

SIDEWALK_WIDTH = 400.0   # matches the walking band the building line respects
SIDEWALK_Z = 14.0        # kerb height props are seated against
PROUD = 0.5              # sits just over the block surface, no z-fighting

CUBE = "/Engine/BasicShapes/Cube.Cube"
PAVEMENT_MATERIAL = "/Game/CityArt/MI_Sidewalk"
LAKE_GX, LAKE_GY = {4, 5, 6}, {0, 1}
PREFIX = "City_Sidewalk_"

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "CitySidewalksReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def block_center(index):
    return -SPAN / 2.0 + STEP / 2.0 + index * STEP


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    cube = unreal.load_asset(CUBE)
    pavement = unreal.load_asset(PAVEMENT_MATERIAL)
    if not cube or not pavement:
        out("FAILED: cube={} pavement={}".format(bool(cube), bool(pavement)))
        return

    removed = 0
    for actor in list(actor_subsystem.get_all_level_actors()):
        if actor and actor.get_actor_label().startswith(PREFIX):
            actor_subsystem.destroy_actor(actor)
            removed += 1
    out("previous sidewalk strips removed: {}".format(removed))

    half = BLOCK / 2.0
    inner = half - SIDEWALK_WIDTH
    top = SIDEWALK_Z + PROUD
    laid = 0

    for gx in range(NUM_BLOCKS):
        for gy in range(NUM_BLOCKS):
            if gx in LAKE_GX and gy in LAKE_GY:
                continue
            cx, cy = block_center(gx), block_center(gy)

            # Four strips: two full-width along X, two shortened along Y so the
            # corners are covered exactly once.
            strips = (
                (cx, cy + (half + inner) / 2.0, BLOCK, SIDEWALK_WIDTH),
                (cx, cy - (half + inner) / 2.0, BLOCK, SIDEWALK_WIDTH),
                (cx + (half + inner) / 2.0, cy, SIDEWALK_WIDTH, 2.0 * inner),
                (cx - (half + inner) / 2.0, cy, SIDEWALK_WIDTH, 2.0 * inner),
            )
            for index, (sx, sy, size_x, size_y) in enumerate(strips):
                actor = actor_subsystem.spawn_actor_from_class(
                    unreal.StaticMeshActor,
                    unreal.Vector(sx, sy, top - SIDEWALK_Z / 2.0),
                    unreal.Rotator(0.0, 0.0, 0.0))
                if not actor:
                    continue
                actor.set_actor_label("{}{}_{}_{}".format(PREFIX, gx, gy, index))
                actor.set_actor_scale3d(unreal.Vector(
                    size_x / 100.0, size_y / 100.0, SIDEWALK_Z / 100.0))
                component = actor.static_mesh_component
                component.set_static_mesh(cube)
                component.set_material(0, pavement)
                component.set_mobility(unreal.ComponentMobility.STATIC)
                component.set_collision_profile_name("NoCollision")
                laid += 1

    out("sidewalk strips laid: {} ({}cm wide ring per block)".format(
        laid, SIDEWALK_WIDTH))

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    out("")
    out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
