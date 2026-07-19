"""
The Connected Sprawl — One-click editor setup.

Run this inside the Unreal Editor's Python console:

    exec(open('/Users/matthewx/code/ConnectedSprawl/Content/Python/setup_connected_sprawl.py').read())

Or via the Output Log command input:

    py /Users/matthewx/code/ConnectedSprawl/Content/Python/setup_connected_sprawl.py

This script:
  1. Creates /Content/Core/ and /Content/Maps/ folders
  2. Creates BP_Zarri, BP_SprawlGameMode, BP_SprawlPlayerController
  3. Assigns Zarri a preferred imported hero mesh, with mannequin fallback
  4. Wires BP_SprawlGameMode defaults (DefaultPawnClass, PlayerControllerClass)
  5. Creates WBP_SprawlHUD
  6. Sets project Default GameMode + Default Maps
  7. Opens a fresh TestMap and adds a PlayerStart
  8. Saves everything

After running, just press Play.
"""

import os
import sys
import unreal

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.append(SCRIPT_DIR)

import avatar_realism

# ------------------------------------------------------------------
# Utilities
# ------------------------------------------------------------------
AT  = unreal.AssetToolsHelpers.get_asset_tools()
EAS = unreal.EditorAssetLibrary
ELS = unreal.EditorLevelLibrary
ELL = unreal.EditorLevelLibrary  # alias for clarity

def log(msg):
    unreal.log(f"[SprawlSetup] {msg}")
    print(f"[SprawlSetup] {msg}")

def ensure_folder(path):
    if not EAS.does_directory_exist(path):
        EAS.make_directory(path)
        log(f"Created folder: {path}")

def create_blueprint(name, folder, parent_class):
    asset_path = f"{folder}/{name}"
    if EAS.does_asset_exist(asset_path):
        log(f"Exists, loading: {asset_path}")
        return EAS.load_asset(asset_path)

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    bp = AT.create_asset(name, folder, unreal.Blueprint, factory)
    EAS.save_asset(asset_path)
    log(f"Created BP: {asset_path}  (parent={parent_class.get_name()})")
    return bp

def create_widget_blueprint(name, folder, parent_class):
    asset_path = f"{folder}/{name}"
    if EAS.does_asset_exist(asset_path):
        log(f"Exists, loading: {asset_path}")
        return EAS.load_asset(asset_path)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    wbp = AT.create_asset(name, folder, unreal.WidgetBlueprint, factory)
    EAS.save_asset(asset_path)
    log(f"Created WBP: {asset_path}  (parent={parent_class.get_name()})")
    return wbp

def find_asset_by_name(short_name):
    """Search /Game/** for an asset of this name; return the path or None."""
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    data = ar.get_assets_by_package_name(short_name) or []
    if data:
        return str(data[0].package_name)
    # Fallback: broader name search
    all_data = ar.get_all_assets(False)
    for a in all_data:
        if str(a.asset_name) == short_name:
            return str(a.package_name)
    return None

# ------------------------------------------------------------------
# 1. Folders
# ------------------------------------------------------------------
ensure_folder("/Game/Core")
ensure_folder("/Game/Maps")
ensure_folder("/Game/UI")

# ------------------------------------------------------------------
# 2. Resolve C++ classes
# ------------------------------------------------------------------
try:
    CLS_Zarri      = unreal.load_class(None, "/Script/ConnectedSprawl.ZarriCharacter")
    CLS_GameMode   = unreal.load_class(None, "/Script/ConnectedSprawl.SprawlGameMode")
    CLS_Controller = unreal.load_class(None, "/Script/ConnectedSprawl.SprawlPlayerController")
    CLS_HUDWidget  = unreal.load_class(None, "/Script/ConnectedSprawl.SprawlHUDWidget")
except Exception as e:
    log(f"ERROR loading C++ classes — is the module built? {e}")
    raise

# ------------------------------------------------------------------
# 3. Create Blueprints
# ------------------------------------------------------------------
bp_zarri      = create_blueprint("BP_Zarri",                  "/Game/Core", CLS_Zarri)
bp_gamemode   = create_blueprint("BP_SprawlGameMode",         "/Game/Core", CLS_GameMode)
bp_controller = create_blueprint("BP_SprawlPlayerController", "/Game/Core", CLS_Controller)
wbp_hud       = create_widget_blueprint("WBP_SprawlHUD",      "/Game/UI",   CLS_HUDWidget)

# ------------------------------------------------------------------
# 4. Configure BP_Zarri — prefer the dedicated Zarri avatar
# ------------------------------------------------------------------
# Set on the ZarriCharacter CDO's Mesh component default value.
# The cleanest way from Python is to modify the Blueprint's class defaults after load.
bp_zarri_loaded = EAS.load_asset("/Game/Core/BP_Zarri")
cdo = unreal.get_default_object(bp_zarri_loaded.generated_class())
cdo.set_editor_property("hero_variant", "Zarri")
mesh_comp = cdo.get_editor_property("mesh")
configured, source = avatar_realism.apply_hero_mesh_defaults(mesh_comp)
if configured:
    log(f"Zarri mesh configured from {source}.")
    EAS.save_asset("/Game/Core/BP_Zarri")
else:
    log("⚠️  No preferred hero mesh or mannequin fallback was found.")

