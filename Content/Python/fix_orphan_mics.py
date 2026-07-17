"""Re-parent MICs orphaned by the never-committed M_Simple_Opaque master
(ledger 'foliage dependency warnings'). BasicShapeMaterial carries the same
"Color" vector parameter, so their tint overrides keep working.
Safe under -nullrhi (no spawns)."""
import unreal

eal = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

parent = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")
fixed = 0
for name in ("MI_Roof", "MI_Tree_2", "MI_Vivid"):
    path = "/Game/Materials/" + name
    if not eal.does_asset_exist(path):
        continue
    mi = eal.load_asset(path)
    if not mi or not parent:
        continue
    if mi.get_editor_property("parent"):
        unreal.log("[OrphanMIC] {} already has a live parent, skipping".format(name))
        continue
    MEL.set_material_instance_parent(mi, parent)
    MEL.update_material_instance(mi)
    eal.save_loaded_asset(mi)
    fixed += 1
unreal.log_warning("[OrphanMIC] re-parented {} MICs".format(fixed))
