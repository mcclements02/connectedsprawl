"""Author the core-loop gameplay content (safe under -nullrhi, no spawns).

Creates/updates:
  1. Five UStrategicDecision data assets in /Game/Missions/Decisions —
     the GDD's FirstVC opener plus a branching chain across both factions,
     converging on the BlockPartyGig bridge mission.
  2. /Game/Materials/M_SignalGlow — unlit emissive with a GlowColor param
     (ASprawlTrafficLight drives it; was a missing-asset CDO error).
  3. /Game/Materials/MI_RoadPaint — road-paint instance parented to
     M_RoadWhite (ASprawlRoadMarkings expects it).

Also reports whether BP_SprawlPlayerController binds a HUD widget class.
Idempotent: existing assets are updated in place.
"""
import unreal

eal = unreal.EditorAssetLibrary
atools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

DEC_DIR = "/Game/Missions/Decisions"
MAT_DIR = "/Game/Materials"

FACTION = {
    "corp": unreal.Faction.CORPORATE,
    "street": unreal.Faction.STREET,
    "neutral": unreal.Faction.NEUTRAL,
}


def branch(bid, headline, pitch, cash=0.0, burn=0.0, corp=0, street=0,
           heat=0, debt=0, dirty=False, unlocks=""):
    b = unreal.DecisionBranch()
    b.set_editor_property("branch_id", bid)
    b.set_editor_property("headline", headline)
    b.set_editor_property("pitch", pitch)
    b.set_editor_property("cash_delta", float(cash))
    b.set_editor_property("daily_burn_delta", float(burn))
    b.set_editor_property("corporate_rep_delta", corp)
    b.set_editor_property("street_rep_delta", street)
    b.set_editor_property("heat_delta", heat)
    b.set_editor_property("moral_debt_delta", debt)
    b.set_editor_property("cash_is_dirty", dirty)
    b.set_editor_property("unlocks_decision_id", unlocks)
    return b


DECISIONS = [
    {
        "asset": "DA_FirstVC",
        "id": "FirstVC",
        "title": "The Iron Forest Term Sheet",
        "prompt": ("Rohan wants 15% for $50k. Contract is drawn up, DocuSign "
                   "already on your screen. Ty is in the passenger seat, "
                   "listening to every word."),
        "contact": "Rohan_IronForest",
        "faction": "corp",
        "branches": [
            branch("AcceptVC", "Sign the paper.",
                   "Stability. Legitimacy. A boss you did not choose.",
                   cash=50000, burn=80, corp=25, street=-10,
                   unlocks="FirstBoardMeeting"),
            branch("StreetLoan", "Ty knows a guy. No paperwork.",
                   "Faster money, fewer signatures, heavier shadows.",
                   cash=25000, street=20, debt=15, heat=5, dirty=True,
                   unlocks="TheFirstFavor"),
        ],
    },
    {
        "asset": "DA_FirstBoardMeeting",
        "id": "FirstBoardMeeting",
        "title": "The 7am Standup",
        "prompt": ("Maya from A-Gate compliance flags your socials: photos "
                   "from the old block, Marcus tagged in half of them. 'The "
                   "fund needs a clean story,' she says."),
        "contact": "Maya_AGate",
        "faction": "corp",
        "branches": [
            branch("SanitizeBrand", "Scrub it. Wear the button-down.",
                   "PR retainer goes on the books. The block notices.",
                   burn=30, corp=15, street=-15,
                   unlocks="BlockPartyGig"),
            branch("KeepItReal", "The block raised me. It stays.",
                   "The fund frowns. The neighborhood remembers.",
                   corp=-10, street=15,
                   unlocks="BlockPartyGig"),
        ],
    },
    {
        "asset": "DA_TheFirstFavor",
        "id": "TheFirstFavor",
        "title": "Ty's Guy Calls It In",
        "prompt": ("Marcus: 'That paper you took? It came with a string. "
                   "Duffel bag, Rail Yards, tonight. You drive, you don't "
                   "open it, we're square... for now.'"),
        "contact": "Marcus_Lows",
        "faction": "street",
        "branches": [
            branch("RunThePackage", "Drive the duffel. Don't look.",
                   "One run. Everybody's watching how you carry it.",
                   cash=8000, street=10, heat=10, debt=5, dirty=True,
                   unlocks="RailYardRun"),
            branch("BuyYourWayOut", "Wire it back with interest.",
                   "Expensive. Clean. Marcus stops smiling.",
                   cash=-15000, street=-5, debt=-10,
                   unlocks="BlockPartyGig"),
        ],
    },
    {
        "asset": "DA_RailYardRun",
        "id": "RailYardRun",
        "title": "Warehouse 9, After Dark",
        "prompt": ("DeShawn runs logistics out of the Rail Yards. 'Two "
                   "containers, one night, my forklift guy is out. Your car, "
                   "your hands, twenty large.'"),
        "contact": "DeShawn_RailYards",
        "faction": "street",
        "branches": [
            branch("TakeTheJob", "One night. No questions.",
                   "The fastest money in the Sprawl is the heaviest.",
                   cash=20000, street=15, heat=15, debt=10, dirty=True),
            branch("TipTheCops", "Call it in. Anonymous.",
                   "Clean hands. Cold block. The Lows keep receipts.",
                   corp=10, street=-25, heat=-10),
        ],
    },
    {
        "asset": "DA_BlockPartyGig",
        "id": "BlockPartyGig",
        "title": "The Junction Block Party",
        "prompt": ("Ty: 'Block party Saturday. The Junction, our end. You "
                   "could put the company name on it... or you could just "
                   "come home for a day.'"),
        "contact": "Ty_Lows",
        "faction": "neutral",
        "branches": [
            branch("SponsorIt", "Banner over the DJ booth.",
                   "Legit marketing the fund can love. The block eats.",
                   cash=-10000, street=15, corp=10),
            branch("HeadlineIt", "Get on the mic yourself.",
                   "No invoice. Just you, home, loud.",
                   cash=5000, street=20, heat=5),
        ],
    },
]


