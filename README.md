# The Connected Sprawl

A GTA-style open-world game built in **Unreal Engine 5.8**, starring **Zarri** — a 20-year-old founder hustling between the corporate boardroom and the rail yards.

> *You have a car, a laptop, and a dream. You also have 28 days of runway.*

---

## 🚗 Setup

0. Run `git lfs install` once per clone — binary assets (`.uasset`, `.umap`,
   FBX/PNG sources) are stored in Git LFS.
1. Install Unreal Engine 5.8 and set `UE_ROOT` to that installation:
   ```bash
   export UE_ROOT="/path/to/UE_5.8"
   ```
2. From the repository root, right-click `ConnectedSprawl.uproject` and choose
   **Generate Xcode Project Files**.
3. Open the generated workspace in Xcode and build the
   `ConnectedSprawlEditor` target.
4. Launch the editor from `$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app` and
   open the repository's `ConnectedSprawl.uproject`.

Or, from the repository root:
```bash
PROJECT_FILE="$PWD/ConnectedSprawl.uproject"
"$UE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" \
  -project="$PROJECT_FILE" -game
open "$UE_ROOT/Engine/Binaries/Mac/UnrealEditor.app" --args "$PROJECT_FILE"
```

### Marketplace content (not in git)

`Content/Downtown_West/` is the free **Downtown West Modular Pack** from Fab.
It is gitignored (11 GB, re-downloadable) but `TestMap` references it — without
it the city loses its streetlights, storefronts, props, and vistas. Install it
once per clone: open the editor → **Window → Fab** → sign in → search
"Downtown West Modular Pack" → **Add to My Library** → install to
`Content/Downtown_West/`. Then re-run
`Content/Python/mobile_texture_budget.py` to restore the mobile texture clamps.

> Headless runs: pass `-DisablePlugins=Fab,Bridge` to `UnrealEditor-Cmd` —
> restoring either marketplace browser under `-nullrhi` can assert.

---

## 🧠 Core Design

See [`Design/GDD.md`](Design/GDD.md) for the full game design document.

**The short version:** This is GTA, but the meta-game is **founding a startup**. Every mission is a Strategic Decision between:
- **Corporate** — stability, VC money, brand sanitization
- **Street** — fast cash, heavy heat, moral debt

Your car is your mobile office. You have a daily burn rate. You can go bankrupt.

---

## 🏗️ Architecture

| Module | Path | Purpose |
|---|---|---|
| Founder | `Source/ConnectedSprawl/Public/Founder/` | Cash, Burn Rate, Runway |
| Factions | `Source/ConnectedSprawl/Public/Factions/` | Corporate/Street rep, Heat, Moral Debt |
| Characters | `Source/ConnectedSprawl/Public/Characters/` | Character Developer profiles, replicated human actions, melee, MetaHuman wardrobe/footwear, avatar rendering |
| Vehicles | `Source/ConnectedSprawl/Public/Vehicles/` | Mobile Office, traffic AI, and hidden logical driver policy |
| World | `Source/ConnectedSprawl/Public/World/` | City grid, enterable destinations, city map, parking deck, traffic signals, day/night, road markings, ground cover |
| AI | `Source/ConnectedSprawl/Public/AI/` | Sidewalk pedestrians, crowd + traffic density managers |
| UI | `Source/ConnectedSprawl/Public/UI/` | Native HUD, touch controls, decisions, and city-map presentation |
| Streaming | `Source/ConnectedSprawl/Public/Streaming/` | Zonal Density level streaming |
| Missions | `Source/ConnectedSprawl/Public/Missions/` | Strategic Decision data assets |
| Persistence | `Source/ConnectedSprawl/Public/Save/` | Versioned founder, faction, mission, map, and player-state saves |

## 🌆 Living City Systems

The streets are simulated, not decorated:

- **`USprawlCityGridSubsystem`** — single source of truth for the road network:
  lane centerlines, intersections, lake bounds, and a deterministic
  time-based traffic-signal phase for every crossing. The same grid defines
  the playable city perimeter used by physical boundary walls and recovery.
- **Traffic AI (`ASprawlCar`)** — NPC cars keep to right-hand lanes, brake
  for cars and pedestrians ahead, stop at red lights behind the painted stop
  line, and pick legal turns at intersections (never into the lake or off
  the map). `AProceduralTrafficManager` retires distant managed cars but restores
  its target population only through the parking deck. Drivers remain logical
  occupants for entry and carjacking but are hidden while seated behind the
  cars' dark cabin glass; the mounted skeletal mesh is unloaded and unticked,
  preventing roof clipping and saving one avatar render cost per traffic car.
  Ejected drivers render normally as exterior pedestrians.
