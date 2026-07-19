"""Import the credited artwork (human avatars + street props) and dress the city.

What it does:
  1. Imports every Content/Import/Pedestrians/<Name>.fbx as a skeletal mesh
     with its baked animation takes into /Game/Pedestrians/<Name>/ and renames
     the assets to the names the C++ expects:
         SK_<Name>, <Name>_Idle, <Name>_Walk, <Name>_Jog, <Name>_Talk,
         <Name>_WalkFormal, <Name>_Sprint, <Name>_Sit
  2. Imports the KayKit street props into /Game/CityProps/.
  3. Scatters benches, hydrants, dumpsters, boxes, litter and park bushes
     along the sidewalks of TestMap (deterministic, idempotent).

Run AFTER compiling the C++ module, BEFORE/with aaa_realism_overhaul.py:
  UnrealEditor <project> -ExecutePythonScript="<this file>" -unattended -nosplash
"""
import math
import os
import random
import unreal

PROJECT_CONTENT = unreal.SystemLibrary.get_project_content_directory()
PED_SRC = os.path.join(PROJECT_CONTENT, "Import", "Pedestrians")
PROP_SRC = os.path.join(PROJECT_CONTENT, "Import", "CityProps")

EAL = unreal.EditorAssetLibrary
atools = unreal.AssetToolsHelpers.get_asset_tools()

ANIM_SUFFIXES = [
    "Idle", "Walk", "Jog", "Talk", "WalkFormal", "Sprint", "Sit"
]


# ============================================================
# 1) Pedestrian avatars (skeletal, with baked animation takes)
# ============================================================
def import_avatar(fbx_path):
    name = os.path.splitext(os.path.basename(fbx_path))[0]
    dest = "/Game/Pedestrians/{}".format(name)
    if EAL.does_directory_exist(dest):
        EAL.delete_directory(dest)

    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    ui.import_animations = True
    ui.import_materials = True
    ui.import_textures = True
    ui.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
    ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
    ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = dest
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = ui
    atools.import_asset_tasks([task])

    # Rename to the deterministic names the C++ loads.
    renames = 0
    for path in EAL.list_assets(dest, recursive=True):
        asset = EAL.load_asset(path)
        if not asset:
            continue
        base = path.split("/")[-1].split(".")[0]
        new_base = None
        if isinstance(asset, unreal.SkeletalMesh):
            new_base = "SK_" + name
        elif isinstance(asset, unreal.AnimSequence):
            for suffix in ANIM_SUFFIXES:
                # Walk must not swallow WalkFormal
                if base.endswith("A_" + suffix):
                    new_base = "{}_{}".format(name, suffix)
                    break
        if new_base and base != new_base:
            EAL.rename_asset(path, "{}/{}".format(dest, new_base))
            renames += 1
    EAL.save_directory(dest)
    unreal.log("[Artwork] avatar {} imported ({} renames)".format(name, renames))


fbx_files = sorted(
    os.path.join(PED_SRC, f) for f in os.listdir(PED_SRC) if f.endswith(".fbx"))
for f in fbx_files:
    import_avatar(f)
unreal.log("[Artwork] {} avatars imported".format(len(fbx_files)))

# ============================================================
# 2) Street props (static meshes)
# ============================================================
PROP_DEST = "/Game/CityProps"

# prop -> (measure axis: 'h' height/'l' max horizontal length, target size cm)
PROP_SIZES = {
    "bench":       ("l", 170.0),
    "dumpster":    ("l", 190.0),
    "firehydrant": ("h", 75.0),
    "bush":        ("h", 100.0),
    "box_A":       ("h", 60.0),
    "box_B":       ("h", 50.0),
    "trash_A":     ("h", 35.0),
    "trash_B":     ("h", 25.0),
    "watertower":  ("h", 900.0),
}

prop_meshes = {}   # name -> (StaticMesh, uniform scale)


def import_prop(fbx_path):
    name = os.path.splitext(os.path.basename(fbx_path))[0]
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    ui.import_animations = False
    ui.import_materials = True
    ui.import_textures = True
    ui.static_mesh_import_data.set_editor_property("combine_meshes", True)
    ui.static_mesh_import_data.set_editor_property("convert_scene", True)
    ui.static_mesh_import_data.set_editor_property("auto_generate_collision", True)

    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = PROP_DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = ui
    atools.import_asset_tasks([task])

    mesh_path = "{}/{}.{}".format(PROP_DEST, name, name)
    mesh = EAL.load_asset(mesh_path)
    if not mesh:
        unreal.log_warning("[Artwork] prop failed to import: " + name)
        return
    bounds = mesh.get_bounds()
    ext = bounds.box_extent
    axis, target = PROP_SIZES.get(name, ("h", 100.0))
    measured = ext.z * 2.0 if axis == "h" else max(ext.x, ext.y) * 2.0
    scale = target / max(1.0, measured)
    prop_meshes[name] = (mesh, scale)
    unreal.log("[Artwork] prop {}: measured {:.0f}cm -> scale {:.2f}".format(
        name, measured, scale))


