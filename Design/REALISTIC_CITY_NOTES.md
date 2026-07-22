# Realistic City Environment - Research & Implementation Notes

**Project:** The Connected Sprawl
**Date:** 2026-05-31
**Goal:** Make pedestrians, vehicles, and world environment look realistic

---

## 2026-07-21 Swappable Athletic Shoes and Animated Foot Binding

- New `USprawlAthleticShoeModule` provides four stable, validated presets:
  Zarri Velocity, Metro Runner, Court High-Top, and Night Sprint. Each preset
  owns its shoe type, sock type, upper, sock, and accent colors, exposes a
  Blueprint swap call, and cycles predictably for character customization.
- `AthleticTrainers` adds a cushioned human-scale last, flexible low upper,
  twin lateral supports, heel clip, raised toe bumper, laces, collar, layered
  sole, and fitted socks. Zarri now receives the navy/teal Zarri Velocity pair;
  the same system works for assembled resident MetaHumans.
- Runtime fitting still measures each live foot/ball/calf chain before hiding
  the Body feet. Validation found that UE freezes a hidden MetaHuman foot socket
  while its visible calf continues to animate. The final follower therefore
  takes position from the independent calf/usable IK anchor and facing from the
  body transform, keeping both shoes under the ankles and level during motion.
  Swaps clear and rebuild the complete pair without retaining hidden state.

### Validation Status

- UE 5.8 `ConnectedSprawlEditor Mac Development` build passed. The complete
  `ConnectedSprawl.Characters` automation group passed 7/7, including focused
  athletic catalog, human-scale dimension, grounded-follow, wardrobe, crowd,
  character, and melee contracts.
- Real-Metal 1280x720 moving wardrobe audit passed at mean luma 105.81, 0.02%
  crushed, and 0.18% clipped. The live assembly resolved both Zarri sides at a
  14.55 cm heel-to-ball distance into 26.78 x 10.45 cm trainers; screenshot
  inspection confirmed both shoes stayed at the running ankles and horizontal.
- The first moving capture exposed detached shoes because the follower read the
  frozen hidden-foot transforms. A calf-only attachment then kept position but
  inherited an unrealistic vertical pitch. Both failures were corrected before
  this final build/audit. No cook, package, iPhone/device-profile run, physical
  input session, dedicated server/client session, on-device performance capture,
  or long traversal soak was run.

---

## 2026-07-21 Fitted MetaHuman Footwear Repair

- New `USprawlFootwearModule` replaces the wardrobe pass's coarse four-station
  shoe box with a reusable fitted presentation. It derives each side from the
  live foot, ball, and calf chain, clamps the result to human-scale dimensions,
  and builds rounded uppers, layered soles, toe lift, collars/laces, and socks.
- The MetaHuman Body owns the visible bare feet in current assemblies. Shoes
  therefore bind to an independent usable IK-foot or calf anchor before the
  underlying foot render bones are masked. This removes skin overlap without
  attaching geometry to a zero-scaled hidden bone; clear/reapply restores every
  masked bone and any separate Feet component.
- Five shoe types retain distinct width, collar, sole, and roughness profiles.
  Zarri now wears a readable navy low-top with a dark rubber sole and teal
  details. Presentation remains collision-free, navigation-free, non-ticking,
  decal-free, and shadow-free.

### Validation Status

- UE 5.8 `ConnectedSprawlEditor Mac Development` build passed after the final
  independent-anchor and cap-normal changes. The complete
  `ConnectedSprawl.Characters` automation group passed 6/6, including the new
  focused human-scale footwear contract.
- Real-Metal 1280x720 `-SprawlVisualAudit -SprawlAuditWardrobe` passed at mean
  luma 106.77, 0.02% crushed, and 0.16% clipped. Live audit telemetry resolved
  both Zarri foot-to-ball chains at 14.55 cm and produced a fitted 26.78 x 9.91
  cm pair; close inspection confirmed that the bare feet no longer render
  through the shoes.
- The first cap-normal visual run exposed an unsafe in-array append assertion;
  the source vertex is now copied before buffer growth, and the rebuilt visual
  audit passed. No cook, package, iPhone/device-profile run, physical input
  session, dedicated server/client session, on-device performance capture, or
  long walk/run animation soak was run.

---

## 2026-07-21 Deterministic MetaHuman Wardrobe Pass

- New `USprawlWardrobeModule` converts every character's broad wardrobe style
  into a complete deterministic outfit with top, bottom, optional outerwear,
  optional baseball cap/beanie, five shoe types, three sock lengths, and a
  coordinated eight-palette color direction. The outfit is embedded in the
  replicated human customization contract and exposed to Blueprint.
- Zarri, pedestrians, and strict Zarri/Amina/Andre crowd assemblies apply the
  same module after MetaHuman assembly and locomotion setup. Fitted authored
  garments are recolored; procedural footwear, socks, and hats bind to named
  bones. Shoes mask the underlying foot bones to prevent skin clipping and use
  absolute accessory scale so hidden-bone scale never collapses the model.
- Accessories carry no collision, overlaps, navigation, tick, decals, or
  shadows. Optional outerwear uses the fitted base-garment palette plus a small
  chest accent rather than a rigid arm overlay, preserving animation quality.
  Zarri has a fixed founder-streetwear preset; residents remain repeatable by
  seed while spanning hats/no hats, jackets/no jackets, and all footwear types.
- Project-owned wardrobe materials are authored at
  `/Game/Materials/M_WardrobeAccessory` and `M_WardrobeVertexColor`; their
  Python authoring source remains in
  `Content/Python/create_wardrobe_material.py`. `/Game/Materials` is explicitly
  cooked so the soft-loaded presentation remains available on iOS and Mac.

### Validation Status

- Final UE 5.8 `ConnectedSprawlEditor Mac Development` build passed. The full
  `ConnectedSprawl.Characters` automation group passed 5/5: CharacterDeveloper,
  CrowdMetaHuman, HumanCharacterModule, MeleeModule, and WardrobeModule.
- Real-Metal 1280x720 `-SprawlVisualAudit -SprawlAuditWardrobe` passed with a
  104.67 mean-luma frame, 0.02% crushed pixels, and 0.16% clipped pixels. The
  close capture verified the fitted garment palette, clean arms, masked bare
  feet, and bone-bound shoe/sock presentation.
- No cook, package, iPhone/device-profile run, physical touch session,
  dedicated server/client session, on-device performance capture, hat variant
  close-up, or long walk/run animation soak was run.

---

## 2026-07-21 Real Interior Furniture and Item Pass

- New `FSprawlInteriorPropLibrary` owns a reusable ten-type catalog of textured
  Downtown West meshes and deterministic placements for all three enterable
  destinations. It normalizes arbitrary source bounds to real-world target
  sizes, seats every pivot on its intended surface, preserves authored
  materials, and groups repeats into one HISM per resolved mesh.
