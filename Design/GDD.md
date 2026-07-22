# The Connected Sprawl — Game Design Document

> **Logline:** *You are Zarri. You are 20. You have a car, a laptop, and a dream. You have 28 days of runway. Now what?*

---

## 0. The Protagonist — Zarri

**Name:** Zarri
**Age:** 20 (recently aged out of "teenager", fresh into adulthood)
**Identity:** Black, raised in The Lows (South side — Rail Yards neighborhood)
**Look:**
- Medium-deep skin tone
- Fade haircut or short twists (designer choice)
- Athletic build — he runs his hustle out of a car, not a gym
- Wardrobe: tech-forward streetwear. Layered hoodies, clean sneakers, AirPods Pro always in. Evolves as his factions shift (corporate = clean button-downs; street = oversized fits).
**Voice:** Code-switches naturally — boardroom English with VCs, neighborhood dialect with friends. The game's music and dialogue systems reflect both registers.
**Backstory:**
- Taught himself to code on a cracked Chromebook at 14
- Started doing web work for local businesses at 17
- Lost his older brother (his first investor, emotionally) to a street situation — which is why the Street Friends won't leave him alone, and why the Corporate Friends keep trying to "save" him
- Drives a rebuilt '09 sedan — ugly, fast, full of laptops and cables

**Pipeline:** Built in MetaHuman Creator, starting from the **Jesse** or **Orion** preset, age slider lowered, Black streetwear grooms from Quixel Bridge or Fab.

---

## 1. Core Loop

```
  DRIVE → DECIDE → EARN → BURN → ADVANCE DAY → (repeat)
    ↑                                           │
    └───────────── keep the lights on ──────────┘
```

- **Drive** Zarri's Mobile Office around the 15-mile Connected Sprawl.
- **Decide** when opportunities appear (phone call, NPC flag down, zone entry).
- **Earn** through Clean Contracts, VC checks, Street Jobs, or Race Winnings.
- **Burn** daily on payroll, rent, vehicle upkeep, legal.
- **Advance Day** — runway shortens, new events seed, factions remember.

---

## 2. The Founder Mechanic

Zarri isn't gaining "career XP" — he's building a company. Every decision changes his balance sheet.

| Metric | Purpose |
|---|---|
| **Cash** | Liquid USD on hand |
| **Daily Burn** | Sum of recurring daily expenses |
| **Runway** | Cash ÷ Daily Burn — days until bankruptcy |
| **Moral Debt** | Favors owed to the street |
| **Police Heat** | Cops' interest level |
| **Corporate Rep** | Standing with A-Gate / Iron Forest |
| **Street Rep** | Standing with The Lows |

### Death spiral safety valve
If Cash hits 0, the player is **forced** into a dirty job (free cash, heavy tradeoffs). This keeps the game playable while creating narrative stakes.

---

## 3. The Two Paths

### The Safe Path — Corporate Friends (A-Gate Tech / Iron Forest Consulting)
- **Offers:** Stability, legal contracts, VC checks
- **Demands:** Brand sanitization, distance from the old neighborhood
- **Progression:** Outsider → Freelancer → Contract Partner → VC-Backed → Board Member
- **Vibe:** Glass lobbies, quiet drones, 7am standups

### The Fast Path — Street Friends (The Lows)
- **Offers:** Instant liquidity, protection, off-the-books tech
- **Demands:** Heat, moral debt, occasional mid-mission interruptions
- **Progression:** Civilian → Runner → Lieutenant → Kingpin → Untouchable
- **Vibe:** Rail yards, police scanners, late-night warehouses

### The Bridge
Rare "bridge missions" can raise **both** reputations. Finding them is the real meta-game.

---

## 4. The 15-Mile Map — "The Connected Sprawl"

### Zonal Density

| Zone | Vibe | Mechanics |
|---|---|---|
| **The Junction** (3-mile core) | Dense Brooklyn-esque vertical city | Pedestrian-heavy, tight turns, Zarri's office is here |
| **The 85-Express** (arteries) | Atlanta-highway, cinematic speed | 120mph weaving, dialogue calls, motion-blur masking |
| **Iron Forest** (north edge) | Gated tech campuses | Drone patrols, quiet, corporate missions |
| **Rail Yards** (south edge) | Industrial warehouses, docks | Street logistics, cargo heists |

