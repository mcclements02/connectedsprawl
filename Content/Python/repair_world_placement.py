"""Repair module: stand the city's props back up and lay its road paint flat.

Two authoring bugs are corrected in the saved level:

1. TIPPED PROPS. `unreal.Rotator` takes (roll, pitch, yaw), but the placement
   scripts called `unreal.Rotator(0, yaw, 0)` positionally, so every prop's
   intended yaw landed in PITCH and tipped it face-down. The intended angle is
   recovered from the normalized rotation and moved back onto yaw, then the
   prop is re-grounded by tracing to the surface underneath it.

2. FLOATING ROAD PAINT. Lane markings were pushed up to z=11 to dodge a
   z-fight, which left them hovering ~9 cm over the asphalt — high enough to
   slice through a pedestrian's ankles. They are lowered onto the road, just
   under the C++ `ASprawlRoadMarkings` paint plane (z=1.5) so the two systems
   layer cleanly instead of fighting.

Idempotent and non-destructive: nothing is spawned or deleted, only
transforms are corrected. Writes Saved/WorldRepairReport.txt.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/repair_world_placement.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"
TILT_TOLERANCE_DEG = 0.5

# Everything that can only be correct standing vertically. The same positional
# Rotator slip appears in realkit_apply.py, dw_realism2.py and dw_city_v3.py,
# so street furniture, trees, storefronts and building trim are all affected.
UPRIGHT_PREFIXES = (
    "City_RealProp_",
    "City_DW_tree", "City_DW3_tree",
    "City_DW_store", "City_DW3_store",
    "City_DW3_lvl2", "City_DW3_trim",
    "City_DW_chair", "City_DW_tableumb", "City_DW_foodcart",
    "City_DW3_fold", "City_DW3_playground", "City_DW3_mountains",
    "City_DW3_sign",
    "City_R2_recycle", "City_R2_pots",
)

# Deliberately untouched: awnings are authored with a real roll so they slope,
# and the R2 cross/grime/poster actors are decals whose orientation is their
# projection direction. Both are reported instead of guessed at.
REVIEW_PREFIXES = ("City_DW_awning", "City_DW3_awning",
                   "City_R2_cross", "City_R2_grime", "City_R2_poster")

# Surface heights the authoring scripts place against (realkit_apply.py,
# dw_city_v3.py, dw_realism2.py all agree). The city is flat, so these are
# exact — no ground tracing required.
SIDEWALK_Z = 14.0
ROAD_Z = 1.5
# Props the placement script explicitly seats on the asphalt.
ROAD_PROP_TOKENS = ("manhole", "barrier")
# Only snap a non-prop onto the sidewalk when it is already close to it;
# rooftop dressing and the backdrop keep whatever base they have.
MAX_SIDEWALK_SNAP = 200.0

# Flat painted markings that must lie on the asphalt.
PAINT_PREFIXES = ("City_Detail_VCenter_", "City_Detail_HCenter_",
                  "City_Detail_VLane_", "City_Detail_HLane_",
                  "City_Detail_VEdge_", "City_Detail_HEdge_")
# Road surface tops out at z=0; the C++ paint occupies 0..3. Sitting the legacy
# markings just below that keeps both visible with no coplanar flicker.
PAINT_TOP_Z = 2.8
FALLBACK_GROUND_Z = 14.0

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(), "WorldRepairReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def normalize(angle):
    return unreal.MathLibrary.normalize_axis(angle)


def label_prefix(label):
    parts = label.split("_")
    return "_".join(parts[:3]) if len(parts) >= 3 else label


def recovered_yaw(rotation):
    """Undo the (roll, pitch, yaw) argument-order slip.

    The intended yaw was written into pitch. Values past 90 deg normalize into
    the mirrored form (roll=180, pitch=180-intended, yaw=180), so unmirror
    those before reading the angle back out.
    """
    roll = normalize(rotation.roll)
    pitch = normalize(rotation.pitch)
    yaw = normalize(rotation.yaw)
    if abs(abs(roll) - 180.0) < 1.0 and abs(abs(yaw) - 180.0) < 1.0:
        intended = 180.0 - pitch
    else:
        intended = pitch
    return intended % 360.0


def target_base_z(label, base_before):
    """Surface this actor should be resting its base on."""
    if label.startswith("City_RealProp_"):
        # realkit_apply.place() seats every prop on one of two known surfaces.
        return ROAD_Z if any(token in label for token in ROAD_PROP_TOKENS) \
            else SIDEWALK_Z
    # Street dressing sits on the sidewalk; anything far from it (rooftop
    # units, the distant backdrop) keeps the base it already has.
    if abs(base_before - SIDEWALK_Z) <= MAX_SIDEWALK_SNAP:
        return SIDEWALK_Z
    return base_before


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()
    out("actors_total={}".format(len(actors)))

    # --- survey every tilt in the level, so nothing is silently left behind ---
    tilt_by_prefix = {}
    for actor in actors:
        if not actor:
            continue
        rotation = actor.get_actor_rotation()
        if (abs(normalize(rotation.roll)) > TILT_TOLERANCE_DEG
                or abs(normalize(rotation.pitch)) > TILT_TOLERANCE_DEG):
            prefix = label_prefix(actor.get_actor_label())
            tilt_by_prefix[prefix] = tilt_by_prefix.get(prefix, 0) + 1

    out("")
    out("=== tilted actors by label prefix (before) ===")
    for prefix, count in sorted(tilt_by_prefix.items(), key=lambda kv: -kv[1]):
        if prefix.startswith(UPRIGHT_PREFIXES):
            handled = "repair"
        elif prefix.startswith(REVIEW_PREFIXES):
            handled = "left as authored (sloped/decal)"
        else:
            handled = "SKIPPED"
        out("  {:6d}  {:<34} {}".format(count, prefix, handled))

    # --- pass 1: stand things up, then re-seat them on the ground ---
    uprighted = 0
    regrounded = 0
    trace_misses = 0
    for actor in actors:
        if not actor or not actor.get_actor_label().startswith(UPRIGHT_PREFIXES):
            continue
        rotation = actor.get_actor_rotation()
        if (abs(normalize(rotation.roll)) <= TILT_TOLERANCE_DEG
                and abs(normalize(rotation.pitch)) <= TILT_TOLERANCE_DEG):
            continue

        label = actor.get_actor_label()
        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        base_before = origin.z - extent.z

        actor.set_actor_rotation(
            unreal.Rotator(roll=0.0, pitch=0.0, yaw=recovered_yaw(rotation)),
            False)
        uprighted += 1

        # Standing an object up changes its footprint, so re-seat it on the
        # surface it belongs to afterwards.
        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        if extent.z <= 0.0:
            continue
        delta = target_base_z(label, base_before) - (origin.z - extent.z)
        if abs(delta) > 1.0:
            location = actor.get_actor_location()
            actor.set_actor_location(
                unreal.Vector(location.x, location.y, location.z + delta),
                False, False)
            regrounded += 1

    out("")
    out("=== pass 1: upright + reseat ===")
    out("  uprighted={} reseated={}".format(uprighted, regrounded))

    # --- pass 2: lay the road paint back down onto the asphalt ---
    lowered = 0
    for actor in actors:
        if not actor or not actor.get_actor_label().startswith(PAINT_PREFIXES):
            continue
        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        if extent.z <= 0.0:
            continue
        top_z = origin.z + extent.z
        delta = PAINT_TOP_Z - top_z
        if abs(delta) > 0.25:
            location = actor.get_actor_location()
            actor.set_actor_location(
                unreal.Vector(location.x, location.y, location.z + delta),
                False, False)
            lowered += 1

    out("")
    out("=== pass 2: road paint ===")
    out("  lowered={} to top_z={}".format(lowered, PAINT_TOP_Z))

    unreal.get_editor_subsystem(
        unreal.LevelEditorSubsystem).save_current_level()
    out("")
    out("saved {}".format(MAP_PATH))


try:
    main()
finally:
    flush()
