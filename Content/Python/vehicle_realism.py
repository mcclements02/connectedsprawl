"""Shared helpers for placing real, varied car meshes in editor scripts."""
import unreal


VEHICLE_MESH_PATHS = [
    "/Game/Vehicles/Imported/Generic/Car_{}".format(i) for i in range(1, 11)
] + [
    "/Game/Vehicles/Imported/Police/police_interceptor",
]

VEHICLE_PAINT_SPECS = [
    ("/Game/CityArt/MI_Car_Red", unreal.LinearColor(0.42, 0.04, 0.04, 1.0), 0.28, 0.5),
    ("/Game/CityArt/MI_Car_White", unreal.LinearColor(0.72, 0.72, 0.72, 1.0), 0.30, 0.4),
    ("/Game/CityArt/MI_Car_Blue", unreal.LinearColor(0.05, 0.12, 0.34, 1.0), 0.28, 0.5),
    ("/Game/CityArt/MI_Car_Black", unreal.LinearColor(0.08, 0.085, 0.09, 1.0), 0.25, 0.5),
    ("/Game/CityArt/MI_Car_Silver", unreal.LinearColor(0.40, 0.42, 0.44, 1.0), 0.25, 0.7),
    ("/Game/CityArt/MI_Car_Yellow", unreal.LinearColor(0.85, 0.55, 0.05, 1.0), 0.28, 0.5),
]
VEHICLE_PAINT_PATHS = [spec[0] for spec in VEHICLE_PAINT_SPECS]

TARGET_DRIVABLE_LEN = 480.0
TARGET_PARKED_LEN = 470.0
HULL_BOTTOM = -62.0
ANIMATED_ROOT = "/Game/Vehicles/Animated"


def load_vehicle_meshes():
    meshes = []
    for path in VEHICLE_MESH_PATHS:
        mesh = unreal.load_asset(path)
        if mesh and isinstance(mesh, unreal.StaticMesh):
            meshes.append(mesh)
    return meshes


def refresh_vehicle_paint_materials():
    editing = unreal.MaterialEditingLibrary
    for path, color, roughness, metallic in VEHICLE_PAINT_SPECS:
        material = unreal.load_asset(path)
        if not material:
            continue
        try:
            editing.set_material_instance_vector_parameter_value(material, "BaseColor", color)
            editing.set_material_instance_scalar_parameter_value(material, "Roughness", roughness)
            editing.set_material_instance_scalar_parameter_value(material, "Metallic", metallic)
            unreal.EditorAssetLibrary.save_loaded_asset(material)
        except Exception:
            pass


def load_vehicle_paint_materials(fallback=None):
    materials = []
    for path in VEHICLE_PAINT_PATHS:
        material = unreal.load_asset(path)
        if material:
            materials.append(material)
    if materials:
        return materials
    return list(fallback or [])


def mesh_for_index(meshes, idx):
    if not meshes:
        return None
    # Seven is coprime to the ten-car set, so sequential actors use every
    # model instead of alternating between only two variants.
    return meshes[(idx * 7) % len(meshes)]


def paint_for_index(materials, idx):
    if not materials:
        return None
    return materials[idx % len(materials)]


def scale_for_mesh(mesh, target_len):
    bb = mesh.get_bounding_box()
    size = bb.max - bb.min
    natural_len = max(size.x, size.y, 1.0)
    return max(0.45, min(1.4, target_len / natural_len))


def is_paint_slot_name(name):
    lower = str(name).lower()
    excluded = [
        "glass", "screen", "wheel", "tyre", "tire", "rim", "brake",
        "light", "lamp", "grill", "exhaust", "interior", "plate",
    ]
    if any(term in lower for term in excluded):
        return False
    return "body" in lower or "paint" in lower or "carbon" in lower


def apply_paint_to_static_mesh_component(component, material):
    if not component or not material:
        return 0

    try:
        mesh = component.get_static_mesh()
    except AttributeError:
        mesh = component.get_editor_property("static_mesh")
    if not mesh:
        return 0

    try:
        component.empty_override_materials()
    except Exception:
        pass

    painted = 0
    try:
        static_materials = mesh.get_editor_property("static_materials")
    except Exception:
        static_materials = []
    for slot_index, static_material in enumerate(static_materials):
        names = []
        for prop in ("material_slot_name", "imported_material_slot_name"):
            try:
                names.append(static_material.get_editor_property(prop))
            except Exception:
                pass
        if any(is_paint_slot_name(name) for name in names):
            component.set_material(slot_index, material)
            painted += 1

    if painted == 0:
        try:
            component.set_material(0, material)
            painted = 1
        except Exception:
            pass

    return painted