- The running world now contains 77 real prop placements: display/dining
  tables, two chair variants, a bench, product cans and bottles, planters,
  foliage, recycle bins, and poster stands. The store is stocked; the office
  has workstations and meeting furniture; the condo has dining and household
  items.
- The previous large placeholder blocks were replaced with detailed modular
  fixtures: segmented checkout cabinetry, open store shelving, office storage,
  monitors and reception, plus a layered bed, upholstered sofa, lower kitchen
  cabinets, countertop, backsplash, sink, and cooktop. Those fixtures form a
  deterministic fallback if the optional marketplace meshes cannot load.
- Large props keep collision and shadows; small stock is non-colliding and
  shadow-free. The full system uses 169 instances, maintains the central portal
  route, and retains the existing iPhone-first cull distance and no-navigation
  policy.

### Validation Status

- Final UE 5.8 `ConnectedSprawlEditor Mac Development` build passed. Focused
  `ConnectedSprawl.World.InteriorPropLibrary` and
  `ConnectedSprawl.World.EnterableInteriorsLayout` automation passed 1/1 each;
  the catalog test confirmed all ten primary mesh packages are installed.
- Null-RHI `-SprawlInteriorsAudit` passed with 169 total instances, 77 real
  props, all 10 types resolved, three buildings, five map landmarks, successful
  store entry, containment, exterior map projection, exit route, map lifecycle,
  and one paused-safe M binding.
- Real-Metal 1280x720 store, office, and condo audits passed and were visually
  inspected. Their mean luma values were 143.12, 140.30, and 146.98 with 0.01%
  crushed and 0.00% clipped pixels in every frame. No cook, package,
  iPhone/device-profile run, physical touch session, on-device performance
  capture, or long traversal soak has been run.

---

## 2026-07-21 Enterable Destinations and City Map Pass

- New `ASprawlEnterableInteriors` creates three deterministic playable
  destinations: Junction Market, Founder House Offices, and Canal View Condos.
  Exterior portals sit on dry sidewalk frontage and attach lightweight signs,
  canopies, and doors without clearing existing city buildings.
- Each destination leads to its own fully enclosed runtime pocket with a safe
  spawn/exit pair and themed instanced furniture: retail counters and shelves,
  office reception/desks/meeting space, or condo living/sleeping/kitchen space.
  Five shared HISM groups render 60 pieces across all rooms; six text labels
  identify entrances and exits.
- Zarri's E/F context interaction now prioritizes a nearby building portal,
  then an enterable car. A 0.35-second transition guard prevents one press from
  immediately reversing the portal, and the touch HUD reports the same
  ENTER/EXIT action. Existing melee, phone, vehicle, and save transform paths
  remain intact.
- New `USprawlCityMapSubsystem` supplies one runtime and testable source for
  city projection, five landmarks, nearest-destination distance, and interior
  location resolution. `USprawlCityMapWidget` draws roads, the lake, landmark
  labels, and Zarri's live marker. M or MAP opens it paused; M, Escape, or the
  close control returns to play.

### Validation Status

- Final UE 5.8 `ConnectedSprawlEditor Mac Development` build passed. Focused
  `ConnectedSprawl.World.EnterableInteriorsLayout` and
  `ConnectedSprawl.UI.CityMapLayout` automation tests passed 1/1 each.
- The null-RHI `-SprawlInteriorsAudit` passed with three destinations, 60
  instanced pieces, five landmarks, a real store transition, interior
  containment, exterior-door map resolution, a safe exit route, successful
  map open/close, and exactly one paused-safe M binding.
- Real-Metal 1280x720 visual audits passed for the map (mean luma 51.93, 0.00%
  crushed, 0.34% clipped) and Junction Market interior (mean luma 131.31,
  0.01% crushed, 0.00% clipped). No cook, package, iPhone/device-profile run,
  physical touch session, long traversal soak, or on-device performance
  capture has been run.

---

## 2026-07-21 Reliable Punch/Kick Input Pass

- New `FSprawlMeleeInput` owns the direct on-foot keyboard and gamepad key
  contract. It binds keyboard X and the gamepad left face/X button to Zarri's
  possessed pawn input component, above the controller layer that Enhanced
  Input could shadow.
- Left mouse remains controller-owned so the first click safely recaptures a
  freed cursor; touch remains HUD/controller-owned. All four paths converge on
  the same alternating, replicated `USprawlMeleeModule` action.
- A successful action briefly reports PUNCH or KICK and emits a searchable
  `[MeleeInput]` runtime trace. Optional skeleton-compatible one-shots still
  override that fallback confirmation when available.
- UE 5.8 Editor build passed; the complete character automation suite passed
  4/4. `-SprawlMeleeInputAudit` passed against the possessed pawn with exactly
  one keyboard-X binding, one gamepad-X binding, Punch revision 1 active, and
  Kick queued next. Full-Metal gameplay launched with the on-foot melee HUD.

---

## 2026-07-21 Strict MetaHuman Ambient Crowd Pass

- New `FSprawlCrowdMetaHuman` replaces the ambient avatar/mannequin path with a
  strict roster of three project-owned Optimized/Low assemblies: Zarri, Amina,
  and Andre. Each roster entry points to a distinct face package; deterministic
  uniform stature and clothing palettes make reusable copies while preserving
  authored head proportions.
- `create_city_metahuman_residents.py` builds Amina and Andre from project-owned
  MetaHuman Character source assets, promotes staged output only after both
  assemblies validate, and saves only its owned folders. UE 5.8 TextureGraph
  baking requires the full Editor authoring process rather than commandlet mode.
- `AppearanceId` is part of the compact replicated human customization.
  Stand/Walk/Run use the shared MetaHuman locomotion preset; body, face, hair,
  and grooms move through one `ULODSyncComponent`. Population is server-owned
  even though the manager itself is a non-replicated level actor, and each
  rendering client independently selects its nearest high-detail residents.
- First-use spawning is deliberately limited to one resident per population
  pass so Amina and Andre cannot both synchronously load in one iPhone frame.
  Packaging always includes their Blueprint folders, while each Blueprint's
  hard references pull only the shared MetaHuman dependencies it actually uses
  instead of force-cooking the entire Common authoring tree.
- The balanced population cycle guarantees all three identities when at least
  three residents are live. Mac allows eight residents / three high-detail
  slots; iPhone allows three / one. Low-export LODs are selected relative to
  their actual count so a two-LOD resident never clamps near and far to the
  same lowest detail.
- Legacy `/Game/Pedestrians` content remains available to old tools and authored
  references but is neither loaded nor accepted as an ambient visual fallback.
  A failed MetaHuman assembly remains hidden and visible in logs/audits instead
  of producing a toy person.

### Validation Status