### The Lite Tech Strategy (iPhone-first)

1. **Level Streaming** — Only ~1-mile radius around Zarri is loaded at full density. Everything beyond is a low-poly silhouette proxy.
2. **Commute Loop** — Highway driving masks asset swaps. Crossing "The Arteries" at 85mph+ triggers aggressive pre-streaming of the next zone + motion blur.
3. **Procedural Traffic** — Traffic "clumps" are generated around Zarri (not the whole city). Feels populated, costs nothing past the view distance.
4. **Mobile Rendering** — UE 5.8 mobile deferred path, MobileHDR, no Lumen, static lighting on buildings with a small runtime light budget for vehicles and NPCs. Device profiles keep optional screen-space effects scalable rather than making them a universal cost.

---

## 5. System Architecture (Unreal 5.8, C++)

```
GameInstance
 ├─ UFounderSubsystem   → Cash, Burn Rate, Runway, Ledger
 ├─ UFactionSubsystem   → Corporate Rep, Street Rep, Heat, Moral Debt
 ├─ UDecisionSubsystem  → Strategic Decisions (offer → resolve → apply effects)
 └─ USprawlSaveSubsystem → Versioned progression, map, transform, driving state

World
 ├─ ASprawlGameMode      → Day timer, world setup, restored-player placement
 ├─ USprawlMissionDirector → Calls, branch chaining, unresolved-mission resume
 ├─ AZonalStreamingManager → Loads/unloads zones based on player position & velocity
 ├─ USprawlCityGridSubsystem → Lanes, signals, intersections, lake, city bounds
 ├─ USprawlCityMapSubsystem → Landmark registry, projection, nearest destination
 ├─ USprawlCharacterDeveloper → District roles, schedules, gait, wardrobe, authoring briefs
 ├─ USprawlHumanCharacterModule → Replicated customization plus six-action human state
 ├─ USprawlWardrobeModule → Deterministic fitted layers, palette, shoes, socks, hats
 ├─ USprawlFootwearModule → Human-scale shoe geometry, material layers, safe bone anchors
 ├─ USprawlAthleticShoeModule → Validated swappable trainer presets and runtime cycling
 ├─ FSprawlCrowdMetaHuman → Strict assembled-resident roster, styling, locomotion, LOD
 ├─ USprawlMeleeModule     → Replicated punch/kick state, targeting, damage, reactions
 ├─ ASprawlRoadMarkings  → Paint plus physical perimeter boundaries
 ├─ ASprawlParkingGarage → Instanced deck plus obstruction-tested vehicle exits
 ├─ ASprawlEnterableInteriors → Store, office, condo portals and instanced rooms
 ├─ FSprawlInteriorPropLibrary → Real furniture catalog, layouts, mesh fitting, fallback
 └─ ASprawlCar           → Enterable cars, ordered traffic AI, hidden logical occupants

UI
 ├─ USprawlNativeHUD       → Founder status, touch controls, contextual interaction
 └─ USprawlCityMapWidget   → Paused-safe road, water, landmark, and player map

Data Assets
 ├─ UStrategicDecision       → Designer-authored decisions with N branches
 └─ USprawlCharacterDefinition → Named NPC profile plus optional soft MetaHuman binding
```

### Why subsystems, not singletons
The four core state systems are `UGameInstanceSubsystem`s — they survive level
transitions, are testable, auto-instantiated, and exposed to Blueprints cleanly.

### Why `UPrimaryDataAsset` for decisions
Designers can stamp out a new mission in the editor without touching C++. Every decision is a row of data: prompt, branches, deltas.

### Character authoring contract

