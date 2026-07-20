# AI Handoff Ledger — Project State

<!-- Version control: bump Version and Last updated on every edit to this file. -->
**Version:** 32 · **Last updated:** 2026-07-20 12:35 PDT · **Updated by:** claude

Single source of truth for **in-flight work across every worktree, branch, and
AI agent** (claude · gemini · chatgpt · copilot). How to use it is defined in
[AGENTS.md](AGENTS.md) → "Project State Ledger (Cross-Agent Sync)". This file
holds **state, not rules**.

> Update this ledger in the **same change** as any code edit and commit them
> together, so every branch and worktree carries the current picture and no work
> is stranded. Run `bash scripts/ai-sync-status.sh` for the live view.

## Active Work

One row per in-flight branch/worktree. Different branches own different rows, so
this table merges cleanly. Remove a row once its branch is merged or abandoned
(record that in the Log first).

| Branch | Worktree | Agent | Status | Summary | Updated |
|--------|----------|-------|--------|---------|---------|
| main | `/Users/matthewx/code/ConnectedSprawl` | claude | validated, uncommitted | Game-feel polish pass: GTA-style follow camera + single-joystick touch controls with buttons, planted car handling and far chase cam, playable night lighting, translucent car glass with visible drivers, streetwear outfit recolor, on-foot boundary rescue. All four runtime audits PASS. | 2026-07-19 18:30 PDT |

## Log (append newest on top)

Append-only. One entry per handoff. Never rewrite or delete past entries. A merge
conflict here means two agents diverged — keep **both** entries.

### 2026-07-20 · main · claude (block features contained)
- **New `fix_block_features.py`:** a park deck was lying diagonally across a
  junction. Two structural causes, not cosmetic ones: `City_DW3_playground`
  measured **2573cm wide against a 2000cm block**, so it could not fit
  wherever it was centred; and features were authored at arbitrary yaw, whose
  diagonal edges cut the kerb line even when they would otherwise fit. The
  module squares features to the grid (yaw to nearest 90 deg), scales any
  oversized one until its footprint fits the block interior
  (`BlockSize/2 - SIDEWALK_WIDTH` = 600 half-width), and recentres it. The
  playground scaled x0.45; 1 squared, verified clear of the carriageway.
- **MISTAKE — 25 props displaced.** `City_RPG_Junction` was included in the
  feature list on the strength of its name. Those belong at **road
  junctions**, and the pass recentred 25 of them onto block centres before
  that was caught. Their original positions are not recoverable from the saved
  map. The prefix is now excluded with a comment explaining why; they are
  small (51x6x46cm) so the visual cost is low, but it is a real displacement
  and a reminder to verify what a prefix actually is before acting on it.
- **Validation:** feature pass clean, street verified clear in a captured
  frame. VisualAudit still fails the warmth gate only (luma 101.2).
- **Status:** same uncommitted change set on `main`.
- **Next:** optionally re-place `City_RPG_Junction` props at intersections;
  the last lane violator; the warmth gate; the legacy generation scripts.
<!-- entry:block-features-contained -->

### 2026-07-20 · main · claude (sidewalk ring, driver seat, lane tracking)
- **New `city_sidewalks.py`:** `city_surfaces.py` gives each block one surface,
  so a park block met the carriageway with grass running straight to the kerb
  and nowhere to walk. This lays a 400cm pavement ring around every block —
  172 strips — sitting 0.5cm proud of the block surface so it reads as the
  walking surface over grass or paving alike. Spawns actors, so no `-nullrhi`.
- **New `FSprawlDriverSeat`:** the seated occupant was pinned at a fixed offset
  tuned for the prototype kitbash, so on imported bodies drivers sat half
  outside the shell. The seat is now measured from the bodywork the car is
  actually wearing (visible mesh bounds in actor space, occupant excluded):
  ahead of centre, offset to the driver's side, at seated hip height off the
  body floor. Any body, any scale, always inside.
- **Lane tracking — hypothesis tested and corrected by measurement.** I first
  assumed the ~120cm lane error was oscillation and lengthened the lookahead
  while softening the gain: **violators went 2 -> 5**, disproving it. The cars
  were converging too slowly, not hunting. Reversing the direction — lookahead
  `clamp(speed * 0.36, 240, 460)` and gain `YawError / 24` — took violators to
  **1**, better than the 2 it started at, with signal stops 9 -> 14 and min
  spacing 311 -> 831. Recorded because the intuitive read was the wrong one.
- **Validation:** Build clean. TrafficAudit 8/9 gates (1 lane violator, 0
  boundary/upright/unauthorized-entry, occupancy 1, 14 signal stops, 26
  pedestrians). VisualAudit fails only the warmth gate again
  (`red_blue=0.997` vs `>= 1.00`; luma 100.7, crushed 0.01%) — the scene is
  fractionally cool, not broken, and the gate was left alone.
- **Status:** same uncommitted change set on `main`.
- **Next:** the last lane violator, then the warmth gate (either the scene
  wants a touch more warmth or the gate's lower bound is too tight for an
  overcast sky), then the legacy generation scripts.
<!-- entry:sidewalk-ring-driver-seat-lane-tracking -->