- UE 5.8 `ConnectedSprawlEditor Mac Development` compiled successfully after
  the final multiplayer and relative-LOD corrections. The complete
  `ConnectedSprawl.Characters` suite passed 4/4, including the strict roster,
  distinct face packages, deterministic copy styling, replication, actions,
  Character Developer, and melee contracts.
- The strict null-RHI TrafficAudit passed with 8/8 MetaHuman pedestrians, all
  three identities, zero non-MetaHumans, 14 moving traffic cars, 13 signal
  stops, 14 hidden drivers, and zero visibility, lane, wrong-way, boundary,
  upright, intersection, or pedestrian-offside violations.
- CarjackAudit passed with the ejected logical driver's requested roster ID
  matching its visible MetaHuman, no redundant intermediate activation, one
  legal garage backfill, and the 14-car traffic target restored.
- Full-Metal gameplay launched and the close pedestrian view resolved body,
  clothing, and hair correctly after the one-time shader warm-up. No cook,
  package, iPhone/device-profile run, on-device performance capture, dedicated
  server/client session, or long interactive crowd soak has been run.

---

## 2026-07-21 Parking Deck and Vehicle-Origin Pass

- New `ASprawlParkingGarage` replaces one central ordinary block lot at runtime
  with a four-level HISM deck: worn concrete structure, asphalt floors, three
  ramps, perimeter barriers, marked bays, parked-car silhouettes, warm lights,
  ventilation slats, and a facade parking sign. Site preparation clears only
  elevated authored lot/detail actors in the footprint and narrow driveway
  corridors; city ground, street, sidewalk, and thin service surfaces remain
  intact. One conflicting authored parked car moves into a real upper-deck bay
  instead of disappearing.
- Two deterministic covered exits begin behind opposite facades and terminate
  on exact right-hand lane centers with forward routes. Layout validation owns
  the building footprint, portal paths, road index, heading, lane center, and
  dry-route contract in one module shared by runtime and automation.
- `AProceduralTrafficManager` no longer samples a random point and spawns a car
  into an active lane. It checks the full garage route for world, vehicle, and
  pedestrian occupancy, emits at most one car per evaluation pass, and waits
  when both exits are blocked or the player is inside the reveal distance.
- `ASprawlCar` now has a bounded garage-egress phase. The hidden logical driver
  rolls through the driveway at low speed, yields to sensed obstructions, and
  seeds normal signal-aware lane AI only after the final swept merge succeeds.
  Garage-departing cars cannot be carjacked mid-merge.

### Validation Status

- UE 5.8 `ConnectedSprawlEditor Mac Development` compiled successfully. The
  focused `ConnectedSprawl.World.ParkingGarageLayout` automation test passed
  1/1, covering four decks, all detail groups, two unique off-road births,
  exact directed-lane merges, and viable dry routes.
- A live CarjackAudit passed after one replacement spawned inside the
  `NorthWestbound` portal, drove its complete low-speed egress, merged onto road
  3, and restored 14/14 active traffic cars with zero pending garage departures.
- The standalone TrafficAudit passed with 14 moving cars, 51 total / 37
  enterable cars, 12 signal stops, 359.5 cm minimum spacing, 26 pedestrians, 14
  hidden drivers, and zero lane, wrong-way, boundary, upright, visibility, or
  intersection violations.
- A 1600x900 real-Metal `-SprawlVisualAudit -SprawlAuditGarage` capture passed
  (mean luma 108.27, 0.01% crushed, 0.00% clipped, red/blue 1.127) and confirms
  the four-level silhouette, signed facade, covered opening, intact sidewalk,
  and aligned driveway. Cook, package, iPhone/device performance, and a long
  interactive traffic soak remain unrun.

---

## 2026-07-20 Zarri-Compatible Human Character Module

- New `USprawlHumanCharacterModule` provides one reusable character contract
  for Zarri, pedestrians, mission NPCs, and future Mass proxies. It generates
  deterministic age, presentation, build, complexion direction, hair texture,
  wardrobe, height, and lightweight fallback metadata without copying Zarri's
  face; only his joints-only rig/action vocabulary is shared.
- Its compact `FSprawlHumanRuntimeState` is component-replicated and exposes
  Stand, Walk, Run, Talk, Sit, and Drive to Blueprint. Explicit talk/sit/drive
  poses may be held for scripted interactions, while normal movement updates
  stand/walk/run automatically and fleeing safely interrupts a held pedestrian
  pose.
- The locomotion visual now accepts a looping action override. Imported avatars
  use the existing Talk and Sit clips (Drive falls back to Sit); named
  `USprawlCharacterDefinition` assets can soft-bind dedicated MetaHuman talk,
  sit, and drive clips without synchronously loading them into the ordinary
  crowd.
- Zarri and every pedestrian own the new module. Zarri reports logical Drive
  while possessed into a car, but the established hidden-driver policy still
  suppresses the in-cabin body render; traffic-car mounted meshes remain
  unloaded and unticked. Lightweight avatars and the mannequin remain safe
  fallbacks, preserving the iPhone-first budget.

### Automated Acceptance

- Final `ConnectedSprawlEditor Mac Development` build passed.
- `ConnectedSprawl.Characters` passed 2/2 automation tests: the existing
  Character Developer checks and the new Human Character module's deterministic
  diversity, validation, six-action, held-pose, replication-default, revision,
  and MetaHuman/fallback animation assertions.
- The final 30-second null-RHI TrafficAudit passed with 14/14 traffic cars
  moving, 13 signal stops, 344.6 cm minimum spacing, 26 pedestrians, 26 real
  avatars, 14 hidden drivers, and zero driver-visibility, pedestrian-offside,
  lane, wrong-way, boundary, upright, or intersection violations.
- Cook, package, iPhone device-profile, and on-device performance validation
  remain outstanding. Additional unique assembled MetaHuman assets still need
  Creator review/assembly before assignment to named character definitions;
  the ordinary crowd intentionally keeps its mobile avatar tier.

---

## 2026-07-20 Sidewalk Sign and Intersection Signal Pass

- `FSprawlKerbPlacement` now defines a strict sidewalk band rather than merely
  checking whether a prop has left the live lane. Foldout/A-board signs are
  re-seated in that band at runtime, so they cannot remain in parking bays,
  roadway asphalt, or an intersection apron.
- `ASprawlTrafficLight` is now a complete low-poly junction model: four
  sidewalk-corner poles with distinct red, amber, and green lenses for both
  traffic axes. `ASprawlStreetDressing` re-centres legacy signal actors and
  fills any missing dry intersection signal without changing the authoritative
  `USprawlCityGridSubsystem` phase, car stopping logic, or mobile tick budget.
- The city authoring passes now place the signal actor at the intersection
  centre; the actor itself lays out the visible poles onto the sidewalk.

---

## 2026-07-20 Hidden In-Car Driver Pass

