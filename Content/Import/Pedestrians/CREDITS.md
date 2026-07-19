# Pedestrian avatar credits

The character FBX files in this folder were produced by
`Tools/retarget_avatars.py` from these sources:

- **Meshes/textures:** "100 Avatars" rounds 1–3 by **Polygonal Mind**
  (https://github.com/madjin/100avatars). The repository includes a Creative
  Commons Attribution 4.0 license file, while the round-3 VRM metadata declares
  CC0. This project follows the stricter CC BY 4.0 terms: Polygonal Mind is
  credited here, and the models were modified by rig retargeting, animation
  baking, VRM-to-FBX conversion, and game-specific optimization. The five new
  round-3 variants are Bruno, Cursed Amy, Eugenia, Jenny, and Juanita. Zarri is
  a game-specific derivative of Cappy, rebuilt by `Tools/build_zarri_avatar.py`
  with a distinct medium-deep skin, dark-hair, and tech-streetwear palette while
  retaining the lightweight rig and seven baked gameplay clips.
- **Animations:** "Universal Animation Library" (standard/free edition) by
  **Quaternius** (https://quaternius.com), CC0 1.0 Universal; obtained from the
  glTF mirror at https://github.com/J-Ponzo/gltf-universal-animation-library.
  The clips Idle_Loop, Walk_Loop, Jog_Fwd_Loop, Idle_Talking_Loop and
  Walk_Formal_Loop, Sprint_Loop, and Sitting_Idle_Loop were retargeted onto
  each avatar's Mixamo-style rig and baked into the FBX takes A_Idle, A_Walk,
  A_Jog, A_Talk, A_WalkFormal, A_Sprint, and A_Sit.

The original downloaded files remain in generated `staging/` directories; the
FBXs here are adapted build inputs rather than unmodified source redistribution.