### 2026-07-20 · main · claude (two-way street with parallel parking)
- **Changed:** `LanesPerDirection` 2 -> 1 at the user's request — a two-way
  street with parallel parking rather than a 4-lane arterial. Everything
  re-derives from that one constant: `RoadWidth` 1900 -> 1200, `Step` 3200,
  `Span` 22400, boundary 11200, `ParkingOffset` 475, stop lines, and every AI
  approach distance. `ASprawlRoadMarkings` now builds its stripe list
  dynamically, painting lane dividers only *between* travel lanes — so a
  two-way street gets a centreline and parking edge lines and no divider
  (2558 instances, 322 bays, verified "1 lanes/direction on a 1200cm
  carriageway").
- **`expand_streets.py` reworked to anchor by OWNERSHIP, not proximity** —
  the fix for the regression logged in the previous entry. Within
  `BlockSize/2` of a block centre an actor is block-owned and keeps its exact
  block-relative offset; otherwise it is road-owned and its road offset scales
  by `NEW_ROAD/OLD_ROAD`. The 1900 -> 1200 pass moved 2379 block-owned actors
  and **`fix_building_placement.py` then reported 0 in the carriageway, 0 over
  the walking band, 0 moved** — the kerbside damage did not recur, which is
  the proof the anchoring rule was the real fix.
- **`fix_kerbside_anchoring.py` disabled (`ENABLED = False`)** — it was a
  one-shot inverse repair for the proximity-anchored 600 -> 1900 pass. It is
  unnecessary now and its 280-700cm band would straddle the 1200cm
  carriageway and shove road furniture onto the pavement if re-run.
- **Validation:** Build clean. Layout pipeline ran in the documented order
  (expand -> park -> surfaces -> buildings): 36 cars in clear bays, 40
  pavement + 3 grass blocks, 0 building intrusions. TrafficAudit 8/9 gates:
  14 signal stops, box occupancy 1, 0 unauthorized entries, 0 boundary/upright
  violations, min spacing 319, 26 pedestrians.
- **KNOWN ISSUE — lane discipline.** `lane_violators=2`: `SprawlCar_93` at
  (8053,-3332) err 122 and `SprawlCar_90` at (-6739,-7960) err 136, each
  sustained >1.5s. With a 350cm lane the centre is 175 from the road
  centreline, so a ~120cm error puts the car's edge over the parking-bay line
  at 350. Two principled changes were tried and **neither moved the number** —
  measuring error against the nearest legal lane (correct for multi-lane), and
  committing turns on entering the junction rather than 175cm from the centre
  of a 950cm box. Both were kept on their own merits. That elimination points
  at the lane-keeping controller itself — `RunAutoDrive`'s 360cm lookahead and
  `YawError / 30` steering gain, neither of which scales with lane width —
  rather than at the geometry constants. **Next session: scale the lookahead
  and/or gain to `Grid::LaneWidth` and re-run.** Cars otherwise drive, obey
  signals, hold right-of-way and do not collide.
- **VisualAudit** fails by a hair on warmth only: `red_blue=1.000` against a
  `>= 1.00` gate (luma 102.4, crushed 0.01%, clipped 0.18% all comfortably
  inside). The frame is exactly neutral rather than fractionally warm; the
  gate was left alone rather than loosened to force a pass.
- **Status:** same uncommitted change set on `main`.
- **Next:** the lane-keeping tune above; then the legacy generation scripts,
  which still carry the original 2000/600/2600 constants.
<!-- entry:two-way-street-parallel-parking -->

### 2026-07-20 · main · claude (surfaces, kerbside fix, building line)
- **Regression fixed (mine):** `expand_streets.py` anchored each actor to its
  *nearest* grid anchor. Anything in the outer ~350cm of a block is nearer the
  road centre than its own block centre — exactly where kerbside trees,
  hydrants, benches and shopfronts live — so those scaled with the road and
  ended up standing in the widened carriageway (user screenshot: trees and a
  planter in the street). New `fix_kerbside_anchoring.py` applies the exact
  inverse: the mapping is invertible, so anything now 280-700cm from a road
  centre was block-owned and is shifted outward by
  `NEW_ANCHOR_HALF - OLD_ANCHOR_HALF` = 650cm, landing back at its original
  distance from its block centre. **1338 objects returned**, including 97
  trees, 74 street lights and 123 shopfront pieces. Road furniture (<280cm)
  and parked cars (825cm bays) untouched.
- **New `city_surfaces.py`:** the world is one asphalt plane, so street and
  pavement were indistinguishable grey. Lays one surface slab per block flush
  with the kerb — grass on the 3 park blocks, pavement on the other 40 — and
  deliberately leaves the gaps bare so the ground plane reads as the street.
  Collision off; the ground plane already carries it. **Spawns actors, so it
  must run without `-nullrhi`.**
- **New `fix_building_placement.py`:** holds structures to a building line
  1350cm from each road centre (kerb 950 + 400 walking band) and keeps the
  carriageway clear; awnings and signs may overhang the pavement by design.
  629 structures set back; re-run reports **0 in the carriageway, 0 over the
  walking band, 0 moved** — converged and idempotent.
- **New `Design/CITY_MODULES.md`:** documents every layout module, the grid
  cross-section, the required run order after a grid change, which modules
  spawn actors, and the anchoring trap above. All modules are 136-198 lines.
- **Process note:** the building pass was applied before its report was read.
  The first run showed implausible 1348cm "intrusions", which warranted
  inspection first; the result was verified good afterwards, but the correct
  order is audit (`APPLY = False`) then apply.
- **Validation:** VisualAudit PASS (mean_luma 103-107) at each stage; building
  placement converged to zero; street-level frames show a clear carriageway,
  kerbside trees and cars, and visible grass and pavement surfaces.
- **Also corrected (both principled, neither masking):** the traffic audit
  measured lane error against lane 0 only, which is wrong now each direction
  has two legal lanes — it now measures against the nearest legal lane. And
  turns committed `LaneOffset` (175cm) before the road centre, which suited a
  300cm junction but is deep inside a 950cm one, so cars exited wide; they now
  commit on entering the box (`IntersectionHalf - LaneOffset`).
- **KNOWN ISSUE — one stuck traffic car.** TrafficAudit fails a single gate,
  `lane_violators=1`, and it is the same car at the same place across runs:
  `SprawlCar_93` at (9414, -3946), lane_error ~160, barely moving between
  samples (9414.7 -> 9424.0 over the run). That is **stuck, not drifting** —
  something is blocking a travel lane on the vertical road at x~9750, mid-block
  in Y. Every other gate passes (0 boundary/upright/unauthorized-entry
  violations, box occupancy 1, 8-10 signal stops, 26 pedestrians, min spacing
  340). Neither the lane-metric nor the turn-commit change moved it, so the
  cause is an obstruction at that spot, not AI tuning. **Next session: find
  what occupies (9414, -3946)** — likely a prop or building corner left inside
  the carriageway that the placement pass did not classify as a structure.
- **Status:** same uncommitted change set on `main`; traffic audit is at 8/9
  gates with the above understood and localised.
- **Next:** the stuck-car obstruction above, then the legacy generation scripts
  which still carry the old grid constants.
<!-- entry:city-surfaces-and-building-line -->

### 2026-07-20 · main · claude (streets widened to 2 lanes + parking)
- **Why it was structural:** `ParkingOffset` was 410 while the road half-width
  was only 300 — "parking" had always placed cars *past the kerb*, on the
  pavement, which is why trees grew through them. A 600cm street cannot hold
  two lanes each way plus a bay, so the grid itself had to grow.
- **Changed (grid):** `USprawlCityGridSubsystem` now derives the carriageway
  from the lane layout instead of a magic number:
  `RoadWidth = 2 * (2 lanes * 350 + 250 bay) = 1900`, `Step` 2600 -> 3900,
  `Span` 18200 -> 27300, boundary 9100 -> 13650. `LaneCenter()` takes a lane
  index (0 = centreline side, 1 = kerbside) and `ParkingLaneCenter()` is new;
  stop-line distances derive from the road width. Every existing AI call site
  defaults to lane 0, so traffic keeps valid lane discipline unchanged.
- **Changed (paint):** `ASprawlRoadMarkings` now stripes a dashed centreline,
  a dashed divider between the two travel lanes each side, a near-solid edge
  line against the parking bay, and per-bay ticks — 3960 instances, 322 bays,
  crosswalks and stop lines rescaled to the wider approach.
- **New modules:** `expand_streets.py` maps every actor from the old anchor
  spacing onto the new one (anchors alternate block/road centre each half
  step), so block interiors keep their exact internal layout and only the gaps
  open up — 2666 actors remapped, max shift 84.5m, ground plane rescaled, and
  the 264 legacy `City_Detail_*Center/Lane/Edge` stripes retired since the C++
  system now paints them. `park_cars_in_bays.py` then assigns all 36 parked
  cars a real bay, testing each against 1161 tree/prop/building footprints and
  the cars already placed. `audit_street_geometry.py` and
  `audit_vehicle_grounding.py` are the read-only diagnostics behind both.
- **AI constants that had to follow the geometry** (each was silently tied to
  the old 600cm street and broke a different audit gate):
  `DecisionDistance` was 1100, *shorter* than the new 1490 stop line, so cars
  could never evaluate a signal before reaching it; the right-of-way lease was
  only refreshed on the approach, so it expired mid-crossing of a 19m box and
  read as an intrusion; the lease released at centre+400, admitting a second
  car while the first still occupied the box; and `SpawnNeeded` snapped cars
  onto a lane at a random along-road position — with junctions now covering
  ~half of each road, cars materialised inside intersections holding no lease.
  All four now derive from `Grid::RoadWidth`/`VehicleStopDistance`. The traffic
  audit's own hold window and lane-departure margin were hard-coded to the old
  road too and now derive from the grid.
- **Validation:** Build clean. TrafficAudit **PASS** (16 movers, 15 signal
  stops, box occupancy 1, **0 unauthorized entries**, min spacing 338, zero
  lane/boundary/upright violations, 26 pedestrians). ProgressionAudit,
  LocomotionAudit (0.00 deg) and CarjackAudit all PASS. VisualAudit PASS
  (mean_luma 103-113). Street-level frame shows the wider carriageway, painted
  lanes and cars parked along the kerb.
- **Status:** same uncommitted change set on `main`.
- **Next:** the older generation scripts (`build_city.py`, `add_city_detail.py`,
  `realkit_apply.py`, `dw_*.py`) still hard-code `BLOCK/ROAD/STEP = 2000/600/2600`.
  They are no longer consistent with the live grid — re-running any of them
  would rebuild at the old spacing. Update those constants before using them
  again, or treat `expand_streets.py` as the migration step after any rebuild.
<!-- entry:streets-widened-two-lanes-parking -->

### 2026-07-20 · main · claude (parked cars: runtime stance seating)
- **Changed:** the earlier construction-time stance fix could not reach the
  city's 51 `City_ParkedCar_*`/`City_TrafficCar_*` actors, because an authoring
  pass had assigned each an external body mesh and **serialised its offset into
  the level** — so their visuals still hung ~7-19 cm below the hull's contact
  plane and the wheels stayed under the tarmac (user screenshot: red Mini with
  its wheels cut off at road level). `FSprawlVehicleStance::ComputeVisibleBottomZ`
  now measures the lowest point of everything actually visible on a vehicle
  from the live components (each re-derived in actor space, so wheels on their
  steering pivots measure correctly), and `ASprawlCar::SeatVisualOnContactPlane`
  shifts every visual child of the hull by the difference at BeginPlay. This
  works regardless of how a body was authored — constructor, runtime split
  parts, or baked into the map — and is a no-op once correct.
- **Validation:** Build clean. All **51 cars seated** at BeginPlay, lifted
  7.1-12.9 cm each (e.g. `SprawlCar_2 seated by 12.9 cm, visual bottom -74.9 ->
  contact plane -62.0`). TrafficAudit PASS (14/14 movers, wheel motion on all
  14, zero boundary/upright/lane violations); VisualAudit PASS (mean_luma
  98.5); parked-car close-up shows tyres meeting the surface.
- **Gotcha recorded:** `UE_LOG(LogTemp, Log, ...)` is **filtered out** of
  `-game` runs in this project — `[RoadMarkings]` and the first `[Stance]` pass
  were both invisible despite running. Use `Display` for anything that needs to
  be verifiable headlessly.
- **Status:** same uncommitted change set on `main`.
- **Next:** unchanged.
<!-- entry:parked-car-stance-seating -->

### 2026-07-20 · main · claude (vehicle stance, decals off characters)
- **Changed (tyres):** new `FSprawlVehicleStance` module. `ASprawlCar` rests on
  its collision hull, so the hull underside is the contact plane, but the
  assembly seated the *body* mesh's underside there — and since tyres hang
  below a car's floorpan, every wheel sank into the road. The module measures
  the lowest point of body **and** wheels and seats that instead, which is what
  produces real ride height. Prototype wheels had the same fault arithmetically
  (hull bottom -62, wheel centre -54, radius 41 -> tread at -95, i.e. 33 cm
  under the tarmac); their centre height is now derived from the hull extent
  and wheel radius rather than hard-coded.
- **Changed (paint on Zarri):** new `FSprawlCharacterRender` module. The city
  uses 115 DecalActors for crosswalk paint and grime, and a decal projects onto
  any primitive that has not opted out — so characters walking over paint wore
  it. Decal reception is now disabled for the hero (including every component
  of the assembled MetaHuman: body, face, hair, clothing), all carried
  equipment, pedestrians and seated drivers, via `ApplyAvatar`, the locomotion
  component and the MetaHuman activation path.
- **Validation:** Build clean. TrafficAudit PASS after the stance change
  (14/14 movers, wheel motion on all 14, zero violations); VisualAudit PASS
  (mean_luma 98.2); parked-car close-up shows wheels resting on the surface
  rather than buried.
- **Investigated and reverted:** residual white speckling on Zarri in captures
  is temporal-AA ghosting, not decals — proved by capturing with
  `r.AntiAliasingMethod 0` (completely clean) and with
  `r.BasePassForceOutputsVelocity 1` (also clean). The permanent fix
  `r.VelocityOutputPass=1` regressed scene lighting badly (mean luma 98 -> 72,
  red/blue 1.01 -> 0.83, VisualAudit FAIL), so it was reverted and the config
  carries a comment recording the trade. Ghosting remains a known cosmetic
  issue, worst right after a teleport.
- **Status:** same uncommitted change set on `main`.
- **Next:** if the ghosting matters in play, the lead is why base-pass velocity
  darkens this scene (likely interaction with the mobile shading path or the
  `r.Velocity.EnableVertexDeformation=2` setting) rather than the cvar itself.
<!-- entry:vehicle-stance-and-character-decals -->

### 2026-07-20 · main · claude (crowd casting, world placement repair)
- **Changed (crowd):** New `FSprawlCrowdAppearance` module casts the pedestrian
  pool by measurement rather than a hand-kept blocklist. Each variant's head
  length is read from its reference skeleton as a fraction of standing height
  (`FSprawlAvatarLibrary::ComputeHeadHeightRatio`); adults sit near an eighth,
  caricatures far higher, and only variants at or under 0.20 walk the city or
  ride in traffic. Both call sites (`ASprawlPedestrian`, `ASprawlCar`'s seated
  driver) now draw from it. Measured spread: **cast 8** — Lydia 0.067, Chill
  0.122, BizDude/Bruno 0.150, Kyle 0.162, Erika 0.172, Samuela 0.174, Baldman
  0.197; **cut 13** — Rose 0.207 … Juanita 0.475 (includes Cappy 0.285, the
  big-head figure in the reported screenshots). The table is logged every run
  so the cut stays auditable; an empty result falls back to the full pool.
- **Changed (shared):** the reference-skeleton helpers moved out of
  `USprawlLocomotionComponent` into `FSprawlAvatarLibrary`
  (`ComputeMeshForwardYaw`, `ComputeHeadHeightRatio`), so facing and
  proportion measurement share one implementation.
- **Changed (world):** new `audit_world_placement.py` (read-only diagnostic)
  and `repair_world_placement.py` (idempotent, non-destructive repair). Root
  cause of both reported world bugs was one authoring slip: Python's
  `unreal.Rotator` takes **(roll, pitch, yaw)** but `realkit_apply.py:248`,
  `dw_realism2.py` and friends called it positionally as
  `Rotator(0, yaw, 0)`, so every prop's intended yaw landed in PITCH and tipped
  it over — 722 tilted actors including 134 trees, 40 hydrants, 40 bins,
  storefronts and trim. The repair recovers the intended angle (unmirroring the
  `roll=180/yaw=180` normalized form), puts it back on yaw, and re-seats each
  actor on its authored surface (`SIDEWALK_Z=14`, `ROAD_Z=1.5`, both flat and
  exact — no tracing). Separately, lane markings had been pushed to z=11,
  hovering ~9 cm over the asphalt and slicing through pedestrians' ankles
  (the "paint going through Zarri" report); 264 were laid back down to
  top_z=2.8, just under the C++ `ASprawlRoadMarkings` plane so the two systems
  layer instead of z-fighting.
- **Validation:** Build clean. Repair moved 555 upright + 485 re-seated + 264
  markings, then re-ran as a no-op (idempotent). Audit tilt count 722 -> 168,
  and every remaining tilt is deliberate: 89 awnings (authored with a real
  slope), 78 R2 decals (orientation is their projection), and the sun.
  TrafficAudit PASS after the repair (26 pedestrians, 26 real avatars, 14
  visible drivers, zero violations); VisualAudit PASS (mean_luma 98.2);
  LocomotionAudit still PASS (0.00 deg) after the helper move.
- **Status:** same uncommitted change set on `main`.
- **Next:** awnings carry the same Rotator slip (intended `roll=-90`) but were
  left alone rather than guessed at — worth a look in-editor. Minor floating
  canopy cards remain on the DW3 tree asset (art, not placement). Threshold
  `MaxHumanHeadRatio` is one constant if the crowd should be stricter (0.18
  would also cut Baldman) or looser.
<!-- entry:crowd-casting-and-placement-repair -->

### 2026-07-20 · main · claude (modular locomotion; sideways run fixed)
- **Changed:** New standalone `USprawlLocomotionComponent`
  (`Public/Characters/SprawlLocomotionComponent.h` + private impl) owns on-foot
  gait selection and body facing for any visual — imported avatar, assembled
  MetaHuman, or future pawns. Facing is derived from the mesh's own reference
  skeleton (first usable left/right bone pair, hips preferred over arms;
  forward = right × up) and reconciled against the visual's live world
  transform, so rotations already baked into a Blueprint or applied by
  `FSprawlAvatarLibrary::ApplyAvatar` are respected and never doubled. Gaits
  are data: a fastest-first `FSprawlGait` band list (clip, min speed,
  reference speed, play-rate clamps) replacing the hard-coded if/else ladders.
  `AZarriCharacter` now just feeds it a ground speed; ~60 lines of duplicated
  animation branching removed, and `HeroCurrentAnim`/`MetaHumanCurrentAnim`
  are gone (the component owns clip state).
- **Root cause fixed:** the fallback path applied the Mixamo `-90` yaw inside
  `ApplyAvatar`, but the MetaHuman child actor was placed with
  `FRotator::ZeroRotator`, so the assembled body sat 90 deg off the capsule and
  Zarri ran sideways. Measured correction is now `-90.0` for
  `/Game/MetaHumans/Zarri/BP_Zarri` (Body reads as facing 90 deg locally) and
  `-0.6` for the imported avatar.
- **Added audits:** `-SprawlLocomotionAudit` sweeps six yaws and gates on the
  body's facing error vs the pawn heading (<= 1 deg); `-SprawlAuditRun` holds
  forward input so `-SprawlVisualAudit` captures a running pose instead of an
  idle one.
- **Validation:** Build clean. LocomotionAudit PASS (`visual=metahuman
  worst_facing_error=0.00 deg applied_correction=-90.0`). VisualAudit PASS
  running (mean_luma 113.4, crushed 0.02%) and the captured frame shows Zarri
  square to his direction of travel, back to camera, mid-stride.
- **Status:** same uncommitted change set on `main`.
- **Next:** `ASprawlPedestrian` and the seated driver visual still run their
  own copies of the avatar/anim logic and could adopt this component next;
  otherwise unchanged.
<!-- entry:modular-locomotion-component -->

### 2026-07-19 · main · claude (nose-first kits + calmer run camera)
- **Changed:** `TryApplyRuntimeVehicleParts` no longer hard-codes a +90° mesh
  yaw: it derives each kit's forward from the FBX-named front axle (front vs
  rear wheel centroids) so every variant drives nose-first — live play showed
  some kits rendering tail-first at speed. Follow camera: correction is now
  proportional to yaw error (soft dead-band, no wobble while running
  straight), turn speed 3.4→2.6, sprint no longer over-drives the swing; the
  MetaHuman run-anim rate cap rises to 1.5 so the 760 cm/s sprint stops
  foot-sliding.