- New `FSprawlDriverVisibility` separates logical occupancy from rendering.
  AI and player drivers still occupy the seat for access, possession,
  carjacking, deterministic identity, and save behavior, but the legacy
  car-mounted skeletal component is forcibly hidden, cleared, and unticked.
- `ASprawlCar` applies that policy before the first rendered frame and at every
  enter/exit/restore transition. Traffic drivers no longer synchronously load
  avatar meshes or seated animations; an ejected driver still resolves the
  same identity as a normal exterior `ASprawlPedestrian`.
- `AProceduralTrafficManager` now audits the intended contract directly:
  logically occupied traffic must report hidden drivers, any visible mounted
  driver is a violation, and the carjack setup selects a hidden logical driver
  instead of depending on a rendered mesh.

### Automated Acceptance

- `ConnectedSprawlEditor Mac Development` compiled successfully.
- `ConnectedSprawl.Vehicles.DriverVisibility` passed all empty/seated logical
  occupancy, render, avatar-load, and pose-tick assertions.
- The 30-second standalone TrafficAudit passed with 14/14 traffic cars moving,
  13 signal stops, 368.0 cm minimum spacing, `hidden_drivers=14`,
  `visible_driver_violations=0`, `missing_hidden_drivers=0`, 26 real pedestrian
  avatars, and zero lane, wrong-way, boundary, upright, intersection, or
  pedestrian-offside violations.
- The broader vehicle-suite and separate CarjackAudit retries were blocked
  before engine initialization by macOS LaunchServices connection failures.
  No failure came from game code; cook, package, iPhone, and sustained device
  performance validation remain outstanding.

---

## 2026-07-20 Character Developer and District Population Pass

- `USprawlCharacterDeveloper` now turns a seed, world location, and live city
  hour into a validated `FSprawlCharacterProfile`: stable identity, home/work
  district, civilian role, current activity, destination, schedule, wardrobe,
  stature, gait, crossing tendency, phone-idle probability, lightweight avatar,
  MetaHuman Creator brief, and reference-sheet prompt.
- `USprawlCharacterDefinition` is the Blueprint/Data Asset contract for named
  cast members. It can soft-bind an **Optimized/Low** assembled MetaHuman and
  locomotion clips without forcing full MetaHumans into the ordinary crowd or
  synchronously loading them on iPhone.
- The crowd manager develops profiles before `BeginPlay`, keeps the existing 26
  actor ceiling, and applies place/time density: The Junction remains busiest;
  Iron Forest and Rail Yards are quieter; every district thins naturally after
  midnight. The existing sidewalk, signal, flee, avatar-completeness, and
  mannequin-fallback contracts are unchanged.
- `Tools/CharacterDeveloper/character_developer.py` optionally drafts richer
  JSON through Hugging Face's inference router using `Qwen/Qwen3.5-4B`; the
  reference prompt records the Apache-2.0 `FLUX.1-schnell` model. It is a
  token-from-environment, editor-only workflow with deterministic `--offline`
  mode, strict schema/range checks, and no game dependency.
- Adding the new translation unit exposed a committed anonymous-namespace
  unity collision between city-gate and skyline bay constants. The gate names
  are now unique; their values and layout behavior did not change.

### Automated Acceptance

- `ConnectedSprawlEditor Mac Development` compiled successfully.
- `ConnectedSprawl.Characters.CharacterDeveloper` passed all district,
  determinism, 128-profile validation, mobile-brief, and density assertions.
- The 30-second headless TrafficAudit passed with 14/14 traffic cars moving and
  animating wheels, 13 signal stops, 344.6 minimum spacing, 26 pedestrians, 26
  real avatars, zero pedestrian off-side violations, and zero lane, wrong-way,
  boundary, upright, or intersection-lease violations.
- The standalone Python suite passed 3/3 tests, and an offline Rail Yards
  profile generated and validated without network access.

---

## 2026-07-19 Living-City Humanization and Authentic Daylight

- The avatar library now carries 22 lightweight variants and seven clips per
  variant (`Idle`, `Walk`, `Jog`, `Talk`, `WalkFormal`, `Sprint`, `Sit`). The
  player uses a dedicated young Black male Zarri avatar and a real sprint band,
  and moving traffic lazily creates a seated,
  cullable driver with no collision or shadow cost. That mounted visual was
  superseded by the 2026-07-20 hidden logical-driver policy. Parked cars remain
  empty.
- Slow occupied traffic can be carjacked: the driver is ejected at a clear
  sidewalk-side point, keeps the same visual identity, flees, and rejoins the
  crowd manager while the stolen car leaves the autonomous traffic pool.
- Pedestrians wait for the signal phase parallel to their crossing, require
  enough green time to finish, abandon a blocked curb after 12 seconds, and
  recover to a sidewalk corner if ordinary walking drifts onto a road. Flee
  paths turn back toward the current block instead of lingering in traffic.
- The native HUD, repeatable clean-income city gigs, one-time dirty bailout,
  second-bankruptcy loss, and `$25,000` victory checkpoint close the playable
  earn → burn → win/lose loop without requiring Widget Blueprint bindings or a
  new save schema.

`Content/Python/city_color_pass.py` is the canonical final look writer and must
run after the RealKit and city authoring passes. Its authored daytime target is:

- directional sun: intensity `5.2`, color `(1.00, 0.86, 0.70)`, rotation
  `pitch=-32`, `yaw=-38`;
- captured skylight: intensity `1.85` with one final recapture;
- fixed mobile exposure bias `0.70`, white temperature `6200 K`, subtle
  warm-neutral scene tint `(1.06,1.00,0.94)`, AO `0.50`, vignette `0.15`,
  saturation `1.02`, and contrast `1.03`;
- height fog density `0.006`; and
- facade palette: brick `(0.45,0.26,0.20)`, tan `(0.58,0.47,0.36)`, warm grey
  `(0.48,0.46,0.43)`, cream `(0.64,0.58,0.48)`, with asphalt
  `(0.16,0.16,0.17)`, sidewalk `(0.42,0.42,0.40)`, and `RoughnessMul=0.90`.

The matching runtime environment controller starts at `08:04` game time, which
reproduces the authored `-32°` sun elevation before continuing the day/night
cycle. It pools at most 22 shadowless streetlights and never adds per-lamp tick
or collision work, preserving the iPhone-first budget.

The final real-RHI 1280×720 audit with Zarri and exposure bias `0.70` measured
mean Rec.709 luma `96.05`, crushed pixels `0.00%`, clipped pixels `0.26%`, and
red:blue `1.007`.

---

## 2026-07-18 Enterable Parking and Vehicle Safety Pass

- Parking authoring now tests each full 4.7-metre vehicle footprint against
  blocking building and prop bounds. The saved map contains 36 non-overlapping
  curbside `ASprawlCar` pawns instead of decorative static-mesh cars, and Zarri
  starts 180 units from the nearest extra-clear player bay.
