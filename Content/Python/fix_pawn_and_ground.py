"""Point the GameMode straight at the self-contained C++ AZarriCharacter
(bypassing the flaky BP_Zarri inherited-component overrides), and fix the
z-fighting between the ground slab and sidewalk slabs.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# ---- 1) GameMode DefaultPawnClass -> C++ AZarriCharacter ----
zarri_cls = unreal.ZarriCharacter.static_class()

for gm_path in ["/Game/Core/BP_SprawlGameMode"]:
    bp = unreal.load_asset(gm_path)
    if not bp:
        unreal.log_warning("[FixPawn] {} not found".format(gm_path))
        continue
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    cdo.set_editor_property("default_pawn_class", zarri_cls)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log("[FixPawn] {} DefaultPawnClass -> AZarriCharacter".format(gm_path))

# ---- 2) Fix z-fighting: drop the ground slab so its top sits below sidewalks ----
les.load_level("/Game/Maps/TestMap")
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "City_Ground":
        # cube base 100u, scale z=1 -> 100 tall. Center at -56 => top at -6.
        a.set_actor_location(unreal.Vector(0, 0, -56), False, False)
        unreal.log("[FixPawn] lowered City_Ground to avoid z-fight")

saved = les.save_current_level()
unreal.log("[FixPawn] level saved: {}".format(saved))
unreal.log("[FixPawn] DONE")
