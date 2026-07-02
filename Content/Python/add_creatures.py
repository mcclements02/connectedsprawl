"""Procedural Cyberpunk Creatures Integration.

Imports creature textures, creates a master holographic material,
instantiates material variants for each creature type, and spawns
double-sided glowing billboard screens (using thin cubes) across the city.
"""
import os
import random
import unreal

random.seed(42)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

# Load the city level
les.load_level("/Game/Maps/TestMap")

# Helper to clear old creature actors on run
def clear_old_creatures(prefix="City_Creature_"):
    killed = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith(prefix):
            eas.destroy_actor(a)
            killed += 1
    unreal.log(f"[Creatures] Cleared {killed} old creature actors.")
    return killed

clear_old_creatures()

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
creature_files = {
    "cyber_raccoon": "cyber_raccoon.png",
    "security_drone": "security_drone.png",
    "neon_critter": "neon_critter.png"
}

textures = {}
for key, filename in creature_files.items():
    file_path = os.path.join(staging_dir, filename)
    asset_path = f"{texture_dir}/{key}"
    
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"[Creatures] Texture already exists, loading: {asset_path}")
        tex = unreal.load_asset(asset_path)
    else:
        unreal.log(f"[Creatures] Importing texture from {file_path}")
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', file_path)
        task.set_editor_property('destination_path', texture_dir)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        
        atools.import_asset_tasks([task])
        tex = unreal.load_asset(asset_path)
        
    textures[key] = tex

# ------------------------------------------------------------------
# 3. Create Master Holographic Material (Additive/Unlit)
# ------------------------------------------------------------------
master_material_path = f"{material_dir}/M_CreatureHoloMaster"

if unreal.EditorAssetLibrary.does_asset_exist(master_material_path):
    master_mat = unreal.load_asset(master_material_path)
else:
    unreal.log(f"[Creatures] Creating master material at {master_material_path}")
    master_mat = atools.create_asset("M_CreatureHoloMaster", material_dir, unreal.Material, unreal.MaterialFactoryNew())
    
    # Configure Additive + Unlit properties
    master_mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    master_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    
    # 1) Texture parameter expression
    tex_param = MEL.create_material_expression(master_mat, unreal.MaterialExpressionTextureSampleParameter2D, -450, 0)
    tex_param.set_editor_property("parameter_name", "CreatureTex")
    
    # 2) Vector tint expression
    tint_param = MEL.create_material_expression(master_mat, unreal.MaterialExpressionVectorParameter, -450, -200)
    tint_param.set_editor_property("parameter_name", "ColorTint")
    tint_param.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    
    # 3) Scalar intensity expression
    intensity_param = MEL.create_material_expression(master_mat, unreal.MaterialExpressionScalarParameter, -450, 200)
    intensity_param.set_editor_property("parameter_name", "EmissiveIntensity")
    intensity_param.set_editor_property("default_value", 5.0)
    
    # 4) Multiply node 1 (Tex * Tint)
    mul1 = MEL.create_material_expression(master_mat, unreal.MaterialExpressionMultiply, -250, -50)
    MEL.connect_material_expressions(tex_param, "", mul1, "A")
    MEL.connect_material_expressions(tint_param, "", mul1, "B")
    
    # 5) Multiply node 2 (Mul1 * Intensity)
    mul2 = MEL.create_material_expression(master_mat, unreal.MaterialExpressionMultiply, -100, 50)
    MEL.connect_material_expressions(mul1, "", mul2, "A")
    MEL.connect_material_expressions(intensity_param, "", mul2, "B")
    
    # 6) Connect to Emissive Color
    MEL.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    
    # Compile & Save
    MEL.recompile_material(master_mat)
    unreal.EditorAssetLibrary.save_loaded_asset(master_mat)

# ------------------------------------------------------------------
# 4. Create Material Instances
# ------------------------------------------------------------------
def get_or_create_material_instance(name, texture, tint, intensity):
    instance_path = f"{material_dir}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(instance_path):
        mi = unreal.load_asset(instance_path)
    else:
        unreal.log(f"[Creatures] Creating material instance at {instance_path}")
        mi = atools.create_asset(name, material_dir, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master_mat)
        
    MEL.set_material_instance_texture_parameter_value(mi, "CreatureTex", texture)
    MEL.set_material_instance_vector_parameter_value(mi, "ColorTint", tint)
    MEL.set_material_instance_scalar_parameter_value(mi, "EmissiveIntensity", intensity)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi

mi_raccoon = get_or_create_material_instance("MI_Creature_Raccoon", textures["cyber_raccoon"], unreal.LinearColor(0.0, 1.0, 1.0, 1.0), 6.0)
mi_drone = get_or_create_material_instance("MI_Creature_Drone", textures["security_drone"], unreal.LinearColor(0.9, 0.95, 1.0, 1.0), 8.0)
mi_critter = get_or_create_material_instance("MI_Creature_Critter", textures["neon_critter"], unreal.LinearColor(0.2, 1.0, 0.2, 1.0), 7.0)