- A stopped, unoccupied car can be entered. Possession suspends its autonomous
  route and intersection lease; exiting restores prior traffic behavior when
  applicable and chooses the first unobstructed driver, passenger, front, or
  rear position.
- Four invisible blocking walls define the prototype city perimeter. A
  last-safe-road transform recovers a car only if it tunnels through a wall or
  falls out of the world.
- Normal driving is stabilized upright. A high-speed lateral collision receives
  a 2.25-second physical reaction window so crashes can roll naturally before
  the self-righting safeguard takes over.
- Post-turn lane lookahead was tightened so traffic settles into its correct
  right-hand lane before the straight-road audit envelope.

### Automated Acceptance

- UE 5.8 `ConnectedSprawlEditor Mac Development` compiled successfully.
- The saved-map audit passed with 51 upright, in-bounds vehicles; 36 enterable
  parked cars; four perimeter walls; zero building/prop or parked-car overlaps;
  0.00 maximum authored lane error; 410-unit parking offset; 1850.68 minimum
  traffic spacing; and a 180-unit player-to-car distance.
- The 38-second runtime audit passed with 51 total cars, 50 simultaneously
  enterable cars at peak, zero boundary/upright violations, all 14 traffic cars
  moving with wheel animation, 12 signal stops, one-car maximum intersection
  occupancy, zero unauthorized entries, 285.2 minimum spacing, and zero
  persistent lane violators.

---

## 2026-07-17 Living Traffic and Character Pass

This pass replaces the placeholder street population with a mobile-conscious,
functional city layer:

- Sixteen distinct pedestrians now use imported character meshes with complete
  idle, walk, jog, talk, and formal-walk animation sets. Runtime selection only
  uses an imported avatar when its required mesh and locomotion clips are all
  present, preserving the existing fallback for incomplete content.
- Ten split vehicle models provide a body, detail mesh, and four independently
  animated wheels. Wheel spin follows true road speed and the front wheels steer
  with the car instead of remaining static.
- Traffic follows right-hand lanes, queues behind other vehicles, respects a
  green/amber/all-red signal cycle, reserves intersections before entering,
  checks that the far side is clear, and releases stale reservations. A
  speed-aware stopping-distance rule lets a car already committed to a late
  amber proceed instead of braking unrealistically inside the junction.
- Parked cars use recessed curb bays at a 410-unit road-center offset. Runtime
  spawning rejects overlaps with traffic, parked vehicles, pedestrians, and
  solid world geometry so a replenished car cannot appear in an occupied lane.
- Pedestrian routes use a 360-unit sidewalk inset and validate clearance before
  accepting a destination, keeping crowds out of traffic and solid props.
- The authored test map now contains 14 moving cars, 36 legally recessed parked
  cars, one player car, 30 working signals, and a target crowd of 26. Decorative
  collision that reached into travel lanes was removed or moved inward without
  changing the visible city composition.

The implementation retains the lightweight kinematic actor model instead of
moving traffic to Chaos Vehicles, preserving the project's iPhone-first actor
and pooling budget. The map authoring and audit scripts are idempotent so this
layout can be rebuilt and checked after future city edits.

### Automated Acceptance

- The saved-map structural audit passed with 14 traffic cars, 36 parked cars,
  30 signals, 26 pedestrians, zero lane-position error, a 410-unit parking
  offset, 1850.68 minimum authored traffic spacing, and the player car 623.70
  units from the nearest travel lane.
- A 38-second headless runtime audit passed with all 14 cars moving, all 14
  showing wheel motion, 10 observed signal stops, at most one car per
  intersection box, zero unauthorized intersection entries, 326.0 minimum live
  spacing, zero persistent lane violators, and all 26 pedestrians using real
  avatars.

---

## 2026-07-14 RPG Developer-Informed Pass

This pass uses guidance from developers who shipped open-world games, rather
than treating realism as a polygon-count problem:

