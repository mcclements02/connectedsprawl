# AI Handoff Ledger — Project State

<!-- Version control: bump Version and Last updated on every edit to this file. -->
**Version:** 10 · **Last updated:** 2026-07-18 09:33 PDT · **Updated by:** codex

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
| main | /Users/matthewx/code/ConnectedSprawl | codex | ready; uncommitted | UE 5.8 migration, saves, real avatars, animated ordered traffic, enterable building-clear parking, crash recovery, and city boundaries; Mac audits pass | 2026-07-18 |

## Log (append newest on top)

Append-only. One entry per handoff. Never rewrite or delete past entries. A merge
conflict here means two agents diverged — keep **both** entries.

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
