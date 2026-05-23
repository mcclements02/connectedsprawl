"""Add arrow-key movement (Up/Down/Left/Right) to IMC_Default,
mirroring the existing WASD bindings on IA_Move.
"""
import unreal

ets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

imc = unreal.load_asset("/Game/Input/IMC_Default")
ia_move = unreal.load_asset("/Game/Input/IA_Move")

if not imc or not ia_move:
    raise RuntimeError("IMC_Default or IA_Move missing")


def add(key_name, swizzle=None, negate=False):
    k = unreal.Key()
    k.import_text(key_name)
    mapping = imc.map_key(ia_move, k)
    mods = []
    if swizzle is not None:
        sw = unreal.InputModifierSwizzleAxis()
        sw.order = swizzle
        mods.append(sw)
    if negate:
        mods.append(unreal.InputModifierNegate())
    if mods:
        mapping.modifiers = mods
    unreal.log("[Arrows] mapped {}".format(key_name))


# Left  = (-1, 0)
add("Left", negate=True)
# Right = (1, 0)
add("Right")
# Up    = (0, 1)
add("Up", swizzle=unreal.InputAxisSwizzle.YXZ)
# Down  = (0, -1)
add("Down", swizzle=unreal.InputAxisSwizzle.YXZ, negate=True)

ets.save_asset("/Game/Input/IMC_Default")
unreal.log("[Arrows] IMC_Default saved. DONE")