- **Validation:** Build clean; TrafficAudit PASS (14/14 movers, visible
  drivers 14, min_spacing 414, zero violations); before/after boulevard
  frames confirm the flipped kit's taillights now face the rear while an
  already-correct kit is unchanged.
- **Status:** same uncommitted polish change set on `main`.
- **Next:** unchanged.
<!-- entry:kit-orientation-and-follow-cam -->

### 2026-07-19 · main · claude (Esc frees the cursor)
- **Changed:** Bare Esc now saves AND releases the OS mouse cursor from the
  game window (`FInputModeGameAndUI`, no viewport lock, cursor shown) — live
  play showed the Esc reflex was really "give me my mouse back". Clicking the
  game recaptures (`LeftMouseButton` binding with `bConsumeInput = false`,
  suppressed while a modal is up). Shift+Esc remains the quit chord.
- **Validation:** `ConnectedSprawlEditor Mac Development` rebuilt clean.
- **Status:** part of the same uncommitted polish change set on `main`.
- **Next:** unchanged.
<!-- entry:esc-cursor-release -->

### 2026-07-19 · main · claude (Esc chord, supersedes entry below)
- **Changed:** Per user request after live play, quitting is now the deliberate
  chord **Shift+Esc** (`ASprawlPlayerController::OnEscapePressed` checks the
  held modifier). A bare Esc only saves and shows "Progress saved — Shift+Esc
  quits the game". The 3-second double-press window from the previous entry is
  removed — reflex double-taps were still killing the session.