def find_component_by_name(actor, component_class, component_name):
    for component in actor.get_components_by_class(component_class):
        if component.get_name() == component_name:
            return component
    return None


def configure_drivable_car(actor, idx, meshes, paint_materials):
    paint = paint_for_index(paint_materials, idx)
    if paint and hasattr(actor, "set_body_material"):
        actor.set_body_material(paint)

    mesh = mesh_for_index(meshes, idx)
    if not mesh or not hasattr(actor, "set_external_vehicle_mesh"):
        return False

    bb = mesh.get_bounding_box()
    scale = scale_for_mesh(mesh, TARGET_DRIVABLE_LEN)
    rel_z = HULL_BOTTOM - bb.min.z * scale
    actor.set_external_vehicle_mesh(
        mesh,
        unreal.Vector(0.0, 0.0, rel_z),
        # Blender vehicle exports use +Y as their nose; UE physics drives +X.
        # The car module repeats this policy at runtime for saved instances.
        # NB: Python's unreal.Rotator positional args are (roll, pitch, yaw) —
        # NOT (pitch, yaw, roll) like C++. Use keyword to avoid surprises.
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0),
        unreal.Vector(scale, scale, scale),
    )

    external = find_component_by_name(actor, unreal.StaticMeshComponent, "ExternalVehicleMesh")
    apply_paint_to_static_mesh_component(external, paint)
    return True


def load_animated_vehicle_parts(idx):
    variant = (idx * 7) % 10 + 1
    folder = "{}/Car_{}".format(ANIMATED_ROOT, variant)

    def load(suffix):
        return unreal.load_asset(
            "{}/SM_Car_{}_{}".format(folder, variant, suffix)
        )

    bodies = [load("Body")]
    for detail_index in range(32):
        detail = load("Detail_{:02d}".format(detail_index))
        if not detail:
            break
        bodies.append(detail)
    wheels = [
        load("Wheel_FL"), load("Wheel_FR"),
        load("Wheel_RL"), load("Wheel_RR"),
    ]
    if not bodies[0] or any(not wheel for wheel in wheels):
        return None
    return bodies, wheels


def configure_animated_car(actor, idx, paint_materials):
    """Give an ASprawlCar a real body and four independently moving wheels."""
    parts = load_animated_vehicle_parts(idx)
    if not parts or not hasattr(actor, "set_external_vehicle_parts"):
        return False

    bodies, wheels = parts
    body = bodies[0]
    paint = paint_for_index(paint_materials, idx)
    if paint and hasattr(actor, "set_body_material"):
        actor.set_body_material(paint)

    body_bounds = body.get_bounding_box()
    scale = scale_for_mesh(body, TARGET_DRIVABLE_LEN)
    rel_z = HULL_BOTTOM - body_bounds.min.z * scale
    centers = []
    for wheel in wheels:
        bounds = wheel.get_bounding_box()
        centers.append(bounds.min + (bounds.max - bounds.min) * 0.5)

    actor.set_external_vehicle_parts(
        bodies,
        wheels,
        centers,
        unreal.Vector(0.0, 0.0, rel_z),
        # Named front/rear axles are authoritative in C++; this is the same
        # correct default for Blender Y-forward imports in editor tooling.
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0),
        unreal.Vector(scale, scale, scale),
    )
    external = find_component_by_name(
        actor, unreal.StaticMeshComponent, "ExternalVehicleMesh")
    apply_paint_to_static_mesh_component(external, paint)
    return True


def spawn_real_parked_car(eas, idx, loc, car_yaw, meshes, paint_materials):
    mesh = mesh_for_index(meshes, idx)
    if not mesh:
        return None

    bb = mesh.get_bounding_box()
    scale = scale_for_mesh(mesh, TARGET_PARKED_LEN)
    z = 8.0 - bb.min.z * scale
    actor_loc = unreal.Vector(loc.x, loc.y, z)
    actor_rot = unreal.Rotator(roll=0.0, pitch=0.0, yaw=car_yaw + 90.0)

    actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, actor_loc, actor_rot)
    actor.set_actor_label("City_Car_{}".format(idx))
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))

    smc = actor.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    apply_paint_to_static_mesh_component(smc, paint_for_index(paint_materials, idx))
    return actor
