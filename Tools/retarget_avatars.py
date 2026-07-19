"""Retarget Quaternius Universal Animation Library locomotion clips onto
100Avatars Mixamo-rigged characters and export game-ready FBX files.

Run headless:
  /opt/homebrew/bin/blender -b --factory-startup \
    --python Tools/retarget_avatars.py

Inputs:
  staging/ual/AnimationLibrary_Godot_Standard.gltf
  Content/Import/Pedestrians/*.fbx       existing Mixamo-rigged targets
  staging/avatars_new/*.vrm              additional round-3 targets
Outputs:
  staging/avatars_out/<Name>.fbx         mesh + skeleton + baked anim takes
  staging/qa/<Name>_{Walk,Sit}.png        QA renders

Retarget algorithm: world-space rotation transfer with rest-pose offset.
The legacy targets face -Y; round-3 VRM targets arrive facing +Y and receive a
single object-level half-turn before export. For each mapped bone:
R_goal = (R_src_pose @ R_src_rest^-1) @ R_tgt_rest. Hips also receives the
source root translation scaled by hip-height ratio.
"""
import sys
import math
import shutil
import struct
import tempfile
from pathlib import Path
import bpy
from mathutils import Matrix, Quaternion, Vector

ROOT_DIR = Path(__file__).resolve().parents[1]
UAL_GLTF = ROOT_DIR / "staging/ual/AnimationLibrary_Godot_Standard.gltf"
VENDORED_AVATAR_DIR = ROOT_DIR / "Content/Import/Pedestrians"
NEW_AVATAR_DIR = ROOT_DIR / "staging/avatars_new"
OUT_DIR = ROOT_DIR / "staging/avatars_out"
QA_DIR = ROOT_DIR / "staging/qa"

# Round 3 currently ships VRM (glTF binary) sources rather than the FBXs used
# by rounds 1-2. Keep public names stable and friendly to Unreal asset paths.
NEW_AVATAR_VARIANTS = {
    "bruno": "Bruno",
    "cursed_amy": "CursedAmy",
    "eugenia": "Eugenia",
    "jenny": "Jenny",
    "juanita": "Juanita",
}

# Export name -> UAL action name
ANIMS = [
    ("Idle", "Idle_Loop"),
    ("Walk", "Walk_Loop"),
    ("Jog", "Jog_Fwd_Loop"),
    ("Talk", "Idle_Talking_Loop"),
    ("WalkFormal", "Walk_Formal_Loop"),
    ("Sprint", "Sprint_Loop"),
    ("Sit", "Sitting_Idle_Loop"),
]

# mixamo target bone -> UAL source bone
BONE_MAP = {
    "Hips": "DEF-hips",
    "Spine": "DEF-spine.001",
    "Spine1": "DEF-spine.002",
    "Spine2": "DEF-spine.003",
    "Neck": "DEF-neck",
    "Head": "DEF-head",
    "LeftShoulder": "DEF-shoulder.L",
    "LeftArm": "DEF-upper_arm.L",
    "LeftForeArm": "DEF-forearm.L",
    "LeftHand": "DEF-hand.L",
    "RightShoulder": "DEF-shoulder.R",
    "RightArm": "DEF-upper_arm.R",
    "RightForeArm": "DEF-forearm.R",
    "RightHand": "DEF-hand.R",
    "LeftUpLeg": "DEF-thigh.L",
    "LeftLeg": "DEF-shin.L",
    "LeftFoot": "DEF-foot.L",
    "LeftToeBase": "DEF-toe.L",
    "RightUpLeg": "DEF-thigh.R",
    "RightLeg": "DEF-shin.R",
    "RightFoot": "DEF-foot.R",
    "RightToeBase": "DEF-toe.R",
}