- **Validation:** `ConnectedSprawlEditor Mac Development` rebuilt clean
  (Result: Succeeded, 21.95 s).
- **Status:** part of the same uncommitted polish change set on `main`.
- **Next:** unchanged.
<!-- entry:esc-shift-quit-chord -->

### 2026-07-19 · main · claude (evening follow-up)
- **Changed:** Two-stage Esc in `ASprawlPlayerController`: first press saves and
  shows "Progress saved — press Esc again to quit"; a second press within 3 s
  quits. Two consecutive interactive play sessions ended ~50 s in via
  `UGameEngine::HandleExitCommand` — the old Esc=instant-save-and-quit read as
  a crash-to-desktop to GTA pause-menu reflexes.
- **Validation:** `ConnectedSprawlEditor Mac Development` rebuilt clean
  (21.8 s); windowed `-game` launches on this Mac bring TestMap up with the
  MetaHuman active and exit only via the new confirmed path. Interactive play
  logs live at `~/Library/Logs/ConnectedSprawl/ConnectedSprawl.log` (the .app
  relaunch does not write `Saved/Logs/`).
- **Status:** part of the same uncommitted polish change set on `main`.
- **Next:** unchanged from the entry below.
<!-- entry:esc-two-stage-quit -->

### 2026-07-19 · main · claude
- **Changed:** Game-feel polish pass across controls, cameras, lighting, cars,
  and wardrobe. `AZarriCharacter` gains a GTA-style auto-follow camera (eases
  behind travel after look input goes idle; fixes curved "drunk" forward
  walking), shoulder-offset framing, a public sprint setter, and an on-foot
  boundary rescue that teleports back to the last safe sidewalk location on a
  fall or perimeter escape. `ASprawlCar` gains lateral tyre-grip damping and a
  brake-force multiplier (no more hovercraft skating), a farther/higher chase
  camera (arm 780→1000, FOV 88), and a touch-pedal throttle override.
  `ASprawlPlayerController` clamps view pitch and routes a new touch API.
  `USprawlNativeHUD` builds touch controls on touch platforms only: right-half
  drag-to-look pane plus contextual JUMP/SPRINT/ENTER ↔ GAS/BRAKE/EXIT buttons;
  `DefaultInput.ini` switches to the engine's LeftVirtualJoystickOnly interface
  (no second joystick). `ASprawlEnvironmentController` gets playable-night
  floors (moon 1.35, sky 0.60, thinner night fog, streetlights 13.5k/30m/26
  pooled) and `-SprawlStartHour=`/`-SprawlFreezeClock` capture hooks; the
  visual audit accepts `-SprawlAuditShowUI` to composite the HUD. New
  `game_ready_polish.py` authored translucent `M_CarGlassClear`, reparented
  `MI_CarGlass` (opacity 0.30) so drivers are visible through real glass,
  retargeted glass slots across the Car_1–10 kits, and recolored the MetaHuman
  garment MIs from near-black indigo to a light heather tee + charcoal shorts.