Crowd extras are deterministic profiles cast from district, city hour, and
seed. `FSprawlCrowdMetaHuman` resolves every ambient profile through a strict
project-owned Optimized/Low roster: Zarri, Amina, and Andre. The population
manager balances those identities, varies wardrobe and stature without
reshaping an authored face, and never reintroduces the old mannequin/toy
fallback. Missing assembled art leaves the visual hidden and fails the runtime
audit instead of silently lowering the character standard. Named characters
can promote the same profile into a `USprawlCharacterDefinition` with explicit
soft MetaHuman and action-animation bindings. Hugging Face is an offline
authoring assistant only; no model, token, or network dependency ships in the
iPhone runtime.

`USprawlHumanCharacterModule` turns each profile into compact, deterministic
appearance metadata spanning age, presentation, body build, skin-tone
direction, hair texture, wardrobe, and stature. Zarri is the shared
joints-only rig and Stand/Walk/Run/Talk/Sit/Drive action baseline, not a face
template: every resident remains a unique identity. The state is
Blueprint-facing and replicated, including the authored appearance ID, while
long Creator prompts remain local. The server owns population and simulation;
clients render the replicated residents and independently promote only their
nearest people. Mac is capped at eight full residents with three high-detail
slots; iPhone is capped at three with one high-detail slot. LOD selection is
relative to each assembly, so a two-LOD mobile export still keeps a distinct
near and far presentation. First-use class work is staggered to one resident
per population pass. Packaging names only the assembled resident roots and
lets their hard references collect the shared body, groom, clothing, and
material dependencies they actually use.

`USprawlWardrobeModule` resolves the broad wardrobe direction into a complete,
replication-safe outfit: top, bottom, optional outerwear, optional cap/beanie,
shoe type, sock length, and coordinated colors. It recolors each assembly's
fitted default garment and binds low-cost footwear, sock, hat, and jacket
details to the MetaHuman skeleton. `USprawlFootwearModule` measures the live
foot-to-ball chain, clamps it to human-scale dimensions, and builds a rounded
upper, layered sole, collar/laces, and fitted sock for each side. Shoes bind to
an independent calf or usable IK anchor before the underlying Body foot bones
are masked. A two-transform post-animation follower takes position from that
live anchor but facing from the body, keeping both shoes at animated ankles and
roughly level through a run cycle even though hidden MetaHuman foot transforms
freeze. `USprawlAthleticShoeModule` supplies four validated, Blueprint-swappable
trainer presets and wraps the same footwear replacement path; Zarri defaults to
the navy/teal Zarri Velocity pair while residents can cycle deterministic
variants. Added presentation stays non-colliding, shadow-free, decal-free, and
navigation-free; only the fitted shoe pair ticks for the iPhone crowd budget.

`USprawlMeleeModule` is a separate gameplay contract rather than another
locomotion state. It gives Zarri explicit Punch/Kick Blueprint calls plus one
alternating player action, validates a short forward arc on the authority,
emits Unreal point damage, and sends struck pedestrians into their existing
flee behavior. The native HUD adds a dedicated mobile button, while desktop
and gamepad input share the same call. Optional skeleton-compatible one-shots
play above locomotion for a bounded recovery window; absent or incompatible
art cannot remove targeting, reaction, or input functionality.

### Playable destinations and map contract

`ASprawlEnterableInteriors` owns one deterministic contract for Junction
Market, Founder House Offices, and Canal View Condos. Each destination has a
signed sidewalk entrance, an isolated enclosed interior, a themed low-cost
furniture set, and paired entry/exit transforms. The city remains intact:
facade dressing does not collide or clear authored buildings. The reusable
`FSprawlInteriorPropLibrary` fits ten installed textured mesh types into 77
deterministic placements across the rooms, retaining their authored materials
and using one HISM per repeated type. Detailed modular counters, shelves,
desks, monitors, bed, sofa, and kitchen provide distinctive silhouettes and a
safe fallback if the optional pack is absent. E/F and the touch context action
use the same cooldown-safe portal path before vehicle entry is considered.

`USprawlCityMapSubsystem` registers the three playable destinations beside the
parking deck and waterfront, projects them against city bounds, and resolves
an interior player back to its exterior door. `USprawlCityMapWidget` renders
that data as a native paused overlay with roads, water, named markers, Zarri's
live position, and the nearest-destination distance. M, Escape, and the touch
MAP/close controls are all valid while paused; no web service, model, or map
asset is required at runtime.

