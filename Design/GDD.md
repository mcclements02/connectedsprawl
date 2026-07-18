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
 ├─ ASprawlRoadMarkings  → Paint plus physical perimeter boundaries
 └─ ASprawlCar           → Enterable player/parked cars and ordered traffic AI

Data Assets
 └─ UStrategicDecision  → Designer-authored decisions with N branches
```

### Why subsystems, not singletons
The four core state systems are `UGameInstanceSubsystem`s — they survive level
transitions, are testable, auto-instantiated, and exposed to Blueprints cleanly.

### Why `UPrimaryDataAsset` for decisions
Designers can stamp out a new mission in the editor without touching C++. Every decision is a row of data: prompt, branches, deltas.

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
- [x] Gray-box Junction + one Artery + one Outskirt
- [x] 5 authored Strategic Decisions
- [x] UI: Cash/Runway HUD + Decision modal
- [x] Versioned Save/Load of founder, faction, mission, and player state

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