- **Validation:** `ConnectedSprawlEditor Mac Development` builds clean.
  Real-Metal offscreen runs: VisualAudit PASS day (mean_luma 96–99, crushed 0%),
  TrafficAudit PASS (14/14 movers, visible_drivers=14, 0 boundary/upright/lane
  violations, min_spacing 312), ProgressionAudit PASS, CarjackAudit PASS.
  Night frame at 22:00 verified visually (moody but readable; luma 22 vs 19.5
  before). UI frame confirms single left joystick + labeled buttons, no
  overlap. UE 5.8 headless python now needs `-run=pythonscript -script=...`
  (`-ExecutePythonScript` is ignored; LogPython output is swallowed — the
  script writes `Saved/GameReadyPolishReport.txt` instead).
- **Status:** complete and validated on macOS; all changes uncommitted in the
  `main` worktree awaiting user review/commit.
- **Next:** iOS cook + on-device performance pass (translucent glass and the
  higher night light floors are the new costs to profile), optional rounded
  styling for the touch buttons, shoes/wardrobe assets remain the known
  MetaHuman gap, and Git LFS setup is still outstanding for the 97.5 MiB
  `MHC_Zarri.uasset` already on origin.
<!-- entry:game-feel-polish-pass -->

### 2026-07-19 · main · codex
- **Changed:** Completed the UE 5.8 MetaHuman Zarri pass after installing Epic's
  Creator Core Data. Added the reproducible `create_zarri_metahuman.py` authoring
  pass and generated `/Game/MetaHumans/Source/MHC_Zarri` plus the Optimized Low
  `/Game/MetaHumans/Zarri/BP_Zarri` runtime actor. Zarri is now a 177.5 cm slim,
  young Black male with a warm medium-deep complexion, clean face, thick flat
  brows, fine lashes, and a short black Afro Fade. The installed default outfit
  is baked dark indigo/charcoal. `AZarriCharacter` keeps native movement,
  capsule, camera, equipment, and vehicle authority while the assembled actor
  provides the on-foot visual and Core Data idle/walk/run clips; validation
  failures still restore the lightweight Zarri/Cappy/mannequin path.
- **Validation:** UE 5.8 MetaHuman cloud rig and 2K-source requests completed;
  final Optimized Low assembly logged `MetaHuman Character assembly succeeded`
  and the script's `PASS`, followed by validation of all 52 generated runtime
  assets. The project now contains 252 MetaHuman assets (about 461 MB). The final
  `ConnectedSprawlEditor Mac Development` native build passed (37.63 s), the
  Python authoring pass compiled with Epic's Python 3.11, and repeated 1280x720
  real-Metal standalone launches showed the assembled face/body, black card
  Afro, dark outfit, HUD, city, and locomotion in-game.
- **Status:** the playable on-foot MetaHuman is complete and remains unstaged
  and uncommitted. Epic's installed Core Data only provides a T-shirt/shorts
  outfit, so it cannot supply the reference's denim/shearling jacket, hoodie,
  cargo pants, high-top shoes, headphones, or cross-body strap. The seated
  player-car visual also remains the lightweight Zarri proxy by design.
- **Next:** acquire or author license-safe, mobile MetaHuman-compatible wardrobe
  and accessory assets for the exact reference silhouette, replace the seated
  proxy if device profiling allows it, run an iOS cook/device performance pass,
  and configure Git LFS before publishing the 97.5 MiB source character asset.
<!-- entry:metahuman-zarri-assembled -->

### 2026-07-19 · main · codex
- **Changed:** Added an optional assembled-MetaHuman visual boundary to native
  `AZarriCharacter`, with stable soft references for
  `/Game/MetaHumans/Zarri/BP_Zarri` and its project-owned locomotion AnimBP.
  The MetaHuman activates only after its actor, `Body` mesh, and animation
  contract validate; otherwise the existing Zarri/Cappy/mannequin path remains
  playable. The native capsule, movement, camera, vehicle possession, seated
  driver proxy, and equipment systems remain authoritative. MetaHuman collision
  is disabled, equipment reattaches to standard MetaHuman sockets, and on-foot
  visual visibility stays coherent across vehicle entry/exit.
- **Validation:** `ConnectedSprawlEditor Mac Development` compiled successfully
  with UE 5.8 (4 actions, 22.62 s); `git diff --check` passed before this ledger
  update. The installed MetaHuman code/pipelines and official guidance were
  audited for assembly output, runtime integration, and iOS constraints.
- **Status:** blocked on an external local prerequisite: this UE 5.8 install is
  missing **MetaHuman Creator Core Data**, so the requested face, skin, hair,
  and wardrobe cannot yet be authored or assembled in-editor.
