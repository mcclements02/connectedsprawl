"""Fix BP_Zarri mesh transform so the character stands upright.

UE Character convention: mesh relative location (0,0,-89), rotation yaw=-90.
Python's unreal.Rotator(roll, pitch, yaw) order tripped the first attempt
(yaw was passed as pitch). This corrects it.
"""
import unreal

BP_ZARRI = "/Game/Core/BP_Zarri"
bp = unreal.load_asset(BP_ZARRI)
gen_class = bp.generated_class()
cdo = unreal.get_default_object(gen_class)

mesh_comp = cdo.get_editor_property("mesh")
if mesh_comp:
    mesh_comp.set_relative_location_and_rotation(
        unreal.Vector(0.0, 0.0, -92.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0),
        False, False,
    )
    # Verify anim class is still set
    ac = mesh_comp.get_editor_property("anim_class")
    unreal.log("[FixPose] mesh transform corrected; anim_class={}".format(ac))
else:
    unreal.log_warning("[FixPose] no mesh component found")

unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.log("[FixPose] BP_Zarri saved. DONE")
