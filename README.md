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
| Vehicles | `Source/ConnectedSprawl/Public/Vehicles/` | Zarri's Mobile Office car |
| Streaming | `Source/ConnectedSprawl/Public/Streaming/` | Zonal Density level streaming |
| Missions | `Source/ConnectedSprawl/Public/Missions/` | Strategic Decision data assets |

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