- **Parking-deck traffic source (`ASprawlParkingGarage`)** — a four-level,
  instanced central garage replaces one authored downtown lot with concrete
  decks, ramps, marked bays, parked-car silhouettes, lighting, barriers, and
  two covered driveways. Replenishment cars are created behind its facade only
  when an entire exit path is clear, roll out at garage speed, and switch to
  directed-lane AI after an exact legal merge. A blocked garage waits; there is
  no fallback that materializes a vehicle in an active street. Site preparation
  preserves street and sidewalk surfaces and moves a conflicting authored car
  into an upper-deck bay rather than deleting it.
- **Enterable destinations (`ASprawlEnterableInteriors`)** — Junction Market,
  Founder House Offices, and Canal View Condos add distinct signed entrances
  on legal sidewalk frontage and three enclosed runtime interiors. The shared
  layout contract supplies each doorway, interior pocket, furniture set, map
  marker, and safe return point without deleting authored city buildings.
  Press **E** or **F** at a door to enter or leave; the touch HUD exposes the
  same context action. `FSprawlInteriorPropLibrary` replaces placeholder boxes
  with 77 normalized placements across ten textured furniture/item meshes:
  tables, chairs, benches, stock cans and bottles, planters, foliage, bins, and
  poster stands. Detailed modular counters, shelves, desks, monitors, bed,
  sofa, and kitchen remain as the deterministic fallback, while one HISM per
  resolved mesh keeps repeated props inside the iPhone-first draw budget.
- **City map (`USprawlCityMapSubsystem` + `USprawlCityMapWidget`)** — press
  **M** or tap **MAP** to pause into a native map showing the road grid, lake,
  five named destinations, Zarri's live position, and distance to the nearest
  destination. Interior positions resolve to their exterior doorway so the
  marker remains useful while Zarri is inside. **M**, **Escape**, or the map's
  close button returns to play.
- **Parking, crashes, and access** — all 36 curbside cars are real, enterable
  `ASprawlCar` pawns placed by full-footprint building/prop clearance checks.
  Cars normally remain upright and inside the city, but a severe impact gets a
  short physical rollover window before self-righting. Press **E** or **F**
  near a stopped car to enter it, drive with **WASD**, and press **E** or **F**
  again to exit at the first unobstructed door/end position. **W/Up** always
  accelerates toward the visible nose: imported split-body cars use their named
  front axle and legacy Blender Y-forward meshes are normalized to physics +X.
- **On-foot melee (`USprawlMeleeModule`)** — Zarri can punch and kick through
  one alternating action: **left mouse** or **X** on desktop, the left face
  button on a gamepad, and the new **PUNCH / KICK** touch button. A short
  forward cone selects one visible character, sends the standard Unreal damage
  event, and makes pedestrians recoil and flee. Cadence and reach are bounded;
  attacks are disabled while Zarri is hidden in a car. The companion
  `FSprawlMeleeInput` module binds X/gamepad X directly on the possessed pawn
  so Enhanced Input cannot shadow the action; successful attacks briefly show
  PUNCH or KICK even when an optional character-specific one-shot is absent.
- **Traffic signals (`ASprawlTrafficLight`)** — each dry intersection owns a
  four-corner, three-lens signal model. Missing map signals self-spawn at
  runtime, while the red/amber/green heads display the exact same phase
  function the cars obey, so visuals and AI can never disagree.
- **Street dressing (`ASprawlStreetDressing`)** — freestanding foldout signs
  are confined to the explicit sidewalk band beyond the kerb; parking bays,
  asphalt aprons, travel lanes, and junctions are rejected as sign ground.
- **Pedestrians (`ASprawlPedestrian`)** — walk the sidewalk ring of each
  block, cross at corners (lined up with the painted crosswalks) when no
  traffic is coming, and bolt when a speeding car gets close. The deterministic
  **Character Developer** gives each person a district, role, schedule,
  activity, wardrobe, stature, gait, and stable identity before spawning. The
  reusable **Human Character module** derives a distinct Zarri-compatible
  appearance contract and exposes replicated Stand/Walk/Run/Talk/Sit/Drive
  state to Blueprint. A strict **Crowd MetaHuman module** maps that compact
  state onto the project-owned Zarri, Amina, and Andre Optimized/Low residents;
  no mannequin or toy avatar can enter the ambient roster. The authority owns
  spawning and replicated identity, while each rendering client budgets LOD.
  `APedestrianCrowdManager` keeps The Junction busy while naturally thinning
  Iron Forest, Rail Yards, and late-night streets, capped at eight residents on
  Mac and three on iPhone.