SEMANTIC_CHILD = {
    "Spine": "Spine1",
    "Spine1": "Spine2",
    "Spine2": "Neck",
    "Neck": "Head",
    "LeftShoulder": "LeftArm",
    "LeftArm": "LeftForeArm",
    "LeftForeArm": "LeftHand",
    "RightShoulder": "RightArm",
    "RightArm": "RightForeArm",
    "RightForeArm": "RightHand",
    "LeftUpLeg": "LeftLeg",
    "LeftLeg": "LeftFoot",
    "LeftFoot": "LeftToeBase",
    "RightUpLeg": "RightLeg",
    "RightLeg": "RightFoot",
    "RightFoot": "RightToeBase",
}


def rot_of(m):
    return m.to_quaternion()


def import_sources():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(UAL_GLTF))
    src = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    for obj in bpy.data.objects:
        if obj is not src:
            obj.hide_render = True
    return src


def _repair_vrm_header(path):
    """Return an importable GLB path without modifying the downloaded VRM.

    The round-3 exporter wrote a correct BIN chunk but left the GLB header's
    total-length field short by a few bytes. Blender rightly rejects that. A
    temporary copy with the total length corrected is lossless and keeps the
    approved source untouched.
    """
    data = bytearray(path.read_bytes())
    if len(data) < 12 or data[:4] != b"glTF":
        raise RuntimeError("not a glTF binary: {}".format(path))
    declared = struct.unpack_from("<I", data, 8)[0]
    if declared == len(data):
        return path, None
    struct.pack_into("<I", data, 8, len(data))
    handle = tempfile.NamedTemporaryFile(suffix=".glb", delete=False)
    handle.write(data)
    handle.close()
    return Path(handle.name), Path(handle.name)


def import_avatar(path):
    before = set(bpy.data.objects)
    before_actions = set(bpy.data.actions)
    before_images = set(bpy.data.images)
    temporary_path = None
    temporary_directory = None
    try:
        if path.suffix.lower() == ".fbx":
            # Blender extracts embedded FBX images into a sibling <name>.fbm
            # folder. Import a temporary copy and pack the loaded images so the
            # canonical Content/Import source directory stays read-only/clean.
            temporary_directory = tempfile.TemporaryDirectory(
                prefix="sprawl-avatar-import-")
            import_path = Path(temporary_directory.name) / path.name
            shutil.copy2(path, import_path)
            bpy.ops.import_scene.fbx(filepath=str(import_path))
            for image in [item for item in bpy.data.images
                          if item not in before_images]:
                try:
                    if image.source == 'FILE' and not image.packed_file:
                        image.pack()
                except RuntimeError:
                    pass
        else:
            import_path, temporary_path = _repair_vrm_header(path)
            bpy.ops.import_scene.gltf(filepath=str(import_path))
    finally:
        if temporary_path and temporary_path.exists():
            temporary_path.unlink()
        if temporary_directory:
            temporary_directory.cleanup()

    new = [o for o in bpy.data.objects if o not in before]
    tgt = next(o for o in new if o.type == 'ARMATURE')
    meshes = [
        o for o in new if o.type == 'MESH' and (
            o.parent == tgt or any(
                mod.type == 'ARMATURE' and mod.object == tgt
                for mod in o.modifiers
            )
        )
    ]
    if not meshes:
        meshes = [o for o in new if o.type == 'MESH']

    # Existing vendored FBXs already contain the old five clips. Strip their
    # imported actions before rebaking so the export cannot retain stale takes
    # or auto-suffixed duplicates.
    for obj in new:
        if obj.animation_data:
            obj.animation_data_clear()
    for action in [a for a in bpy.data.actions if a not in before_actions]:
        bpy.data.actions.remove(action)

    # Legacy raw FBXs may have a same-stem texture beside them. Vendored FBXs
    # and round-3 VRMs embed textures, so this step is deliberately optional.
    texture_path = path.with_suffix(".png")
    if texture_path.exists():
        img = bpy.data.images.load(str(texture_path))
        for mesh in meshes:
            for slot in mesh.material_slots:
                mat = slot.material
                if not mat or not mat.node_tree:
                    continue
                for node in mat.node_tree.nodes:
                    if node.type == 'TEX_IMAGE':
                        node.image = img
    return tgt, meshes


