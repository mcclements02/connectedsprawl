"""Spider-Man / GTA Style Visual Overhaul.

Creates a wet asphalt road shader, spawns neon storefronts at building bases,
places skyscraper ads and Miles Morales style graffiti decals, and swaps
pedestrian mannequins with walking cyberpunk character billboards.
"""
import os
import random
import unreal

random.seed(999)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

# Load the city level
les.load_level("/Game/Maps/TestMap")

# ------------------------------------------------------------------
# Helpers to clear previous overhaul actors
# ------------------------------------------------------------------
def clear_overhaul_actors():
    killed = 0
    for a in list(eas.get_all_level_actors()):
        label = a.get_actor_label()
        if (label.startswith("City_Morales_") or 
            label.startswith("City_Ped_Billboard_")):
            eas.destroy_actor(a)
            killed += 1
            
    # Also restore visibility to all character mannequins in case we rerun
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "SprawlPedestrian":
            mesh_comp = a.get_component_by_class(unreal.SkeletalMeshComponent)
            if mesh_comp:
                mesh_comp.set_visibility(True)
                
    unreal.log(f"[Morales] Cleared {killed} old visual overhaul actors.")

clear_overhaul_actors()

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
new_textures = {
    "storefront_neon_1": "storefront_neon_1.png",
    "storefront_neon_2": "storefront_neon_2.png",
    "graffiti_spiderman": "graffiti_spiderman.png",
    "graffiti_street_1": "graffiti_street_1.png",
    "billboard_ad_1": "billboard_ad_1.png",
    "billboard_ad_2": "billboard_ad_2.png",
    "pedestrian_cyberpunk_1": "pedestrian_cyberpunk_1.png",
    "pedestrian_cyberpunk_2": "pedestrian_cyberpunk_2.png"
}

textures = {}
for key, filename in new_textures.items():
    file_path = os.path.join(staging_dir, filename)
    asset_path = f"{texture_dir}/{key}"
    
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"[Morales] Texture already exists, loading: {asset_path}")
        tex = unreal.load_asset(asset_path)
    else:
        unreal.log(f"[Morales] Importing texture from {file_path}")
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', file_path)
        task.set_editor_property('destination_path', texture_dir)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        
        atools.import_asset_tasks([task])
        tex = unreal.load_asset(asset_path)
        
    textures[key] = tex