- **Day/night (`ASprawlEnvironmentController`)** — an 18-minute full day:
  the sun arcs and warms toward the horizon, a blue moon takes over at
  night, fog shifts mood at dawn/dusk, and streetlights tagged in the level
  switch on around the player after sundown.
- **Roads & nature** — `ASprawlRoadMarkings` paints dashed centerlines,
  crosswalks, and stop lines as instanced meshes; `ASprawlGroundCover`
  plants thousands of instanced grass blades and flowers in the parks; the
  lake gets an animated, depth-faded water material.

## 💾 Progression & Saves

Progress is stored in a versioned `USprawlSaveGame` payload. It restores cash,
day, burn rate, ledger history, faction reputation, heat, moral debt, resolved
decision branches, map, player transform, and whether Zarri was driving. The
mission director follows the saved branch chain and resumes at the next
unresolved call instead of paying the same reward twice.

- Autosaves run after decisions, day advances, app backgrounding, and a clean
  desktop quit.
- Press **F5** for a manual save and **F9** to load and restart at the saved
  location.
- Blueprint code can call `SaveProgress`, `LoadProgress`,
  `LoadProgressAndRestart`, `HasSave`, or `StartNewGame` on
  `USprawlSaveSubsystem`.

## 🎨 Art & Credits

The ambient streets are walked by assembled MetaHumans, not mannequins:

- **Three project-owned crowd identities** ship today: Zarri, Amina, and
  Andre. Each has a distinct authored face; Amina and Andre add separate body,
  clothing, and groom assemblies under `Content/MetaHumans/Residents/`.
  Deterministic height and wardrobe palettes create repeatable copies without
  distorting faces. The former **22 lightweight avatars** from the CC0
  **[100 Avatars](https://github.com/madjin/100avatars)** packs by
  **Polygonal Mind** remain credited legacy/reference content, but are not an
  ambient crowd fallback.
- **Reusable wardrobe development** comes from `USprawlWardrobeModule`. Every
  generated person receives a deterministic top, bottom, optional jacket,
  optional cap or beanie, footwear, socks, and coordinated palette. It tints
  the fitted MetaHuman base garment while `USprawlFootwearModule` develops
  rounded uppers, fitted soles, collars, laces, and socks from each live
  heel-to-ball distance. `USprawlAthleticShoeModule` adds four validated,
  Blueprint-swappable athletic presets—Zarri Velocity, Metro Runner, Court
  High-Top, and Night Sprint—with coordinated upper, sole, sock, and accent
  styling. `USprawlBiomedicalShoeModule` adds the project-owned Cordero
  Biomedical Bio-Circuit high-top profile: an editable Blender/FBX pair with a
  nanofiber upper, deep-red supports, circuit window, heel air unit, and podded
  outsole, plus a validated one-call high-top fallback for any bound character.
  Shoes take position from an independently animated calf/IK anchor
  while retaining body-relative forward facing; the underlying Body foot bones
  can therefore be masked without detaching the pair or turning a running shoe
  vertically. Zarri's Nanobanana preset takes a dedicated authored path through
  `USprawlStreetwearModule`: 45 imported hoodie, tech-bomber, cargo, beanie, and
  Zarri Velocity mesh assets replace his default shirt and shorts. Bounds-based
  body fitting and post-animation upper/lower limb followers keep the rigid
  source art on the live MetaHuman pose; failure to resolve any required asset
  leaves the fitted defaults intact. The shoe and hero-streetwear followers are
  the only ticking presentation elements; every generated component remains
  collision-, navigation-, decal-, and shadow-free. The complete outfit is
  replicated, so all clients render the same look.
- **Zarri's current fitted reference outfit** is generated in Blender by
  `Tools/build_reference_clothing.py`, imported by
  `Content/Python/import_reference_clothing.py`, and mapped at runtime by
  `USprawlReferenceClothingModule`. Its 17 project-owned sections form the
  reference image's fitted gray long-sleeve crewneck and dark straight pants;
  12 normalized local-`+Z` limb shells refit to Zarri after animation while
  rounded elbow/knee transitions cover the joins. The module replaces the
  bulkier hoodie, bomber, and cargo presentation only after every required
  asset resolves, retaining the validated shoes and beanie and preserving the
  old outfit as a safe fallback. The editable source, viewport preview, and
  build report live under `Content/MetaHumans/Source/Clothing/`. A single image
  is enough to establish this silhouette; front, back, side, and fabric closeup
  references are still needed for production-accurate seams and materials.
  Poliigon is an optional Blender material source, not a runtime dependency;
  the project source remains reproducible with its procedural fabric material.
- **Unreal 5.8 Panel Cloth authoring** is available without CLO. The
  `ConnectedSprawlEditor` module and
  `Content/Python/build_zarri_panel_cloth.py` import the Blender simulation
  shell, build an editable Dataflow asset, and bake a graph-free runtime hoodie
  under `Content/Import/Characters/Streetwear/PanelCloth/`. Run Zarri with
  `-SprawlPanelClothPreview` to inspect the Chaos solver, MetaHuman collision,
  and current skin-weight transfer. That transfer still folds the sleeve
  panels, so normal launches deliberately retain the validated rigid outfit
  until its weights are hand-fitted in the Panel Cloth Editor. Shoes remain a
  Blender-first rigid/skinned topology workflow; ZBrush is optional for
  high-poly leather, wrinkle, and tread detail baked into game-ready maps.
- **MetaHuman stand, walk, and run animations** come from Epic's installed
  MetaHuman Character locomotion preset and are LOD-synchronized across body,
  face, hair, and grooms. The older CC0 **Universal Animation Library** by
  **Quaternius** remains with the legacy avatar source pipeline at
  `Content/Import/Pedestrians/` and `Tools/retarget_avatars.py`.
- **Street dressing** (benches, dumpsters, fire hydrants, bushes, boxes,
  litter, a park water tower) from the CC0
  **[KayKit City Builder Bits](https://github.com/KayKit-Game-Assets/KayKit-City-Builder-Bits-1.0)**
  pack by **Kay Lousberg**, scattered deterministically along the sidewalks.

Pedestrians receive a balanced Zarri/Amina/Andre identity cycle, scale uniformly
to a believable height, and drive Stand/Walk/Run clips by actual ground speed.
The shared action module retains Talk/Sit/Drive for scripted and named-character
bindings. Missing crowd art fails closed by hiding that visual and logging the
problem; it never restores the mannequin that this roster replaces. The runtime
traffic audit enforces at least three distinct MetaHuman identities and zero
non-MetaHuman pedestrians after warm-up.

Named and mission characters can still use `USprawlCharacterDefinition` Data Assets to
bind the same profile to an optional assembled MetaHuman class plus stand,
walk, run, talk, sit, and drive clips. Generated appearances use Zarri's
joints-only animation contract as a technical baseline while explicitly
remaining unique people. Zarri's melee presentation also accepts optional
skeleton-compatible punch and kick one-shots; missing clips never disable hit
gameplay or replace a working locomotion set. [`Tools/CharacterDeveloper`](Tools/CharacterDeveloper/README.md) can
draft those profiles and MetaHuman Creator briefs with Hugging Face, but the
cloud authoring path and credentials never enter the runtime build.

To regenerate the two resident assemblies, run the authoring script in the full
UE 5.8 Editor (not `UnrealEditor-Cmd`; TextureGraph baking needs the editor
runtime):

```bash
UnrealEditor ConnectedSprawl.uproject \
  -ExecutePythonScript="Content/Python/create_city_metahuman_residents.py" \
  -ScriptErrorsAreFatal -unattended -RenderOffscreen -nosplash -NoSound
```

To (re)build the world after compiling the C++ module:

```bash
UnrealEditor ConnectedSprawl.uproject \
  -ExecutePythonScript="Content/Python/import_artwork.py" -unattended -nosplash
UnrealEditor ConnectedSprawl.uproject \
  -ExecutePythonScript="Content/Python/aaa_realism_overhaul.py" -unattended -nosplash
```

(or run the full pipeline via `Content/Python/rebuild_open_world_city.py`).

### iOS startup branding

The standard Unreal iOS launch storyboard loads the project-owned, opaque
2048×2048 image at `Build/IOS/Resources/Graphics/LaunchScreenIOS.png`. It
presents **Angry Cordero Production Studio** together with the **Unreal Engine**
lockup, centered inside the square safe area for both iPhone and iPad
orientations. Keep that file opaque and square when revising the artwork; the
rest of `Build/` remains generated output and is ignored.

---

## 🗺️ The 15-Mile Map

| Zone | Vibe |
|---|---|
| **The Junction** (core, 3 mi) | Brooklyn-dense, vertical, pedestrian |
| **The 85-Express** (arteries) | 120mph cinematic highways |
| **Iron Forest** (north) | Gated tech campuses |
| **Rail Yards** (south) | Industrial warehouses, docks |

Built for **iPhone 60fps** using a 1-mile high-density streaming radius + low-poly silhouette proxies everywhere else.

---

## 🎮 Target Platforms

- **Primary:** iOS (iPhone 13+)
- **Secondary:** macOS

---

## 📋 Status

**Phase 2 — Gray-box playable** ✅ The drive/decide/earn/burn loop, living-city
traffic and pedestrians, decision UI, and save/resume progression are working.