def resolve_bones(tgt):
    """Map canonical mixamo names -> actual bone names (prefixes vary per avatar)."""
    actual = {}
    for b in tgt.data.bones:
        canon = b.name.split(":")[-1]
        if canon in BONE_MAP and canon not in actual:
            actual[canon] = b.name
    missing = [c for c in BONE_MAP if c not in actual]
    if missing:
        raise RuntimeError("unmapped bones: " + ", ".join(missing))
    return actual


def bake_all(src, tgt, align_rest_directions=True):
    """Bake every ANIMS clip from src armature onto tgt armature."""
    sc = bpy.context.scene
    actual = resolve_bones(tgt)
    canon_of = {v: k for k, v in actual.items()}

    # rest world rotations (armature objects are at identity, but be exact)
    src_rest = {}
    src_dir = {}
    for b in src.data.bones:
        src_rest[b.name] = rot_of(src.matrix_world @ b.matrix_local)
        d = (src.matrix_world @ b.tail_local) - (src.matrix_world @ b.head_local)
        src_dir[b.name] = d.normalized()
    tgt_rest = {}
    tgt_rest_mat = {}
    for b in tgt.data.bones:
        tgt_rest[b.name] = rot_of(tgt.matrix_world @ b.matrix_local)
        tgt_rest_mat[b.name] = b.matrix_local.copy()

    src_hip_h = (src.matrix_world @ src.data.bones["DEF-hips"].head_local).z
    tgt_hip_h = (tgt.matrix_world @ tgt.data.bones[actual["Hips"]].head_local).z
    hip_scale = tgt_hip_h / src_hip_h
    src_hips_rest_loc = (src.matrix_world @ src.data.bones["DEF-hips"].head_local)
    tgt_hips_rest_loc = (tgt.matrix_world @ tgt.data.bones[actual["Hips"]].head_local)

    # mapped bones in parent-first order
    ordered = []
    def walk(b):
        if b.name in canon_of:
            ordered.append(b.name)
        for c in b.children:
            walk(c)
    for b in tgt.data.bones:
        if b.parent is None:
            walk(b)

    # Virtual aligned rest: older FBXs do not all rest in a clean T-pose, so
    # rotate their mapped bones until their directions match the source. glTF
    # nodes do not encode Blender-style bone tails; Blender synthesizes them
    # from child nodes, making direction alignment destructive for VRM rigs.
    # Those targets keep their exact skinning rest orientation instead.
    virt = {}        # bone name -> aligned world rest rotation
    for bn in ordered:
        if not align_rest_directions:
            virt[bn] = tgt_rest[bn]
            continue
        bone = tgt.data.bones[bn]
        anc = bone.parent
        while anc is not None and anc.name not in virt:
            anc = anc.parent
        if anc is None:
            w_pre = tgt_rest[bn]
        else:
            rel = rot_of(tgt_rest_mat[anc.name].inverted() @ tgt_rest_mat[bn])
            w_pre = virt[anc.name] @ rel
        d_pre = (w_pre @ Vector((0.0, 1.0, 0.0))).normalized()
        q = d_pre.rotation_difference(src_dir[BONE_MAP[canon_of[bn]]])
        virt[bn] = q @ w_pre

    tgt.animation_data_create()
    src.animation_data_create()
    baked = []
    for out_name, act_name in ANIMS:
        act = bpy.data.actions.get(act_name)
        if not act:
            if out_name == "Sit":
                procedural = build_procedural_sit(tgt, actual)
                baked.append((out_name, procedural, 0, 1))
                print("  baked Sit 0 1 (procedural fallback)")
                continue
            print("  !! missing source action", act_name)
            continue
        src.animation_data.action = act
        try:
            src.animation_data.action_slot = act.slots[0]
        except Exception:
            pass

        new_act = bpy.data.actions.new("A_" + out_name)
        tgt.animation_data.action = new_act

        f0, f1 = int(act.frame_range[0]), int(act.frame_range[1])
        prev_q = {}
        for f in range(f0, f1 + 1):
            sc.frame_set(f)
            goal_world = {}   # bone -> world rotation quaternion
            for bn in ordered:
                sn = BONE_MAP[canon_of[bn]]
                spb = src.pose.bones[sn]
                pb = tgt.pose.bones[bn]

                s_pose = rot_of(src.matrix_world @ spb.matrix)
                canonical = canon_of[bn]
                semantic_child = SEMANTIC_CHILD.get(canonical)
                if not align_rest_directions and semantic_child:
                    source_child = src.pose.bones[BONE_MAP[semantic_child]]
                    source_head = (src.matrix_world @ spb.matrix).translation
                    source_tail = (src.matrix_world @ source_child.matrix).translation
                    source_direction = (source_tail - source_head).normalized()

                    target_data_bone = tgt.data.bones[bn]
                    target_child_bone = tgt.data.bones[actual[semantic_child]]
                    target_direction = (
                        tgt.matrix_world.to_3x3()
                        @ (target_child_bone.head_local
                           - target_data_bone.head_local)).normalized()
                    swing = target_direction.rotation_difference(source_direction)
                    g = swing @ tgt_rest[bn]
                else:
                    delta = s_pose @ src_rest[sn].inverted()
                    g = delta @ virt[bn]
                goal_world[bn] = g

                bone = tgt.data.bones[bn]
                if bone.parent is None:
                    # full basis matrix for the root (hips): rotation + location
                    s_loc = (src.matrix_world @ src.pose.bones[sn].matrix).translation
                    d_loc = (s_loc - src_hips_rest_loc) * hip_scale
                    g_loc = tgt_hips_rest_loc + d_loc
                    goal_m = Matrix.Translation(g_loc) @ g.to_matrix().to_4x4()
                    basis = tgt_rest_mat[bn].inverted() @ tgt.matrix_world.inverted() @ goal_m
                    q = basis.to_quaternion()
                    if bn in prev_q and q.dot(prev_q[bn]) < 0.0:
                        q.negate()
                    prev_q[bn] = q.copy()
                    pb.rotation_mode = 'QUATERNION'
                    pb.rotation_quaternion = q
                    pb.location = basis.to_translation()
                    pb.keyframe_insert("rotation_quaternion", frame=f)
                    pb.keyframe_insert("location", frame=f)
                else:
                    # find nearest mapped ancestor for parent world rotation
                    anc = bone.parent
                    while anc is not None and anc.name not in goal_world:
                        anc = anc.parent
                    if anc is None:
                        p_world = rot_of(tgt.matrix_world)
                        l_rot = rot_of(tgt_rest_mat[bn])
                    else:
                        p_world = goal_world[anc.name]
                        l_rot = (tgt_rest_mat[anc.name].inverted() @ tgt_rest_mat[bn]).to_quaternion()
                    # M_pose_world_rot = P_world @ L @ basis  =>  basis = L^-1 @ P_world^-1 @ goal
                    q = l_rot.inverted() @ p_world.inverted() @ g
                    if bn in prev_q and q.dot(prev_q[bn]) < 0.0:
                        q.negate()
                    prev_q[bn] = q.copy()
                    pb.rotation_mode = 'QUATERNION'
                    pb.rotation_quaternion = q
                    pb.keyframe_insert("rotation_quaternion", frame=f)
        baked.append((out_name, new_act, f0, f1))
        print("  baked", out_name, f0, f1)
    return baked