# ------------------------------------------------------------------
# 5. Configure BP_SprawlGameMode defaults
# ------------------------------------------------------------------
gm_loaded = EAS.load_asset("/Game/Core/BP_SprawlGameMode")
gm_cdo = unreal.get_default_object(gm_loaded.generated_class())
gm_cdo.set_editor_property("default_pawn_class",      bp_zarri.generated_class())
gm_cdo.set_editor_property("player_controller_class", bp_controller.generated_class())
EAS.save_asset("/Game/Core/BP_SprawlGameMode")
log("GameMode defaults set.")

# ------------------------------------------------------------------
# 6. Configure BP_SprawlPlayerController — assign HUD widget class
# ------------------------------------------------------------------
pc_loaded = EAS.load_asset("/Game/Core/BP_SprawlPlayerController")
pc_cdo = unreal.get_default_object(pc_loaded.generated_class())
pc_cdo.set_editor_property("hud_widget_class", wbp_hud.generated_class())
EAS.save_asset("/Game/Core/BP_SprawlPlayerController")
log("PlayerController HUD widget set.")

# ------------------------------------------------------------------
# 7. Create TestMap + add PlayerStart
# ------------------------------------------------------------------
map_path = "/Game/Maps/TestMap"
if not EAS.does_asset_exist(map_path):
    ELS.new_level(map_path)
    log(f"Created new level: {map_path}")
else:
    ELS.load_level(map_path)

# Add a floor if none exists (Basic level already has one, but just in case).
ELS.load_level(map_path)

# Place a PlayerStart at origin+200
ps_class = unreal.PlayerStart
actors = ELS.get_all_level_actors()
has_ps = any(isinstance(a, unreal.PlayerStart) for a in actors)
if not has_ps:
    ELS.spawn_actor_from_class(ps_class, unreal.Vector(0, 0, 200))
    log("Spawned PlayerStart.")

# Add a simple floor + directional light if it's an empty level
has_floor = any("Floor" in a.get_actor_label() for a in ELS.get_all_level_actors())
if not has_floor:
    cube_path = "/Engine/BasicShapes/Cube.Cube"
    cube = unreal.load_asset(cube_path)
    if cube:
        floor = ELS.spawn_actor_from_object(cube, unreal.Vector(0, 0, -50))
        floor.set_actor_scale3d(unreal.Vector(50, 50, 1))
        floor.set_actor_label("Floor")
        log("Spawned floor.")

    # Spawn Movable Directional Light (Sun)
    dir_light_class = unreal.DirectionalLight
    dir_light = ELS.spawn_actor_from_class(dir_light_class, unreal.Vector(0, 0, 500), unreal.Rotator(-50, 45, 0))
    dir_light.set_actor_label("DirectionalLight")
    dir_light_comp = dir_light.get_component_by_class(unreal.DirectionalLightComponent)
    if dir_light_comp:
        dir_light_comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        dir_light_comp.set_editor_property("atmosphere_sun_light", True)
        dir_light_comp.set_editor_property("cast_shadows", True)
        dir_light_comp.set_editor_property("intensity", 10.0)
    log("Spawned Directional Light (Movable).")

    # Spawn Sky Atmosphere
    sky_atm_class = unreal.SkyAtmosphere
    sky_atm = ELS.spawn_actor_from_class(sky_atm_class, unreal.Vector(0, 0, 0))
    sky_atm.set_actor_label("SkyAtmosphere")
    log("Spawned Sky Atmosphere.")

    # Spawn Movable Sky Light
    sky_light_class = unreal.SkyLight
    sky_light = ELS.spawn_actor_from_class(sky_light_class, unreal.Vector(0, 0, 100))
    sky_light.set_actor_label("SkyLight")
    sky_light_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
    if sky_light_comp:
        sky_light_comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        sky_light_comp.set_editor_property("real_time_capture", True)
    log("Spawned Sky Light (Movable, Real-time).")

    # Spawn unbound PostProcessVolume
    pp_volume_class = unreal.PostProcessVolume
    pp_volume = ELS.spawn_actor_from_class(pp_volume_class, unreal.Vector(0, 0, 0))
    pp_volume.set_actor_label("PostProcessVolume")
    pp_comp = pp_volume.get_component_by_class(unreal.PostProcessComponent)
    if pp_comp:
        pp_comp.set_editor_property("unbound", True)
    log("Spawned unbound PostProcessVolume.")

ELS.save_current_level()

# ------------------------------------------------------------------
# 8. Project settings — default GameMode + maps
# ------------------------------------------------------------------
maps_settings = unreal.get_default_object(unreal.GameMapsSettings)
maps_settings.set_editor_property("game_default_map", unreal.SoftObjectPath(map_path))
maps_settings.set_editor_property("editor_startup_map", unreal.SoftObjectPath(map_path))
maps_settings.set_editor_property("global_default_game_mode", unreal.SoftClassPath("/Game/Core/BP_SprawlGameMode.BP_SprawlGameMode_C"))
try:
    maps_settings.save_config()
    log("Saved project GameMode + default map settings.")
except Exception as e:
    log(f"Could not persist project config (may require manual save): {e}")

# ------------------------------------------------------------------
# 9. DONE
# ------------------------------------------------------------------
log("=" * 50)
log("✅ SETUP COMPLETE")
log("Now press the ▶ PLAY button in the editor.")
log("Expected: Zarri uses his dedicated imported avatar when artwork exists.")
log("Fallback: mannequin if the avatar art has not been imported yet.")
log("Startup log should show:")
log("   [Founder] Initialized. Cash=$2500 DailyBurn=$175 Runway=14.3 days")
log("=" * 50)