- **Next:** Epic Games Launcher → Unreal Engine → Library → UE 5.8 dropdown →
  Options → enable **MetaHuman Creator Core Data** → Apply. Then create a young
  Black male Zarri from the supplied reference, use card/mesh hair, assemble
  Optimized Low/Medium under `/Game/MetaHumans/Zarri`, author the locomotion
  AnimBP at the configured path, and run PIE plus iPhone device profiling before
  considering MetaHuman validator skinning-setting changes.
<!-- entry:metahuman-zarri-integration-blocked-core-data -->

### 2026-07-19 · main · codex
- **Changed:** Started the requested UE 5.8 MetaHuman Zarri pass and enabled
  Epic's installed MetaHuman Creator plugin for this project.
- **Validation:** Mandatory Git/worktree preflight was clean on `main` at
  `823ab7f`; the UE 5.8 installation contains MetaHuman Creator, SDK, Animator,
  Crowd, and related Mac binaries. Editor authoring and gameplay validation are
  still pending.
- **Status:** in progress; the current lightweight Zarri remains the safe
  fallback until the MetaHuman is assembled and validated.
- **Next:** create the MetaHuman in-editor from a young Black male preset,
  visually match the supplied face, hair, skin, and silhouette, then assemble
  an iPhone-appropriate output and integrate it with Zarri.
<!-- entry:metahuman-zarri-start -->

### 2026-07-19 · main · codex
- **Changed:** Finalized the complete living-city change set for the requested
  clean local commit and added a dedicated Zarri hero variant: a lightweight
  young Black male avatar with medium-deep skin, dark hair, tech-streetwear,
  all seven animation clips, and seated-driver support. The focused importer
  can now target named variants, configures `BP_Zarri`, and keeps Zarri out of
  the ambient crowd pool. Raised the canonical exposure bias from `0.65` to
  `0.70` to restore visual-audit headroom with the darker hero palette.
- **Validation:** Zarri's Blender build changed 506,924 atlas pixels, passed a
  clean FBX re-import (`1` rig, `1` embedded texture, `7` clips), and produced
  reviewed Walk/Sit QA renders. Changed Python compiled and `git diff --check`
  passed. The UE 5.8 Mac editor build passed (`9` actions, 36.82 s); the focused
  Unreal import created `SK_Zarri`, material/texture/skeleton, and all seven
  sequences, then bound `BP_Zarri`. The mobile texture pass and 22-variant
  static audit passed. Final traffic audit passed (`cars=14`, `moved=14`,
  `visible_drivers=14`, `missing_drivers=0`, `ped_offside=0`,
  `lane_violators=0`), and the real-Metal visual audit passed at 1280×720
  (`mean_luma=96.05`, `crushed=0.00%`, `clipped=0.26%`, `red_blue=1.007`). The
  final canonical color-pass rerun was a true no-op (`map_changed=False`).
- **Status:** complete and included in the user-authorized local cleanup
  commit; no remote push was requested.
- **Next:** push `main` when requested, then run the deferred interactive
  editor and iPhone device playtests.
<!-- entry:zarri-and-clean-local-main -->

### 2026-07-19 · main · codex
- **Changed:** Completed the living-city pass: 21 seven-clip pedestrian avatar
  sets with new sprint/seated animation support; five new CC0 variants; visible
  seated AI/player drivers; slow-car takeover with an ejected fleeing driver;
  legal retirement/backfill and lease handling; sidewalk recovery, contained
  fleeing, and signal-aware crossings; native HUD, repeatable clean-income gig,
  dirty-bankruptcy bailout, second-bankruptcy loss, and persistent victory
  sentinel; plus a deterministic, measured daylight/material pass and a clean
  opening PlayerStart orientation. The real-RHI visual audit now saves the
  exact scored PNG for human review.
- **Validation:** Blender clean-reimport and Walk/Sit QA passed for all five new
  avatars; Unreal imported 21 variants × 7 clips and the mobile texture budget
  clamped 0 textures. Changed Python compiled. The final UE 5.8 Mac editor build
  passed (UHT + 3 actions, 192.19 s). Gameplay authoring and the idempotent city
  pass saved 1 player car + 14 traffic + 36 enterable parked cars, 30 signals,
  and 26 humans. Static audit passed (`upright=51`, `avatars=21`, `clips=7`).
  Final traffic audit passed (`visible_drivers=14`, `missing_drivers=0`,
  `ped_offside=0`, `lane_violators=0`); carjack audit passed possession,
  fleeing-driver, lease-free, retirement, and 14-car backfill gates; progression
  audit passed save round-trip, `$400` gig payout, bailout, and victory. Two
  consecutive 1280×720 real-Metal visual audits passed (`mean_luma=96.45/96.36`,
  `crushed=0.00%`, `clipped=0.25%`, `red_blue=1.008/1.008`). The final color
  pass rerun was a true no-op (`changed_materials=0`, `map_changed=False`).
- **Status:** implementation complete and audit-green; all authored changes
  remain unstaged and uncommitted per the user's explicit instruction.
- **Next:** perform an interactive editor playtest and install Epic's optional
  UE 5.8 iOS component before the deferred iPhone device build/profile; commit
  code, assets, map, config, docs, and this ledger together only when authorized.
  Preserve pristine legacy avatar sources before any future rebake to avoid
  cumulative FBX round-tripping.
<!-- entry:living-city-humanization-complete -->

### 2026-07-19 · main · codex
- **Changed:** Started the approved living-city humanization pass covering new
  sprint/seated avatar clips and variants, visible vehicle occupants and
  carjacking, stronger pedestrian boundaries, a balanced daylight/color pass,
  and a native HUD plus repeatable earn/bankruptcy/victory loop.
- **Validation:** Mandatory Git/worktree preflight and cross-worktree sync check
  passed on a clean `main`; project, gameplay, and city design contracts were
  read before implementation. Asset probe, builds, audits, and visual QA remain
  pending.
- **Status:** in progress; all work will remain unstaged and uncommitted per the
  user's explicit instruction.
- **Next:** download only the two approved CC0 sources, probe the animation rig,
  validate retargeted walk/sit renders, then implement and audit each runtime
  layer in phase order.
<!-- entry:living-city-humanization-start -->

### 2026-07-18 · main · codex
- **Changed:** Tuned the iPhone experience for faster, smoother character and
  vehicle input; added analog drift filtering and response interpolation;
  improved on-foot, standard-car, and Mobile Office camera framing and lag;
  added `F` as a reliable enter/exit fallback; and introduced an iOS device
  profile plus lower-cost rendering defaults for a steadier 60 FPS target.
- **Validation:** The UE 5.8 `ConnectedSprawlEditor Mac Development` build
  passed (5 actions). The signed `ConnectedSprawl IOS Development` build passed
  (4 actions), including Xcode asset compilation, restored cooked-data sync,
  provisioning, signing, and bundle validation. Epic's missing optional
  MetalShaderConverter include and duplicate Vorbis-library messages remained
  non-blocking engine warnings.
- **Status:** done; validated mobile-control and performance tuning is published
  with this ledger entry.
- **Next:** install the newly compiled iOS executable over the validated cooked
  payload and perform an interactive touch-driving and frame-rate playtest.
<!-- entry:mobile-controls-performance -->