def build_procedural_sit(tgt, actual):
    """Build a two-frame seated loop if the source library lacks one."""
    tgt.animation_data_create()
    action = bpy.data.actions.new("A_Sit")
    tgt.animation_data.action = action
    rotations = {
        "LeftUpLeg": (-80.0, 0.0, 0.0),
        "RightUpLeg": (-80.0, 0.0, 0.0),
        "LeftLeg": (75.0, 0.0, 0.0),
        "RightLeg": (75.0, 0.0, 0.0),
        "LeftArm": (-25.0, 0.0, -22.0),
        "RightArm": (-25.0, 0.0, 22.0),
        "LeftForeArm": (-55.0, 0.0, 0.0),
        "RightForeArm": (-55.0, 0.0, 0.0),
    }
    for frame in (0, 1):
        for canonical, bone_name in actual.items():
            pose_bone = tgt.pose.bones[bone_name]
            pose_bone.rotation_mode = 'QUATERNION'
            euler = rotations.get(canonical, (0.0, 0.0, 0.0))
            pose_bone.rotation_quaternion = Quaternion(
                (1.0, 0.0, 0.0), math.radians(euler[0]))
            if euler[1] or euler[2]:
                pose_bone.rotation_mode = 'XYZ'
                pose_bone.rotation_euler = tuple(math.radians(v) for v in euler)
                pose_bone.keyframe_insert("rotation_euler", frame=frame)
            else:
                pose_bone.keyframe_insert("rotation_quaternion", frame=frame)
        hips = tgt.pose.bones[actual["Hips"]]
        hips.location.z = -0.22 * tgt.data.bones[actual["Hips"]].head_local.z
        hips.keyframe_insert("location", frame=frame)
    return action