# ------------------------------------------------------------------
# 3. Create or Resolve Master Hologram Material
# ------------------------------------------------------------------
master_material_path = f"{material_dir}/M_CreatureHoloMaster"
if not unreal.EditorAssetLibrary.does_asset_exist(master_material_path):
    unreal.log(f"[Morales] Master material missing, creating at {master_material_path}")
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
# 4. Create Material Instances for Screens & Billboards
# ------------------------------------------------------------------
def get_or_create_material_instance(name, texture, tint, intensity):
    instance_path = f"{material_dir}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(instance_path):
        mi = unreal.load_asset(instance_path)
    else:
        unreal.log(f"[Morales] Creating material instance at {instance_path}")
        mi = atools.create_asset(name, material_dir, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", master_mat)
        
    MEL.set_material_instance_texture_parameter_value(mi, "CreatureTex", texture)
    MEL.set_material_instance_vector_parameter_value(mi, "ColorTint", tint)
    MEL.set_material_instance_scalar_parameter_value(mi, "EmissiveIntensity", intensity)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi

mi_ramen = get_or_create_material_instance("MI_Store_Ramen", textures["storefront_neon_1"], unreal.LinearColor(1.0, 0.9, 0.7, 1.0), 3.0)
mi_hacking = get_or_create_material_instance("MI_Store_Hacking", textures["storefront_neon_2"], unreal.LinearColor(0.8, 0.9, 1.0, 1.0), 3.0)
mi_spidey = get_or_create_material_instance("MI_Graffiti_Spidey", textures["graffiti_spiderman"], unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 2.2)
mi_street_tag = get_or_create_material_instance("MI_Graffiti_Street", textures["graffiti_street_1"], unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 2.2)
mi_ad_gate = get_or_create_material_instance("MI_Ad_Gate", textures["billboard_ad_1"], unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 4.5)
mi_ad_beast = get_or_create_material_instance("MI_Ad_Beast", textures["billboard_ad_2"], unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 4.5)
mi_ped_citizen = get_or_create_material_instance("MI_Ped_Citizen", textures["pedestrian_cyberpunk_1"], unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 2.5)
mi_ped_hacker = get_or_create_material_instance("MI_Ped_Hacker", textures["pedestrian_cyberpunk_2"], unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 2.5)

# ------------------------------------------------------------------
# 5. Overhaul Ground / Road Material to Look Wet and Reflective
# ------------------------------------------------------------------
master_city = unreal.load_asset("/Game/CityArt/M_CityMaster")
mi_wet_road = get_or_create_material_instance("MI_WetAsphalt_Road", None, unreal.LinearColor(0.015, 0.015, 0.018, 1.0), 0.0)
# Override roughness and metallic explicitly for mirror-like wet asphalt
mi_wet_road.set_editor_property("parent", master_city)
MEL.set_material_instance_vector_parameter_value(mi_wet_road, "BaseColor", unreal.LinearColor(0.015, 0.015, 0.018, 1.0))
MEL.set_material_instance_scalar_parameter_value(mi_wet_road, "Roughness", 0.08) # Super wet and reflective
MEL.set_material_instance_scalar_parameter_value(mi_wet_road, "Metallic", 0.20)  # Faint metallic reflection
unreal.EditorAssetLibrary.save_loaded_asset(mi_wet_road)

mi_wet_sidewalk = get_or_create_material_instance("MI_WetConcrete_Sidewalk", None, unreal.LinearColor(0.20, 0.20, 0.22, 1.0), 0.0)
mi_wet_sidewalk.set_editor_property("parent", master_city)
MEL.set_material_instance_vector_parameter_value(mi_wet_sidewalk, "BaseColor", unreal.LinearColor(0.20, 0.20, 0.22, 1.0))
MEL.set_material_instance_scalar_parameter_value(mi_wet_sidewalk, "Roughness", 0.25) # Wet sidewalk concrete
MEL.set_material_instance_scalar_parameter_value(mi_wet_sidewalk, "Metallic", 0.0)
unreal.EditorAssetLibrary.save_loaded_asset(mi_wet_sidewalk)

for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    
    smc = a.static_mesh_component
    if label == "City_Ground":
        smc.set_material(0, mi_wet_road)
        unreal.log("[Morales] Applied wet reflective asphalt to City_Ground.")
    elif label.startswith("City_Surf_"):
        mat = smc.get_material(0)
        if mat and "Grass" not in mat.get_name() and "Water" not in mat.get_name():
            smc.set_material(0, mi_wet_sidewalk)

# ------------------------------------------------------------------
# 6. Spawning Billboard Storefronts, High-Rise Ads, and Decals
# ------------------------------------------------------------------
CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")

def spawn_morales_actor(label, loc, scale, mat, rot=None):
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

# Find all building actors
actors = eas.get_all_level_actors()
buildings = []
for a in actors:
    if a.get_actor_label().startswith("City_Bldg_"):
        buildings.append(a)

unreal.log(f"[Morales] Found {len(buildings)} buildings to dress with visual assets.")

storefront_count = 0
highrise_ad_count = 0
graffiti_count = 0

for b in buildings:
    loc = b.get_actor_location()
    scale = b.get_actor_scale3d()
    
    # Calculate building visual height and widths
    smc = b.static_mesh_component
    if not smc:
        continue
    try:
        mesh = smc.get_static_mesh()
    except AttributeError:
        mesh = smc.get_editor_property("static_mesh")
    if not mesh:
        continue
        
    bb = mesh.get_bounding_box()
    width_x = (bb.max.x - bb.min.x) * scale.x
    width_y = (bb.max.y - bb.min.y) * scale.y
    height = (bb.max.z - bb.min.z) * scale.z
    
    # 1) Spawning storefront screens at base (Z height around 110)
    if height > 500:
        # Spawn storefront on the North and South faces of building lot bottoms
        # North face
        ny = loc.y + width_y / 2.0 + 3.0
        pos_n = unreal.Vector(loc.x, ny, 110.0)
        rot_n = unreal.Rotator(0.0, 0.0, 90.0) # face south (toward street)
        scale_n = unreal.Vector(0.05, 3.2, 1.8) # 320 wide, 180 high
        mat_n = mi_ramen if storefront_count % 2 == 0 else mi_hacking
        spawn_morales_actor(f"City_Morales_Storefront_{storefront_count}", pos_n, scale_n, mat_n, rot_n)
        storefront_count += 1
        
        # South face
        sy = loc.y - width_y / 2.0 - 3.0
        pos_s = unreal.Vector(loc.x, sy, 110.0)
        rot_s = unreal.Rotator(0.0, 0.0, 270.0)
        scale_s = unreal.Vector(0.05, 3.2, 1.8)
        mat_s = mi_hacking if storefront_count % 2 == 0 else mi_ramen
        spawn_morales_actor(f"City_Morales_Storefront_{storefront_count}", pos_s, scale_s, mat_s, rot_s)
        storefront_count += 1
        
    # 2) Spawning large high-rise skyscraper ads and graffiti (Z height > 1000)
    if height > 2200.0:
        # Large ad on East face
        ex = loc.x + width_x / 2.0 + 3.0
        pos_e = unreal.Vector(ex, loc.y, height * 0.6)
        rot_e = unreal.Rotator(0.0, 0.0, 0.0)
        scale_e = unreal.Vector(0.05, 5.0, 8.0) # Huge skyscraper ad
        mat_e = mi_ad_gate if highrise_ad_count % 2 == 0 else mi_ad_beast
        spawn_morales_actor(f"City_Morales_HighriseAd_{highrise_ad_count}", pos_e, scale_e, mat_e, rot_e)
        highrise_ad_count += 1
        
        # Large spider-graffiti decal on West face
        wx = loc.x - width_x / 2.0 - 3.0
        pos_w = unreal.Vector(wx, loc.y, height * 0.45)
        rot_w = unreal.Rotator(0.0, 0.0, 180.0)
        scale_w = unreal.Vector(0.05, 4.0, 4.0) # Huge graffiti decal
        mat_w = mi_spidey if graffiti_count % 2 == 0 else mi_street_tag
        spawn_morales_actor(f"City_Morales_Graffiti_{graffiti_count}", pos_w, scale_w, mat_w, rot_w)
        graffiti_count += 1

unreal.log(f"[Morales] Procedurally spawned {storefront_count} shop storefronts, {highrise_ad_count} high-rise billboards, and {graffiti_count} graffiti decals.")

# ------------------------------------------------------------------
# 7. Swapping Mannequin Pedestrians with Detailed Character Billboards
# ------------------------------------------------------------------
pedestrian_actors = []
for a in actors:
    if a.get_class().get_name() == "SprawlPedestrian":
        pedestrian_actors.append(a)

unreal.log(f"[Morales] Swapping mesh visual on {len(pedestrian_actors)} walking pedestrians.")

swapped_peds = 0
for idx, a in enumerate(pedestrian_actors):
    # Hide the default gray mannequin mesh
    mesh_comp = a.get_component_by_class(unreal.SkeletalMeshComponent)
    if mesh_comp:
        mesh_comp.set_visibility(False)
        
    loc = a.get_actor_location()
    
    # Spawn a static mesh billboard actor
    billboard = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(0, 0, 0))
    billboard.set_actor_label(f"City_Ped_Billboard_{swapped_peds}")
    
    smc = billboard.static_mesh_component
    smc.set_static_mesh(CUBE)
    
    # Alternate character textures
    mat = mi_ped_citizen if idx % 2 == 0 else mi_ped_hacker
    smc.set_material(0, mat)
    
    # Scale to match character height (Z height: 180 units)
    billboard.set_actor_scale3d(unreal.Vector(0.05, 1.2, 1.8))
    
    # Attach billboard to walking character actor root (capsule component)
    billboard.attach_to_actor(
        a, 
        unreal.Name(), 
        unreal.AttachmentRule.SNAP_TO_TARGET,  # snap position to root
        unreal.AttachmentRule.KEEP_RELATIVE,   # keep relative rotation so it faces character's forward dir
        unreal.AttachmentRule.KEEP_WORLD,      # keep the custom scale we just set
        False
    )
    
    # Center billboard relative to capsule
    billboard.set_actor_relative_location(unreal.Vector(0.0, 0.0, -10.0), False, False)
    # Rotate 90 degrees around Yaw so the flat portrait faces sideways/forward appropriately
    billboard.set_actor_relative_rotation(unreal.Rotator(0.0, 0.0, 90.0), False, False)
    
    swapped_peds += 1

unreal.log(f"[Morales] Swapped {swapped_peds} grey mannequins with detailed, walking character screens.")

# ------------------------------------------------------------------
# 8. Save & Finalize
# ------------------------------------------------------------------
saved = les.save_current_level()
unreal.EditorAssetLibrary.save_directory(material_dir, only_if_is_dirty=False)
unreal.log(f"[Morales] Overhaul completed. Level saved: {saved}. DONE")
