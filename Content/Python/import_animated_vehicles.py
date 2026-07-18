"""Reimport the existing car FBXs as detailed body parts plus four wheels.

The original import deliberately combined each FBX into one StaticMesh. That
is cheap, but makes wheel spin impossible. These assets preserve the source
object transforms, retains separate lights/mirrors/grilles, and gives
ASprawlCar four real wheel meshes to animate.
"""
import glob
import os
import re
import unreal


PROJECT_DIR = unreal.SystemLibrary.get_project_directory()
SOURCE_GLOB = os.path.join(PROJECT_DIR, "staging", "cars", "*.fbx")
DEST_ROOT = "/Game/Vehicles/Animated"

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def normalized(name):
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")


def classify_wheels(meshes):
    result = {}
    for path, mesh in meshes:
        name = normalized(mesh.get_name())
        if "front" in name and "left" in name and "wheel" in name:
            result["Wheel_FL"] = (path, mesh)
        elif "front" in name and "right" in name and "wheel" in name:
            result["Wheel_FR"] = (path, mesh)
        elif "rear" in name and "left" in name and "wheel" in name:
            result["Wheel_RL"] = (path, mesh)
        elif "rear" in name and "right" in name and "wheel" in name:
            result["Wheel_RR"] = (path, mesh)
    return result


def choose_main_body(meshes, wheel_parts):
    wheel_paths = {path for path, _ in wheel_parts.values()}
    candidates = [(path, mesh) for path, mesh in meshes if path not in wheel_paths]
    if not candidates:
        return None

    named_bodies = [
        item for item in candidates
        if re.fullmatch(r"body\d*", normalized(item[1].get_name()))
        or normalized(item[1].get_name()).endswith("_body")
    ]
    pool = named_bodies or candidates

    def bounds_volume(item):
        bounds = item[1].get_bounding_box()
        size = bounds.max - bounds.min
        return max(size.x, 1.0) * max(size.y, 1.0) * max(size.z, 1.0)

    return max(pool, key=bounds_volume)


def import_vehicle(index, source_path):
    destination = "{}/Car_{}".format(DEST_ROOT, index)
    if eal.does_directory_exist(destination):
        eal.delete_directory(destination)

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_animations = False
    options.import_materials = True
    options.import_textures = True
    options.static_mesh_import_data.set_editor_property("combine_meshes", False)
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    options.static_mesh_import_data.set_editor_property("transform_vertex_to_absolute", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", False)

    task = unreal.AssetImportTask()
    task.filename = source_path
    task.destination_path = destination
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = options
    asset_tools.import_asset_tasks([task])

    meshes = []
    for path in eal.list_assets(destination, recursive=True):
        asset = eal.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            meshes.append((path, asset))
    wheel_parts = classify_wheels(meshes)
    main_body = choose_main_body(meshes, wheel_parts)
    expected_wheels = ["Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"]
    missing = [part for part in expected_wheels if part not in wheel_parts]
    if not main_body:
        missing.append("Body")
    if missing:
        found = ", ".join(sorted(asset.get_name() for _, asset in meshes))
        raise RuntimeError(
            "Car {} missing {} (found: {})".format(index, ", ".join(missing), found)
        )

    parts = {"Body": main_body, **wheel_parts}
    for suffix in ["Body"] + expected_wheels:
        old_path, _ = parts[suffix]
        new_name = "SM_Car_{}_{}".format(index, suffix)
        new_path = "{}/{}".format(destination, new_name)
        if old_path != new_path:
            if not eal.rename_asset(old_path, new_path):
                raise RuntimeError("Could not rename {} to {}".format(old_path, new_path))

    reserved_paths = {path for path, _ in parts.values()}
    detail_parts = sorted(
        [(path, mesh) for path, mesh in meshes if path not in reserved_paths],
        key=lambda item: normalized(item[1].get_name()),
    )
    detail_names = []
    for detail_index, (old_path, mesh) in enumerate(detail_parts):
        new_name = "SM_Car_{}_Detail_{:02d}".format(index, detail_index)
        new_path = "{}/{}".format(destination, new_name)
        detail_names.append(mesh.get_name())
        if old_path != new_path and not eal.rename_asset(old_path, new_path):
            raise RuntimeError("Could not rename {} to {}".format(old_path, new_path))

    eal.save_directory(destination, only_if_is_dirty=False)
    centers = []
    for suffix in expected_wheels:
        name = "SM_Car_{}_{}".format(index, suffix)
        wheel = eal.load_asset("{}/{}".format(destination, name))
        bounds = wheel.get_bounding_box()
        centers.append(bounds.min + (bounds.max - bounds.min) * 0.5)
    unreal.log("[AnimatedVehicles] Car {} wheel centers: {}".format(index, centers))
    unreal.log("[AnimatedVehicles] Car {} retained details: {}".format(
        index, detail_names))


sources = {}
for path in glob.glob(SOURCE_GLOB):
    match = re.search(r"Car\s+(\d+)\.fbx$", os.path.basename(path), re.IGNORECASE)
    if match:
        sources[int(match.group(1))] = path

missing_sources = [index for index in range(1, 11) if index not in sources]
if missing_sources:
    raise RuntimeError("Missing source FBXs for cars: {}".format(missing_sources))

for car_index in range(1, 11):
    import_vehicle(car_index, sources[car_index])

unreal.log(
    "[AnimatedVehicles] DONE: 10 detailed bodies + 40 independently animated wheels")