def qa_render(tgt, meshes, stem, baked):
    """Render an upright walk frame and a seated frame for visual QA."""
    sc = bpy.context.scene
    clips = [
        next((b for b in baked if b[0] == name), None)
        for name in ("Walk", "Sit")
    ]
    if not any(clips):
        return

    for obj in bpy.data.objects:
        obj.hide_render = obj not in meshes

    cam_data = bpy.data.cameras.new("QACam")
    cam = bpy.data.objects.new("QACam", cam_data)
    sc.collection.objects.link(cam)
    cam.hide_render = False
    sc.camera = cam
    sun = bpy.data.objects.new("QASun", bpy.data.lights.new("QASun", 'SUN'))
    sc.collection.objects.link(sun)
    sun.hide_render = False
    sun.data.energy = 4
    sun.rotation_euler = (math.radians(50), math.radians(15), 0)
    world = bpy.data.worlds.new("QAWorld")
    sc.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.7, 0.8, 1.0, 1)

    sc.render.engine = 'BLENDER_EEVEE'
    sc.render.image_settings.file_format = 'PNG'
    sc.render.film_transparent = False
    sc.render.resolution_x = 420
    sc.render.resolution_y = 520
    sc.render.resolution_percentage = 100
    QA_DIR.mkdir(parents=True, exist_ok=True)
    for clip in clips:
        if not clip:
            continue
        tag, act, f0, f1 = clip
        tgt.animation_data.action = act
        try:
            tgt.animation_data.action_slot = next(
                slot for slot in act.slots if slot.target_id_type == 'OBJECT')
        except Exception:
            pass
        f = f0 + (f1 - f0) // 2
        sc.frame_set(f)
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        evaluated = [mesh.evaluated_get(depsgraph) for mesh in meshes]
        points = [
            mesh.matrix_world @ Vector(corner)
            for mesh in evaluated
            for corner in mesh.bound_box
        ]
        minimum = Vector(tuple(min(p[i] for p in points) for i in range(3)))
        maximum = Vector(tuple(max(p[i] for p in points) for i in range(3)))
        center = (minimum + maximum) * 0.5
        size = maximum - minimum
        frame_size = max(1.0, size.z, size.x * 1.35, size.y * 1.35)
        cam.location = center + Vector((1.10 * frame_size, -2.45 * frame_size,
                                        0.10 * size.z))
        cam.rotation_euler = (
            center - cam.location).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = str(QA_DIR / f"{stem}_{tag}.png")
        bpy.ops.render.render(write_still=True)


