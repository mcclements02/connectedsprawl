"""Clean up the over-busy street markings: keep yellow centerlines and
crosswalk stripes; drop the white lane/edge lines and curb strips that
were stacking into a grid pattern.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

REMOVE_PREFIXES = (
    "City_Detail_VLane_",   # white lane stripes (vertical roads)
    "City_Detail_HLane_",   # white lane stripes (horizontal roads)
    "City_Detail_VEdge_",   # white road-edge stripes
    "City_Detail_HEdge_",
    "City_Detail_Curb_",    # thick white curb bars
    "City_Detail_Crosswalk_EW_",  # zebra stripes crossing through intersections
    "City_Detail_Crosswalk_NS_",
)

removed = 0
for a in list(eas.get_all_level_actors()):
    lbl = a.get_actor_label()
    if any(lbl.startswith(p) for p in REMOVE_PREFIXES):
        eas.destroy_actor(a)
        removed += 1

unreal.log("[CleanStreets] removed {} redundant markings".format(removed))
les.save_current_level()
unreal.log("[CleanStreets] DONE")