- Sucker Punch's *inFamous* GDC talk describes standardized building
  footprints, tiling, and layout shortcuts as the way a small art team produced
  a dense city. The takeaway for this prototype is to preserve the repeatable
  grid and spend authored detail at player height. ([GDC Vault](https://www.gdcvault.com/play/1012233/Building-an-Open-World-Game))
- Ubisoft's World Logic Designer describes believable worlds as linked layers
  of transport, work, ecology, history, and local activity. Each Sprawl district
  therefore needs visible evidence of who uses it and why. ([Ubisoft](https://www.ubisoft.com/en-us/company/about-us/our-people/articles/he-works-where-you-play-world-logic-designer))
- The *Watch Dogs: Legion* team connected generated residents to roles,
  relationships, and schedules. The lightweight equivalent here is to cluster
  pedestrians around plausible destinations instead of distributing every NPC
  uniformly. ([Ubisoft](https://news.ubisoft.com/en-ca/article/4po3S9Pwp1YcgBmGPmQxAh/watch-dogs-legion-the-tools-that-built-london))
- CD Projekt Red makes Night City's districts distinct through architecture,
  fashion, history, verticality, and the marks residents leave behind. The
  Junction, Iron Forest, and Rail Yards should be recognizable from their prop
  language before the HUD names them. ([CD Projekt Red](https://cdn-s-cyberpunk.cdprojektred.com/CP2077-UE-Booklet-EN-1.pdf))
- Arkane/LucasArts' environmental-storytelling talk identifies props,
  texturing, lighting, and composition as tools for implying history. Props are
  placed as small cause-and-effect scenes, not as uniform decoration.
  ([GDC Vault](https://www.gdcvault.com/play/1012696/What-Happened-Here-Environmental))

### Implemented Direction

`Content/Python/rpg_city_realism.py` is an additive, idempotent pass that:

- concentrates varied storefront signs, a food cart, cafe seating, and five
  additional pedestrians around The Junction's player-start corridor;
- gives Iron Forest an ordered civic landmark and planters, while Rail Yards
  receives a reused fence-and-tarp service edge;
- adds two civic anchors so the grid has memorable navigation points;
- tunes the existing sun, skylight, fog, and post-process actors toward a warm
  late-afternoon look inspired by the supplied open-world references; and
- limits the expensive lighting addition to four shadowless local lights near
  the focal street, preserving the iPhone-first rendering strategy.

The pass clears only `City_RPG_*` actors and reuses assets already present in
the project. It does not rebuild the base city or alter engine configuration.

---

## 📋 Current State Analysis

### Existing Systems
- **Pedestrians** (`SprawlPedestrian.cpp`): Basic wandering AI with simple collision avoidance
  - Uses UE4 Mannequin mesh
  - Random direction changes every 3-7 seconds
  - Simple stuck detection
  - RVO avoidance enabled

- **Vehicles** (`SprawlCar.cpp`): Physics-based car with basic geometry
  - Kitbashed cube/cylinder primitives for body
  - Static wheel meshes (no rotation animation)
  - Physics hull with locked X/Y rotation
  - Auto-drive AI for traffic

- **World**: Basic materials for buildings, roads, grass
  - Simple material instances
  - No foliage system
  - Basic procedural traffic spawning

---

## 🎯 Improvement Goals

### 1. Realistic Pedestrians
- [ ] Replace UE4 Mannequin with diverse, realistic characters
- [ ] Implement natural walking animations
- [ ] Add variety in clothing, body types, ages
- [ ] Improve AI behaviors (crossing streets, waiting at lights, interacting)
- [ ] Add ambient animations (phone usage, conversations, sitting)

### 2. Realistic Vehicles
- [ ] Replace primitive geometry with detailed car models
- [ ] Implement animated wheel rotation
- [ ] Add suspension animation
- [ ] Improve vehicle variety (sedans, SUVs, trucks)
- [ ] Add realistic headlights/taillights
- [ ] Implement brake lights and turn signals

### 3. Realistic World Environment
- [ ] Add detailed building facades with proper LODs
- [ ] Implement foliage system (grass, trees, bushes)
- [ ] Add urban props (trash cans, benches, signs, lights)
- [ ] Improve road materials with wear and markings
- [ ] Add ambient life (birds, wind effects)

---

## 🔍 Reference Projects & Resources

### Open Source GTA-Style Games

#### **San Andreas Unity**
- **Repo:** https://github.com/in0finite/SanAndreasUnity
- **Engine:** Unity
- **Features:**
  - Full vehicle physics with wheel animation
  - Pedestrian AI with navigation
  - City traffic system
  - Mission framework
- **Key Learnings:**
  - Modular traffic spawning around player
  - NavMesh-based pedestrian movement
  - Vehicle pooling for performance

#### **GTA Clone Prototype**
- **Repo:** https://github.com/akshayyrathore/Gta-clone-on-
- **Engine:** Unity/Unreal flexible
- **Features:**
  - Third-person controller
  - Vehicle enter/exit system
  - Basic AI and missions
- **Key Learnings:**
  - Simple but effective character switching
  - Basic traffic patterns

#### **Gorka Games UE5 GTA Tutorial**
- **Source:** YouTube tutorial series with free project files
- **Engine:** Unreal Engine 5
- **Features:**
  - Full city environment
  - Moving pedestrians and cars
  - Police AI and wanted system
  - Phone apps and parkour
- **Key Learnings:**
  - UE5-specific implementations
  - Modern BP patterns for open world

---

## 🚶 Pedestrian System Improvements

### Recommended Approach: Mass Entity Framework

**Mass AI** (UE5 native) is designed for large-scale crowd simulation:
- Data-oriented architecture
- Can handle 10,000+ agents
- Efficient LOD system
- Integrated with UE5 animation system

### Implementation Steps

1. **Character Assets**
   - **MetaHuman Creator** for high-quality, diverse characters
   - Alternative: Mixamo characters for lighter weight
   - Need 8-12 base variations (age, gender, ethnicity, style)

2. **Animation System**
   - **Motion Capture Data:**
     - Mixamo: Free mocap library (walking, idle, talking)
     - Rokoko: Real-time mocap (if budget allows)
     - Move.ai: AI-powered mocap from video
   - **Animation Blueprint:**
     - Blend space for walk speeds
     - State machine: Idle → Walk → Run → Special actions
     - IK for feet placement on uneven terrain

3. **AI Behaviors** (Mass AI Processors)
   ```cpp
   // Suggested Mass processors:
   - MassPedestrianWalkProcessor: Path following
   - MassPedestrianAvoidanceProcessor: RVO collision
   - MassPedestrianLODProcessor: Switch detail levels
   - MassPedestrianReactionProcessor: React to player/vehicles
   ```

4. **Performance Optimization**
   - **LOD System:**
     - LOD 0 (< 10m): Full animation, IK, facial
     - LOD 1 (10-30m): Simplified animation
     - LOD 2 (30-100m): Animated imposters
     - LOD 3 (> 100m): Static billboards
   - **Culling:** Only animate visible agents
   - **Pooling:** Reuse actors beyond spawn radius

### Current Code Migration Path

**From** `SprawlPedestrian.cpp`:
```cpp
// Current simple system
void Tick(float DeltaSeconds) {
    AddMovementInput(WanderDir, 1.f);
    // Simple direction changes
}
```

**To** Mass Entity approach:
```cpp
// Mass processor handles bulk updates
UMassPedestrianProcessor::Execute(FMassEntityManager& EntityManager,
                                   FMassExecutionContext& Context)
{
    // Process thousands of entities efficiently
    // Use entity fragments for position, velocity, state
}
```

---

## 🚗 Vehicle System Improvements

### Wheel Animation System

**Problem:** Current wheels are static meshes with no rotation

**Solution:** Chaos Vehicle Plugin + Animation Blueprint

#### Implementation Steps

1. **Enable Chaos Vehicles Plugin**
   - Edit → Plugins → Enable "Chaos Vehicles"

2. **Vehicle Setup**
   ```cpp
   // Replace ASprawlCar inheritance
   class ASprawlCar : public AWheeledVehiclePawn // Instead of APawn
   {
       // Add UChaosWheeledVehicleMovementComponent
       UPROPERTY()
       UChaosWheeledVehicleMovementComponent* VehicleMovement;
   };
   ```

3. **Wheel Blueprints**
   - Create `BP_SprawlWheelFront` and `BP_SprawlWheelRear`
   - Configure:
     - Radius: 41cm (matches current scale)
     - Friction: 3.0 (asphalt)
     - Suspension: Natural frequency 8Hz, damping 0.7

4. **Animation Blueprint for Wheels**
   ```
   AnimGraph:
   - WheelHandler node (automatic rotation based on physics)
   - Connects wheel bones to actual physics simulation
   ```

5. **Skeletal Mesh Requirements**
   - Vehicle mesh needs bones for each wheel
   - Bones named: `Wheel_FL`, `Wheel_FR`, `Wheel_RL`, `Wheel_RR`
   - Suspension bones optional but recommended

### Vehicle Assets

**Recommended Sources:**
- **Unreal Marketplace:**
  - "City Vehicles Pack" (realistic low-poly cars)
  - "Modular Street Vehicles" (customizable)
- **Quixel Megascans:**
  - High-quality vehicle scans (if performance allows)
- **Free Resources:**
  - Sketchfab (CC licensed models)
  - TurboSquid free section

**Specifications for Mobile (iPhone target):**
- Triangles: 5,000-15,000 per vehicle
- LOD0: Full detail for player's car
- LOD1: 50% tri count for nearby traffic
- LOD2: 25% tri count for distant traffic
- Materials: 2K textures max, 1K for traffic

### Physics Tuning

```cpp
// Realistic sedan values (in SprawlCar.cpp)
Mass: 1500kg                    // Current: 1500kg ✓
Engine Torque: 400 Nm @ 5000rpm
Max RPM: 6500
Drag Coefficient: 0.3           // Aerodynamic
Rolling Resistance: 0.015
Steering Angle: 35° (front wheels)
```

---

## 🏙️ World Environment Improvements

### Building System

**Current:** Simple geometry with basic materials
**Goal:** Detailed, varied, performant urban environment

#### Nanite for Buildings (UE5 Feature)

Nanite allows millions of polygons with no performance cost:
- Import high-detail building models
- Enable "Nanite" on static mesh
- No manual LOD creation needed
- Perfect for hero buildings in The Junction

**Implementation:**
1. Create/acquire detailed building meshes
2. Import to UE5
3. Enable Nanite in static mesh settings
4. Place in level - Nanite handles streaming/LOD

#### Building Variety System

```cpp
// Procedural building placement
class AZonalVisualsManager : public AActor
{
    // Current system can be extended:
    UPROPERTY()
    TArray<UStaticMesh*> JunctionBuildings;    // High-density

    UPROPERTY()
    TArray<UStaticMesh*> SuburbanBuildings;    // Low-rise

    UPROPERTY()
    TArray<UStaticMesh*> IndustrialBuildings;  // Warehouses

    void SpawnBuildingsForZone(FString ZoneName);
};
```

**Building Asset Packs (Unreal Marketplace):**
- "City Downtown Pack" - Skyscrapers and offices
- "Modular Neighborhood" - Residential buildings
- "Industrial Environment" - Warehouses for Rail Yards
- "Urban Props" - Street furniture and details

### Foliage System

**Unreal Foliage Tool** for grass, trees, shrubs:

1. **Asset Sources:**
   - **Quixel Megascans:** Photo-real vegetation (FREE with UE5)
     - Search: "grass", "tree", "bush", "shrub"
     - Types: Oak, Maple, Pine for urban areas
   - **SpeedTree:** Procedural trees with wind animation

2. **Foliage Types:**
   - **The Junction:** Street trees (London Plane, Oak)
   - **Parks:** Grass, flowers, benches
   - **Rail Yards:** Weeds, sparse vegetation
   - **Iron Forest:** Manicured lawns, ornamental trees

3. **Implementation:**
   ```
   Tools → Foliage → Add Foliage Type
   - Select mesh
   - Configure density (instances per 1000cm²)
   - Set scale variation (0.8-1.2)
   - Enable rotation variation
   - Paint onto landscape
   ```

4. **Performance:**
   - Use Hierarchical Instanced Static Meshes (HISM)
   - Cull distance: 0-5000cm based on detail level
   - Max draw distance for grass: 3000cm
   - Trees: 10000cm

### Lighting: Lumen Global Illumination

**Enable in Project Settings:**
- Rendering → Dynamic Global Illumination: Lumen
- Rendering → Reflections: Lumen

**Benefits:**
- Real-time bounce lighting
- Realistic ambient occlusion
- No need to bake lightmaps
- Perfect for day/night cycle

**Setup:**
```
1. Add Directional Light (Sun)
   - Intensity: 10 lux (day) / 0.1 lux (night)
   - Temperature: 5500K (day) / 2800K (sunset)

2. Add Sky Atmosphere
   - Automatic realistic sky

3. Add Exponential Height Fog
   - Fog Density: 0.02
   - Fog Height Falloff: 0.2

4. Post Process Volume
   - Unbound: True
   - Exposure: Auto
   - Bloom: Enabled
```

### Material System

**PBR Materials** for realism:

```cpp
// Material master for city surfaces
M_CityMaster:
- Base Color: RGB texture
- Normal: Normal map
- Roughness: 0.4-0.8 (varies by surface)
- Metallic: 0.0 (most surfaces) / 1.0 (metal)
- AO: Ambient occlusion map

// Material instances:
MI_Asphalt:    Roughness 0.8, subtle tire marks
MI_Concrete:   Roughness 0.6, weathering
MI_Brick:      Roughness 0.7, worn edges
MI_Glass:      Roughness 0.1, Metallic 0.3, opacity
```

**Texture Sources:**
- Quixel Megascans (free high-quality PBR)
- Polyhaven (free CC0 textures)
- Substance Source (subscription)

---

## 📊 Performance Targets (iPhone 13+)

### Mobile Optimization Strategy

**Current:** Forward rendering, Mobile HDR, static lighting
**Goal:** Maintain 60fps with realistic assets

#### Budget Breakdown

| System | Budget | Notes |
|--------|--------|-------|
| Pedestrians | 100-200 visible | Mass AI LOD system |
| Vehicles | 20-40 visible | Physics vehicles |
| Draw Calls | < 2000 | Instancing, Nanite |
| Triangles | 2-3M on screen | Nanite for buildings |
| Texture Memory | 500MB | Streaming, compression |

#### Optimization Techniques

1. **Level Streaming**
   ```cpp
   // Current ZonalStreamingManager extended:
   - Stream 1-mile radius (current) ✓
   - Add aggressive unloading beyond 1.5 miles
   - Preload along velocity vector (highway streaming)
   ```

2. **Asset LODs**
   - Buildings: Nanite (automatic)
   - Vehicles: 3 LODs (15k → 7k → 2k tris)
   - Pedestrians: 4 LODs including imposters
   - Props: 2-3 LODs

3. **Texture Streaming**
   - Enable Virtual Textures for terrain
   - Compress with ASTC (mobile)
   - Mip streaming budget: 200MB

4. **Rendering**
   - Keep Forward rendering ✓
   - Use Mobile HDR ✓
   - Limit dynamic lights: 3 max per object
   - Shadow distance: 2000cm

---

## 🛠️ Implementation Phases

### Phase 1: Pedestrians (1-2 agents)
**Tasks:**
- [ ] Research and acquire 3-5 MetaHuman characters
- [ ] Set up basic Mass Entity system
- [ ] Implement walk animation blend space
- [ ] Create pedestrian spawner with Mass
- [ ] Add simple avoidance behaviors
- [ ] Test performance with 100+ pedestrians

**Files to modify:**
- `SprawlPedestrian.cpp/.h` → Migrate to Mass Entity fragments
- Create `MassPedestrianProcessors.cpp`
- Create Animation Blueprint for pedestrians

### Phase 2: Vehicles (1-2 agents)
**Tasks:**
- [ ] Enable Chaos Vehicles plugin
- [ ] Acquire 3-5 vehicle meshes with wheel bones
- [ ] Create wheel blueprints (front/rear)
- [ ] Set up vehicle animation blueprint
- [ ] Refactor SprawlCar to use Chaos Vehicle
- [ ] Implement wheel rotation animation
- [ ] Add suspension visual feedback
- [ ] Test physics and handling

**Files to modify:**
- `SprawlCar.cpp/.h` → Inherit from AWheeledVehiclePawn
- `MobileOfficeVehicle.cpp/.h` → Update for new base class
- Create `BP_VehicleWheel_Front/Rear`
- Create `ABP_VehicleAnimation`

### Phase 3: World Environment (1-2 agents)
**Tasks:**
- [ ] Acquire building asset packs
- [ ] Enable Nanite on hero buildings
- [ ] Set up modular building placement system
- [ ] Integrate Quixel Megascans vegetation
- [ ] Configure foliage system with grass/trees
- [ ] Set up Lumen lighting
- [ ] Create material library (roads, buildings, nature)
- [ ] Add urban props (street lights, signs, benches)
- [ ] Optimize for mobile (LODs, culling)

**Files to modify:**
- `ZonalVisualsManager.cpp/.h` → Add building/foliage spawning
- Create material instances in Content/CityArt/
- Set up foliage types in Content/Foliage/

---

## 📚 Learning Resources

### Documentation
- [UE5 Mass Entity System](https://docs.unrealengine.com/5.0/en-US/mass-entity-in-unreal-engine/)
- [Chaos Vehicles](https://docs.unrealengine.com/5.0/en-US/chaos-vehicles-overview-in-unreal-engine/)
- [Nanite](https://docs.unrealengine.com/5.0/en-US/nanite-virtualized-geometry-in-unreal-engine/)
- [Lumen](https://docs.unrealengine.com/5.0/en-US/lumen-global-illumination-and-reflections-in-unreal-engine/)

### Sample Projects
- **City Sample** (Epic Games) - Free UE5 demo showcasing:
  - Mass Entity crowds
  - Nanite buildings
  - Lumen lighting
  - Vehicle traffic
  - Available: Unreal Marketplace

### Video Tutorials
- "UE5 Mass AI Tutorial" (YouTube)
- "Chaos Vehicle Setup" (YouTube)
- "Creating a Complete GTA Game in UE5" (Gorka Games)
- "Nanite Best Practices" (Unreal Engine channel)

---

## 🔄 Migration Strategy

### Gradual Upgrade Path

**Advantage:** Don't break existing gameplay systems

1. **Add alongside current systems**
   - New Mass pedestrians spawn separately from old SprawlPedestrians
   - New Chaos vehicles coexist with current SprawlCars
   - Compare performance and quality

2. **Parallel testing**
   - Toggle between old/new systems with Blueprint switch
   - Performance profiler comparisons
   - Visual quality comparisons

3. **Phased rollout**
   - Enable new system in one zone (e.g., The Junction only)
   - Once stable, expand to other zones
   - Remove old system last

### Backward Compatibility

Keep current systems functional:
```cpp
// Configuration flag
UPROPERTY(Config, EditAnywhere)
bool bUseLegacyPedestrians = false;

UPROPERTY(Config, EditAnywhere)
bool bUseLegacyVehicles = false;

// Spawn appropriate type based on flag
```

---

## 💡 Quick Wins (Low Effort, High Impact)

While planning major refactors, these can be done quickly:

1. **Pedestrian variety** (1 hour)
   - Download 3 free Mixamo characters
   - Swap out UE4 Mannequin
   - Randomize on spawn

2. **Vehicle paint variety** (30 min)
   - Already supported in SprawlCar!
   - Just expand the material array
   - Random on traffic spawn

3. **Basic foliage** (2 hours)
   - Download 5 Megascans plants
   - Paint grass along sidewalks
   - Place trees in strategic spots

4. **Improved materials** (1 hour)
   - Download Megascans asphalt/concrete
   - Replace current simple materials
   - Instant visual upgrade

5. **Wheel rotation shader trick** (1 hour)
   - Use material animation for fake rotation
   - Scroll tire texture based on velocity
   - Not physics-accurate but looks decent

---

## 🎯 Success Metrics

### Visual Quality
- [ ] Pedestrians indistinguishable from AAA games at 10m distance
- [ ] Vehicle wheels rotate in sync with speed
- [ ] Buildings have proper depth and detail
- [ ] Vegetation looks natural and varied
- [ ] Lighting feels realistic (time of day)

### Performance
- [ ] Maintain 60fps on iPhone 13+
- [ ] 100+ visible pedestrians
- [ ] 30+ visible vehicles
- [ ] Smooth streaming between zones
- [ ] < 2 second load times for zone transitions

### Gameplay Feel
- [ ] City feels alive and populated
- [ ] Traffic behaves believably
- [ ] Pedestrians react to player
- [ ] World has visual variety
- [ ] Maintains GDD's "Brooklyn-dense" vibe in The Junction

---

## 📝 Notes & Decisions

### Technical Decisions
- **Why Mass Entity vs traditional AI?**
  - Need 100+ pedestrians for city feel
  - Traditional AI too expensive (CPU)
  - Mass AI data-oriented, scales better

- **Why Chaos Vehicles?**
  - Modern UE5 standard
  - Better physics simulation
  - Built-in wheel animation
  - Replaces deprecated WheeledVehicle

- **Why Nanite?**
  - Target is iPhone 13+ (A15 chip supports)
  - Allows detailed buildings with no performance cost
  - Eliminates LOD creation work

### Design Decisions
- **Character realism level?**
  - Target: Stylized-realistic (not uncanny valley)
  - MetaHuman quality for Zarri
  - Simplified MetaHumans for pedestrians

- **Vehicle variety?**
  - Need 8-12 types minimum
  - Match zones: Junction = sedans, Rail Yards = trucks
  - Player car (Mobile Office) can be more detailed

- **World density?**
  - The Junction: Dense (per GDD)
  - Outskirts: Sparse
  - Use streaming to maintain density only where needed

---

## 🚀 Next Steps

1. **Prioritize** which phase to tackle first
2. **Assign agents** for parallel work if available
3. **Set up test map** for isolated feature testing
4. **Acquire assets** (MetaHumans, vehicles, buildings)
5. **Prototype** one feature fully before expanding
6. **Measure performance** continuously during development

---

## 📎 Asset Shopping List

### Characters
- [ ] 5 MetaHuman presets (diverse ages, genders)
- [ ] Mixamo animation pack (backup/lightweight option)

### Vehicles
- [ ] 3 sedan models (low, mid, high-end)
- [ ] 2 SUV models
- [ ] 1 truck model
- [ ] 1 sports car
- [ ] 1 beater (old car for Rail Yards)

### Environment
- [ ] Urban building pack (20+ buildings)
- [ ] Street props pack (lights, signs, benches)
- [ ] Megascans vegetation (10+ types)
- [ ] Road material pack
- [ ] Sky/weather system

### Sounds (future phase)
- [ ] Footstep sounds (concrete, asphalt, grass)
- [ ] Vehicle engine sounds
- [ ] Ambient city sounds

---

**Last Updated:** 2026-07-17
**Next Review:** Interactive editor and iPhone performance validation