for f in sorted(os.listdir(PROP_SRC)):
    if f.endswith(".fbx"):
        import_prop(os.path.join(PROP_SRC, f))

# ============================================================
# 3) Dress the sidewalks of TestMap
# ============================================================
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

# Grid constants (must match build_city.py / SprawlCityGridSubsystem)
N = 7
BLOCK = 2000.0
ROAD = 600.0
STEP = BLOCK + ROAD
SPAN = N * STEP
START = -SPAN / 2 + STEP / 2
PARKS = {(1, 5), (2, 2), (5, 5)}
SURFACE_Z = 14.0


def cx(i):
    return START + i * STEP


LX0, LX1 = cx(4) - STEP / 2, cx(6) + STEP / 2
LY0, LY1 = cx(0) - STEP / 2, cx(1) + STEP / 2


def over_lake(x, y, margin=0.0):
    return (LX0 - margin < x < LX1 + margin and
            LY0 - margin < y < LY1 + margin)


for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_Prop_"):
        eas.destroy_actor(a)

rng = random.Random(20260610)
spawned = [0]


def place(prop, x, y, yaw, scale_mul=1.0):
    if prop not in prop_meshes:
        return
    mesh, scale = prop_meshes[prop]
    actor = eas.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, SURFACE_Z),
        unreal.Rotator(0.0, 0.0, yaw))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(scale * scale_mul,
                                          scale * scale_mul,
                                          scale * scale_mul))
    actor.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    actor.set_actor_label("City_Prop_{}_{}".format(prop, spawned[0]))
    spawned[0] += 1


# Edge index -> (outward normal x, y). Props sit on the sidewalk band just
# inside the curb; the pedestrians' walking line is at inset 120, so keep
# props between inset 40 and 75 to leave the lane clear.
EDGES = [(1, 0), (-1, 0), (0, 1), (0, -1)]


def edge_point(bx, by, edge, along, inset):
    ox, oy = EDGES[edge]
    half = BLOCK / 2.0 - inset
    x = cx(bx) + ox * half + (oy != 0) * along
    y = cx(by) + oy * half + (ox != 0) * along
    return x, y


def edge_yaw(edge):
    ox, oy = EDGES[edge]
    return math.degrees(math.atan2(oy, ox))


for bx in range(N):
    for by in range(N):
        if over_lake(cx(bx), cx(by)):
            continue
        if (bx, by) in PARKS:
            # Parks: a loose ring of bushes, benches facing inward,
            # and one landmark water tower in the (2,2) park.
            for k in range(rng.randint(10, 14)):
                ang = k * (2 * math.pi / 12.0) + rng.uniform(-0.2, 0.2)
                r = rng.uniform(620.0, 860.0)
                place("bush", cx(bx) + math.cos(ang) * r,
                      cx(by) + math.sin(ang) * r,
                      rng.uniform(0.0, 360.0),
                      rng.uniform(0.8, 1.35))
            for k in range(4):
                ang = k * (math.pi / 2.0) + math.pi / 4.0
                bx_, by_ = (cx(bx) + math.cos(ang) * 520.0,
                            cx(by) + math.sin(ang) * 520.0)
                place("bench", bx_, by_, math.degrees(ang) + 180.0)
            if (bx, by) == (2, 2):
                place("watertower", cx(bx) + 180.0, cx(by) - 140.0,
                      rng.uniform(0.0, 360.0))
            continue

        # Urban block: benches, a hydrant, a service corner, some litter.
        edges = [0, 1, 2, 3]
        for k in range(2):
            e = edges[rng.randint(0, 3)]
            along = rng.uniform(-680.0, 680.0)
            x, y = edge_point(bx, by, e, along, 60.0)
            # Long axis parallel to the street, seat facing the road.
            place("bench", x, y, edge_yaw(e) + 90.0)
        e = edges[rng.randint(0, 3)]
        x, y = edge_point(bx, by, e, rng.uniform(-700.0, 700.0), 45.0)
        place("firehydrant", x, y, rng.uniform(0.0, 360.0))

        # Service corner: dumpster plus a box or two.
        e = edges[rng.randint(0, 3)]
        along = rng.uniform(-600.0, 600.0)
        x, y = edge_point(bx, by, e, along, 95.0)
        place("dumpster", x, y, edge_yaw(e) + 90.0)
        for k in range(rng.randint(1, 2)):
            x, y = edge_point(bx, by, e, along + 130.0 + k * 75.0, 95.0)
            place("box_A" if rng.random() < 0.5 else "box_B", x, y,
                  rng.uniform(0.0, 360.0))

        # Litter keeps it lived-in.
        for k in range(rng.randint(1, 3)):
            e = edges[rng.randint(0, 3)]
            x, y = edge_point(bx, by, e, rng.uniform(-700.0, 700.0), 38.0)
            place("trash_A" if rng.random() < 0.5 else "trash_B", x, y,
                  rng.uniform(0.0, 360.0))

unreal.log("[Artwork] placed {} street props".format(spawned[0]))
saved = les.save_current_level()
unreal.log("[Artwork] level saved: {}. DONE".format(saved))
