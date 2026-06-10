# The Connected Sprawl

A GTA-style open-world game built in **Unreal Engine 5.7**, starring **Zarri** — a 20-year-old founder hustling between the corporate boardroom and the rail yards.

> *You have a car, a laptop, and a dream. You also have 28 days of runway.*

---

## 🚗 Setup

1. Ensure Unreal Engine 5.7 is installed at `/Users/matthewx/UE_5.7`
2. Right-click `ConnectedSprawl.uproject` → **Generate Xcode Project Files**
3. Open in Xcode, build the `ConnectedSprawlEditor` target
4. Launch the editor: `/Users/matthewx/UE_5.7/Engine/Binaries/Mac/UnrealEditor.app` and open the `.uproject`

Or from the terminal:
```bash
/Users/matthewx/UE_5.7/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh \
  -project=/Users/matthewx/code/ConnectedSprawl/ConnectedSprawl.uproject -game
```

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
| Vehicles | `Source/ConnectedSprawl/Public/Vehicles/` | Zarri's Mobile Office car + lane-following traffic AI |
| World | `Source/ConnectedSprawl/Public/World/` | City grid, traffic signals, day/night, road markings, ground cover |
| AI | `Source/ConnectedSprawl/Public/AI/` | Sidewalk pedestrians, crowd + traffic density managers |
| Streaming | `Source/ConnectedSprawl/Public/Streaming/` | Zonal Density level streaming |
| Missions | `Source/ConnectedSprawl/Public/Missions/` | Strategic Decision data assets |

## 🌆 Living City Systems

The streets are simulated, not decorated:

- **`USprawlCityGridSubsystem`** — single source of truth for the road network:
  lane centerlines, intersections, lake bounds, and a deterministic
  time-based traffic-signal phase for every crossing.
- **Traffic AI (`ASprawlCar`)** — NPC cars keep to right-hand lanes, brake
  for cars and pedestrians ahead, stop at red lights behind the painted stop
  line, and pick legal turns at intersections (never into the lake or off
  the map). `AProceduralTrafficManager` recycles traffic in a ring around
  the player.
- **Traffic signals (`ASprawlTrafficLight`)** — signal poles at every dry
  intersection; the lamp heads display the exact same phase function the
  cars obey, so the visuals and the AI can never disagree.
- **Pedestrians (`ASprawlPedestrian`)** — walk the sidewalk ring of each
  block, cross at corners (lined up with the painted crosswalks) when no
  traffic is coming, and bolt when a speeding car gets close.
  `APedestrianCrowdManager` keeps the sidewalks populated near the player.
- **Day/night (`ASprawlEnvironmentController`)** — an 18-minute full day:
  the sun arcs and warms toward the horizon, a blue moon takes over at
  night, fog shifts mood at dawn/dusk, and streetlights tagged in the level
  switch on around the player after sundown.
- **Roads & nature** — `ASprawlRoadMarkings` paints dashed centerlines,
  crosswalks, and stop lines as instanced meshes; `ASprawlGroundCover`
  plants thousands of instanced grass blades and flowers in the parks; the
  lake gets an animated, depth-faded water material.

## 🎨 Art & Credits

The streets are walked by real humans now, not mannequins:

- **16 human characters** (faces, hair, clothes — a suit guy, an old man,
  women in streetwear, a mafia don, kids in hoodies…) from the CC0
  **[100 Avatars](https://github.com/madjin/100avatars)** packs by
  **Polygonal Mind**. Zarri's hero look is *Cappy* (cap + hoodie).
- **Locomotion animations** (idle, walk, formal walk, jog, phone-talk) from
  the CC0 **Universal Animation Library** by **Quaternius**, retargeted onto
  every avatar's Mixamo rig by `Tools/retarget_avatars.py` (headless Blender:
  world-space rotation transfer with per-bone rest alignment) and baked into
  the FBX files at `Content/Import/Pedestrians/`.
- **Street dressing** (benches, dumpsters, fire hydrants, bushes, boxes,
  litter, a park water tower) from the CC0
  **[KayKit City Builder Bits](https://github.com/KayKit-Game-Assets/KayKit-City-Builder-Bits-1.0)**
  pack by **Kay Lousberg**, scattered deterministically along the sidewalks.

Pedestrians pick a random avatar at spawn, scale to a believable height with
per-person variance, and drive Idle/Walk/Jog clips by actual ground speed
(play-rate matched so feet don't skate). A quarter of them idle chatting on
the phone. Suits walk formally. Everything falls back to the mannequin if the
art import hasn't run yet.

To (re)build the world after compiling the C++ module:

```bash
UnrealEditor ConnectedSprawl.uproject \
  -ExecutePythonScript="Content/Python/import_artwork.py" -unattended -nosplash
UnrealEditor ConnectedSprawl.uproject \
  -ExecutePythonScript="Content/Python/aaa_realism_overhaul.py" -unattended -nosplash
```

(or run the full pipeline via `Content/Python/rebuild_open_world_city.py`).

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

**Phase 1 — Prototype** ✅ Scaffolded. Ready for editor import and Blueprint wiring.
