"""Shared helpers for preferring imported human avatars over mannequins."""
import unreal


HERO_MESH_PATH = "/Game/Pedestrians/Cappy/SK_Cappy"
MANNEQUIN_MESH_PATH = "/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"
MANNEQUIN_ANIM_BP_PATH = "/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP"


def load_preferred_hero_assets():
    hero_mesh = unreal.load_asset(HERO_MESH_PATH)
    if hero_mesh:
        return {
            "mesh": hero_mesh,
            "anim_bp": None,
            "source": "imported hero avatar",
        }

    unreal.log_warning("[AvatarRealism] Imported hero avatar not found at {}; using mannequin fallback.".format(HERO_MESH_PATH))
    anim_bp = unreal.load_object(None, MANNEQUIN_ANIM_BP_PATH)
    return {
        "mesh": unreal.load_object(None, MANNEQUIN_MESH_PATH),
        "anim_bp": anim_bp,
        "source": "mannequin fallback",
    }


def apply_hero_mesh_defaults(mesh_comp):
    assets = load_preferred_hero_assets()
    mesh = assets["mesh"]
    if not mesh_comp:
        return False, "no mesh component"
    if not mesh:
        return False, "no mesh available"

    if hasattr(mesh_comp, "set_skeletal_mesh_asset"):
        mesh_comp.set_skeletal_mesh_asset(mesh)
    else:
        mesh_comp.set_editor_property("skeletal_mesh", mesh)

    # Match AZarriCharacter's C++ default: -92 keeps feet on the capsule bottom.
    # Use named Rotator args to avoid Unreal Python's positional order pitfalls.
    mesh_comp.set_relative_location_and_rotation(
        unreal.Vector(0.0, 0.0, -92.0),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0),
        False, False,
    )

    anim_bp = assets["anim_bp"]
    mesh_comp.set_editor_property(
        "anim_class",
        anim_bp.generated_class() if anim_bp and hasattr(anim_bp, "generated_class") else None,
    )
    return True, assets["source"]
