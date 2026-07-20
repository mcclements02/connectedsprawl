"""Diagnostic: what actually forms the road surface, curbs and sidewalks.

Needed before any street-widening pass: writes Saved/StreetGeometryReport.txt.
"""
import os
import unreal

MAP_PATH = "/Game/Maps/TestMap"
lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "StreetGeometryReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def main():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP_PATH)
    actors = unreal.get_editor_subsystem(
        unreal.EditorActorSubsystem).get_all_level_actors()

    groups = {}
    for actor in actors:
        if not actor:
            continue
        label = actor.get_actor_label()
        prefix = "_".join(label.split("_")[:3])
        entry = groups.setdefault(prefix, {"count": 0, "sample": None})
        entry["count"] += 1
        if entry["sample"] is None:
            origin, extent = actor.get_actor_bounds(only_colliding_components=False)
            entry["sample"] = (label, actor.get_class().get_name(),
                               actor.get_actor_location(), extent)

    out("=== label groups relevant to streets ===")
    keywords = ("road", "street", "ground", "asphalt", "sidewalk", "walk",
                "curb", "pave", "block", "plaza", "lane", "detail", "parked")
    for prefix, entry in sorted(groups.items(), key=lambda kv: -kv[1]["count"]):
        if not any(k in prefix.lower() for k in keywords):
            continue
        label, class_name, location, extent = entry["sample"]
        out("{:6d}  {:<34} {:<16} sample_loc=({:.0f},{:.0f},{:.0f}) "
            "half_extent=({:.0f},{:.0f},{:.0f})".format(
                entry["count"], prefix, class_name,
                location.x, location.y, location.z,
                extent.x, extent.y, extent.z))

    out("")
    out("=== all groups over 20 actors ===")
    for prefix, entry in sorted(groups.items(), key=lambda kv: -kv[1]["count"])[:28]:
        label, class_name, location, extent = entry["sample"]
        out("{:6d}  {:<34} half_extent=({:.0f},{:.0f},{:.0f})".format(
            entry["count"], prefix, extent.x, extent.y, extent.z))


try:
    main()
finally:
    flush()
