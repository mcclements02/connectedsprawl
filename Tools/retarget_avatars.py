"""Retarget Quaternius Universal Animation Library locomotion clips onto
100Avatars Mixamo-rigged characters and export game-ready FBX files.

Run headless:  python3 bake_avatars.py  (uses the bpy module, Blender 5.x API)

Inputs:
  /tmp/ual/AnimLib.gltf                  Universal Animation Library (CC0, Quaternius)
  /tmp/artwork/avatars/<NNN_Name>.fbx    100Avatars character (CC0, Polygonal Mind)
  /tmp/artwork/avatars/<NNN_Name>.png    its texture
Outputs:
  /tmp/artwork/out/<Name>.fbx            mesh + skeleton + baked anim takes
  /tmp/artwork/qa/<Name>_*.png           QA renders

Retarget algorithm: world-space rotation transfer with rest-pose offset.
Both rigs are T-pose, facing -Y, meters, armature object at identity, so for
each mapped bone:   R_goal = (R_src_pose @ R_src_rest^-1) @ R_tgt_rest
Hips also receives the source root translation scaled by hip-height ratio.
"""
import os
import sys
import math
import bpy
from mathutils import Matrix, Quaternion, Vector

UAL_GLTF = "/tmp/ual/AnimLib.gltf"
AVATAR_DIR = "/tmp/artwork/avatars"
OUT_DIR = "/tmp/artwork/out"
QA_DIR = "/tmp/artwork/qa"

# Export name -> UAL action name
ANIMS = [
    ("Idle", "Idle_Loop"),
    ("Walk", "Walk_Loop"),
    ("Jog", "Jog_Fwd_Loop"),
    ("Talk", "Idle_Talking_Loop"),
    ("WalkFormal", "Walk_Formal_Loop"),
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


def rot_of(m):
    return m.to_quaternion()


def import_sources():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=UAL_GLTF)
    src = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    return src


def import_avatar(stem):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(AVATAR_DIR, stem + ".fbx"))
    new = [o for o in bpy.data.objects if o not in before]
    tgt = next(o for o in new if o.type == 'ARMATURE')
    meshes = [o for o in new if o.type == 'MESH']

    # Rewire the texture: the FBX references the original repo filename which
    # is not on disk; point every image node at our downloaded png instead.
    img = bpy.data.images.load(os.path.join(AVATAR_DIR, stem + ".png"))
    for m in meshes:
        for slot in m.material_slots:
            mat = slot.material
            if not mat or not mat.node_tree:
                continue
            for n in mat.node_tree.nodes:
                if n.type == 'TEX_IMAGE':
                    n.image = img
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


def bake_all(src, tgt):
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

    # Virtual aligned rest: not every avatar rests in a clean T-pose, so rotate
    # each mapped target bone (in parent-first order, accumulating down the
    # chain) until its rest direction matches the source bone's rest direction.
    # The transfer below then treats this aligned orientation as the rest.
    virt = {}        # bone name -> aligned world rest rotation
    for bn in ordered:
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
                s_pose = rot_of(src.matrix_world @ spb.matrix)
                delta = s_pose @ src_rest[sn].inverted()
                g = delta @ virt[bn]
                goal_world[bn] = g

                pb = tgt.pose.bones[bn]
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
                        p_rest = rot_of(tgt.matrix_world)
                        l_rot = rot_of(tgt_rest_mat[bn])
                    else:
                        p_world = goal_world[anc.name]
                        p_rest = tgt_rest[anc.name]
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


def qa_render(tgt, meshes, stem, baked):
    """Render a few frames of the Walk bake for visual QA."""
    sc = bpy.context.scene
    walk = next((b for b in baked if b[0] == "Walk"), None)
    if not walk:
        return
    _, act, f0, f1 = walk
    tgt.animation_data.action = act
    try:
        tgt.animation_data.action_slot = act.slots[0]
    except Exception:
        pass

    cam_data = bpy.data.cameras.new("QACam")
    cam = bpy.data.objects.new("QACam", cam_data)
    sc.collection.objects.link(cam)
    cam.location = (1.6, -2.2, 1.0)
    cam.rotation_euler = (math.radians(78), 0, math.radians(34))
    sc.camera = cam
    sun = bpy.data.objects.new("QASun", bpy.data.lights.new("QASun", 'SUN'))
    sc.collection.objects.link(sun)
    sun.data.energy = 4
    sun.rotation_euler = (math.radians(50), math.radians(15), 0)
    world = bpy.data.worlds.new("QAWorld")
    sc.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.7, 0.8, 1.0, 1)

    sc.render.engine = 'CYCLES'
    sc.cycles.samples = 24
    sc.cycles.device = 'CPU'
    sc.render.resolution_x = 420
    sc.render.resolution_y = 520
    os.makedirs(QA_DIR, exist_ok=True)
    for tag, f in (("a", f0 + (f1 - f0) // 4), ("b", f0 + 3 * (f1 - f0) // 4)):
        sc.frame_set(f)
        sc.render.filepath = os.path.join(QA_DIR, f"{stem}_{tag}.png")
        bpy.ops.render.render(write_still=True)


def export_avatar(src, tgt, meshes, stem, baked):
    name = stem.split("_", 1)[1]
    # select only the avatar
    bpy.ops.object.select_all(action='DESELECT')
    tgt.select_set(True)
    for m in meshes:
        m.select_set(True)
    bpy.context.view_layer.objects.active = tgt
    os.makedirs(OUT_DIR, exist_ok=True)
    out = os.path.join(OUT_DIR, name + ".fbx")
    bpy.ops.export_scene.fbx(
        filepath=out,
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
    print("  exported", out, os.path.getsize(out) // 1024, "KB")


def main():
    stems = sorted(p[:-4] for p in os.listdir(AVATAR_DIR) if p.endswith(".fbx"))
    only = sys.argv[1:] if len(sys.argv) > 1 else None
    for stem in stems:
        if only and not any(o in stem for o in only):
            continue
        print("=== " + stem)
        try:
            src = import_sources()
            tgt, meshes = import_avatar(stem)
            baked = bake_all(src, tgt)
            export_avatar(src, tgt, meshes, stem, baked)
            qa_render(tgt, meshes, stem, baked)
        except Exception as e:
            print("  !! FAILED", stem, repr(e))


if __name__ == "__main__":
    main()
