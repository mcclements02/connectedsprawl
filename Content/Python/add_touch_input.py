"""Append gamepad-stick mappings to IMC_Default so the iOS virtual
joysticks (which emit Gamepad_* events) and real MFi controllers work.
Keyboard/mouse mappings are preserved.

Run:
  UnrealEditor-Cmd ConnectedSprawl.uproject -ExecutePythonScript="<this file>" -unattended -nullrhi
"""
import unreal

ets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

imc = unreal.load_asset("/Game/Input/IMC_Default")
ia = {n: unreal.load_asset("/Game/Input/IA_" + n)
      for n in ("Move", "Look", "Jump", "Sprint", "Interact")}
assert imc and all(ia.values())


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


dkm = imc.get_editor_property("default_key_mappings")
entries = list(dkm.get_editor_property("mappings"))
existing = {m.get_editor_property("key").export_text() for m in entries}

new = [
    # move: paired 2D stick key drives IA_Move directly (X=strafe, Y=forward)
    ("Gamepad_Left2D", mapping(ia["Move"], "Gamepad_Left2D", [])),
    # look: two 1D axes, mirroring the MouseX/MouseY pattern above
    ("Gamepad_RightX", mapping(ia["Look"], "Gamepad_RightX", [])),
    ("Gamepad_RightY", mapping(ia["Look"], "Gamepad_RightY", [swizzle_yxz(), negate()])),
    ("Gamepad_FaceButton_Bottom", mapping(ia["Jump"], "Gamepad_FaceButton_Bottom", [])),
    ("Gamepad_LeftThumbstick", mapping(ia["Sprint"], "Gamepad_LeftThumbstick", [])),
    ("Gamepad_FaceButton_Left", mapping(ia["Interact"], "Gamepad_FaceButton_Left", [])),
]
added = 0
for key_name, m in new:
    if any(key_name in e for e in existing):
        continue
    entries.append(m)
    added += 1

dkm.set_editor_property("mappings", entries)
imc.set_editor_property("default_key_mappings", dkm)
ets.save_asset("/Game/Input/IMC_Default")
unreal.log("[TouchInput] added {} gamepad mappings (total {})".format(added, len(entries)))
