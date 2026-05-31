"""Fix the world edges:
  - Spawn 4 invisible blocking walls around the city perimeter so the player
    and AI cars can't fall off the map.
  - Lift road centre-lines and street-stripe actors a hair so they no longer
    z-fight with the asphalt.
  - Ease back the motion blur a touch (was producing ghost trails on the road).
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/TestMap")

CUBE = unreal.load_object(None, "/Engine/BasicShapes/Cube.Cube")

# ============================================================
# 1) Invisible perimeter walls
# ============================================================
# Clear any previous walls (idempotent)
removed = 0
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("City_EdgeWall_"):
        eas.destroy_actor(a)
        removed += 1

# Match city span (build_city: N=7, STEP=2600 -> 18200 total)
CITY_HALF = 9100.0
WALL_PAD = 250.0           # walls sit just outside the city
WALL_HEIGHT = 2500.0       # tall enough that no car can launch over
WALL_THICKNESS = 200.0     # solid
EDGE = CITY_HALF + WALL_PAD


def spawn_wall(label, location, scale):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, location,
                                   unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    smc = a.static_mesh_component
    smc.set_static_mesh(CUBE)
    smc.set_mobility(unreal.ComponentMobility.STATIC)
    smc.set_collision_profile_name("BlockAllDynamic")
    # Invisible — gameplay collision only
    a.set_actor_hidden_in_game(True)
    return a


span_x = (EDGE * 2 + WALL_THICKNESS) / 100.0
span_y = (EDGE * 2 + WALL_THICKNESS) / 100.0
th = WALL_THICKNESS / 100.0
ht = WALL_HEIGHT / 100.0
z_c = WALL_HEIGHT / 2.0 - 200.0  # bottom sits below the road, top well above

# North / South (long along X)
spawn_wall("City_EdgeWall_N",
           unreal.Vector(0, EDGE, z_c),
           unreal.Vector(span_x, th, ht))
spawn_wall("City_EdgeWall_S",
           unreal.Vector(0, -EDGE, z_c),
           unreal.Vector(span_x, th, ht))
# East / West (long along Y)
spawn_wall("City_EdgeWall_E",
           unreal.Vector(EDGE, 0, z_c),
           unreal.Vector(th, span_y, ht))
spawn_wall("City_EdgeWall_W",
           unreal.Vector(-EDGE, 0, z_c),
           unreal.Vector(th, span_y, ht))
unreal.log("[Edges] spawned 4 invisible perimeter walls (removed {} prior)".format(removed))


# ============================================================
# 2) Z-fight cleanup: lift road centre-lines to z=10
# ============================================================
nudged = 0
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if (lbl.startswith("City_Detail_VCenter_")
            or lbl.startswith("City_Detail_HCenter_")
            or lbl.startswith("City_Detail_VLine_")
            or lbl.startswith("City_Detail_HLine_")):
        loc = a.get_actor_location()
        # Only nudge if currently low (~4)
        if loc.z < 9.0:
            a.set_actor_location(unreal.Vector(loc.x, loc.y, 11.0), False, False)
            nudged += 1
unreal.log("[Edges] lifted {} road markings to z=11".format(nudged))


# ============================================================
# 3) Tone down motion blur (no more ghost trails)
# ============================================================
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "FixPostProcess":
        s = a.settings
        try:
            s.set_editor_property("override_motion_blur_amount", True)
            s.set_editor_property("motion_blur_amount", 0.05)
            s.set_editor_property("override_motion_blur_max", True)
            s.set_editor_property("motion_blur_max", 2.0)
            a.settings = s
            unreal.log("[Edges] motion blur reduced (0.05)")
        except Exception as e:
            unreal.log_warning("[Edges] motion blur tweak failed: {}".format(e))


saved = les.save_current_level()
unreal.log("[Edges] level saved: {}. DONE".format(saved))
