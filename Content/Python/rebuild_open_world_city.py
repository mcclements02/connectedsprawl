"""One-shot rebuild for the richer open-world city prototype.

Run:
  UnrealEditor ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended -nosplash
"""
import os
import unreal


SCRIPT_ORDER = [
    "build_city.py",
    "add_city_detail.py",
    "populate.py",
    "build_real.py",
    "fix_trees.py",
    "build_facade_v2.py",
    "realism_pass.py",
    "populate_world.py",
    "fix_cars.py",
    "import_artwork.py",
    "aaa_realism_overhaul.py",
    "import_animated_vehicles.py",
    "apply_living_city.py",
]


def run_script(path):
    unreal.log("[OpenWorldCity] running {}".format(os.path.basename(path)))
    with open(path, "r") as f:
        code = compile(f.read(), path, "exec")
    scope = {"__file__": path, "__name__": "__main__"}
    exec(code, scope, scope)


root = os.path.dirname(os.path.abspath(__file__))
for script_name in SCRIPT_ORDER:
    run_script(os.path.join(root, script_name))

unreal.log("[OpenWorldCity] rebuild complete")
