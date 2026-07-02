"""Procedural Cyberpunk Humans Integration.

Imports human character textures, instantiates material variants,
and spawns character screens/billboards in strategic city locations (e.g. Zarri at
PlayerStart, Rohan/Maya downtown, Ty/Marcus in Rail Yards).
"""
import os
import random
import unreal

random.seed(101)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

# Load the city level
les.load_level("/Game/Maps/TestMap")

# Helper to clear old human actors on run
def clear_old_humans(prefix="City_Human_"):
    killed = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith(prefix):
            eas.destroy_actor(a)
            killed += 1
    unreal.log(f"[Humans] Cleared {killed} old human actors.")
    return killed

clear_old_humans()

# ------------------------------------------------------------------
# 1. Folders and Paths Setup
# ------------------------------------------------------------------
staging_dir = "/Users/matthewx/code/ConnectedSprawl/staging"
texture_dir = "/Game/CityArt/Textures"
material_dir = "/Game/CityArt/Materials"

unreal.EditorAssetLibrary.make_directory(texture_dir)
unreal.EditorAssetLibrary.make_directory(material_dir)

# ------------------------------------------------------------------
# 2. Import Textures
# ------------------------------------------------------------------
human_files = {
    "zarri": "zarri.png",
    "ty": "ty.png",
    "rohan": "rohan.png",
    "maya": "maya.png",
    "marcus": "marcus.png"
}

textures = {}
for key, filename in human_files.items():
    file_path = os.path.join(staging_dir, filename)
    asset_path = f"{texture_dir}/{key}"
    
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"[Humans] Texture already exists, loading: {asset_path}")
        tex = unreal.load_asset(asset_path)
    else:
        unreal.log(f"[Humans] Importing texture from {file_path}")
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', file_path)
        task.set_editor_property('destination_path', texture_dir)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        
        atools.import_asset_tasks([task])
        tex = unreal.load_asset(asset_path)
        
    textures[key] = tex

# ------------------------------------------------------------------
# 3. Resolve Master Holographic Material (Create if missing)
# ------------------------------------------------------------------
master_material_path = f"{material_dir}/M_CreatureHoloMaster"
if not unreal.EditorAssetLibrary.does_asset_exist(master_material_path):
    unreal.log(f"[Humans] Master material missing, creating master material at {master_material_path}")
    master_mat = atools.create_asset("M_CreatureHoloMaster", material_dir, unreal.Material, unreal.MaterialFactoryNew())
    master_mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    master_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    
    tex_param = MEL.create_material_expression(master_mat, unreal.MaterialExpressionTextureSampleParameter2D, -450, 0)
    tex_param.set_editor_property("parameter_name", "CreatureTex")
    
    tint_param = MEL.create_material_expression(master_mat, unreal.MaterialExpressionVectorParameter, -450, -200)
    tint_param.set_editor_property("parameter_name", "ColorTint")
    tint_param.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    
    intensity_param = MEL.create_material_expression(master_mat, unreal.MaterialExpressionScalarParameter, -450, 200)
    intensity_param.set_editor_property("parameter_name", "EmissiveIntensity")
    intensity_param.set_editor_property("default_value", 5.0)
    
    mul1 = MEL.create_material_expression(master_mat, unreal.MaterialExpressionMultiply, -250, -50)
    MEL.connect_material_expressions(tex_param, "", mul1, "A")
    MEL.connect_material_expressions(tint_param, "", mul1, "B")
    
    mul2 = MEL.create_material_expression(master_mat, unreal.MaterialExpressionMultiply, -100, 50)
    MEL.connect_material_expressions(mul1, "", mul2, "A")
    MEL.connect_material_expressions(intensity_param, "", mul2, "B")
    
    MEL.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(master_mat)
    unreal.EditorAssetLibrary.save_loaded_asset(master_mat)
else:
    master_mat = unreal.load_asset(master_material_path)

# ------------------------------------------------------------------
# 4. Create Material Instances
# ------------------------------------------------------------------
def get_or_create_material_instance(name, texture, tint, intensity):
    instance_path = f"{material_dir}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(instance_path):
        mi = unreal.load_asset(instance_path)
    else:
        unreal.log(f"[Humans] Creating material instance at {instance_path}")
        mi = atools.create_asset(name, material_dir, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master_mat)
        
    MEL.set_material_instance_texture_parameter_value(mi, "CreatureTex", texture)
    MEL.set_material_instance_vector_parameter_value(mi, "ColorTint", tint)
    MEL.set_material_instance_scalar_parameter_value(mi, "EmissiveIntensity", intensity)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi

mi_zarri = get_or_create_material_instance("MI_Human_Zarri", textures["zarri"], unreal.LinearColor(1.0, 0.95, 0.85, 1.0), 5.0)
mi_ty = get_or_create_material_instance("MI_Human_Ty", textures["ty"], unreal.LinearColor(0.2, 0.7, 1.0, 1.0), 5.0)
mi_rohan = get_or_create_material_instance("MI_Human_Rohan", textures["rohan"], unreal.LinearColor(1.0, 0.65, 0.2, 1.0), 4.5)
mi_maya = get_or_create_material_instance("MI_Human_Maya", textures["maya"], unreal.LinearColor(0.8, 0.4, 1.0, 1.0), 5.0)
mi_marcus = get_or_create_material_instance("MI_Human_Marcus", textures["marcus"], unreal.LinearColor(1.0, 0.3, 0.2, 1.0), 4.0)

