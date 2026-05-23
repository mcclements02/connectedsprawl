"""Big one-shot: fix exposure, wire BP_Zarri to Manny mesh + AnimBP,
   create EnhancedInput assets, build a procedural city block in TestMap.

Run with:
  UnrealEditor <project.uproject> -ExecutePythonScript="<abs path>" -unattended -nosplash
"""
import unreal
import random

LEVEL_PATH = "/Game/Maps/TestMap"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
atools = unreal.AssetToolsHelpers.get_asset_tools()


# ============================================================
# 1) Load TestMap, tame exposure, ensure floor is big enough
# ============================================================
unreal.log("[Playable] loading {}".format(LEVEL_PATH))
les.load_level(LEVEL_PATH)

for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FixPostProcess":
        s = a.settings
        s.set_editor_property("auto_exposure_bias", 0.5)
        s.set_editor_property("override_auto_exposure_bias", True)
        s.set_editor_property("auto_exposure_min_brightness", 0.1)
        s.set_editor_property("auto_exposure_max_brightness", 4.0)
        a.settings = s
        unreal.log("[Playable] tamed FixPostProcess exposure")
    if a.get_actor_label() == "FixFloor":
        eas.destroy_actor(a)
        unreal.log("[Playable] removed old FixFloor")


# ============================================================
# 2) Procedural city block: streets, sidewalks, ~24 buildings
# ============================================================
random.seed(7)
cube = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")

# Materials we'll color via instanced static mesh? For simplicity use cubes with vertex colors via material instances.
# Use built-in Basic material with color overrides via dynamic material instance is heavy in a python script;
# we'll just use the default cube material on everything for now.

CITY_LABEL_PREFIX = "City_"

# Wipe any prior city actors so the script is idempotent
to_kill = []
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if lbl.startswith(CITY_LABEL_PREFIX):
        to_kill.append(a)
for a in to_kill:
    eas.destroy_actor(a)
unreal.log("[Playable] cleared {} prior City_* actors".format(len(to_kill)))


def spawn_cube(label, location, scale):
    sma = eas.spawn_actor_from_class(unreal.StaticMeshActor, location, unreal.Rotator(0, 0, 0))
    sma.set_actor_label(label)
    sma.set_actor_scale3d(scale)
    sma.static_mesh_component.set_static_mesh(cube)
    sma.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
    return sma


# Ground plane: 8000x8000 units flat slab
spawn_cube("City_Ground", unreal.Vector(0, 0, -50), unreal.Vector(80, 80, 1))

# 4x4 grid of city blocks. Each block: 1600x1600 units, with 400u street gaps.
GRID = 4
BLOCK = 1600
ROAD = 400
STEP = BLOCK + ROAD  # 2000
START = -(GRID * STEP) / 2 + STEP / 2  # -3000

for gx in range(GRID):
    for gy in range(GRID):
        cx = START + gx * STEP
        cy = START + gy * STEP

        # Sidewalk slab inside the block (slightly raised so streets are visibly distinct)
        spawn_cube(
            "City_Sidewalk_{}_{}".format(gx, gy),
            unreal.Vector(cx, cy, 5),
            unreal.Vector(BLOCK / 100.0, BLOCK / 100.0, 0.1),
        )

        # 1 or 2 buildings per block
        nbldgs = random.choice([1, 1, 2])
        for b in range(nbldgs):
            bw = random.uniform(400, 800)
            bd = random.uniform(400, 800)
            bh = random.uniform(600, 2400)
            ox = random.uniform(-(BLOCK / 2 - bw / 2 - 60), (BLOCK / 2 - bw / 2 - 60))
            oy = random.uniform(-(BLOCK / 2 - bd / 2 - 60), (BLOCK / 2 - bd / 2 - 60))
            spawn_cube(
                "City_Building_{}_{}_{}".format(gx, gy, b),
                unreal.Vector(cx + ox, cy + oy, bh / 2 + 10),
                unreal.Vector(bw / 100.0, bd / 100.0, bh / 100.0),
            )

# Move existing PlayerStart to street corner
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "PlayerStart":
        a.set_actor_location(unreal.Vector(0, 0, 200), False, False)
        a.set_actor_rotation(unreal.Rotator(0, 0, 0), False)

unreal.log("[Playable] city grid built")


# ============================================================
# 3) Create EnhancedInput assets (IA + IMC) under /Game/Input
# ============================================================
INPUT_DIR = "/Game/Input"

def make_input_action(name, value_type):
    pkg = INPUT_DIR + "/" + name
    if ets.does_asset_exist(pkg):
        return unreal.load_asset(pkg)
    factory = unreal.InputAction_Factory()
    asset = atools.create_asset(name, INPUT_DIR, unreal.InputAction, factory)
    asset.set_editor_property("value_type", value_type)
    ets.save_asset(pkg)
    unreal.log("[Playable] created IA {}".format(name))
    return asset

