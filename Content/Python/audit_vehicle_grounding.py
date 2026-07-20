"""Diagnostic: how far every level-placed vehicle is sunk into the ground.

Writes Saved/VehicleGroundingReport.txt.
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"
lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "VehicleGroundingReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def mesh_path_of(actor):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        return ""
    mesh = component.get_editor_property("static_mesh")
    return mesh.get_path_name() if mesh else ""


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actors = unreal.get_editor_subsystem(
        unreal.EditorActorSubsystem).get_all_level_actors()

    rows = []
    class_counts = {}
    for actor in actors:
        if not actor:
            continue
        label = actor.get_actor_label()
        class_name = actor.get_class().get_name()
        mesh_path = mesh_path_of(actor) if class_name == "StaticMeshActor" else ""
        looks_vehicular = (
            "/Game/Vehicles/" in mesh_path
            or class_name == "SprawlCar"
            or any(token in label.lower()
                   for token in ("car", "vehicle", "parked", "taxi", "van")))
        if not looks_vehicular:
            continue

        origin, extent = actor.get_actor_bounds(only_colliding_components=False)
        location = actor.get_actor_location()
        rows.append((label, class_name, location.z, origin.z - extent.z,
                     extent.z * 2.0, mesh_path.split(".")[-1]))
        class_counts[class_name] = class_counts.get(class_name, 0) + 1

    out("vehicle_actors={}".format(len(rows)))
    out("by_class=" + ", ".join(
        "{}:{}".format(k, v) for k, v in sorted(class_counts.items())))
    out("")
    out("{:<40} {:<16} {:>9} {:>10} {:>8}  {}".format(
        "label", "class", "loc_z", "bottom_z", "height", "mesh"))
    for row in sorted(rows, key=lambda r: r[3])[:70]:
        out("{:<40} {:<16} {:9.1f} {:10.1f} {:8.1f}  {}".format(*row))

    below = [r for r in rows if r[3] < 0.0]
    out("")
    out("=== sunk below z=0: {} of {} ===".format(len(below), len(rows)))
    if below:
        depths = sorted(r[3] for r in below)
        out("  deepest={:.1f}  median={:.1f}".format(
            depths[0], depths[len(depths) // 2]))


try:
    main()
finally:
    flush()