# ------------------------------------------------------------------
# 5. Spawning Billboard Screens
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

# Find locations in the level to spawn creatures
actors = eas.get_all_level_actors()

alley_locs = []
high_bldg_locs = []
park_locs = []

for a in actors:
    label = a.get_actor_label()
    loc = a.get_actor_location()
    
    if label.startswith("City_Alley_"):
        alley_locs.append(loc)
    elif label.startswith("City_Bldg_"):
        # Fetch bounding box height to locate downtown high-rises
        smc = a.static_mesh_component
        if smc:
            try:
                mesh = smc.get_static_mesh()
            except AttributeError:
                mesh = smc.get_editor_property("static_mesh")
            if mesh:
                bb = mesh.get_bounding_box()
                height = (bb.max.z - bb.min.z) * a.get_actor_scale3d().z
                # High-rises are usually > 2500 units high
                if height > 2500.0:
                    high_bldg_locs.append((loc, height))
    elif label.startswith("City_TreeTrunk_"):
        park_locs.append(loc)

unreal.log(f"[Creatures] Scanning world: found {len(alley_locs)} alleys, {len(high_bldg_locs)} high buildings, {len(park_locs)} trees.")

# Spawn Cyber Raccoons (Alleys/Ground level)
raccoon_count = 0
for idx, loc in enumerate(alley_locs):
    # Spawn in the alleyway slightly offset on the ground
    offset_x = random.uniform(-100, 100)
    offset_y = random.uniform(-100, 100)
    pos = unreal.Vector(loc.x + offset_x, loc.y + offset_y, 110.0) # slightly off ground
    rot = unreal.Rotator(0, 0, random.choice([0, 90, 180, 270]))
    # Flat standing poster
    scale = unreal.Vector(0.05, 1.2, 1.2)
    spawn_billboard(f"City_Creature_Raccoon_{raccoon_count}", pos, scale, mi_raccoon, rot)
    raccoon_count += 1

# If there were few alleys, spawn a few more in random spots
if raccoon_count < 10:
    for i in range(10 - raccoon_count):
        # Spawn near random building base
        bldg = random.choice([a for a in actors if a.get_actor_label().startswith("City_Bldg_")])
        loc = bldg.get_actor_location()
        pos = unreal.Vector(loc.x + random.uniform(400, 600) * random.choice([-1, 1]),
                            loc.y + random.uniform(400, 600) * random.choice([-1, 1]),
                            110.0)
        rot = unreal.Rotator(0, 0, random.uniform(0, 360))
        scale = unreal.Vector(0.05, 1.2, 1.2)
        spawn_billboard(f"City_Creature_Raccoon_{raccoon_count}", pos, scale, mi_raccoon, rot)
        raccoon_count += 1
unreal.log(f"[Creatures] Spawned {raccoon_count} Cyber Raccoons in alleys and corners.")

# Spawn Security Drones (Floating between skyscrapers)
drone_count = 0
for idx, (loc, height) in enumerate(high_bldg_locs):
    # Spawn hovering high in the sky between building lots
    pos = unreal.Vector(loc.x + random.uniform(400, 700) * random.choice([-1, 1]),
                        loc.y + random.uniform(400, 700) * random.choice([-1, 1]),
                        random.uniform(1200, 2800))
    rot = unreal.Rotator(0, 0, random.uniform(0, 360))
    # Hovering drone screen
    scale = unreal.Vector(0.05, 1.8, 1.8)
    spawn_billboard(f"City_Creature_Drone_{drone_count}", pos, scale, mi_drone, rot)
    drone_count += 1
unreal.log(f"[Creatures] Spawned {drone_count} Security Drones hovering high in downtown.")

# Spawn Neon Critters (Parks and vegetation)
critter_count = 0
for idx, loc in enumerate(park_locs):
    # Spawn on the grass around tree trunks
    if random.random() < 0.6:  # 60% chance per tree trunk
        pos = unreal.Vector(loc.x + random.uniform(150, 300) * random.choice([-1, 1]),
                            loc.y + random.uniform(150, 300) * random.choice([-1, 1]),
                            40.0)  # low to ground
        rot = unreal.Rotator(0, 0, random.uniform(0, 360))
        scale = unreal.Vector(0.05, 0.9, 0.9)
        spawn_billboard(f"City_Creature_Critter_{critter_count}", pos, scale, mi_critter, rot)
        critter_count += 1
unreal.log(f"[Creatures] Spawned {critter_count} Neon Critters around park trees.")

# ------------------------------------------------------------------
# 6. Save & Finalize
# ------------------------------------------------------------------
saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(material_dir, only_if_is_dirty=False)
unreal.log(f"[Creatures] Spawning completed. Level saved: {saved}. DONE")