### 2026-07-18 · main · codex
- **Changed:** Published the complete UE 5.8, progression-save, living-city,
  real-character, animated-vehicle, enterable-parking, crash-recovery, and city
  boundary change set as implementation commit `36355b6`. Repointed `origin`
  from the prior Buckwick repository to the user-requested, previously empty
  `mcclements02/connectedsprawl` repository and initialized its `main` branch
  with the project's full existing history.
- **Validation:** The pre-publish staged diff contained 490 intended files
  (43 tracked updates and 447 new files), passed `git diff --cached --check`,
  contained no sensitive filenames, and had no new file above 4.4 MB. GitHub
  accepted `main`; it emitted only a size advisory for the already-versioned
  91.29 MB RealKit jacaranda asset, which remains below GitHub's 100 MB limit.
- **Status:** done; no active implementation work remains in this worktree.
- **Next:** run the optional interactive driving/crash playtest and install the
  UE 5.8 iOS component before signed-device build/performance validation.
<!-- entry:publish-connectedsprawl -->

### 2026-07-18 · main · codex
- **Changed:** Converted all 36 authored parked vehicles from decorative static
  meshes to stopped, enterable `ASprawlCar` pawns with real bodies and four
  animated wheels. Parking generation now validates the complete car footprint
  against buildings/props and other parking; the player starts beside an
  extra-clear bay. Added safe entry-speed/occupancy checks, AI suspension and
  resume around possession, obstruction-tested exit positions, four physical
  perimeter walls, last-safe-road out-of-bounds recovery, and a severe-crash
  grace window before self-righting. Tightened post-turn lane centering and
  extended map/runtime audits for enterability, orientation, bounds, wheel
  parts, and geometry overlaps.
- **Validation:** UE 5.8 Mac editor builds passed after both source batches;
  Python compilation passed for the two living-city scripts; the idempotent
  map application saved 1 player + 14 traffic + 36 enterable parked cars. The
  structural audit passed (`upright=51`, `enterable_parked=36`, `boundaries=4`,
  `max_lane_error=0.00`, `parking_offset=410.00`,
  `min_traffic_spacing=1850.68`, `player_car_distance=180.00`). The final
  38-second runtime audit passed (`cars=14`, `total_cars=51`,
  `enterable_cars=50`, `boundary_violators=0`, `upright_violators=0`,
  `moved=14`, `signal_stops=12`, `wheel_cars=14`,
  `max_box_occupancy=1`, `unauthorized_entries=0`, `min_spacing=285.2`,
  `lane_violators=0`, `pedestrians=26`, `real_avatars=26`).
- **Status:** implementation complete; changes remain unstaged and uncommitted
  with the existing UE 5.8/persistence/living-city work pending user
  authorization.
- **Next:** run an interactive editor driving/crash playtest and an iPhone
  performance profile after installing Epic's optional UE 5.8 iOS component;
  commit the complete in-flight change set together when authorized.
<!-- entry:enterable-parking-boundaries -->

### 2026-07-18 · main · codex
- **Changed:** Migrated the project association and targets to Unreal Engine
  5.8 (`BuildSettingsVersion.V7` and 5.8 include order), made AssetRegistry an
  explicit dependency, replaced the removed mobile virtual-texture CVar with
  an iOS platform override, and disabled the irrelevant Android file-server
  plugin so it no longer writes generated connection state. Added a versioned
  progression system that persists founder finances and ledger, faction state,
  resolved decision branches, map, player transform, and driving state;
  autosaves after decisions/day advances/background/clean quit; exposes F5/F9
  and Blueprint save/load/new-game APIs; rejects future schemas; prevents
  duplicate decision rewards; and resumes the saved mission branch at its next
  unresolved decision.
- **Validation:** The initial 5.8 editor build correctly failed on legacy V6
  shared-build settings; after migration, full editor builds passed (12 actions,
  then 16 actions after persistence) and the final incremental build passed.
  All 58 project Python scripts compiled. The progression audit passed
  (`round_trip=true`, `future_version_rejected=true`, `day=4`, `cash=6821`,
  `ledger=1`, `decisions=1`) and removed its temporary slots. The UE 5.8 saved
  map audit passed with 0 map errors/warnings (`cars=1+14+36`, `signals=30`,
  `crowd=26`, `max_lane_error=0.00`, `parking_offset=410.00`); the runtime
  audit passed (`cars=14`, `moved=14`, `signal_stops=10`, `wheel_cars=14`,
  `max_box_occupancy=1`, `unauthorized_entries=0`, `min_spacing=325.4`,
  `lane_violators=0`, `pedestrians=26`, `real_avatars=26`). The iOS target
  build could not start because this engine installation lacks Epic's optional
  iOS component.
- **Status:** implementation complete; all UE 5.8, persistence, and prior
  living-city changes remain unstaged and uncommitted pending user
  authorization.
- **Next:** install the UE 5.8 iOS optional component, then run a signed iPhone
  build/performance profile and an interactive editor playtest; commit the code,
  assets, map, configuration, documentation, and ledger together when
  authorized.
<!-- entry:ue58-persistence -->

### 2026-07-17 · main · codex
- **Changed:** Added sixteen complete imported pedestrian avatar sets and ten
  split animated vehicle models; made character fallback require a complete
  locomotion set; added speed-driven wheel spin and steering; implemented
  right-hand-lane routing, signal phases, intersection leases, queue and exit
  clearance, speed-aware amber behavior, and collision-safe spawning. Authored
  `TestMap` with 14 moving cars, 36 recessed parked cars, one player car, 30
  signals, and a 26-person crowd; added idempotent import/apply/audit scripts;
  synchronized legacy city scripts and cook paths; and cleared decorative
  collision that intruded into traffic or pedestrian space.
- **Validation:** `ConnectedSprawlEditor Mac Development` build passed (UHT + 7
  actions); modified Python scripts compiled; saved-map audit passed
  (`cars=1+14+36`, `signals=30`, `crowd=26`, `max_lane_error=0.00`,
  `parking_offset=410.00`, `min_traffic_spacing=1850.68`); 38-second runtime
  audit passed (`cars=14`, `moved=14`, `signal_stops=10`, `wheel_cars=14`,
  `max_box_occupancy=1`, `unauthorized_entries=0`, `min_spacing=326.0`,
  `lane_violators=0`, `pedestrians=26`, `real_avatars=26`).
- **Status:** implementation complete; all changes remain unstaged and
  uncommitted pending user authorization.
- **Next:** inspect the result interactively in the editor and profile on an
  iPhone target; commit the code, assets, map, documentation, and ledger
  together when authorized.
<!-- entry:living-traffic-characters -->

### 2026-07-17 · main · claude
- **Changed:** Merged `pre-5.8-upgrade` into `main` (fast-forward to 266c071)
  and pushed to origin. The branch was already contained in origin/main up to
  its merge base; the core-loop + city-pass commit was the only new work.
  Removed the pre-5.8-upgrade Active Work row per the merge rule — the local
  branch still exists but carries nothing unmerged.
- **Validation:** `git merge --ff-only` (no merge commit, no conflicts);
  `git rev-list origin/main..main` showed exactly 266c071 before push.
