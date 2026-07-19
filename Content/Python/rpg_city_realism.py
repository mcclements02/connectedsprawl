"""Add RPG-developer-inspired street life and district identity to TestMap.

The pass is deliberately additive. It preserves the existing RealKit and
Downtown West work, clears only City_RPG_* actors, and reuses project assets.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject \
    -ExecutePythonScript="<absolute path>/rpg_city_realism.py" -unattended

Set CONNECTED_SPRAWL_REALISM_DRY_RUN=1 to validate assets without editing the
level. Actor spawning requires a rendered commandlet run (do not use -nullrhi).
"""
import os
import random

import unreal


SEED = 2077
PREFIX = "City_RPG_"
DRY_RUN = os.environ.get("CONNECTED_SPRAWL_REALISM_DRY_RUN") == "1"

N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
START = -N * STEP / 2.0 + STEP / 2.0
SIDEWALK_Z = 14.0

DW = "/Game/Downtown_West/Assets"
ASSET_PATHS = {
    "bench": DW + "/props/prop_bench_wood/SM_bench_wood_a",
    "chair": DW + "/props/prop_table/SM_prop_dining_chair_a",
    "food_cart": DW + "/props/prop_hotdog_cart/SM_food_cart",
    "fountain": DW + "/props/prop_fountain/SM_prop_fountain_full",
    "light_sign_a": DW + "/props/props_lights/SM_lamps_light_sign_a",
    "light_sign_b": DW + "/props/props_lights/SM_lamps_light_sign_b",
    "poster": DW + "/props/prop_poster_stands/SM_prop_poster_pillar_with_posters",
    "pot_a": DW + "/props/prop_pots/SM_pot_a",
    "pot_b": DW + "/props/prop_pots/SM_pot_b",
    "recycle": DW + "/props/prop_recycle_bin/SM_prop_recycle_bin",
    "statue": DW + "/props/prop_statue_eagle/SM_prop_statue_eagle",
    "table": DW + "/props/prop_table/SM_prop_dining_table_a",
    "table_umbrella": DW + "/props/prop_table/SM_prop_table_umbrella_a",
    "tarp": DW + "/props/prop_hanging_tarps/SM_tarp",
    "tarp_post_a": DW + "/props/prop_hanging_tarps/SM_tarppost_a",
    "tarp_post_b": DW + "/props/prop_hanging_tarps/SM_tarppost_b",
    "wood_fence_a": DW + "/props/prop_fence_wood/SM_wood_fence_wood_fence_a",
    "wood_fence_b": DW + "/props/prop_fence_wood/SM_wood_fence_wood_fence_b",
}

SIGN_PATHS = [
    DW + "/props/prop_store_signs/SM_signs_sign_bistro",
    DW + "/props/prop_store_signs/SM_signs_sign_cafe",
    DW + "/props/prop_store_signs/SM_signs_sign_coffeecup",
    DW + "/props/prop_store_signs/SM_signs_sign_deli",
    DW + "/props/prop_store_signs/SM_signs_sign_electronics",
    DW + "/props/prop_store_signs/SM_signs_sign_sandwich",
    DW + "/props/prop_store_signs/SM_signs_sign_shop",
    DW + "/props/prop_store_signs/SM_signs_sign_thrifty",
]


def cx(index):
    return START + index * STEP


def block_index(value):
    return int(round((value - START) / STEP))


def load_assets():
    assets = {name: unreal.load_asset(path) for name, path in ASSET_PATHS.items()}
    signs = [unreal.load_asset(path) for path in SIGN_PATHS]
    signs = [mesh for mesh in signs if mesh]
    missing = [name for name, mesh in assets.items() if not mesh]
    if missing:
        unreal.log_warning("[RPGCity] missing optional meshes: {}".format(", ".join(missing)))
    unreal.log("[RPGCity] loaded {} props and {} signs".format(
        len(assets) - len(missing), len(signs)))
    return assets, signs


def fit_ground(actor, ground_z):
    origin, extent = actor.get_actor_bounds(False)
    location = actor.get_actor_location()
    actor.set_actor_location(
        unreal.Vector(location.x, location.y, location.z + ground_z - (origin.z - extent.z)),
        False,
        False,
    )


def fit_center(actor, target):
    origin, _ = actor.get_actor_bounds(False)
    location = actor.get_actor_location()
    actor.set_actor_location(
        unreal.Vector(
            location.x + target.x - origin.x,
            location.y + target.y - origin.y,
            location.z + target.z - origin.z,
        ),
        False,
        False,
    )


