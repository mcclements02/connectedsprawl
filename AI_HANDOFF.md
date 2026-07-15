# AI Handoff Ledger — Project State

<!-- Version control: bump Version and Last updated on every edit to this file. -->
**Version:** 5 · **Last updated:** 2026-07-14 12:54 PDT · **Updated by:** codex

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
| main | . | — | idle | baseline | 2026-07-13 |
| pre-5.8-upgrade | /Users/matthewx/code/ConnectedSprawl | codex | done; uncommitted | City realism and upright-car fixes | 2026-07-14 |

## Log (append newest on top)

Append-only. One entry per handoff. Never rewrite or delete past entries. A merge
conflict here means two agents diverged — keep **both** entries.

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