- **Status:** done.
- **Next:** future work continues on `main` (or fresh feature branches);
  see the previous entry's Next list for the open items.
<!-- entry:merge-to-main -->

### 2026-07-17 · pre-5.8-upgrade · claude
- **Changed:** Wired the DRIVE→DECIDE→EARN→BURN loop end-to-end: new
  `USprawlMissionDirector` world subsystem (indexes StrategicDecision assets,
  schedules the FirstVC opening call, auto-answers after the ring window,
  chains follow-ups via UnlocksDecisionId), new native `USprawlDecisionModal`
  (WidgetTree-built C++ UI, default when no WBP is assigned), five authored
  decision assets in Content/Missions/Decisions (FirstVC → board/favor →
  RailYardRun / BlockPartyGig bridge), and a delta-based
  `AdjustRecurringExpense` fixing DecisionSubsystem's burn double-count.
  Controller UI init is now BP-override-proof (BeginPlay + SetupInputComponent).
  Created the missing `M_SignalGlow`/`MI_RoadPaint` the C++ CDOs referenced and
  made those finders optional. Rebuilt `M_RealFacade2`/`M_RealGround` with
  complete linear defaults (Bricks085/Ground037) — the AOTex-on-sRGB-default
  mismatch was failing the whole material on Metal — and re-parented 3 MICs
  orphaned by the never-committed `M_Simple_Opaque`. Visuals (dw_city_v3):
  74 ring storefronts + awnings, 128 second-story window modules + 128
  roofline trims over every storefront, 25 hanging + 11 foldout signs, 134
  street trees, fountain plaza (5,5), playground (2,2), 2 mountain vistas.
  Clamped fountain 4K→1K and mountain 8K→2K/1K textures (edits live in the
  untracked Fab pack — re-run mobile_texture_budget.py after a pack refresh).
  README + .gitignore document the Downtown_West Fab dependency.
- **Validation:** `ConnectedSprawlEditor` built clean ×4; all python passes ran
  headlessly; offscreen 45s game run (90 frames dumped): 5 decisions indexed,
  FirstVC rang at 20s, auto-answered, modal presented and world paused (frame
  sizes plateau after frame 62); zero "Failed to compile Material", zero CDO
  missing-asset errors. Saved-map audit: all DW3 actor groups present.
  Caveat: `-dumpmovie` captures the scene before Slate composition, so
  HUD/modal don't appear in dumped PNGs — verified via logs + pause behavior;
  eyeball the UI in an interactive run.
- **Status:** done; committed on this branch.
- **Next:** (1) iPhone profile — an iPhone 15 Pro Max is paired
  (`xcrun devicectl list devices`); BuildCookRun will be a long first cook with
  the DW pack. (2) 16 pedestrian avatars under /Game/Pedestrians were never
  committed by the avatar PR — re-import (Tools/retarget_avatars.py) or accept
  mannequin fallback. (3) Phone-call UI (ring banner + answer/decline input)
  can now replace auto-answer via `bAutoAnswer=false`. (4) Save/Load of founder
  state (GDD Phase 2 remainder). (5) Headless runs MUST pass
  `-DisablePlugins=Fab` — the saved Fab browser tab asserts under nullrhi.
<!-- entry:core-loop-live -->

### 2026-07-14 · pre-5.8-upgrade · codex
- **Changed:** Made `ASprawlCar` discard rollover spin and self-right, taught both traffic generators to reserve the other fleet's positions, and regenerated 13 AI cars without changing the 28 drivable cars. The traffic application also refreshed the six existing car-paint material instances.
- **Validation:** Python compile and `git diff --check` passed; `ConnectedSprawlEditor` built successfully after correcting one UE 5.7 API call; a saved-map audit found 41 cars and zero pairs inside 600 cm; the game ran with active traffic for about 50 seconds and the player-area car was visually upright.
- **Status:** done; changes remain uncommitted.
- **Next:** Profile the stabilized traffic on an iPhone build and commit the source, map, paint assets, and ledger together when authorized. The null-RHI placement attempt failed because actor placement requires a viewport; the rendered retry saved successfully.
<!-- entry:upright-cars-complete -->

### 2026-07-14 · pre-5.8-upgrade · codex
- **Changed:** Hardening `ASprawlCar` upright physics and deconflicting the independently generated drivable and AI traffic fleets.
- **Validation:** In progress; map diagnostics found three cross-fleet overlaps, including a 90 cm pair inside full-size car hulls. Source FBX geometry and saved actor transforms are upright.
- **Status:** in progress.
- **Next:** Rebuild traffic, verify no overlaps, compile the editor, and inspect the running game.
<!-- entry:upright-cars -->

### 2026-07-14 · pre-5.8-upgrade · codex
- **Changed:** Added `Content/Python/rpg_city_realism.py`, applied 47 `City_RPG_*` actors to `TestMap`, and documented the developer research and implementation decisions.
- **Validation:** Python compile passed; Unreal dry run passed with Fab disabled; rendered application passed twice, with the second run clearing and recreating all 47 actors; the macOS editor build succeeded; the live game launched and the final player view was inspected.
- **Status:** done; changes remain uncommitted.
- **Next:** Resolve the pre-existing RealKit sampler and foliage dependency warnings, then profile or downsize the fountain's 4K textures before an iPhone build. Commit the script, map, notes, and ledger together when authorized.
<!-- entry:rpg-city-realism-complete -->

### 2026-07-14 · pre-5.8-upgrade · codex
- **Changed:** Adding an idempotent RPG city-realism pass and documenting the game-developer research behind its district, street-life, and lighting decisions.
- **Validation:** In progress; Python compile, Unreal dry run, rendered map application, build, and play screenshot remain.
- **Status:** in progress.
- **Next:** Validate assets, apply the pass to TestMap, build, and inspect the result from the player camera.
<!-- entry:rpg-city-realism -->

### 2026-07-14 · pre-5.8-upgrade · claude
- **Changed:** Added AI_WORKSPACE.md — a pointer routing to AGENTS.md (rules) and AI_HANDOFF.md (state); no rules duplicated.
- **Validation:** docs-only; no code paths touched. Verified file untracked and pre-existing worktree changes left untouched.
- **Status:** done.
- **Next:** commit AI_WORKSPACE.md alongside this repo's other in-flight work (docs-only — `git commit --no-verify` or the `skip-ledger` label).
<!-- entry:ai-workspace -->

### 2026-07-13 · main · claude
- **Changed:** Added CI ledger check (.github/workflows/ai-sync.yml + scripts/ai-sync-ci-check.sh); wired auto-install into Node package.json where present.
- **Validation:** hook + CI-check logic tested in isolation; workflow YAML linted.
- **Status:** done.
- **Next:** make "AI-SYNC ledger check" a required status check in branch protection.
<!-- entry:ci-deploy -->

### 2026-07-13 · main · claude
- **Changed:** Initialized AI sync ledger, AGENTS.md rule, `.githooks/pre-commit`, and `scripts/ai-sync-*`.
- **Validation:** scaffolding only — no code paths touched.
- **Status:** done.
- **Next:** run `bash scripts/ai-sync-install.sh` once per clone to enable the pre-commit reminder.