def spawn_mesh(eas, mesh, label, location, yaw=0.0, ground=None, scale=1.0):
    if not mesh:
        return None
    actor = eas.spawn_actor_from_class(
        unreal.StaticMeshActor,
        location,
        unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0),
    )
    actor.set_actor_label(PREFIX + label)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_mobility(unreal.ComponentMobility.STATIC)
    if ground is not None:
        fit_ground(actor, ground)
    return actor


def building_bounds(actors):
    return [actor.get_actor_bounds(False) for actor in actors
            if actor.get_actor_label().startswith("City_Bldg")]


def point_is_clear(bounds, x, y, margin=80.0):
    for origin, extent in bounds:
        if (abs(x - origin.x) < extent.x + margin and
                abs(y - origin.y) < extent.y + margin):
            return False
    return True


def find_clear(bounds, candidates, margin=80.0):
    for x, y, yaw in candidates:
        if point_is_clear(bounds, x, y, margin):
            return x, y, yaw
    return None


def add_wall_signs(eas, actors, signs, randomizer):
    if not signs:
        return 0

    candidates = []
    for actor in actors:
        if not actor.get_actor_label().startswith("City_Bldg"):
            continue
        origin, extent = actor.get_actor_bounds(False)
        gx, gy = block_index(origin.x), block_index(origin.y)
        if not (2 <= gx <= 4 and 2 <= gy <= 4):
            continue
        bx, by = cx(gx), cx(gy)
        faces = [
            ("x", 1, origin.x + extent.x, bx + BLOCK / 2.0, extent.y),
            ("x", -1, origin.x - extent.x, bx - BLOCK / 2.0, extent.y),
            ("y", 1, origin.y + extent.y, by + BLOCK / 2.0, extent.x),
            ("y", -1, origin.y - extent.y, by - BLOCK / 2.0, extent.x),
        ]
        for axis, sign, face, street_edge, half_length in faces:
            if abs(face - street_edge) <= 430.0 and half_length >= 180.0:
                candidates.append((actor, axis, sign, face, half_length))

    candidates.sort(key=lambda item: (
        item[0].get_actor_location().x + 1300.0) ** 2 +
        item[0].get_actor_location().y ** 2)
    candidates = candidates[:18]

    made = 0
    for index, (actor, axis, sign, face, half_length) in enumerate(candidates):
        origin, _ = actor.get_actor_bounds(False)
        along = randomizer.uniform(-half_length * 0.35, half_length * 0.35)
        if axis == "x":
            target = unreal.Vector(face + sign * 18.0, origin.y + along, SIDEWALK_Z + 390.0)
            yaw = 0.0 if sign > 0 else 180.0
        else:
            target = unreal.Vector(origin.x + along, face + sign * 18.0, SIDEWALK_Z + 390.0)
            yaw = 90.0 if sign > 0 else 270.0
        sign_actor = spawn_mesh(
            eas, signs[index % len(signs)], "Junction_Sign_{:02d}".format(index),
            target, yaw=yaw)
        if sign_actor:
            fit_center(sign_actor, target)
            made += 1
    return made


def add_junction_activity(eas, assets, bounds):
    placements = [
        ("food_cart", "Junction_FoodCart", [(-955, 620, 270), (-1645, 620, 90)]),
        ("poster", "Junction_Poster", [(-955, 120, 270), (-1645, 120, 90)]),
        ("recycle", "Junction_Recycle", [(-955, -180, 0), (-1645, -180, 0)]),
        ("bench", "Junction_Bench", [(-955, -620, 90), (-1645, -620, 270)]),
        ("light_sign_a", "Junction_LightSignA", [(-950, 840, 270), (-1650, 840, 90)]),
        ("light_sign_b", "Junction_LightSignB", [(-950, -840, 270), (-1650, -840, 90)]),
    ]
    made = 0
    for key, label, candidates in placements:
        clear = find_clear(bounds, candidates, margin=45.0)
        if not clear:
            unreal.log_warning("[RPGCity] no clear Junction spot for {}".format(label))
            continue
        x, y, yaw = clear
        made += bool(spawn_mesh(
            eas, assets.get(key), label, unreal.Vector(x, y, SIDEWALK_Z + 5.0),
            yaw=yaw, ground=SIDEWALK_Z))

    cafe_candidates = [
        (-820, 760, 0), (-1780, 760, 180),
        (-820, -760, 0), (-1780, -760, 180),
    ]
    clear = find_clear(bounds, cafe_candidates, margin=120.0)
    if clear:
        x, y, yaw = clear
        made += bool(spawn_mesh(
            eas, assets.get("table_umbrella"), "Junction_CafeTable",
            unreal.Vector(x, y, SIDEWALK_Z + 5.0), yaw=yaw, ground=SIDEWALK_Z))
        for index, (dx, dy, chair_yaw) in enumerate((
                (120, 0, 90), (-120, 0, 270), (0, 120, 180))):
            made += bool(spawn_mesh(
                eas, assets.get("chair"), "Junction_CafeChair_{}".format(index),
                unreal.Vector(x + dx, y + dy, SIDEWALK_Z + 5.0),
                yaw=chair_yaw, ground=SIDEWALK_Z))
    return made


