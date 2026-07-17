"""Read-only project audit for the enhancement session (safe under -nullrhi).

Reports, into Saved/AuditState.txt:
  1. TestMap actor counts by label prefix + total.
  2. Actors whose StaticMeshComponent has a null mesh (broken refs).
  3. Fountain texture dimensions (ledger flagged 4K textures for mobile).
  4. RealKit material texture/sampler inventory (sampler-warning suspects).
  5. Downtown_West asset availability sanity check (pack installed?).
"""
import os
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary

les.load_level("/Game/Maps/TestMap")

lines = []

# 1) actor census
actors = eas.get_all_level_actors()
prefix_counts = {}
for a in actors:
    label = a.get_actor_label()
    prefix = label.split("_")[0] if "_" not in label else "_".join(label.split("_")[:2])
    prefix_counts[prefix] = prefix_counts.get(prefix, 0) + 1
lines.append("== Actor census: {} total ==".format(len(actors)))
for p, c in sorted(prefix_counts.items(), key=lambda kv: -kv[1]):
    lines.append("  {:5d}  {}".format(c, p))

# 2) broken mesh refs
lines.append("")
lines.append("== Actors with null static mesh ==")
broken = 0
for a in actors:
    smc = getattr(a, "static_mesh_component", None)
    if smc is not None and smc.static_mesh is None:
        lines.append("  NULL MESH: {}".format(a.get_actor_label()))
        broken += 1
if not broken:
    lines.append("  none")

# 3) fountain textures
lines.append("")
lines.append("== Fountain textures ==")
fountain_dir = "/Game/Downtown_West/Textures/Fountain"
found_any = False
for root in ["/Game/Downtown_West/Textures", "/Game/Downtown_West/Assets/props/prop_fountain"]:
    if not eal.does_directory_exist(root):
        continue
    for path in eal.list_assets(root, recursive=True):
        if "fountain" not in path.lower():
            continue
        asset = eal.load_asset(path)
        if isinstance(asset, unreal.Texture2D):
            found_any = True
            lines.append("  {}x{}  {}".format(
                asset.blueprint_get_size_x(), asset.blueprint_get_size_y(), path))
if not found_any:
    lines.append("  no fountain textures found under Downtown_West")

# 4) RealKit materials + their textures
lines.append("")
lines.append("== RealKit materials ==")
rk = "/Game/RealKit"
if eal.does_directory_exist(rk):
    for path in eal.list_assets(rk, recursive=True):
        asset = eal.load_asset(path)
        if isinstance(asset, unreal.Material):
            lines.append("  Material: {}".format(path))
        elif isinstance(asset, unreal.Texture2D):
            lines.append("  Texture {}x{}: {}".format(
                asset.blueprint_get_size_x(), asset.blueprint_get_size_y(), path))
else:
    lines.append("  /Game/RealKit missing")

# 5) DW availability
lines.append("")
lines.append("== Downtown_West availability ==")
probe = [
    "/Game/Downtown_West/Assets/props/props_streetlight/SM_light_streetlight",
    "/Game/Downtown_West/Assets/building_b/SM_build_b_mod_lvl1_storefront_b_wall5m",
    "/Game/Downtown_West/Maps/Demo_Environment",
]
for p in probe:
    lines.append("  {}: {}".format(p, "OK" if eal.does_asset_exist(p) else "MISSING"))

out = os.path.join(unreal.Paths.project_saved_dir(), "AuditState.txt")
with open(out, "w") as f:
    f.write("\n".join(lines))
unreal.log("[Audit] wrote {}".format(out))