def ensure_decision(spec):
    path = "{}/{}".format(DEC_DIR, spec["asset"])
    if eal.does_asset_exist(path):
        asset = eal.load_asset(path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.StrategicDecision)
        asset = atools.create_asset(spec["asset"], DEC_DIR,
                                    unreal.StrategicDecision, factory)
    if not asset:
        unreal.log_error("[Author] could not create {}".format(path))
        return None
    asset.set_editor_property("decision_id", spec["id"])
    asset.set_editor_property("title", spec["title"])
    asset.set_editor_property("prompt", spec["prompt"])
    asset.set_editor_property("proposer_contact_id", spec["contact"])
    asset.set_editor_property("context_faction", FACTION[spec["faction"]])
    asset.set_editor_property("branches", spec["branches"])
    eal.save_loaded_asset(asset)
    unreal.log("[Author] decision {} ({} branches)".format(
        path, len(spec["branches"])))
    return asset


def ensure_signal_glow():
    path = MAT_DIR + "/M_SignalGlow"
    if eal.does_asset_exist(path):
        unreal.log("[Author] {} already exists".format(path))
        return eal.load_asset(path)
    mat = atools.create_asset("M_SignalGlow", MAT_DIR, unreal.Material,
                              unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    param = MEL.create_material_expression(
        mat, unreal.MaterialExpressionVectorParameter, -320, 0)
    param.set_editor_property("parameter_name", "GlowColor")
    param.set_editor_property("default_value", unreal.LinearColor(2.0, 0.06, 0.04, 1.0))
    MEL.connect_material_property(param, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    eal.save_loaded_asset(mat)
    unreal.log("[Author] created {}".format(path))
    return mat


def ensure_road_paint():
    path = MAT_DIR + "/MI_RoadPaint"
    if eal.does_asset_exist(path):
        unreal.log("[Author] {} already exists".format(path))
        return eal.load_asset(path)
    parent = eal.load_asset("/Game/CityArt/M_RoadWhite")
    if not parent:
        unreal.log_error("[Author] CityArt/M_RoadWhite missing; cannot parent MI_RoadPaint")
        return None
    mi = atools.create_asset("MI_RoadPaint", MAT_DIR,
                             unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    eal.save_loaded_asset(mi)
    unreal.log("[Author] created {} (parent M_RoadWhite)".format(path))
    return mi


for spec in DECISIONS:
    ensure_decision(spec)
ensure_signal_glow()
ensure_road_paint()

# HUD wiring report: does the BP controller bind a HUD widget class?
bp_class = eal.load_blueprint_class("/Game/Core/BP_SprawlPlayerController")
if bp_class:
    cdo = unreal.get_default_object(bp_class)
    hud = cdo.get_editor_property("hud_widget_class")
    modal = cdo.get_editor_property("decision_modal_class")
    unreal.log("[Author] BP controller HUDWidgetClass={} DecisionModalClass={}".format(
        hud.get_name() if hud else "None", modal.get_name() if modal else "None"))
else:
    unreal.log_warning("[Author] BP_SprawlPlayerController not found")

unreal.log("[Author] done")