def add_district_anchors(eas, assets):
    made = 0

    # The Junction: a civic focal point within walking distance of spawn.
    made += bool(spawn_mesh(
        eas, assets.get("fountain"), "Junction_Fountain",
        unreal.Vector(cx(2), cx(2), SIDEWALK_Z + 5.0), ground=SIDEWALK_Z))

    # Iron Forest: clean, ordered public space and a vertical landmark.
    iron_x, iron_y = cx(5), cx(5)
    made += bool(spawn_mesh(
        eas, assets.get("statue"), "IronForest_Landmark",
        unreal.Vector(iron_x, iron_y, SIDEWALK_Z + 5.0),
        yaw=225.0, ground=SIDEWALK_Z, scale=1.8))
    for index, (dx, dy, yaw) in enumerate((
            (-420, -260, 45), (420, -260, 315), (-420, 260, 135), (420, 260, 225))):
        made += bool(spawn_mesh(
            eas, assets.get("pot_a" if index % 2 == 0 else "pot_b"),
            "IronForest_Planter_{}".format(index),
            unreal.Vector(iron_x + dx, iron_y + dy, SIDEWALK_Z + 5.0),
            yaw=yaw, ground=SIDEWALK_Z))

    # Rail Yards: a compact service edge with imperfect, reused materials.
    rail_x, rail_y = cx(1), cx(0)
    for index, (dx, mesh_key) in enumerate((
            (-620, "wood_fence_a"), (-210, "wood_fence_b"),
            (210, "wood_fence_a"), (620, "wood_fence_b"))):
        made += bool(spawn_mesh(
            eas, assets.get(mesh_key), "RailYards_Fence_{}".format(index),
            unreal.Vector(rail_x + dx, rail_y - 930.0, SIDEWALK_Z + 5.0),
            yaw=90.0, ground=SIDEWALK_Z))
    for index, (dx, key) in enumerate((
            (-330, "tarp_post_a"), (330, "tarp_post_b"))):
        made += bool(spawn_mesh(
            eas, assets.get(key), "RailYards_Tarp_{}".format(index),
            unreal.Vector(rail_x + dx, rail_y - 830.0, SIDEWALK_Z + 5.0),
            yaw=0.0, ground=SIDEWALK_Z))
    tarp_target = unreal.Vector(rail_x, rail_y - 830.0, SIDEWALK_Z + 410.0)
    tarp = spawn_mesh(
        eas, assets.get("tarp"), "RailYards_Tarp_Canopy", tarp_target, scale=0.9)
    if tarp:
        fit_center(tarp, tarp_target)
        made += 1
    made += bool(spawn_mesh(
        eas, assets.get("recycle"), "RailYards_Recycle",
        unreal.Vector(rail_x + 720.0, rail_y - 820.0, SIDEWALK_Z + 5.0),
        ground=SIDEWALK_Z))
    return made


def add_activity_pedestrians(eas, bounds):
    location_choices = [
        [(-940, 480, 180), (-1660, 480, 0)],
        [(-940, -430, 0), (-1660, -430, 180)],
        [(-940, 650, 200), (-1660, 650, 20)],
        [(-940, -650, 340), (-1660, -650, 160)],
        [(-920, 820, 220), (-1680, 820, 40)],
        [(-920, -820, 310), (-1680, -820, 130)],
        [(-900, 260, 160), (-1700, 260, 20)],
        [(-900, -260, 20), (-1700, -260, 160)],
    ]
    made = 0
    for index, candidates in enumerate(location_choices):
        clear = find_clear(bounds, candidates, margin=45.0)
        if not clear:
            unreal.log_warning("[RPGCity] no clear pedestrian spot {}".format(index))
            continue
        x, y, yaw = clear
        actor = eas.spawn_actor_from_class(
            unreal.SprawlPedestrian,
            unreal.Vector(x, y, 120.0),
            unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0),
        )
        if actor:
            actor.set_actor_label(PREFIX + "Junction_Ped_{:02d}".format(index))
            made += 1
    return made