### Persistence contract

The versioned save payload captures founder, faction, resolved-decision, map,
transform, and vehicle-possession state. Decisions are idempotent: once a
decision ID is resolved, it cannot be offered or paid again. Autosaves occur
after decisions and day advances, plus app backgrounding and clean quit; F5/F9
provide manual save/load during desktop development. Future schema versions are
rejected without mutating the running session.

---

## 6. Example Decision (Authored Data)

```
DecisionId:        "FirstVC"
Title:             "The Iron Forest Term Sheet"
Prompt:            "Rohan wants 15% for $50k. Contract is drawn up. Your neighborhood friend Ty is in the passenger seat listening."
ProposerContact:   "Rohan_IronForest"
ContextFaction:    Corporate

Branches:
  - BranchId:           "AcceptVC"
    Headline:           "Sign the paper."
    CashDelta:          +50000
    DailyBurnDelta:     +80    (hire 1 more engineer)
    CorporateRepDelta:  +25
    StreetRepDelta:     -10
    UnlocksDecisionId:  "FirstBoardMeeting"

  - BranchId:           "Counter"
    Headline:           "We go back and forth. 20% for $80k."
    (triggers negotiation mini-game; outcome data-driven)

  - BranchId:           "StreetLoan"
    Headline:           "Ty knows a guy. No paperwork."
    CashDelta:          +25000
    bCashIsDirty:       true
    StreetRepDelta:     +20
    MoralDebtDelta:     +15
    HeatDelta:          +5
    UnlocksDecisionId:  "TheFirstFavor"
```

---

## 7. Roadmap

### Phase 1 — Prototype (current)
- [x] Project scaffold (`.uproject`, Build.cs, Target.cs)
- [x] Founder subsystem (cash, burn, runway)
- [x] Faction subsystem (rep, heat, debt)
- [x] Mobile Office vehicle pawn
- [x] Zonal streaming manager
- [x] Strategic Decision data model
- [x] Decision subsystem applying effects
- [x] Game mode with day-tick timer
- [x] Mobile-tuned rendering config

### Phase 2 — Gray-box playable
- [x] Zarri character Blueprint (on-foot)
- [x] Enter/exit Mobile Office action
- [x] Enterable curb parking, crash recovery, and city vehicle boundaries
- [x] Right-hand traffic with signal and intersection ordering
- [x] Parking-deck traffic origin with covered low-speed lane merges
- [x] Enterable store, office, and condo with shared portal/interior contract
- [x] Textured furniture/item catalog with detailed store, office, and condo
  fixtures plus deterministic missing-asset fallback
- [x] Native city map with roads, water, five landmarks, live Zarri marker,
  keyboard, Escape, and touch controls
- [x] Gray-box Junction + one Artery + one Outskirt
- [x] 5 authored Strategic Decisions
- [x] UI: Cash/Runway HUD + Decision modal
- [x] Versioned Save/Load of founder, faction, mission, and player state
- [x] Zarri punch/kick module with pawn-level X/gamepad routing, mouse/touch
  controller routes, cooldown-safe alternation, and visible action confirmation
- [x] iOS launch presentation co-branding Angry Cordero Production Studio and
  Unreal Engine with one crop-safe project launch image

### Phase 3 — Vertical slice
- [ ] Procedural traffic MassEntity system
- [ ] Police AI heat-response
- [ ] Corporate NPC + Street NPC cast (2 each)
- [ ] Full-loop demo: sign term sheet → burn through cash → take dirty job

### Phase 4 — Production
- [ ] 15-mile full map
- [ ] 30+ Strategic Decisions
- [ ] Race/pursuit missions
- [ ] Mission giver phone call system
- [ ] Full iPhone optimization pass (60fps target)

---

## 8. Why This Works

GTA-style games are usually about *power*. This one is about *leverage*. The open world is the same (drive, shoot, deal), but the meta-progression is **company-building**, not body-count. Every mission has a fork, and every fork costs you something — money, trust, or soul.

The 15-mile sprawl isn't big for its own sake. It's big because the distance *between* the corporate campus and your old block is the whole theme of the game.
