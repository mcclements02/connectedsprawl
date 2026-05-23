"""Find and apply imported Fab/City Sample city assets.

Use this after adding marketplace packs to the project through Fab/Epic
Launcher. It does not assume exact package paths. Instead, it searches the
asset registry for likely vehicle, building, and road meshes, then swaps
drivable City_DriveCar_* actors to the best vehicle meshes it finds.

Recommended packs:
  - City Sample Vehicles
  - City Sample Buildings
  - Urban Roads Pack or another modular road/street pack
"""
import os
import unreal


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ar = unreal.AssetRegistryHelpers.get_asset_registry()

les.load_level("/Game/Maps/TestMap")


REPORT_PATH = os.path.abspath(os.path.join(
    unreal.Paths.project_saved_dir(),
    "FabCityAssetCandidates.txt"))


VEHICLE_NAME_TERMS = [
    "sedan", "suv", "taxi", "hatch", "coupe", "car", "vehicle",
    "van", "pickup", "truck", "bus",
]
VEHICLE_PATH_TERMS = [
    "citysample", "city_sample", "vehicle", "vehicles", "traffic", "car",
]
BUILDING_TERMS = [
    "building", "facade", "storefront", "shop", "corner", "block",
    "tower", "office", "apartment",
]
ROAD_TERMS = [
    "road", "street", "intersection", "sidewalk", "curb", "lane",
    "trafficlight", "traffic_light", "sign", "lamp", "streetlight",
]
EXCLUDE_TERMS = [
    "material", "texture", "wheel", "tire", "tyre", "glass", "window",
    "door", "socket", "collision", "proxy", "lod", "preview", "icon",
    "template", "controlrig", "metahuman", "mediaplate", "editor",
]
EXCLUDE_PATH_PREFIXES = [
    "/Engine/",
    "/ControlRigModules/",
    "/MetaHumanSDK/",
    "/MediaPlate/",
]


def class_name(asset_data):
    if hasattr(asset_data, "asset_class_path"):
        return str(asset_data.asset_class_path.asset_name)
    if hasattr(asset_data, "asset_class"):
        return str(asset_data.asset_class)
    return ""


def text_for(asset_data):
    return "{} {}".format(asset_data.asset_name, asset_data.package_name).lower()


def has_any(text, terms):
    return any(t in text for t in terms)


def score_asset(asset_data, terms, path_terms):
    text = text_for(asset_data)
    package_path = str(asset_data.package_name)
    if any(package_path.startswith(prefix) for prefix in EXCLUDE_PATH_PREFIXES):
        return 0
    if has_any(text, EXCLUDE_TERMS):
        return 0

    score = 0
    core_hit = False
    for term in terms:
        if term in text:
            score += 3
            core_hit = True
    for term in path_terms:
        if term in text:
            score += 2
            core_hit = True
    if not core_hit:
        return 0
    if "citysample" in text or "city_sample" in text:
        score += 5
    if str(asset_data.asset_name).startswith("SM_"):
        score += 1
    if str(asset_data.asset_name).startswith("SK_"):
        score += 1
    return score


def find_candidates(allowed_classes, terms, path_terms, limit=40):
    found = []
    for data in ar.get_all_assets(False):
        cls = class_name(data)
        if cls not in allowed_classes:
            continue
        score = score_asset(data, terms, path_terms)
        if score <= 0:
            continue
        found.append((score, str(data.package_name), str(data.asset_name), cls))
    found.sort(reverse=True)
    return found[:limit]


def load_asset(package_path):
    asset = unreal.load_asset(package_path)
    if not asset:
        unreal.log_warning("[FabCity] Could not load {}".format(package_path))
    return asset


def apply_vehicle_meshes(vehicle_candidates):
    meshes = []
    for score, path, _, _ in vehicle_candidates:
        if score < 6:
            continue
        asset = load_asset(path)
        if isinstance(asset, (unreal.StaticMesh, unreal.SkeletalMesh)):
            meshes.append(asset)
        if len(meshes) >= 8:
            break

    cars = [
        a for a in eas.get_all_level_actors()
        if a.get_actor_label().startswith("City_DriveCar_")
    ]
    cars.sort(key=lambda a: a.get_actor_label())

    if not meshes:
        unreal.log_warning("[FabCity] No imported vehicle StaticMesh/SkeletalMesh candidates found.")
        for car in cars:
            if hasattr(car, "clear_external_vehicle_mesh"):
                car.clear_external_vehicle_mesh()
        if cars:
            unreal.log("[FabCity] Cleared external meshes from {} cars".format(len(cars)))
        return 0
    if not cars:
        unreal.log_warning("[FabCity] No City_DriveCar_* actors found. Run fix_cars.py first.")
        return 0

    applied = 0
    for i, car in enumerate(cars):
        mesh = meshes[i % len(meshes)]
        rel_loc = unreal.Vector(0.0, 0.0, -62.0)
        rel_rot = unreal.Rotator(0.0, 0.0, 0.0)
        rel_scale = unreal.Vector(1.0, 1.0, 1.0)
        if isinstance(mesh, unreal.SkeletalMesh) and hasattr(car, "set_external_skeletal_vehicle_mesh"):
            car.set_external_skeletal_vehicle_mesh(mesh, rel_loc, rel_rot, rel_scale)
            applied += 1
        elif isinstance(mesh, unreal.StaticMesh) and hasattr(car, "set_external_vehicle_mesh"):
            car.set_external_vehicle_mesh(mesh, rel_loc, rel_rot, rel_scale)
            applied += 1

    return applied


vehicle_candidates = find_candidates(
    {"StaticMesh", "SkeletalMesh"},
    VEHICLE_NAME_TERMS,
    VEHICLE_PATH_TERMS)
building_candidates = find_candidates(
    {"StaticMesh"},
    BUILDING_TERMS,
    ["citysample", "city_sample", "building", "buildings", "city"],
    limit=60)
road_candidates = find_candidates(
    {"StaticMesh"},
    ROAD_TERMS,
    ["road", "roads", "street", "urban", "city"],
    limit=60)

applied = apply_vehicle_meshes(vehicle_candidates)

lines = []
lines.append("Fab/City Sample candidate report")
lines.append("Applied imported vehicle meshes to {} City_DriveCar_* actors".format(applied))
lines.append("")

for title, candidates in [
    ("Vehicles", vehicle_candidates),
    ("Buildings", building_candidates),
    ("Roads and street props", road_candidates),
]:
    lines.append(title)
    if not candidates:
        lines.append("  No candidates found.")
        continue
    for score, path, name, cls in candidates[:30]:
        lines.append("  score={} class={} name={} path={}".format(score, cls, name, path))
    lines.append("")

with open(REPORT_PATH, "w") as f:
    f.write("\n".join(lines))

saved = les.save_current_level()
unreal.log("[FabCity] Applied vehicle meshes to {} cars".format(applied))
unreal.log("[FabCity] Candidate report: {}".format(REPORT_PATH))
unreal.log("[FabCity] level saved: {}".format(saved))