# ------------------------------------------------------------------
# 5. Spawning Character Billboards
# ------------------------------------------------------------------
CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")

def spawn_billboard(label, loc, scale, mat, rot=None):
    if rot is None:
        rot = unreal.Rotator(0, 0, 0)
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    smc = a.static_mesh_component
    smc.set_static_mesh(CUBE)
    smc.set_material(0, mat)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    return a

# Find locations in the level
actors = eas.get_all_level_actors()

player_start_loc = None
alley_locs = []
high_bldg_locs = []

for a in actors:
    label = a.get_actor_label()
    loc = a.get_actor_location()
    
    if a.get_class().get_name() == "PlayerStart":
        player_start_loc = loc
    elif label.startswith("City_Alley_"):
        alley_locs.append(loc)
    elif label.startswith("City_Bldg_"):
        smc = a.static_mesh_component
        if smc:
            try:
                mesh = smc.get_static_mesh()
            except AttributeError:
                mesh = smc.get_editor_property("static_mesh")
            if mesh:
                bb = mesh.get_bounding_box()
                height = (bb.max.z - bb.min.z) * a.get_actor_scale3d().z
                if height > 2500.0:
                    high_bldg_locs.append((loc, height))

# 1) Spawn Zarri (Right next to Player Start at street level)
if player_start_loc:
    # Placed on the sidewalk facing the street
    pos = unreal.Vector(player_start_loc.x + 300, player_start_loc.y + 300, 200.0)
    rot = unreal.Rotator(0, 0, 135.0)
    scale = unreal.Vector(0.05, 2.5, 2.5) # Large hero billboard
    spawn_billboard("City_Human_Zarri", pos, scale, mi_zarri, rot)
    unreal.log("[Humans] Spawned Zarri hero billboard near PlayerStart.")

# 2) Spawn Ty (Alley/Street level in the south/Rail Yards area)
# Filter alleys that are in the south (y < 0)
south_alleys = [l for l in alley_locs if l.y < 0]
if not south_alleys:
    south_alleys = alley_locs

if south_alleys:
    # Pick two locations for Ty billboards
    for i in range(min(2, len(south_alleys))):
        loc = south_alleys[i]
        pos = unreal.Vector(loc.x + 150 * (i + 1), loc.y - 100, 120.0)
        rot = unreal.Rotator(0, 0, 90.0)
        scale = unreal.Vector(0.05, 1.8, 1.8)
        spawn_billboard(f"City_Human_Ty_{i}", pos, scale, mi_ty, rot)
    unreal.log(f"[Humans] Spawned {min(2, len(south_alleys))} Ty billboards in southern alleys.")

# 3) Spawn Rohan (High floating billboard in the corporate downtown)
if high_bldg_locs:
    # Place Rohan on a skyscraper building facade facing the center road
    loc, height = high_bldg_locs[0]
    pos = unreal.Vector(loc.x + 500, loc.y + 500, height * 0.7)
    rot = unreal.Rotator(0, 0, 45.0)
    scale = unreal.Vector(0.05, 4.0, 4.0) # Huge corporate screen
    spawn_billboard("City_Human_Rohan", pos, scale, mi_rohan, rot)
    unreal.log("[Humans] Spawned Rohan skyscraper screen downtown.")

# 4) Spawn Maya (On another high downtown building facade)
if len(high_bldg_locs) > 1:
    loc, height = high_bldg_locs[1]
    pos = unreal.Vector(loc.x - 500, loc.y + 500, height * 0.8)
    rot = unreal.Rotator(0, 0, 315.0)
    scale = unreal.Vector(0.05, 4.0, 4.0)
    spawn_billboard("City_Human_Maya", pos, scale, mi_maya, rot)
    unreal.log("[Humans] Spawned Maya skyscraper screen downtown.")

# 5) Spawn Marcus (Near docks/Rail Yards in the south)
# Pick a building in the south (y < -2000)
south_bldgs = []
for a in actors:
    if a.get_actor_label().startswith("City_Bldg_") and a.get_actor_location().y < -2000:
        south_bldgs.append(a.get_actor_location())

if south_bldgs:
    loc = random.choice(south_bldgs)
    pos = unreal.Vector(loc.x + 400, loc.y - 400, 150.0)
    rot = unreal.Rotator(0, 0, 225.0)
    scale = unreal.Vector(0.05, 2.2, 2.2)
    spawn_billboard("City_Human_Marcus", pos, scale, mi_marcus, rot)
    unreal.log("[Humans] Spawned Marcus street billboard in Rail Yards.")

# ------------------------------------------------------------------
# 6. Save & Finalize
# ------------------------------------------------------------------
saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(material_dir, only_if_is_dirty=False)
unreal.log(f"[Humans] Spawning completed. Level saved: {saved}. DONE")
