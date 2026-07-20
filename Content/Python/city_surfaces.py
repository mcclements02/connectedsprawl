"""Give the city three readable ground surfaces: street, pavement and grass.

The world sits on one large asphalt plane, so after the street widening the
carriageway and the pavement were the same flat grey — there was no visual
kerb line and no sense of where it is safe to walk.

This lays one surface slab per block, sitting on the ground and rising to the
kerb height the props are already placed against:

  * park blocks      -> grass
  * every other block -> pavement
  * the gaps between blocks are left bare, so the ground plane reads as the
    street, framed by the kerbs that already ring each block

Blocks under the lake are skipped. Idempotent: previous surface slabs are
removed before new ones are laid, so re-running replaces rather than stacks.

NOTE: this spawns actors, so it must run WITHOUT -nullrhi (actor spawning
asserts in the null RHI).

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/city_surfaces.py" -unattended -nosplash -stdout \
    -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"

# Must match USprawlCityGridSubsystem.
NUM_BLOCKS = 7
BLOCK = 2000.0
ROAD = 1200.0        # two-way: 1 lane each way + parking bays
STEP = BLOCK + ROAD
SPAN = NUM_BLOCKS * STEP

# Kerb height the existing props and curbs are built against.
SIDEWALK_Z = 14.0
CUBE = "/Engine/BasicShapes/Cube.Cube"
PAVEMENT_MATERIAL = "/Game/CityArt/MI_Sidewalk"
GRASS_MATERIAL = "/Game/CityArt/MI_Grass"

# Authored park blocks (see realkit_apply.py) and the lake footprint.
PARKS = {(1, 5), (2, 2), (5, 5)}
LAKE_GX, LAKE_GY = {4, 5, 6}, {0, 1}

SURFACE_PREFIX = "City_Surface_"

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "CitySurfacesReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def block_center(index):
    return -SPAN / 2.0 + STEP / 2.0 + index * STEP


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    cube = unreal.load_asset(CUBE)
    pavement = unreal.load_asset(PAVEMENT_MATERIAL)
    grass = unreal.load_asset(GRASS_MATERIAL)
    if not cube:
        out("FAILED: cube mesh missing")
        return
    out("pavement={} grass={}".format(
        "ok" if pavement else "MISSING", "ok" if grass else "MISSING"))

    # --- clear any previous pass so this stays idempotent ---
    removed = 0
    for actor in list(actor_subsystem.get_all_level_actors()):
        if actor and actor.get_actor_label().startswith(SURFACE_PREFIX):
            actor_subsystem.destroy_actor(actor)
            removed += 1
    out("previous surface slabs removed: {}".format(removed))

    # --- lay one slab per block ---
    # A slab spans the block footprint, its top flush with the kerb so props
    # already seated at SIDEWALK_Z rest exactly on it.
    thickness = SIDEWALK_Z
    scale = unreal.Vector(BLOCK / 100.0, BLOCK / 100.0, thickness / 100.0)
    laid_pavement = 0
    laid_grass = 0

    for gx in range(NUM_BLOCKS):
        for gy in range(NUM_BLOCKS):
            if gx in LAKE_GX and gy in LAKE_GY:
                continue

            is_park = (gx, gy) in PARKS
            material = grass if is_park else pavement
            if material is None:
                continue

            location = unreal.Vector(block_center(gx), block_center(gy),
                                     thickness / 2.0)
            actor = actor_subsystem.spawn_actor_from_class(
                unreal.StaticMeshActor, location, unreal.Rotator(0.0, 0.0, 0.0))
            if not actor:
                continue
            actor.set_actor_label("{}{}_{}_{}".format(
                SURFACE_PREFIX, "Grass" if is_park else "Pavement", gx, gy))
            actor.set_actor_scale3d(scale)
            component = actor.static_mesh_component
            component.set_static_mesh(cube)
            component.set_material(0, material)
            component.set_mobility(unreal.ComponentMobility.STATIC)
            # Pedestrians and cars collide with the ground plane already; the
            # slab is dressing and must not add a lip for them to catch on.
            component.set_collision_profile_name("NoCollision")

            if is_park:
                laid_grass += 1
            else:
                laid_pavement += 1

    out("")
    out("pavement blocks: {}".format(laid_pavement))
    out("grass blocks:    {}".format(laid_grass))
    out("street left bare so the ground plane reads as asphalt between kerbs")

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    out("")
    out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