def prepare_vrm_fbx_materials(meshes, stem):
    """Make VRM unlit base maps discoverable by Blender's FBX exporter.

    Round-3 materials arrive as Image -> Emission -> Mix Shader. Blender's FBX
    exporter only carries textures connected through a supported Principled
    surface, and packed glTF images need a real path while export runs. The
    source GLBs declare every material opaque, so deliberately leave Alpha at
    1.0 even when unused texture alpha bytes are present.
    """
    stage = tempfile.TemporaryDirectory(prefix="sprawl-avatar-textures-")
    stage_path = Path(stage.name)
    materialized = {}
    materials = []
    for mesh in meshes:
        for slot in mesh.material_slots:
            if slot.material and slot.material not in materials:
                materials.append(slot.material)

    for material in materials:
        material.use_nodes = True
        nodes = material.node_tree.nodes
        links = material.node_tree.links
        image_nodes = [node for node in nodes
                       if node.type == 'TEX_IMAGE' and node.image]
        if not image_nodes:
            continue
        image_node = image_nodes[0]
        image = image_node.image
        if image not in materialized:
            destination = stage_path / "{}_base_{:02d}.png".format(
                stem, len(materialized))
            image.filepath_raw = str(destination)
            image.file_format = 'PNG'
            image.save()
            if not destination.exists() or destination.stat().st_size == 0:
                raise RuntimeError("failed to materialize texture {}".format(image.name))
            materialized[image] = destination

        principled = next((node for node in nodes
                           if node.type == 'BSDF_PRINCIPLED'), None)
        if principled is None:
            principled = nodes.new('ShaderNodeBsdfPrincipled')
        output = next((node for node in nodes
                       if node.type == 'OUTPUT_MATERIAL'), None)
        if output is None:
            output = nodes.new('ShaderNodeOutputMaterial')
        links.new(image_node.outputs['Color'], principled.inputs['Base Color'])
        principled.inputs['Metallic'].default_value = 0.0
        principled.inputs['Roughness'].default_value = 0.75
        principled.inputs['Alpha'].default_value = 1.0
        links.new(principled.outputs['BSDF'], output.inputs['Surface'])

    if not materialized:
        stage.cleanup()
        raise RuntimeError("no VRM base-map textures found for {}".format(stem))
    return stage, len(materialized)