def tune_lighting(eas):
    tuned = []
    skylight = None
    actors = eas.get_all_level_actors()
    for actor in actors:
        label = actor.get_actor_label()
        class_name = actor.get_class().get_name()
        if label == "FixDirectionalLight" or class_name == "DirectionalLight":
            actor.set_actor_rotation(
                unreal.Rotator(pitch=-32.0, yaw=-38.0, roll=0.0), False)
            component = actor.light_component
            component.set_editor_property("intensity", 5.2)
            component.set_light_color(unreal.LinearColor(1.0, 0.86, 0.70, 1.0))
            component.set_editor_property("atmosphere_sun_light", True)
            tuned.append("sun")
            break

    for actor in actors:
        label = actor.get_actor_label()
        if label == "FixSkyLight":
            component = actor.light_component
            component.set_editor_property("intensity", 1.85)
            component.set_editor_property("real_time_capture", False)
            skylight = component
            tuned.append("sky")
        elif label == "FixHeightFog":
            component = actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
            if component:
                component.set_editor_property("fog_density", 0.006)
                component.set_editor_property("fog_height_falloff", 0.16)
                component.set_editor_property("start_distance", 1200.0)
                tuned.append("fog")
        elif label == "FixPostProcess":
            settings = actor.settings
            values = {
                "auto_exposure_bias": 0.70,
                "auto_exposure_min_brightness": 1.0,
                "auto_exposure_max_brightness": 1.0,
                "ambient_occlusion_intensity": 0.50,
                "ambient_occlusion_radius": 150.0,
                "bloom_intensity": 0.14,
                "film_grain_intensity": 0.0,
                "lens_flare_intensity": 0.12,
                "vignette_intensity": 0.15,
                "white_temp": 6200.0,
                "color_contrast": unreal.Vector4(1.03, 1.03, 1.03, 1.0),
                "color_saturation": unreal.Vector4(1.02, 1.02, 1.02, 1.0),
                "scene_color_tint": unreal.LinearColor(1.06, 1.0, 0.94, 1.0),
            }
            for name, value in values.items():
                settings.set_editor_property("override_" + name, True)
                settings.set_editor_property(name, value)
            actor.settings = settings
            tuned.append("grade")

    if skylight:
        skylight.recapture_sky()
    return tuned


def add_junction_lights(eas):
    made = 0
    for index, (x, y) in enumerate((
            (-970, 620), (-970, -620), (-1630, 620), (-1630, -620))):
        actor = eas.spawn_actor_from_class(
            unreal.PointLight, unreal.Vector(x, y, 330.0), unreal.Rotator())
        if not actor:
            continue
        actor.set_actor_label(PREFIX + "Junction_Light_{:02d}".format(index))
        component = actor.light_component
        component.set_intensity(300.0)
        component.set_light_color(unreal.LinearColor(1.0, 0.48, 0.20, 1.0))
        component.set_editor_property("attenuation_radius", 550.0)
        component.set_editor_property("cast_shadows", False)
        made += 1
    return made


def report_dry_run(assets, signs, actors):
    for name, mesh in assets.items():
        if mesh:
            extent = mesh.get_bounds().box_extent
            unreal.log("[RPGCity][dry] {} extent=({:.0f}, {:.0f}, {:.0f})".format(
                name, extent.x, extent.y, extent.z))
    for mesh in signs:
        extent = mesh.get_bounds().box_extent
        unreal.log("[RPGCity][dry] sign {} extent=({:.0f}, {:.0f}, {:.0f})".format(
            mesh.get_name(), extent.x, extent.y, extent.z))
    unreal.log("[RPGCity][dry] buildings={} signs={} existing_rpg={}".format(
        len([a for a in actors if a.get_actor_label().startswith("City_Bldg")]),
        len(signs),
        len([a for a in actors if a.get_actor_label().startswith(PREFIX)]),
    ))


def main():
    randomizer = random.Random(SEED)
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les.load_level("/Game/Maps/TestMap")
    actors = list(eas.get_all_level_actors())
    assets, signs = load_assets()

    if DRY_RUN:
        report_dry_run(assets, signs, actors)
        return

    cleared = 0
    for actor in actors:
        if actor.get_actor_label().startswith(PREFIX):
            eas.destroy_actor(actor)
            cleared += 1
    actors = list(eas.get_all_level_actors())
    bounds = building_bounds(actors)

    signs_made = add_wall_signs(eas, actors, signs, randomizer)
    activity_made = add_junction_activity(eas, assets, bounds)
    anchors_made = add_district_anchors(eas, assets)
    pedestrians_made = add_activity_pedestrians(eas, bounds)
    local_lights_made = add_junction_lights(eas)
    lighting_tuned = tune_lighting(eas)

    saved = les.save_current_level()
    unreal.log(
        "[RPGCity] cleared={} signs={} activity={} anchors={} pedestrians={} "
        "local_lights={} lighting={} saved={}".format(
            cleared, signs_made, activity_made, anchors_made, pedestrians_made,
            local_lights_made, ",".join(lighting_tuned), saved))


main()
