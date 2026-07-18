"""Rebuild IMC_Default.default_key_mappings from scratch with correct
per-key modifiers. UE 5.8 stores IMC data in `default_key_mappings`
(an InputMappingContextMappingData struct), not the deprecated `mappings`
array, and map_key() returns a copy so modifiers set on it are lost.

IA_Move is Axis2D: X = strafe, Y = forward/back.
A 1D/boolean key feeds the X axis by default, value 1.0.
  - SwizzleAxis YXZ  -> moves the value onto the Y axis  (forward/back)
  - Negate           -> flips sign
"""
import unreal

ets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

imc = unreal.load_asset("/Game/Input/IMC_Default")
ia_move = unreal.load_asset("/Game/Input/IA_Move")
ia_look = unreal.load_asset("/Game/Input/IA_Look")
ia_jump = unreal.load_asset("/Game/Input/IA_Jump")
ia_sprint = unreal.load_asset("/Game/Input/IA_Sprint")
ia_interact = unreal.load_asset("/Game/Input/IA_Interact")
for nm, obj in [("IMC", imc), ("Move", ia_move), ("Look", ia_look),
                ("Jump", ia_jump), ("Sprint", ia_sprint), ("Interact", ia_interact)]:
    if obj is None:
        raise RuntimeError("missing asset: " + nm)


def negate():
    return unreal.new_object(unreal.InputModifierNegate, outer=imc)


def swizzle_yxz():
    m = unreal.new_object(unreal.InputModifierSwizzleAxis, outer=imc)
    m.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
    return m


def mapping(action, key_name, modifiers):
    m = unreal.EnhancedActionKeyMapping()
    k = unreal.Key()
    k.import_text(key_name)
    m.set_editor_property("key", k)
    m.set_editor_property("action", action)
    m.set_editor_property("modifiers", modifiers)
    return m


entries = [
    # --- Move: WASD ---
    mapping(ia_move, "A", [negate()]),                  # (-1, 0) strafe left
    mapping(ia_move, "D", []),                          # ( 1, 0) strafe right
    mapping(ia_move, "W", [swizzle_yxz()]),             # ( 0, 1) forward
    mapping(ia_move, "S", [swizzle_yxz(), negate()]),   # ( 0,-1) back
    # --- Move: Arrow keys ---
    mapping(ia_move, "Left", [negate()]),
    mapping(ia_move, "Right", []),
    mapping(ia_move, "Up", [swizzle_yxz()]),
    mapping(ia_move, "Down", [swizzle_yxz(), negate()]),
    # --- Look: mouse ---
    mapping(ia_look, "MouseX", []),                     # yaw
    mapping(ia_look, "MouseY", [swizzle_yxz(), negate()]),  # pitch, inverted
    # --- Actions ---
    mapping(ia_jump, "SpaceBar", []),
    mapping(ia_sprint, "LeftShift", []),
    mapping(ia_interact, "E", []),
]

dkm = imc.get_editor_property("default_key_mappings")
dkm.set_editor_property("mappings", entries)
imc.set_editor_property("default_key_mappings", dkm)

# also clear the deprecated array to avoid confusion
ets.save_asset("/Game/Input/IMC_Default")

# verify
dkm2 = imc.get_editor_property("default_key_mappings")
arr = dkm2.get_editor_property("mappings")
unreal.log("[RebuildInput] wrote {} mappings".format(len(arr)))
for m in arr:
    key = m.get_editor_property("key")
    ia = m.get_editor_property("action")
    mods = [type(x).__name__ for x in m.get_editor_property("modifiers")]
    unreal.log("  {:10s} -> {:12s} mods={}".format(
        key.export_text(), ia.get_name() if ia else "None", mods))
unreal.log("[RebuildInput] DONE")