def export_avatar(src, tgt, meshes, stem, baked):
    # Keep only freshly baked target actions so stale FBX takes and the source
    # library cannot leak into the exported animation set.
    keep_actions = {entry[1] for entry in baked}
    if src.animation_data:
        src.animation_data.action = None
    for action in list(bpy.data.actions):
        if action not in keep_actions:
            bpy.data.actions.remove(action)

    # select only the avatar
    bpy.ops.object.select_all(action='DESELECT')
    tgt.select_set(True)
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = tgt
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    out = OUT_DIR / (stem + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=str(out),
        use_selection=True,
        object_types={'ARMATURE', 'MESH'},
        add_leaf_bones=False,
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=True,
        bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=0.0,
        mesh_smooth_type='FACE',
        path_mode='COPY',
        embed_textures=True,
    )
    print("  exported", out, out.stat().st_size // 1024, "KB")
    return out


def validate_exported_avatar(out, stem, expected_texture_count):
    """Clean-import the FBX, assert its payload, and render its real output."""
    with tempfile.TemporaryDirectory(prefix="sprawl-avatar-validate-") as directory:
        import_path = Path(directory) / out.name
        shutil.copy2(out, import_path)
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.wm.fbx_import(
            filepath=str(import_path), use_anim=True, ignore_leaf_bones=True)

        armatures = [obj for obj in bpy.data.objects if obj.type == 'ARMATURE']
        meshes = [obj for obj in bpy.data.objects if obj.type == 'MESH' and (
            obj.parent in armatures or any(
                modifier.type == 'ARMATURE' and modifier.object in armatures
                for modifier in obj.modifiers))]
        if len(armatures) != 1 or not meshes:
            raise RuntimeError("invalid clean FBX scene for {}".format(stem))

        assigned_images = {
            node.image
            for mesh in meshes
            for slot in mesh.material_slots
            if slot.material and slot.material.node_tree
            for node in slot.material.node_tree.nodes
            if node.type == 'TEX_IMAGE' and node.image
            and node.image.source == 'FILE' and node.image.packed_file
        }
        if expected_texture_count and len(assigned_images) < expected_texture_count:
            raise RuntimeError(
                "clean FBX lost textures for {}: expected {}, found {}".format(
                    stem, expected_texture_count, len(assigned_images)))

        baked = []
        for clip_name, _ in ANIMS:
            suffix = "|A_" + clip_name
            action = next((candidate for candidate in bpy.data.actions
                           if candidate.name == "A_" + clip_name
                           or candidate.name.endswith(suffix)), None)
            if action is None:
                raise RuntimeError("clean FBX missing {} for {}".format(
                    clip_name, stem))
            f0, f1 = (int(value) for value in action.frame_range)
            baked.append((clip_name, action, f0, f1))

        expected_actions = {"A_" + clip_name for clip_name, _ in ANIMS}
        imported_actions = {
            action.name.rsplit('|', 1)[-1] for action in bpy.data.actions}
        if imported_actions != expected_actions:
            raise RuntimeError("clean FBX actions for {}: expected {}, found {}".format(
                stem, sorted(expected_actions), sorted(imported_actions)))
        qa_render(armatures[0], meshes, stem, baked)
        print("  validated clean FBX: {} textures, {} actions".format(
            len(assigned_images), len(bpy.data.actions)))


def orient_vrm_for_export(tgt, meshes):
    """Match the legacy Mixamo -Y facing without double-turning child meshes."""
    members = set(meshes)
    members.add(tgt)
    turn = Matrix.Rotation(math.pi, 4, 'Z')
    for obj in members:
        if obj.parent not in members:
            obj.matrix_world = turn @ obj.matrix_world
    bpy.context.view_layer.update()


def avatar_targets():
    new_variants = set(NEW_AVATAR_VARIANTS.values())
    targets = [
        (path, path.stem)
        for path in sorted(VENDORED_AVATAR_DIR.glob("*.fbx"))
        if path.stem not in new_variants
    ]
    for source_stem, variant in sorted(NEW_AVATAR_VARIANTS.items()):
        path = NEW_AVATAR_DIR / (source_stem + ".vrm")
        if path.exists():
            targets.append((path, variant))
        else:
            print("  !! missing new avatar source", path)
    return targets


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    only = args or None
    for source_path, variant in avatar_targets():
        if only and not any(token.lower() in variant.lower() for token in only):
            continue
        print("=== {} ({})".format(variant, source_path.name))
        try:
            src = import_sources()
            if source_path.suffix.lower() == ".fbx":
                tgt, meshes = import_avatar(source_path)
                baked = bake_all(src, tgt)
            else:
                tgt, meshes = import_avatar(source_path)
                orient_vrm_for_export(tgt, meshes)
                baked = bake_all(src, tgt, align_rest_directions=False)
            expected = {name for name, _ in ANIMS}
            actual = {name for name, _, _, _ in baked}
            missing = sorted(expected - actual)
            if missing:
                raise RuntimeError("missing baked clips: " + ", ".join(missing))
            texture_stage = None
            expected_texture_count = 0
            try:
                if source_path.suffix.lower() != ".fbx":
                    texture_stage, expected_texture_count = (
                        prepare_vrm_fbx_materials(meshes, variant))
                out = export_avatar(src, tgt, meshes, variant, baked)
            finally:
                if texture_stage:
                    texture_stage.cleanup()
            validate_exported_avatar(out, variant, expected_texture_count)
        except Exception as e:
            print("  !! FAILED", variant, repr(e))
            raise


if __name__ == "__main__":
    main()
