"""Grant the InstancedStaticMeshes usage flag to the city's master materials.

Unreal substitutes the default grey material when a material without
bUsedWithInstancedStaticMeshes is rendered on an instanced component. The
skyline/gate proxies (HISM cubes wearing MI_Facade_* / MI_ConcreteWorn) and
the long-warned MI_RoadPaint stripes all hit this. The flag lives on the
MASTER material, so each listed instance is resolved to its base and the base
is flagged + saved. Asset-only edit: -nullrhi safe.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -run=pythonscript \
    -script="<abs>/fix_ism_usage_flags.py" -unattended -nosplash -nullrhi \
    -stdout -DisablePlugins=Fab
"""
import os
import unreal

MATERIALS = [
    "/Game/CityArt/MI_Facade_Slate",
    "/Game/CityArt/MI_Facade_Tan",
    "/Game/CityArt/MI_Facade_Warm",
    "/Game/CityArt/MI_Facade_Pale",
    "/Game/CityArt/MI_Facade_Teal",
    "/Game/Import/CityKit/MI_ConcreteWorn",
    "/Game/Import/CityKit/MI_AsphaltWorn",
    "/Game/Import/CityKit/MI_GrassField",
    "/Game/Materials/MI_RoadPaint",
]

lines = []


def out(text=""):
    lines.append(text)


def flush():
    path = os.path.join(unreal.Paths.project_saved_dir(),
                        "IsmUsageFlagsReport.txt")
    with open(path, "w") as handle:
        handle.write("\n".join(lines) + "\n")


def main():
    flagged = set()
    for path in MATERIALS:
        mi = unreal.load_asset(path)
        if not mi:
            out("MISSING: " + path)
            continue
        base = mi.get_base_material()
        if not base:
            out("NO BASE: " + path)
            continue
        base_path = base.get_path_name()
        if base_path in flagged:
            out("ok (base already flagged): %s -> %s" % (path, base.get_name()))
            continue
        already = base.get_editor_property("used_with_instanced_static_meshes")
        if not already:
            base.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.EditorAssetLibrary.save_loaded_asset(base)
        flagged.add(base_path)
        out("%s -> base %s used_with_instanced_static_meshes=%s (was %s)" % (
            path, base.get_name(), True, already))
    out("")
    out("PASS")


try:
    main()
except Exception as exc:  # noqa: BLE001 - report every failure to the sidecar
    out("EXCEPTION: %r" % exc)
finally:
    flush()