ia_move    = make_input_action("IA_Move",    unreal.InputActionValueType.AXIS2D)
ia_look    = make_input_action("IA_Look",    unreal.InputActionValueType.AXIS2D)
ia_jump    = make_input_action("IA_Jump",    unreal.InputActionValueType.BOOLEAN)
ia_sprint  = make_input_action("IA_Sprint",  unreal.InputActionValueType.BOOLEAN)
ia_interact= make_input_action("IA_Interact",unreal.InputActionValueType.BOOLEAN)


# IMC — always recreate so we have a clean mapping list
imc_path = INPUT_DIR + "/IMC_Default"
if ets.does_asset_exist(imc_path):
    ets.delete_asset(imc_path)
imc_factory = unreal.InputMappingContext_Factory()
imc = atools.create_asset("IMC_Default", INPUT_DIR, unreal.InputMappingContext, imc_factory)
unreal.log("[Playable] created IMC_Default fresh")


def add_mapping(imc_asset, ia, key_name, modifiers=None, swizzle=None, negate=False):
    k = unreal.Key()
    k.import_text(key_name)
    mapping = imc_asset.map_key(ia, k)
    mods = []
    if swizzle is not None:
        sw = unreal.InputModifierSwizzleAxis()
        sw.order = swizzle
        mods.append(sw)
    if negate:
        mods.append(unreal.InputModifierNegate())
    if modifiers:
        mods += modifiers
    if mods:
        mapping.modifiers = mods
    return mapping


# Move: WASD via Swizzle (W = +Y, S = -Y on axis2D), so we want input to map W=(0,+1), S=(0,-1), A=(-1,0), D=(+1,0)
# Easier: bind X axis to A/D, Y axis to W/S using swizzle.
# Default IA_Move is Axis2D. W should add +Y → swizzle YXZ keeps X for left/right, Y for forward/back.
# UE common pattern: W = Swizzle YXZ + Negate(false?) → we want W = (0,1). Default Key1D produces (1,0,0).
# We use SwizzleAxis YXZ to turn (1,0) into (0,1). For S we add Negate after Swizzle so (1,0)->(0,1)->(0,-1).

# A: (-1, 0)
add_mapping(imc, ia_move, "A", negate=True)
# D: (1, 0)
add_mapping(imc, ia_move, "D")
# W: (0, 1)
add_mapping(imc, ia_move, "W", swizzle=unreal.InputAxisSwizzle.YXZ)
# S: (0, -1)
add_mapping(imc, ia_move, "S", swizzle=unreal.InputAxisSwizzle.YXZ, negate=True)

# Look: mouse X/Y
add_mapping(imc, ia_look, "MouseX")
add_mapping(imc, ia_look, "MouseY", swizzle=unreal.InputAxisSwizzle.YXZ, negate=True)

# Jump / Sprint / Interact
add_mapping(imc, ia_jump, "SpaceBar")
add_mapping(imc, ia_sprint, "LeftShift")
add_mapping(imc, ia_interact, "E")

ets.save_asset(imc_path)
unreal.log("[Playable] IMC mappings written")


# ============================================================
# 4) Wire BP_Zarri: set SkeletalMesh, AnimBP, and Input asset refs on CDO
# ============================================================
BP_ZARRI = "/Game/Core/BP_Zarri"
bp = unreal.load_asset(BP_ZARRI)
unreal.log("[Playable] loaded BP_Zarri: {}".format(bp))

# Get the generated class & CDO
gen_class = bp.generated_class()
cdo = unreal.get_default_object(gen_class)

# Skeletal mesh
sk_mesh = unreal.load_object(None, "/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")
anim_bp = unreal.load_object(None, "/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP")

# The Character's mesh component is named "CharacterMesh0" — accessible on CDO via get_editor_property("mesh")
mesh_comp = cdo.get_editor_property("mesh")
if mesh_comp and sk_mesh:
    mesh_comp.set_skeletal_mesh_asset(sk_mesh)
    mesh_comp.set_relative_location_and_rotation(
        unreal.Vector(0, 0, -90),
        unreal.Rotator(0, -90, 0),
        False, False,
    )
    if anim_bp:
        mesh_comp.set_editor_property("anim_class", anim_bp.generated_class())
    unreal.log("[Playable] BP_Zarri mesh + animBP assigned")
else:
    unreal.log_warning("[Playable] couldn't get mesh component or sk_mesh")

# Input asset refs
cdo.set_editor_property("DefaultMappingContext", imc)
cdo.set_editor_property("IA_Move", ia_move)
cdo.set_editor_property("IA_Look", ia_look)
cdo.set_editor_property("IA_Jump", ia_jump)
cdo.set_editor_property("IA_Sprint", ia_sprint)
cdo.set_editor_property("IA_Interact", ia_interact)

# Mark BP modified & save
unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.log("[Playable] BP_Zarri saved")


# ============================================================
# 5) Save level
# ============================================================
saved = les.save_current_level()
unreal.log("[Playable] save_current_level returned: {}".format(saved))
unreal.log("[Playable] DONE")
