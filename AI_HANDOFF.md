# AI Handoff Ledger — Project State

<!-- Version control: bump Version and Last updated on every edit to this file. -->
**Version:** 89 · **Last updated:** 2026-07-23 13:05 PDT · **Updated by:** claude

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
| `main` | `/Users/matthewx/code/ConnectedSprawl` | claude · codex | committed + pushed | Everything on `main` pushed to `origin/main` at user request: codex's Blender Live MCP + streetwear/wardrobe/footwear/panel-cloth/reference-clothing/minimap runtime, and claude's Blender garment authoring (CC0 base, denim jacket, parameterized outfit). Worktree clean. | 2026-07-23 13:05 PDT |

## Log (append newest on top)

Append-only. One entry per handoff. Never rewrite or delete past entries. A merge
conflict here means two agents diverged — keep **both** entries.

### 2026-07-23 · main · claude (Blender garment authoring + push-all to origin/main)
- **New authoring module `Tools/build_zarri_denim_jacket.py`:** downloads and
  preps the CC0 Blender Studio Human Base Meshes male body (recentred, scaled to
  Zarri's 1.775 m, saved as `Content/MetaHumans/Source/BaseMeshes/HumanBaseMale.blend`),
  then authors a denim trucker jacket by extracting the garment shell from the
  BODY'S OWN FACES (real anatomical fit), with collar, placket, copper buttons,
  flap pockets, cuffs, tan twin-needle topstitch, noise-displace wrinkles and an
  indigo twill material. Exports `Content/Import/Characters/Denim/SM_ZarriDenimJacket.fbx`.
- **New authoring module `Tools/build_zarri_outfit.py`:** parameterized full fit
  (colours via `PARAMS` or a JSON argv override) — fitted top, jeans, bucket hat,
  and sneakers built as a **lofted last** (18 elliptical sections, widest at the
  ball, D-shaped flat-bottomed sections so the upper meets the sole), plus teal
  lace bars and side stripe. Exports `Content/Import/Characters/Outfit/SM_ZarriOutfit.fbx`.
- **Two real technique bugs found and recorded:** (1) inflating a shell *before*
  relaxing it lets smoothing shrink it back inside the body — skin poked through
  as holes at the thighs/crotch; the order must be **relax → then inflate**, and
  `LOOSE` below ~0.017 on the legs reopens them. (2) A full-ellipse shoe section
  pinches to a point at the bottom, so the sole flared out as a bare plate; a
  D-section fixes it.
- **Status of these assets:** source/preview only — **unrigged and NOT imported
  or skinned onto Zarri**; they do not affect the runtime. Nothing in the
  existing streetwear/wardrobe systems was modified.
- **Validation:** all Blender passes ran clean headless; `ConnectedSprawlEditor
  Mac Development` reports Result: Succeeded (up to date) against the full
  working tree, so this push does not break `main`'s compile.
- **NOTE — this commit is a user-requested "push all":** it also carries codex's
  in-flight streetwear/wardrobe/footwear/panel-cloth/reference-clothing/minimap
  C++ and assets, plus an untracked `Source/ConnectedSprawlEditor/` that is **not
  declared in `.uproject` Modules** (inert, not built). claude did not author or
  independently validate that work beyond the compile check above — see codex's
  own entries below for its state.
- **Risk:** no cook, package, iPhone/device, or performance profile was run. LFS
  storage grows by ~115 MB with this push (running total roughly 380 MB against
  GitHub's 1 GiB free tier).
<!-- entry:blender-garment-authoring-and-push-all -->

### 2026-07-22 · main · codex (Blender Live MCP + Zarri fitted reference outfit)
- **Result:** created and installed the external personal `blender-live` Codex
  plugin, whose localhost-only MCP bridge can launch Blender, summarize a
  scene, run scripts, save, and capture the live viewport. Built an editable
  17-piece light-gray long-sleeve shirt and dark straight-pants outfit from the
  supplied reference, normalized the 12 limb exports to local `+Z`, imported
  all pieces, and added Blueprint-facing `USprawlReferenceClothingModule` to
  fit them to Zarri's live MetaHuman pose. Standard gameplay now swaps out the
  bulky hoodie/bomber/cargo only after the complete kit loads, retains shoes
  and beanie, and lets the opt-in Panel Cloth preview hide the rigid shirt.
- **Files:** new Blender builder, editable `.blend`, preview/report, 17 FBX and
  imported mesh assets, Unreal import command, reference-clothing runtime
  module/test, Zarri integration, cook rule, README, and GDD. The personal MCP
  plugin lives outside the repository at `/Users/matthewx/plugins/blender-live`.
- **Validation:** Blender 5.1 regenerated the source, preview, report, and 17
  exports; Unreal imported all 17 meshes without errors or warnings; the UE 5.8
  Mac Development editor build and focused
  `ConnectedSprawl.Characters.ReferenceClothingModule` automation passed. A
  real-Metal 1280x720 wardrobe audit loaded 17 pieces with 12 live fits and
  passed a forced-run visual check (mean luma 105.60, 0.03% crushed, 0.13%
  clipped).
  Plugin manifest, Node syntax, bridge Python syntax, and manual MCP
  scene-summary/viewport-capture calls passed.
- **Risk:** the outfit is a segmented rigid approximation, not a continuously
  skinned or simulated garment. More front/back/side and material references
  are needed for production seam accuracy. Poliigon 1.16.2 is installed,
  enabled, and authenticated locally, but profile/marketing onboarding and a
  licensed fabric download require the user's choices; the procedural material
  remains active. iOS cook, package, device, and performance validation were
  not run.
- **Status:** validated and uncommitted on `main`; unrelated dirty city,
  biomedical, footwear, minimap, UI, world, prior cloth, and local Poliigon
  source work was preserved. No branch, worktree, stage, commit, or remote
  operation was performed.
<!-- entry:blender-live-zarri-reference-outfit -->

### 2026-07-22 · main · codex (Zarri Unreal Panel Cloth integration)
- **Result:** enabled Unreal 5.8 Chaos Cloth Asset/Dataflow editor support and
  added a `ConnectedSprawlEditor` authoring module that imports the Blender
  hoodie shell, builds an editable Panel Cloth Dataflow graph, paints movement,
  transfers MetaHuman weights, configures collision/material/solver properties,
  and bakes a graph-free runtime asset. Zarri owns a runtime cloth bridge, but
  it is gated behind `-SprawlPanelClothPreview`; normal gameplay keeps the
  validated rigid streetwear because visual QA found folded sleeve weights.
  Footwear remains on the rigid/skinned Blender topology and optional ZBrush
  detail-bake path rather than Chaos Cloth.
- **Files:** project/module/plugin configuration; new runtime Panel Cloth module
  and test; new editor module and Python authoring command; Zarri/streetwear
  integration; refined Blender hoodie simulation topology and generated
  source/runtime cloth assets/material; README and GDD contracts.
- **Validation:** Blender 5.1 regenerated the editable cloth kit and FBX files;
  the Unreal authoring command completed cleanly with 1,216 painted simulation
  vertices; UE 5.8 Mac Development editor build passed; focused
  `ConnectedSprawl.Characters.PanelClothModule` automation passed 1/1. Real-
  Metal default and opt-in preview audits both passed at 1280x720: default kept
  rigid streetwear; preview loaded one cloth model with MetaHuman sphyl
  collision and no missing Dataflow-node or cloth-material warnings.
- **Risk:** the preview hoodie is not production-fitted—the automatically
  transferred sleeves fold across the torso and require manual weight/paint
  cleanup in Panel Cloth Editor. iOS cook, package, device, and performance
  validation were not run.
- **Status:** validated and uncommitted on `main`; unrelated dirty city,
  biomedical, footwear, minimap, UI, and world work was preserved. No branch,
  worktree, stage, commit, or remote operation was performed.
<!-- entry:zarri-unreal-panel-cloth-integration -->

### 2026-07-22 · main · codex (commit Zarri authored streetwear)
- **Result:** Prepared the validated Zarri Nanobanana runtime module, its cloth
  physics profiles, exact imported mesh/material dependencies, editable Blender
  sources, FBX exports, authoring scripts, cook rule, preset binding, focused
  tests, and documentation as one cohesive commit. Unrelated dirty city,
  biomedical, minimap, UI, footwear, and world work remains outside the commit.
- **Validation:** Reused the immediately preceding successful UE 5.8 Mac editor
  build, 1/1 focused streetwear automation result, real-Metal idle/run audits,
  and clean whitespace check; inspected the staged diff before committing.
- **Status:** committed in this change on `main`; no branch, worktree, push, or
  other remote operation performed.
<!-- entry:commit-zarri-authored-streetwear -->

### 2026-07-22 · main · codex (Zarri authored streetwear runtime integration)
- **Result:** Added Blueprint-facing `USprawlStreetwearModule`, a validated
  45-asset catalog for the imported Nanobanana hoodie, tech bomber, cargo
  joggers, rib beanie, and Zarri Velocity trainers. Zarri now takes this
  authored path after the assembled MetaHuman activates; all required meshes
  must load before the old shirt/short component and bare feet are hidden, with
  the existing procedural wardrobe retained as the safe fallback. Runtime body
  bounds, owner-facing orientation, and post-animation upper/lower limb
  followers produce 51 fitted presentation pieces without collision,
  navigation, decals, component ticks, or shadows. Added the import directory
  to always-cook configuration and fixed the pre-existing cloth-module setter
  warning that blocked compilation.
- **Files:** new `SprawlStreetwearModule.h/.cpp` and focused automation test;
  Zarri header/runtime integration; `Config/DefaultGame.ini`; cloth-physics
  header compile fix; README and GDD runtime contract updates.
- **Validation:** UE 5.8 Mac Development editor build passed. Focused
  `ConnectedSprawl.Characters.StreetwearModule` automation passed 1/1 and
  synchronously resolved all 45 imported `UStaticMesh` assets. Real-Metal
  1280x720 idle and forced-run wardrobe audits both loaded 51 pieces, hid the
  base garment, and passed; the run capture measured mean luma 103.40, 0.02%
  crushed, and 0.13% clipped. `git diff --check` passed.
- **Risk:** the supplied garment art is rigid, deliberately chunky static-mesh
  geometry rather than a skinned or Chaos-cloth outfit. Segment followers keep
  it attached through gait, but tight bends can still expose small skin slivers;
  eliminating those completely requires a later weight-transfer/cloth-authoring
  pass on the source meshes. iOS cook/package/device performance was not run.
- **Status:** validated and uncommitted on `main`; all unrelated dirty state was
  preserved. No branch, worktree, stage, commit, or remote operation performed.
<!-- entry:zarri-authored-streetwear-runtime -->

### 2026-07-22 · main · gemini (Zarri Hero Nanobanana Outfit Assignment)
- **Result:** Bound `USprawlWardrobeModule::CreateNanobananaOutfit()` directly to Zarri's hero customization in `USprawlHumanCharacterModule::CreateZarriCustomization()` (`Source/ConnectedSprawl/Private/Characters/SprawlHumanCharacterModule.cpp`). Integrated `USprawlClothPhysicsModule::ConfigureOutfitPhysics()` during Zarri's MetaHuman visual initialization (`Source/ConnectedSprawl/Private/Characters/ZarriCharacter.cpp`).
- **Validation:** Updated `SprawlHumanCharacterModuleTests.cpp` to verify Zarri wears the Nanobanana techwear outfit by default. Executed `git diff --check`.
- **Status:** uncommitted on `main`.
<!-- entry:zarri-nanobanana-outfit-assignment -->


### 2026-07-22 · main · gemini (Nanobanana AI Clothing Generation & 3D Integration)
- **Result:** Used Nanobanana AI generative visual modeling (`generate_image`) to create high-detail concept art & texture maps for Nanobanana Cyberpunk Oversized Hoodie and Weatherproof Tech Bomber Jacket. Staged visual concept images under `staging/clothing/` and `Content/MetaHumans/Source/Streetwear/Nanobanana/`. Added `CreateNanobananaOutfit()` preset function to `USprawlWardrobeModule` (`Source/ConnectedSprawl/Public/Characters/SprawlWardrobeModule.h` & `.cpp`), configuring Cyberpunk Navy, Dark Graphite Cargo, and Neon Cyan CyberDrip palettes.
- **Assets:** Staged `nanobanana_hoodie.jpg`, `nanobanana_tech_jacket.jpg`, and updated 3D FBX assets via Blender 5.1 pipeline.
- **Validation:** Executed `build_cloth_realism_kit.py` in Blender 5.1; verified Python syntax (`py_compile`). Updated `SprawlWardrobeModuleTests.cpp` automation test suite. Ran mandatory git preflight and `git diff --check`.
- **Status:** uncommitted on `main`.
<!-- entry:nanobanana-ai-clothing -->


### 2026-07-22 · main · gemini (Blender Cloth Physics & C++ SprawlClothPhysicsModule)
- **Result:** Created new C++ module `USprawlClothPhysicsModule` (`Source/ConnectedSprawl/Public/Characters/SprawlClothPhysicsModule.h` & `.cpp`) supporting physical cloth profiles (`FSprawlClothPhysicsProfile`), fabric type resolvers, validation, description formatting, component binding, and locomotion wind simulation. Integrated module into `USprawlWardrobeModule` and `AZarriCharacter`. Authored dedicated Blender physics script `Tools/build_cloth_realism_kit.py` with `CLOTH` physics modifiers, vertex pinning groups (`Pin_Waist`, `Pin_Shoulders`), collision margins, and realistic fabric presets (`COTTON_FLEECE`, `TECH_NYLON`, `CANVAS_CARGO`, `WOOL_KNIT`, `DENIM`, `LEATHER`).
- **Assets:** Generated `ConnectedSprawl_ClothRealismKit.blend`, exported `SM_RealCloth_Hoodie.fbx`, `SM_RealCloth_Bomber.fbx`, `SM_RealCloth_CargoJoggers.fbx`, and `ClothRealismKitReport.txt`.
- **Validation:** Executed `build_cloth_realism_kit.py` in Blender 5.1; verified Python syntax (`py_compile`). Authored C++ automation test suite `SprawlClothPhysicsModuleTests.cpp` for `ConnectedSprawl.Characters.ClothPhysicsModule`. Ran mandatory git preflight and `git diff --check`.
- **Status:** uncommitted on `main`.
<!-- entry:blender-cloth-physics-module -->


### 2026-07-22 · main · gemini (Launch Game)
- **Result:** Triggered `open ConnectedSprawl.uproject` to launch the Unreal Engine editor and project environment.
- **Validation:** Verified command execution. Ran mandatory git preflight and `git diff --check`.
- **Status:** uncommitted on `main`.
<!-- entry:launch-game -->


### 2026-07-22 · main · gemini (Blender Realistic Clothes & Fabric Wrinkles)
- **Result:** Enhanced the Blender streetwear kit generator (`Tools/build_streetwear_kit.py`) with multi-harmonic radial sine/cosine vertex perturbations for organic fabric folds, seam drapery, and cloth wrinkles across all garments. Upgraded material shaders with fabric sheen weights and micro-weave noise normal maps.
- **Assets:** Re-rendered and re-exported all 5 streetwear FBX assets (`SM_Streetwear_OversizedHoodie.fbx`, `SM_Streetwear_TechBomber.fbx`, `SM_Streetwear_CargoJoggers.fbx`, `SM_Streetwear_RibBeanie.fbx`, `SM_ZarriVelocity_ShoePair.fbx`) and high-resolution preview images.
- **Validation:** Executed `build_streetwear_kit.py` in Blender 5.1. Ran mandatory git preflight and `git diff --check`.
- **Status:** uncommitted on `main`.
<!-- entry:blender-realistic-clothes-wrinkles -->

### 2026-07-22 · main · gemini (Blender 3D Streetwear/Shoes & Zarri Hero Mesh)
- **Result:** Authored and generated high-quality 3D streetwear clothes and shoes for Zarri in Blender (Oversized Hoodie, Tech Bomber, Cargo Joggers, Rib Beanie, Zarri Velocity Shoes, and Cordero Biomedical Bio-Circuit High-Top Pair). Updated `AZarriCharacter` constructor in C++ to load the high-detail `SK_Zarri` avatar mesh by default instead of falling back to the blank engine mannequin.
- **Assets:** Exported `SM_ZarriVelocity_ShoePair.fbx`, `SM_Streetwear_OversizedHoodie.fbx`, `SM_Streetwear_TechBomber.fbx`, `SM_Streetwear_CargoJoggers.fbx`, `SM_Streetwear_RibBeanie.fbx`, `SM_CorderoBiomedical_HighTopPair.fbx`, and `Zarri.fbx` via Blender 5.1 pipeline.
- **Validation:** Executed `build_streetwear_kit.py`, `build_biomedical_shoe.py`, and `build_zarri_avatar.py` in Blender 5.1 to completion. Validated FBX exports and previews. Updated `Source/ConnectedSprawl/Private/Characters/ZarriCharacter.cpp`. Ran mandatory git preflight and `git diff --check`.
- **Status:** uncommitted on `main`.
<!-- entry:blender-streetwear-zarri-hero-mesh -->

### 2026-07-22 · main · codex (Cordero Biomedical Bio-Circuit high-top)
- **Result:** added a standalone Blueprint-facing biomedical-shoe definition
  and swap module for Zarri, named characters, and pedestrian variants. The
  validated profile retains the supplied design's gray nanofiber upper,
  deep-red supports, gold/red circuit window, heel air unit, padded high-top
  collar, and ten traction pods per shoe while reusing the proven animated-foot
  high-top fallback at runtime.
- **Assets:** added a deterministic Blender authoring script, editable `.blend`,
  1280x720 preview, generation report, and material-bearing FBX 7.4 pair at
  metre scale with +X toe-forward orientation. The source pair is separated
  into named components for later MetaHuman weight transfer and LOD work.
- **Validation:** Python parsing passed. Blender 5.1 generated, saved, exported,
  and rendered the pair; full-resolution visual QA corrected an Eevee API name,
  preview exposure, and floating tongue/eyestay geometry before the final pass.
  `file` recognized Blender/Zstandard, PNG 1280x720, and Kaydara FBX 7400;
  `.blend` and `.fbx` resolve through Git LFS. UE 5.8 editor compilation passed,
  focused `ConnectedSprawl.Characters.BiomedicalShoeModule` automation passed
  1/1, `git diff --check` passed, and the AI sync audit found no stranded branch.
- **Status:** uncommitted on `main`; pre-existing wardrobe, footwear, minimap,
  city, and streetwear changes remain untouched.
<!-- entry:cordero-biomedical-high-top -->

### 2026-07-22 · main · codex (reference-inspired trainer refinement)
- **Result:** refined the project-owned Zarri Velocity trainers from the supplied
  visual reference while deliberately omitting Nike names, wordmarks, and
  swooshes. The model now has micro-weave upper materials, layered vamp panels,
  fitted instep straps, compact lace clusters, slim moulded toe bumpers, inset
  quarter panels, and arch-following segmented outsole tread.
- **Assets:** regenerated the editable Blender scene, shoe/detail and complete
  kit previews, report, and all five FBX exports; the shoe-pair FBX remains at
  metre scale with +X toe-forward orientation and LFS coverage.
- **Validation:** Python parsing and whitespace checks passed; Blender 5.1
  completed save/export/render without errors; full-resolution visual inspection
  verified seated straps/vamps, closed geometry, coordinated materials, and
  attached tread. `file` recognized the shoe output as Kaydara FBX 7400.
- **Status:** refinement is uncommitted on `main`. Unreal import, MetaHuman
  rigging/weight transfer, animation fitting, cook, and device validation were
  not run; unrelated in-progress C++ and minimap changes remain untouched.
<!-- entry:reference-inspired-trainer-refinement -->

### 2026-07-22 · main · codex (Blender athletic shoes + streetwear kit)
- **Result:** authored a reusable Blender 5.1 streetwear scene with a fitted
  logo-free Zarri Velocity trainer pair, oversized hoodie, tech bomber, cargo
  joggers, and rib beanie. The shoes include moulded/arched soles, closed
  uppers, tongues, heel clips, collars, five lace rows, and side accents.
- **Assets:** saved the editable `.blend`, overview and shoe-detail PNGs, a
  generation report, and five material-bearing FBX 7.4 exports under the
  project-owned MetaHuman source/import folders. The deterministic authoring
  script preserves metre scale and +X shoe-forward orientation.
- **Validation:** authoring script parse passed; Blender executed the complete
  generation, save, FBX export, and two-render workflow; `file` recognized the
  source as Blender/Zstandard and every export as Kaydara FBX 7400. Full-size
  visual inspection verified apparel layout, coordinated materials, shoe-pair
  proportions, visible detail, and closed toe/heel surfaces.
- **Status:** assets and script are uncommitted on `main`; no C++ wardrobe or
  footwear changes were made in this pass. Rigging/weight transfer, Unreal
  import, MetaHuman fitting, cook, device runtime, and performance capture were
  not run.
<!-- entry:blender-athletic-shoes-streetwear-kit -->

### 2026-07-22 · main · codex (minimap runtime hitch guard)
- **Result:** reduced the always-on corner minimap's runtime cost after a
  reported freeze/close. The minimap now refreshes geometry at 15 Hz, refreshes
  nearest-location text at 2 Hz, skips unchanged slot/visibility updates, and
  avoids rebuilding equivalent label text every frame.
- **Validation:** Xcode `BuildProject` passed after the minimap optimization.
  Per-file Xcode diagnostics could not inspect the new minimap source/header
  because those files are still untracked in the current project structure.
- **Status:** focused mitigation is uncommitted on `main`; no branch, staging,
  commit, push, or remote operation was performed. No on-device crash log,
  editor runtime, iPhone launch, or performance capture was available in this
  turn.
<!-- entry:minimap-runtime-hitch-guard -->

### 2026-07-21 · main · codex (iPhone X build collision repair)
- **Result:** device-targeted Xcode build reached Unreal compilation but exposed
  unity-build collisions among file-local helpers. Renamed the interior,
  waterfront, wardrobe, and human-test helper symbols without changing runtime
  behavior.
- **Validation:** first signed iPhone build failed only on those compile errors;
  the repaired arm64 Development build passed with `xcodebuild`, the signed
  bundle installed as version `55116800.0.18`, and `devicectl` launched
  `com.sangolabs.connectedsprawl` on the connected `X iPhone`.
- **Status:** repair is uncommitted on `main`; no branch, staging, commit, push,
  or remote operation was performed. Generated build output remains ignored.
<!-- entry:iphone-x-build-collision-repair -->

### 2026-07-21 · main · codex (validated city/MetaHuman work published)
- **Published:** committed the accumulated validated city, character, vehicle,
  interiors/map, wardrobe, athletic-shoe, melee, parking, and iOS launch work as
  `009c659` (`Expand city systems and MetaHuman characters`) and pushed it to
  `origin/main`.
- **Assets:** Git LFS uploaded all 134 new resident, clothing, groom, material,
  and launch-image objects successfully (608 MB transferred). Generated build
  output remained ignored except the explicitly authored iOS launch graphic.
- **Checks before publication:** staged whitespace check and credential-pattern
  scans passed; the prior feature logs retain their exact editor, automation,
  runtime-audit, and visual-audit validation records.
- **Status:** published to `origin/main`; the main worktree was clean after the
  feature commit, and its completed Active Work row was removed here.
<!-- entry:validated-city-metahuman-work-published -->

### 2026-07-21 · main · codex (swappable athletic shoes + animated binding — validated)
- **Result:** new Blueprint-facing `USprawlAthleticShoeModule` develops four
  validated presets—Zarri Velocity, Metro Runner, Court High-Top, and Night
  Sprint—with stable IDs, shoe/sock types, coordinated colors, deterministic
  cycling, and a one-call runtime swap through `USprawlFootwearModule`.
- **Presentation/integration:** new `AthleticTrainers` geometry adds a cushioned
  last, layered sole, low flexible upper, laces/collar, twin support stripes,
  heel clip, and toe bumper. Wardrobe integration gives Zarri the navy/teal
  Zarri Velocity pair and allows other characters to use the same catalog.
- **Motion correction:** moving visual validation proved hidden MetaHuman foot
  sockets freeze. The final two-shoe post-animation follower uses the live
  calf/usable IK anchor for position and body-relative facing for orientation,
  so the pair follows running legs without detaching or pointing vertically.
- **Validation:** final UE 5.8 editor build PASS. Complete
  `ConnectedSprawl.Characters` automation PASS 7/7. Real-Metal 1280x720 moving
  wardrobe audit PASS at mean luma 105.81, 0.02% crushed, and 0.18% clipped;
  Zarri resolved 14.55 cm heel-to-ball chains into a 26.78 x 10.45 cm pair and
  screenshot inspection confirmed both trainers fitted at the running ankles.
- **Files:** new athletic-shoe module and focused test; footwear, wardrobe,
  wardrobe/footwear tests, Zarri moving visual-audit setup, README, GDD, city
  notes, and this ledger updated.
- **Remaining risk:** no cook, package, iPhone/device-profile run, physical
  input session, dedicated server/client session, on-device performance
  capture, or long traversal soak.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or remote operation was performed.
<!-- entry:swappable-athletic-shoes-validated -->

### 2026-07-21 · main · codex (fitted MetaHuman footwear repair — validated)
- **Result:** new Blueprint-facing `USprawlFootwearModule` measures each live
  MetaHuman foot/ball/calf chain and develops a human-scale shoe pair. Rounded
  uppers, layered soles, toe lift, collars/laces, fitted socks, style-specific
  proportions, and lit material layers replace the coarse four-station boxes.
- **Attachment fix:** current assemblies render bare feet in the main Body.
  Shoes now bind to an independent usable IK-foot or calf anchor before the
  Body foot bones are masked, eliminating skin overlap without scaling or
  displacing the new presentation. Clear/reapply restores all hidden state.
- **Validation:** final UE 5.8 editor build PASS. Complete
  `ConnectedSprawl.Characters` automation PASS 6/6, including the new footwear
  dimension contract. Real-Metal 1280x720 wardrobe audit PASS at mean luma
  106.77, 0.02% crushed, and 0.16% clipped; both Zarri sides resolved a 14.55 cm
  heel-to-ball chain into a 26.78 x 9.91 cm shoe and close inspection confirmed
  that bare feet no longer draw through it.
- **Corrected during validation:** one cap-normal visual run asserted because a
  `TArray` append referenced its own reallocating buffer. Copying the source
  vertex before growth removed the unsafe access; rebuild and visual rerun pass.
- **Remaining risk:** no cook, package, iPhone/device-profile run, physical
  input session, dedicated server/client session, on-device performance
  capture, or long walk/run animation soak.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or remote operation was performed.
<!-- entry:fitted-metahuman-footwear-validated -->

### 2026-07-21 · main · codex (deterministic MetaHuman wardrobe — validated)
- **Result:** new `USprawlWardrobeModule` develops a complete deterministic,
  Blueprint-facing outfit from each profile's broad style: top, bottom,
  optional outerwear, optional cap/beanie, five shoe types, three sock lengths,
  and a coordinated palette. The outfit is embedded in replicated human
  customization and applied to Zarri and every strict crowd MetaHuman.
- **Presentation:** fitted authored garments receive per-outfit palettes;
  low-cost procedural shoes, socks, and hats follow the MetaHuman skeleton.
  Underlying foot bones are masked to prevent skin clipping while absolute
  accessory scale preserves footwear. Outerwear uses fitted garment color and
  a chest accent; rejected rigid arm overlays are not shipped. Accessories
  have no collision, navigation, tick, decals, or shadows.
- **Assets and integration:** two project-owned, explicitly cooked wardrobe
  materials plus reproducible authoring script; ProceduralMeshComponent
  runtime dependency; Zarri, pedestrians, crowd assembly, game-mode visual
  audit, human validation, and authoring descriptions wired to the wardrobe
  contract.
- **Validation:** final UE 5.8 editor build PASS. Full
  `ConnectedSprawl.Characters` automation PASS 5/5 after correcting its
  footwear-description contract. Real-Metal 1280x720 wardrobe audit PASS at
  mean luma 104.67, 0.02% crushed, and 0.16% clipped; close inspection verified
  garment color, clean arms, masked feet, and shoe/sock attachment.
- **Remaining risk:** no cook, package, iPhone/device-profile run, physical
  touch session, dedicated server/client session, on-device performance
  capture, close hat-variant capture, or long walk/run animation soak.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or other remote operation was performed.
<!-- entry:deterministic-metahuman-wardrobe-validated -->

### 2026-07-21 · main · codex (real interior furniture and items — validated)
- **Result:** new `FSprawlInteriorPropLibrary` resolves ten textured Downtown
  West furniture/item mesh types into 77 fitted, grounded placements across
  Junction Market, Founder House Offices, and Canal View Condos. Repeated
  tables, chairs, bench, stock, planters, foliage, bins, and poster stands use
  one HISM per mesh while retaining authored materials.
- **Fallback and presentation:** the former oversized blocks are now segmented
  counters, open shelves, reception/storage, monitors, layered bed, upholstered
  sofa, and a cabinet/sink/cooktop kitchen. These modular fixtures remain if an
  optional mesh is absent. Large furniture owns collision/shadows; small stock
  stays non-colliding and shadow-free. Total interior instances rose from 60 to
  169 without changing portals, city geometry, map resolution, or controls.
- **Validation:** final UE 5.8 editor build PASS. Focused prop-library and
  enterable-layout automation PASS 1/1 each; all ten primary packages were
  found. Null-RHI InteriorsAudit PASS with 169 instances, 77 real props, all 10
  types, 3 buildings, 5 landmarks, real entry/containment/map/exit behavior,
  map lifecycle, and one M binding. Real-Metal 1280x720 store, office, and condo
  audits PASS their luma/clipping gates and visual inspection.
- **Files:** new interior-prop library and focused test; enterable-interiors
  layout/renderer and game-mode live/visual audit enhanced; README, GDD, city
  notes, and this ledger updated.
- **Remaining risk:** no cook, package, iPhone/device-profile run, physical
  touch session, dedicated server/client session, on-device performance
  capture, or long traversal soak. Downtown West remains the documented
  re-downloadable marketplace dependency; modular fixtures remain if its real
  prop meshes are unavailable, but the lower-detail fallback was not separately
  captured in this pass.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or other remote operation was performed.
<!-- entry:real-interior-furniture-validated -->

### 2026-07-21 · main · codex (enterable destinations + native city map — validated)
- **Result:** new `ASprawlEnterableInteriors` supplies signed sidewalk entrances
  and isolated enclosed interiors for Junction Market, Founder House Offices,
  and Canal View Condos. Five HISM groups render 60 themed room pieces, paired
  entry/exit transforms prevent immediate bounce-back, and existing authored
  city actors remain intact.
- **Interaction and map:** E/F and touch context input prioritize nearby
  building doors before cars. New `USprawlCityMapSubsystem` and
  `USprawlCityMapWidget` expose five landmarks, city projection, interior-to-
  exterior position resolution, Zarri's live marker, nearest distance, and a
  paused-safe M/MAP/Escape lifecycle.
- **Validation:** final UE 5.8 `ConnectedSprawlEditor Mac Development` build
  PASS. The first compile reached all production sources and failed only on two
  test-only float/double assertion overloads; those literals were corrected and
  the full rebuild passed. Focused interiors and map automation PASS 1/1 each.
  Null-RHI `-SprawlInteriorsAudit` PASS with 3 buildings, 60 instances, 5
  landmarks, real store entry, containment, door-map resolution, exit route,
  map open/close, and one M binding. Real-Metal 1280x720 map and store-interior
  captures passed their luma/clipping/readability gates and visual inspection.
- **Files:** new enterable-interiors actor/layout, city-map subsystem/widget,
  and focused tests; Zarri interaction, controller input, native HUD, game-mode
  setup/audit, README, GDD, city notes, and this ledger updated.
- **Remaining risk:** no cook, package, iPhone/device-profile run, physical
  touch session, dedicated server/client session, long traversal soak, or
  on-device performance capture. The live audit traversed the store; office and
  condo use the same validated portal contract but were not each manually
  walked in this pass.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or other remote operation was performed.
<!-- entry:enterable-destinations-city-map-validated -->

### 2026-07-21 · main · codex (Angry Cordero iOS startup branding — validated)
- **Result:** the standard Unreal iOS launch storyboard now resolves a
  project-owned 2048×2048 startup image that places the supplied Angry Cordero
  ram mark and Production Studio wordmark above the preserved Unreal Engine
  lockup. The composition stays centered on an opaque black square for iPhone
  and iPad orientation safety.
- **Validation:** rendered launch artwork inspection PASS; `file`, `sips`, and
  ImageMagick metadata checks PASS (2048×2048, 8-bit RGB/sRGB, no alpha);
  `xcrun pngcrush -n -q` PASS; Git's exclude-aware file listing finds the one
  authored launch image while other `Build/` output stays ignored. UE 5.8
  Xcode project source inspection confirms the project path is checked before
  the engine default. `ConnectedSprawlEditor Mac Development` build PASS
  (`Result: Succeeded`, target up to date).
- **Files:** new `Build/IOS/Resources/Graphics/LaunchScreenIOS.png`; narrow
  `.gitignore` exception for that authored asset; README and GDD startup-brand
  documentation; this ledger.
- **Remaining risk:** no packaged IPA, iOS Simulator/device launch, signing,
  startup-time measurement, or physical iPhone/iPad orientation sweep was run.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or other remote operation was performed.
<!-- entry:angry-cordero-ios-launch-validated -->

### 2026-07-21 · main · codex (reliable X punch/kick routing — validated)
- **Result:** keyboard X and the gamepad left-face/X button now bind once on
  Zarri's possessed pawn input component through new `FSprawlMeleeInput`, above
  the controller layer that Enhanced Input could shadow. Mouse recapture/melee
  and touch stay controller-owned; every path reaches the same alternating
  replicated melee module and shows short PUNCH/KICK confirmation.
- **Validation:** final UE 5.8 `ConnectedSprawlEditor Mac Development` build
  PASS. Focused melee automation PASS 1/1 and final complete
  `ConnectedSprawl.Characters` PASS 4/4. The new headless
  `-SprawlMeleeInputAudit` PASS inspected Zarri's live possessed input component
  (`keyboard_x=1`, `gamepad_x=1`), invoked the X handler, advanced revision to
  1 with Punch active, and confirmed Kick next. Full-Metal gameplay also
  launched and rendered the on-foot melee HUD.
- **Files:** new melee input module; Zarri/controller routing and live audit;
  expanded melee automation; README, GDD, city notes, and this ledger.
- **Remaining risk:** no packaged build, iPhone/device-profile run, physical
  gamepad/touch-device session, dedicated client/server session, or long combat
  soak. The desktop accessibility harness did not deliver a trustworthy raw
  keyboard event before its game window closed, so acceptance uses Unreal's
  deterministic possessed-input audit instead.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or other remote operation was performed.
<!-- entry:reliable-melee-input-validated -->

### 2026-07-21 · main · codex (reliable X punch/kick routing — validation pending)
- **Input result:** new `FSprawlMeleeInput` binds keyboard X and gamepad
  left-face/X directly on Zarri's possessed pawn input component, eliminating
  the lower-priority controller route that Enhanced Input could shadow.
  Controller-owned left mouse still handles cursor recapture and melee; the
  existing touch button remains routed through the controller.
- **Feedback and tests:** successful shared actions emit a `[MeleeInput]` trace
  and short PUNCH/KICK on-screen confirmation. The melee automation contract
  now covers the exact direct-key set and rejects an unrelated movement key.
- **Files:** new melee input module; Zarri/controller routing; melee automation;
  README, GDD, city notes, and this ledger.
- **Validation:** pending final editor build, focused automation, and live X-key
  smoke test.
- **Status:** implementation complete and uncommitted on `main`. No branch,
  worktree, staging, commit, push, or other remote operation was performed.
<!-- entry:reliable-melee-input-pending -->

### 2026-07-21 · main · codex (strict MetaHuman ambient residents — validated)
- **Character result:** new `FSprawlCrowdMetaHuman` maps replicated human
  customization onto a strict Zarri/Amina/Andre roster with distinct face
  packages, deterministic uniform stature and wardrobe copies, and shared
  MetaHuman Stand/Walk/Run locomotion. `/Game/Pedestrians` and the mannequin are
  no longer runtime crowd fallbacks. Mac budgets 8 residents / 3 near LODs;
  iPhone budgets 3 / 1.
- **Runtime and network result:** the population manager guarantees one complete
  roster cycle, owns spawning only on standalone/server net modes, and lets
  each rendering client select LOD across replicated pedestrians. Dedicated
  servers skip visual loads; Optimized/Low assemblies select near LOD 0 and
  their actual farthest LOD instead of collapsing two-LOD residents to one
  presentation. First-use assembly work is staggered to one new resident per
  population pass, and ejected drivers resolve their stable logical variant to
  the same requested/rendered roster identity without an intermediate load.
- **Authored content:** `create_city_metahuman_residents.py` generated and saved
  `/Game/MetaHumans/Residents/Amina`, `/Andre`, their source characters, and
  required project-owned groom/clothing dependencies. Full Editor authoring
  logged both verified runtime classes and PASS; commandlet mode is invalid for
  this operation because UE 5.8 TextureGraph baking is editor-initialized. The
  script now stages and validates future rebuilds before promotion.
- **Validation:** final UE 5.8 Editor target build PASS. Complete
  `ConnectedSprawl.Characters` automation PASS 4/4. Strict 30-second
  TrafficAudit PASS with 8 pedestrians / 8 MetaHumans / 3 distinct identities /
  0 non-MetaHumans, 14 moving cars, 13 signal stops, 14 hidden drivers, and zero
  traffic, visibility, or sidewalk violations. Final CarjackAudit PASS with a
  matching real-human ejected driver, legal garage backfill, and 14/14 traffic.
  Full-Metal gameplay launched; the close view resolved body, clothing, and
  hair after shader warm-up.
- **Files:** new crowd module/test and resident generator; generated Amina/Andre
  source/runtime assets plus owned dependencies; human customization,
  pedestrian, crowd manager, traffic audit, cook configuration, README, GDD,
  city notes, and this ledger updated.
- **Remaining risk:** no cook, package, iPhone/device-profile/on-device capture,
  dedicated server/client session, or long crowd soak. A single first-use class
  load can still hitch before caching, though loads no longer batch in one
  population frame. The broad Common force-cook was removed so resident hard
  references pull only used dependencies, but cooked size is unmeasured. The
  generator's new staging promotion path is syntax-checked but was added after
  the successful asset run.
- **Status:** validated and uncommitted on `main`. No branch, worktree, staging,
  commit, push, or other remote operation was performed.
<!-- entry:strict-metahuman-ambient-residents-validated -->

### 2026-07-21 · main · codex (parking deck + garage traffic origins — validated)
- **Building and site result:** the runtime deck reports four levels, two valid
  exits, 121 instanced pieces, 83 cleared elevated lot/corridor actors, and one
  conflicting authored car preserved in an upper-deck bay. Ground-level city,
  street, sidewalk, and thin service surfaces remain intact.
- **Garage-only traffic result:** replacement cars no longer materialize in a
  lane. Full-path occupancy gates at most one covered departure per manager
  pass; the car follows low-speed waypoints and performs a swept legal merge
  before existing directed, signal-aware traffic AI resumes. The managed cull
  radius spans the city diagonal so a cross-city player does not immediately
  retire a newly merged replacement; the target count remains 14.
- **Validation:** final UE 5.8 editor build PASS; focused
  `ConnectedSprawl.World.ParkingGarageLayout` automation PASS 1/1. CarjackAudit
  PASS with an observed `NorthWestbound` garage spawn, road-3 merge, 14/14
  target recovery, and zero pending departures. Standalone TrafficAudit PASS:
  14 movers, 51 total / 37 enterable cars, 12 signal stops, 359.5 cm minimum
  spacing, 26 pedestrians, 14 hidden drivers, and zero lane, wrong-way,
  boundary, upright, visibility, or intersection violations. A 1600x900
  real-Metal garage capture passed its visual gates (luma 108.27, red/blue
  1.127) and shows the deck, signed portal, sidewalk, and road connection.
- **Remaining validation:** no cook, package, iPhone/device-profile run,
  on-device performance capture, or long interactive traffic soak was run.
- **Status:** validated and uncommitted on `main`. New parking-garage module and
  focused test; edits to traffic manager, car, game mode, README, GDD, city
  notes, and this ledger. No map, Blueprint, material, binary asset, branch,
  worktree, staging, commit, or remote operation was performed.
<!-- entry:parking-garage-traffic-validated -->

### 2026-07-21 · main · codex (parking deck + garage traffic origins — validation pending)
- **New building module:** `ASprawlParkingGarage` procedurally replaces one
  central lot with a four-level instanced deck containing concrete structure,
  asphalt slabs, ramps, barriers, marked bays, static parked-car silhouettes,
  warm lights, ventilation detail, a parking sign, and two covered exits. Site
  preparation preserves the block/sidewalk surfaces and hides only taller
  authored geometry intersecting the exact garage footprint.
- **No more lane-pop replenishment:** the traffic manager now validates an
  entire garage exit corridor, emits no more than one replacement per pass,
  waits when the route/player reveal area is occupied, and has no active-lane
  spawn fallback. Cars use a new obstruction-aware low-speed egress phase and
  seed their existing directed, signal-aware lane state only after a swept
  legal merge completes.
- **Regression scope:** map-independent automation covers deck/detail counts,
  unique off-road birth points, exact right-hand merge lanes, and dry forward
  routes. Editor build, focused automation, live garage departure, TrafficAudit,
  and documentation validation remain pending.
- **Status:** implementation in progress and uncommitted on `main`. No map,
  Blueprint, material, binary asset, branch, worktree, staging, commit, or
  remote operation has been performed for this pass.
<!-- entry:parking-garage-traffic-pending -->

### 2026-07-21 · main · codex (Zarri punch/kick module — validated)
- **Build and automation:** `ConnectedSprawlEditor Mac Development` build PASS.
  The focused `ConnectedSprawl.Characters.MeleeModule` test passed 1/1, then
  the complete `ConnectedSprawl.Characters` suite passed 3/3 (Character
  Developer, Human Character, and Melee). This exercises attack balance/spec
  validation, forward-cone/reach/height rejection, alternation, replication
  defaults, and safe rejection without an owner.
- **Runtime safety:** final 30-second null-RHI TrafficAudit PASS with 14/14
  moving traffic cars, 51 total / 37 enterable cars, 12 signal stops, 14 wheel
  animations, 359.5 cm minimum spacing, 26 pedestrians / 26 real avatars, 14
  hidden drivers, and zero visible/missing driver, pedestrian-offside, lane,
  wrong-way, boundary, upright, or intersection violations.
- **Remaining validation:** no cook, package, iPhone device-profile run,
  on-device performance capture, or hands-on combat playback was performed.
  No skeleton-compatible punch/kick clips exist in the project or installed
  MetaHuman content, so the optional one-shot bridge compiled but remains
  visually dormant until art is assigned; damage, targeting, reaction, and all
  input paths remain asset-independent.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  binary asset, branch, worktree, staging, commit, or remote operation was
  performed for this pass.
<!-- entry:zarri-melee-module-validated -->

### 2026-07-21 · main · codex (Zarri punch/kick module — validation pending)
- **New combat module:** `USprawlMeleeModule` supplies explicit Punch/Kick calls
  plus a one-button alternating action. Authority-owned, replicated state
  drives a validated short-range cone, cooldown/recovery timing, standard point
  damage, modest knockback, and the existing pedestrian flee response. Hidden
  in-car Zarri cannot attack, and only one visible character can be hit.
- **Input and presentation:** left mouse, X, and the gamepad left face button
  route through the persistent player controller. The native mobile HUD gains
  one PUNCH/KICK button that disappears while driving. Locomotion now supports
  optional bounded one-shots for assigned lightweight or MetaHuman combat
  clips; no such binary assets exist locally, so missing clips preserve the
  working body and gameplay instead of forcing an incompatible animation.
- **Regression scope:** map-independent automation covers default attack
  balance, spec validation, cone/reach/height rejection, alternation,
  replication defaults, and rejected unowned attacks. Editor build, focused
  character automation, and living-city runtime audit remain pending.
- **Status:** implementation in progress and uncommitted on `main`. No map,
  Blueprint, material, binary asset, branch, worktree, staging, commit, or
  remote operation has been performed for this pass.
<!-- entry:zarri-melee-module-pending -->

### 2026-07-20 · main · codex (replicated diverse human characters — validated)
- **Build and focused automation:** final `ConnectedSprawlEditor Mac
  Development` build PASS. `ConnectedSprawl.Characters` found and passed 2/2
  tests: the prior Character Developer suite plus the new Human Character
  module suite. The latter covers all customization ranges, deterministic
  diversity, exact Zarri profile validity, six actions, held poses, revisions,
  replication defaults, and MetaHuman/fallback animation selection.
- **Runtime safety:** final 30-second null-RHI TrafficAudit PASS with 14/14
  traffic cars moving, 51 total / 37 enterable cars, 13 signal stops, 14 wheel
  animations, 344.6 cm minimum spacing, 26 pedestrians, 26 real avatars, 14
  hidden drivers, and zero visible/missing driver, pedestrian-offside, lane,
  wrong-way, boundary, upright, or intersection violations.
- **Remaining validation:** no cook, package, iPhone device-profile run, or
  on-device performance capture. Unique assembled MetaHuman assets beyond the
  existing Zarri must still be created/reviewed in Creator and assigned through
  `USprawlCharacterDefinition`; ordinary crowds intentionally retain the
  lightweight mobile avatar tier.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  binary asset, branch, worktree, staging, commit, or remote operation was
  performed for this pass.
<!-- entry:replicated-diverse-human-characters-validated -->

### 2026-07-20 · main · codex (replicated diverse human characters — validation pending)
- **New human module:** `USprawlHumanCharacterModule` holds compact replicated
  identity/customization plus Stand/Walk/Run/Talk/Sit/Drive state. It derives
  deterministic age, presentation, build, skin-tone direction, hair texture,
  wardrobe, stature, and safe fallback avatar data from each developed profile.
  Zarri is a technical joints-only rig/action baseline, never a cloned face.
- **Runtime integration:** Zarri and every pedestrian now own the same module.
  Lightweight crowds play optional Talk/Sit action clips without weakening the
  existing Idle/Walk/Jog completeness gate; named definitions gain soft
  MetaHuman talk/sit/drive bindings. Zarri records Drive while possessed, but
  the hidden in-car driver policy remains authoritative for rendering and tick.
- **Regression scope:** new automation covers deterministic population
  diversity, customization validation, all six action transitions, held poses,
  revision changes, component replication defaults, fallback clip mapping, and
  named MetaHuman drive-to-sit fallback. Build and focused automation remain
  pending.
- **Status:** implementation in progress and uncommitted on `main`. No map,
  Blueprint, material, binary asset, branch, worktree, staging, commit, or
  remote operation has been performed for this pass.
<!-- entry:replicated-diverse-human-characters-pending -->

### 2026-07-20 · main · codex (sidewalk signs + complete signals — validated)
- **Validation:** `ConnectedSprawlEditor Mac Development` build PASS, including
  the new signal model, street-dressing module, and expanded kerb-placement
  automation source. The two changed city-authoring scripts passed AST syntax
  validation, and `git diff --check` PASS.
- **Focused automation limitation:** the `ConnectedSprawl.World.KerbPlacement`
  command was submitted, but the headless launcher exited after platform
  validation before the editor opened because the interactive Unreal instance
  remains active. No test assertion or game-code failure was reported.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  binary asset, branch, worktree, staging, commit, or remote operation was
  performed for this pass.
<!-- entry:sidewalk-signals-validated -->

### 2026-07-20 · main · codex (sidewalk signs + complete signals — validation pending)
- **Sidewalk contract:** expanded the shared kerb-placement module with an
  explicit pedestrian-band predicate. Freestanding foldout signs now repair
  whenever they are anywhere outside that band, including parking bays and
  road-free asphalt that the former lane-only check missed.
- **Intersection model:** replaced the single ambiguous pole with one
  low-poly, four-corner, two-axis, three-lens `ASprawlTrafficLight` model per
  dry intersection. Street dressing recentres legacy instances and creates a
  missing model at runtime, while the actor continues to source every phase
  from `USprawlCityGridSubsystem`.
- **Authoring and docs:** both living-city scripts now place a signal actor at
  its intersection centre, and the map-independent runtime model lays the
  poles out on sidewalks. C++ build and focused world tests remain pending.
- **Status:** implementation in progress and uncommitted on `main`. No map,
  Blueprint, material, binary asset, branch, worktree, staging, commit, or
  remote operation has been performed for this pass.
<!-- entry:sidewalk-signals-pending -->

### 2026-07-20 · main · codex (vehicle visual-forward repair — validated)
- **Validation:** `ConnectedSprawlEditor Mac Development` rebuild PASS after
  compiling the changed car, visual-forward module, and expanded traffic-rule
  automation source. `vehicle_realism.py` AST syntax check PASS and
  `git diff --check` PASS.
- **Focused automation limitation:** the `ConnectedSprawl.Vehicles.VisualForward.NamedFrontAxle`
  command was submitted twice, but the headless launcher exited after platform
  validation before opening the editor because an interactive Unreal instance
  was already active. No test assertion or game-code failure was reported; the
  newly compiled automation will run on the next editor-safe test pass.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  binary asset, branch, worktree, staging, commit, or remote operation was
  performed for this repair.
<!-- entry:vehicle-visual-forward-repair-validated -->

### 2026-07-20 · main · codex (vehicle visual-forward repair — validation pending)
- **Root cause:** vehicle physics already treats pawn +X as forward, and the
  shared input action maps W/Up to positive throttle. The legacy importer used
  a +90° Blender Y-forward visual transform, which turns a visible vehicle nose
  toward physics -X and makes correct forward motion look like reversing.
- **Module update:** `FSprawlVehicleVisualForward` now resolves a complete
  rotation from named FL/FR/RL/RR axle centers while retaining authored pitch
  and roll. It also exposes the documented Blender Y-forward fallback.
  `ASprawlCar` uses the named-axle result for all newly installed split kits and
  corrects serialized animated or one-piece imported car visuals in `BeginPlay`
  without changing the hull, camera, AI route, throttle, or reverse/brake
  contract.
- **Editor tooling / regression:** `vehicle_realism.py` now uses -90° for the
  source convention instead of +90°, and the vehicle automation covers the
  stale +90° correction plus pitch/roll preservation. Build and focused
  automation are still pending for this entry.
- **Status:** implementation in progress and uncommitted on `main`. No map,
  Blueprint, material, binary asset, branch, worktree, staging, commit, or
  remote operation has been performed for this repair.
<!-- entry:vehicle-visual-forward-repair-pending -->

### 2026-07-20 · main · codex (hidden in-car drivers — validated)
- **New `FSprawlDriverVisibility` module:** treats occupancy and rendering as
  separate concerns. A seated AI/player driver remains a logical occupant but
  the legacy car-mounted skeletal mesh is hidden, cleared, and unticked. This
  removes the roof-clipping failure shown in the supplied city screenshot and
  avoids a skeletal avatar load/tick per active traffic car.
- **Vehicle integration:** `ASprawlCar` enforces suppression before the first
  rendered frame and through enter, exit, carjack, restore, and hot-reload
  paths. AI drivers retain a deterministic variant string so an ejected driver
  can still spawn as the same visible exterior pedestrian. Parked-car access,
  possession, save state, physical driving, and pedestrian visuals are not
  coupled to the mounted component.
- **Regression contract:** TrafficAudit now requires hidden logical drivers,
  fails on any visible mounted driver, and reports missing hidden state after
  warmup. CarjackAudit selects a hidden logical occupant rather than requiring
  a visible mesh. New deterministic automation covers empty and seated policy
  decisions, including no avatar load and no pose tick.
- **Validation:** `git diff --check` PASS before docs; final editor build PASS;
  `ConnectedSprawl.Vehicles.DriverVisibility` 1/1 PASS; 30-second standalone
  TrafficAudit PASS (`cars=14`, `total_cars=51`, `moved=14`, `signal_stops=13`,
  `min_spacing=368.0`, `hidden_drivers=14`, `visible_driver_violations=0`,
  `missing_hidden_drivers=0`, `pedestrians=26`, `real_avatars=26`, every other
  violation count zero).
- **Unavailable validation / risk:** retries of the broader vehicle automation
  suite and standalone CarjackAudit were blocked before engine initialization
  by macOS LaunchServices `Connection invalid`; no game-code assertion or test
  failure was reported. The focused test and full runtime traffic audit passed,
  and the updated carjack selection path compiled, but that transition was not
  re-exercised in this run. No cook, package, iPhone, or sustained device
  performance capture.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  binary asset, branch, worktree, staging, commit, or remote operation was
  performed. Two stale unattended Unreal automation processes were terminated;
  no interactive game process was targeted.
<!-- entry:hidden-in-car-drivers-validated -->

### 2026-07-20 · main · codex (Hugging Face Character Developer — validated)
- **Character contract:** new Blueprint-facing `FSprawlCharacterProfile`,
  `USprawlCharacterDeveloper`, and `USprawlCharacterDefinition` generate and
  validate stable names, districts, work/home roles, schedules, live activity,
  destination, wardrobe, stature/gait, behavior chances, lightweight avatar,
  MetaHuman Creator brief, reference prompt, and optional soft
  MetaHuman/locomotion bindings. Generated crowd height remains inside the
  existing capsule; named Data Assets may use the wider validated range.
- **Living-city integration:** the crowd manager now develops each profile
  before `BeginPlay`, and pedestrians consume its avatar/gait/stature/crossing/
  phone-idle values. The Junction keeps the 26-person cap while Iron Forest,
  Rail Yards, arteries, and late-night hours thin below it. Existing movement,
  crossings, flee recovery, complete-art checks, and mannequin fallback remain.
- **Hugging Face authoring:** new standard-library-only
  `Tools/CharacterDeveloper/character_developer.py` calls the inference router
  only when `HF_TOKEN` is present, defaults to the Apache-2.0 endpoint-compatible
  `Qwen/Qwen3.5-4B`, records an Apache-2.0 `FLUX.1-schnell` reference prompt,
  supports model override, validates strict JSON, and has deterministic offline
  mode. No token/model/network code ships in Unreal. The local `hf` executable
  was unavailable, so current public model metadata was verified through the
  Hugging Face Hub API instead; no remote inference was attempted without a
  user token.
- **Build resilience:** the added translation unit changed unity grouping and
  exposed a pre-existing duplicate `BayGapMinX/MaxX` pair in gates/skyline.
  Gate constants now have unique names; values and gate layout are unchanged.
- **Validation:** Python `unittest` 3/3 PASS; offline CLI sample PASS;
  `git diff --check` PASS; final `ConnectedSprawlEditor Mac Development` build
  PASS; `ConnectedSprawl.Characters.CharacterDeveloper` PASS; 30-second null-RHI
  TrafficAudit PASS (`cars=14`, `total=51`, `signal_stops=13`,
  `min_spacing=344.6`, `pedestrians=26`, `real_avatars=26`, all violation counts
  zero). The first build failed on the new single-result iterator warning and
  the existing unity-name collision; both were fixed before the clean rebuild.
- **Risk / remaining validation:** no cook, package, iPhone device profile, or
  on-device performance capture; district density was exercised at the default
  Junction start, while all district/hour multipliers are deterministic-tested.
  Cloud output still requires human review before creating a MetaHuman asset.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  binary asset, branch, worktree, staging, commit, or remote operation was
  performed.
<!-- entry:hugging-face-character-developer-validated -->

### 2026-07-20 · main · claude (Git LFS configured — closes the standing warning)
- **LFS live (user-authorized):** `git lfs install` wrote its four hooks into
  the repo's tracked `.githooks/` (core.hooksPath) beside the untouched ledger
  pre-commit, so every clone gets them. New root `.gitattributes` tracks
  `*.uasset`/`*.umap` plus binary source art (fbx/abc/png/tga/exr/hdr/psd) and
  audio (wav/mp3/ogg). Existing files migrate lazily — each converts the next
  time it is re-staged; history was NOT rewritten (old blobs remain in git).
- **Three oversized files converted at tip** (all were past GitHub's 50 MB
  warning band; the largest within 3 MB of the 100 MB hard block):
  `MHC_Zarri.uasset` 97.5 MB, `jacaranda_tree_2k.uasset` 91.3 MB,
  `T_Body_N.uasset` 62.5 MB — now LFS pointers, ~251 MB uploaded to LFS
  storage on push. README gains a `git lfs install` setup step 0.
- **Quota note:** GitHub free LFS is 1 GiB storage / 1 GiB-month bandwidth.
  Current usage ~251 MB. As python passes re-save uassets they migrate to LFS
  and storage grows with churn — watch the quota; data packs are $5/50 GB.
- **Validation:** `git lfs status`/`ls-files` confirm the three pointers;
  hooks verified in `.githooks/`; ledger pre-commit intact. No engine build
  required (no source change).
- **Status:** committed and pushed to origin/main (user-authorized).
<!-- entry:git-lfs-configured -->

### 2026-07-20 · main · claude (city gates, real skyline, full mountain ring — validated)
- **Root cause of the flat-grey outer buildings found and fixed:** the skyline
  facade MIs were bound correctly all along, but their masters lacked
  `bUsedWithInstancedStaticMeshes`, so Unreal substituted the default grey on
  every HISM — the exact failure class the ledger has warned about via
  `MI_RoadPaint` for weeks. New `Content/Python/fix_ism_usage_flags.py`
  resolves each instance to its base and flags + saves the masters:
  `M_RealFacade2`, `M_RealGround`, and `M_RoadWhite` (all were False). This
  also finally clears the road-paint substitution on the instanced stripes.
  Belt-and-braces rebinding + per-facade logging added to
  `ASprawlCitySkyline::BuildRing`, plus dark rooftop-mechanical crowns on
  towers over 60 m (`CrownTransforms`, rendered via the roof HISM).
- **New `ASprawlCityGates` module:** every crossing road now ends at a
  city-limit gate — two concrete pillars carrying a beam over the carriageway
  with flanking wall stubs, wearing the kit concrete (tint fallback). 19 gates
  / 95 instanced pieces: 6 north, 3 south (bay skipped), 6 west, 4 east
  (lake's east face skipped). Deterministic `FSprawlCityGateLayout` with
  `ConnectedSprawl.World.CityGateLayout` (count, piece structure, bay
  exclusion, beam clearance >5 m). Pillars block like street furniture.
- **Mountain ring completed:** `FSprawlWaterfrontSceneryLayout` now rings all
  four horizons (8 ranges + 11 foothills; north/west added) so no outward
  view ends in empty sky. The lake remains the south-east bay by design —
  from north/west edges you see mountains, not water. Layout test updated.
- **Validation:** editor build clean; `ConnectedSprawl.World` 5/5 (gate test
  included). Real-Metal `-SprawlAuditEdge` capture down a west street: the
  concrete gate frames the mouth, the skyline behind it now shows real brick /
  glass-grid / pale facades, and the new west ridge is silhouetted through the
  arch. Engine "missing usage flag" warnings dropped 6 -> 1 in the capture log.
- **Risk / remaining validation:** no cook/device/perf profile. The flag edit
  re-saves three authored material masters (binary uassets — intentional,
  user-visible in the diff). One residual usage-flag warning remains to chase.
  Gate beams assume <=5 m vehicles; nothing taller exists today.
- **Status:** validated and uncommitted on `main`. New files:
  `Public/World/SprawlCityGates.h`, `Private/World/SprawlCityGates.cpp`,
  `Private/Tests/SprawlCityGatesTests.cpp`,
  `Content/Python/fix_ism_usage_flags.py`; modified masters
  `Content/RealKit/Materials/M_RealFacade2.uasset`, `M_RealGround.uasset`,
  `Content/CityArt/M_RoadWhite.uasset` (usage flag only). Edits:
  `SprawlCitySkyline.{h,cpp}`, `SprawlWaterfrontScenery.cpp`,
  `SprawlGameMode.cpp`, both scenery/skyline test files. No branch, worktree,
  commit, or remote operation performed.
<!-- entry:city-gates-real-skyline-mountain-ring-validated -->

### 2026-07-20 · main · claude (drive verification, headroom, street dressing — validated)
- **Enter/drive/reverse verified, not rebuilt:** the existing modules already
  own this (`E`/`F` -> `EnterNearbyVehicle` -> possession;
  `FSprawlVehicleDriveLogic` forward force + brake-before-reverse). CarjackAudit
  re-run PASSED (possessed/fleeing_driver/lease_free/retired/backfilled all 1),
  so no duplicate drive module was written. Discoverability was the real gap:
  `USprawlNativeHUD` now shows a desktop-only contextual hint — "E — enter car
  and drive" near an enterable car, "W accelerate · S brake/reverse · E exit"
  while driving (touch keeps its real buttons; new public
  `AZarriCharacter::HasNearbyEnterableVehicle`).
- **New `FSprawlOccupantHeadroom` module:** the pelvis clamp had no head term,
  so seated drivers' heads could breach low rooflines. The fit sinks the seat
  within the containment volume first, then applies a bounded uniform shrink
  (floor 0.80), and refuses the visual when even that cannot contain it.
  Wired into `ASprawlCar::ApplySeatedDriverVariant`; covered by
  `ConnectedSprawl.Vehicles.OccupantHeadroom` (tall cabin unchanged, low coupe
  sunk-and-contained, uninhabitable body refused).
- **New `ASprawlStreetDressing` module:** re-seats provably-wrong street
  furniture by static-mesh identity (labels are editor-only, so cooked-safe):
  A-board `sign_foldout_*` onto the kerb band facing traffic; stranded
  junction dressing (food cart, fountain, light-signs, poster pillar) off
  non-park block centres onto their junction corners (parks keep their plaza
  dressing); wall shop signs pulled out of the carriageway; and **every
  `ASprawlTrafficLight` pole snapped to its sidewalk corner at the live
  1200 cm road width** — first run logged `foldout_signs=6
  stranded_junction_props=4 wall_signs_off_road=1 signal_poles_snapped=30`,
  confirming all 30 poles still stood at the pre-widening road edge. Pure
  placement math is unit-tested (`ConnectedSprawl.World.KerbPlacement`);
  phase logic untouched (indices snap identically). Added
  `-SprawlAuditJunction` vantage.
- **Validation:** editor build clean; automation 7/7 across
  `ConnectedSprawl.Vehicles` + `ConnectedSprawl.World` (one stale skyline test
  bound aligned to the tuned 2400-4400 near-row band). Real-Metal TrafficAudit
  PASS with every gate green (14/14 movers, 12 signal stops at the snapped
  poles, visible_drivers=14, outside_drivers=0, zero lane/wrong-way/boundary
  violations, box occupancy 1). CarjackAudit PASS. Junction capture shows
  crosswalks, corner islands, kerbside cars, and the corner signal pole.
- **Risk / remaining validation:** no cook/device/perf profile. The headroom
  shrink floor (0.80) and seated-head fraction (0.40) are estimates validated
  by audit + tests, not by an in-cabin camera pass; a close-up drive-by eyeball
  in play is the remaining check. Dressing moves are runtime-only (map
  untouched) and idempotent.
- **Status:** validated and uncommitted on `main`. New files:
  `Public/Vehicles/SprawlOccupantHeadroom.h`,
  `Private/Vehicles/SprawlOccupantHeadroom.cpp`,
  `Public/World/SprawlStreetDressing.h`,
  `Private/World/SprawlStreetDressing.cpp`,
  `Private/Tests/SprawlOccupantHeadroomTests.cpp`,
  `Private/Tests/SprawlStreetDressingTests.cpp`. Edits: `SprawlCar.cpp`,
  `SprawlGameMode.cpp`, `SprawlNativeHUD.{h,cpp}`, `ZarriCharacter.h`,
  `SprawlCitySkylineTests.cpp`. No branch/worktree/commit/remote operation
  performed.
<!-- entry:drive-headroom-street-dressing-validated -->

### 2026-07-20 · main · claude (skyline ring + Blender mountains — validated)
- **New Blender module `Tools/build_mountain_kit.py`:** two clean flat-shaded
  ridges (`SM_MountainRidgeA` 400x80 m/1,957 tris, `SM_MountainRidgeB`
  280x60 m/1,728 tris) — gaussian peak envelopes x cosine cross-falloff +
  Perlin fBm, collapse-decimated, with rock/scree/snow CORNER vertex colours
  (snow gated by altitude + face slope). New
  `Content/Python/import_mountain_kit.py` imported both (bounds verified) and
  built `M_MountainRock` (VertexColor->BaseColor, roughness 0.95, zero
  textures). `ASprawlWaterfrontScenery` prefers the ridges for both mountain
  and foothill HISMs — the pixelated marketplace vista mesh stays hidden.
- **New module `ASprawlCitySkyline`:** deterministic `FSprawlSkylineLayout`
  (unit-tested) rings the city with 113 low-poly proxy buildings + roof caps —
  two rows (mid-rise near, towers behind) starting 30 m past the boundary,
  wearing the same world-projected facade MIs as the real city, with a gap
  across the lake's south face so the bay still opens to the mountains. A
  four-slab ground apron runs from the city floor's edge (11,400) out past the
  far row (20,400), so the towers stand on land. Spawned by the game mode; no
  tick/collision/shadows. New test `ConnectedSprawl.World.CitySkylineLayout`.
- **Legacy edge cards hidden:** the four ~95 m planes floating at Z=+1050
  outside the city (the "edge of the world" horizon) are now hidden by
  `ASprawlSurfaceRepair` instead of repainted — any repaint left them as a
  white shelf slicing through the skyline. Added `-SprawlAuditEdge` vantage.
- **Validation:** editor build clean; `ConnectedSprawl.World` automation 3/3
  (new skyline test included). Real-Metal captures: the west-perimeter view
  shows continuous ground to grounded towers framed by real city brick (no
  void, no white shelf), and the waterfront shows rippled water -> quay ->
  dark far shore -> skyline -> clean faceted snow-capped ridges. The elevated
  edge capture passed the full visual gate (luma 112.9, red_blue 1.11).
- **Risk / remaining validation:** no cook/device/perf profile. Proxy facades
  read flat on their shadow side up close — acceptable at distance; a window
  emissive pass at night is a natural follow-up. Skyline adds ~113 instanced
  cubes + 4 slabs; mountains total ~25k background tris (textureless).
- **Status:** validated and uncommitted on `main`. New files:
  `Tools/build_mountain_kit.py`, `Content/Python/import_mountain_kit.py`,
  `Public/World/SprawlCitySkyline.h`, `Private/World/SprawlCitySkyline.cpp`,
  `Private/Tests/SprawlCitySkylineTests.cpp`, 3 kit uassets + 2 FBX. Edits:
  `SprawlSurfaceRepair.cpp`, `SprawlWaterfrontScenery.cpp`,
  `SprawlGameMode.cpp`. No branch/worktree/commit/remote operation performed.
<!-- entry:skyline-ring-blender-mountains-validated -->

### 2026-07-20 · main · claude (Blender authentic-city kit — validated)
- **New Blender authoring module `Tools/build_city_kit.py`** (headless Blender
  5.1, deterministic seed): bakes three seamless 1K PBR sets — worn street
  asphalt with region-gated crack networks, jointed quay concrete, patchy grass
  field (colour/roughness via 1-sample emit bakes, tangent normals from
  procedural height; seamlessness via 4D-torus-mapped noise so the
  world-projected tiling never seams) — and models `SM_GrassClump` (14 bent
  tapered solid blades, 126 tris, root->tip vertex-colour ramp) plus
  `SM_WaterSurface` (96x64 m, 19,200 tris, centred pivot). Outputs under
  `Content/Import/CityKit/` with a report.
- **New import module `Content/Python/import_city_kit.py`** (UE 5.8
  pythonscript commandlet, sidecar report): imported 9 textures (normals
  TC_Normalmap, roughness linear masks), both meshes (bounds verified in the
  report: water exactly 4800x3200 cm half-extent, clump 36 cm tall), created
  `MI_AsphaltWorn`/`MI_ConcreteWorn`/`MI_GrassField` on the world-projected
  `M_RealGround` master (live param names verified: AlbedoTex/NormalTex/
  RoughTex/AOTex) and a `M_GrassBlade` material driven by the clump's vertex
  colours. `mobile_texture_budget.py` re-run after import (1K maps, no clamps).
- **Runtime consumption (streets · grass · water):** `ASprawlSurfaceRepair`
  and `ASprawlLakeBasin` now bind the textured MIs (worn asphalt for
  ground/ring, jointed concrete for the promenade band and quay walls) with
  the flat-tint MIDs kept as last-resort fallbacks; `ASprawlGroundCover`
  replaces its squashed engine cones with uniform-scaled grass clumps
  (GrassPerPark 1400 -> 800 since each clump carries 14 blades);
  `ASprawlWaterfrontScenery` prefers the centred high-tessellation water mesh
  so the GPU waves visibly ripple. Added `-SprawlAuditPark` capture vantage.
- **Validation:** Blender bake + export clean (report in Content/Import/
  CityKit/). UE import PASS with live param introspection. Editor build clean;
  `ConnectedSprawl.World` automation 2/2. Real-Metal captures at the
  waterfront, park, and boulevard show cracked worn asphalt streets, a lawn of
  readable 3D grass clumps around the fountain plaza, and finely rippled water
  in the sunken basin; `[SurfaceRepair] remaining large checker/missing = 0`
  still holds. Visual-audit warmth/luma gates miss on the water/green-heavy
  frames as before (crushed 0.00-0.01%) — judged by the saved PNGs.
- **Risk / remaining validation:** no cook, package, iPhone/device, or
  sustained performance profile. Grass worst case ~300k tris across 3 parks
  before the 140 m cull — profile on device before shipping. The park/water
  frames still read cool against the audit warmth gate. Kit assets are
  tracked source; re-running the Blender + import modules regenerates them
  deterministically.
- **Status:** validated and uncommitted on `main`. New files:
  `Tools/build_city_kit.py`, `Content/Python/import_city_kit.py`, 15 uassets +
  9 PNGs + 2 FBX under `Content/Import/CityKit/`. Edits:
  `SprawlGroundCover.{h,cpp}`, `SprawlSurfaceRepair.{h,cpp}`,
  `SprawlLakeBasin.{h,cpp}`, `SprawlWaterfrontScenery.cpp`,
  `SprawlGameMode.cpp`. No branch, worktree, commit, or remote operation
  performed.
<!-- entry:blender-authentic-city-kit-validated -->

### 2026-07-20 · main · claude (flowing water + sunken lake basin — validated)
- **Water now flows and moves:** a runtime probe confirmed the live lake material
  is the animated DatasmithContent `MI_WaterPond` shader (`authoredBase=MI_WaterPond
  broken=0`), so it was GPU-animated but far too subtle over the 96 m lake.
  `FSprawlOceanWaveProfile` flow 0.15 -> 0.40 and wave strength 2.0 -> 2.6 (wave
  size 1100 -> 900) for a clearly moving surface.
- **New `ASprawlLakeBasin` module — the lake is a real dip below the ground:** the
  probe showed `City_Ground` is a single opaque cube (top Z0, half-extent 11446)
  that also spans the lake, so it occluded any lowered water. The module hides that
  plane's visual (its collision is kept as an invisible no-fall floor), rebuilds
  the visible city floor as a four-rectangle ring around the lake, and drops a
  walled basin — four quay walls (a Z0 curb down to the bed, with collision so the
  player stops at the water's edge) plus a bed — beneath the water, now at Z-120
  (120 cm below the promenade). It also hides dressing left floating over the
  lowered water (19 props).
- **Waterfront cleanup:** the old flush-water far banks are removed (the basin
  walls the water; the ring + mountains close the horizon), and
  `-SprawlAuditWaterfront` now frames the shore from a raised angle so the dip is
  visible headlessly.
- **Validation:** `ConnectedSprawlEditor Mac Development` built clean.
  `ConnectedSprawl.World` automation passed 2/2 after the far-bank removal. A
  real-Metal `-SprawlVisualAudit -SprawlAuditWaterfront` logged `[LakeBasin] hid 1
  city-ground plane(s), 19 floating prop(s); basin waterZ=-120` and `[SurfaceRepair]
  ... remaining large checker/missing = 0`; the capture shows the promenade dropping
  over a quay wall to a recessed water surface with the checker still gone. Water
  motion rides the confirmed-animated material at 2.7x the previously-proven flow
  (to be confirmed in interactive play).
- **Risk / remaining validation:** no cook, package, iPhone/device, or performance
  profile. The basin relies on the invisible `City_Ground` collision as the no-fall
  floor and the quay curb to keep the player ashore. Vertex waves still come from
  the DatasmithContent path in-editor; a cooked iOS build uses the static
  translucent fallback until an engine-owned wave material is authored. A deeper
  basin or sloped (vs vertical quay) banks are easy follow-ups if wanted.
- **Status:** validated and uncommitted on `main`. New files
  `Public/World/SprawlLakeBasin.h`, `Private/World/SprawlLakeBasin.cpp`; edits to
  `SprawlGameMode.cpp`, `SprawlOceanSurface.{h,cpp}`, `SprawlWaterfrontScenery.cpp`,
  and `Tests/SprawlWaterfrontSceneryTests.cpp`. No map, Blueprint, material, or
  binary asset was authored; no branch, worktree, commit, or remote operation
  performed.
<!-- entry:flowing-water-sunken-basin-validated -->

### 2026-07-20 · main · claude (waterfront checker + water repair — validated)
- **New `ASprawlSurfaceRepair` module:** the bare ground plane and the four
  ~95 m world-edge planes fall back to the engine checker (`WorldGridMaterial`)
  because authored RealKit/marketplace parents do not resolve at runtime, and
  `city_surfaces.py` skips the lake-edge blocks, so the checker was fully
  exposed where the player stands (the reported tan diamond plaza). The new
  actor — spawned once by `ASprawlGameMode` beside the waterfront scenery —
  scans the live world at BeginPlay and rebinds every checker/default slot to a
  project-owned `M_Simple_Opaque` MID: warm stone within a Step of the lake,
  dark asphalt elsewhere. No tick, collision, or map/binary edit; a post-pass
  re-scan proves 0 checker/missing large surfaces remain.
- **Water hardened (`FSprawlOceanSurface` + `ASprawlWaterfrontScenery`):**
  `MI_WaterPond`'s parent is `/DatasmithContent/Materials/Water/M_Water`, a
  plugin material NOT in the enabled set, so its waves/flow/colour silently died
  and the lake read as raw teal. `BuildWaveMaterial` now swaps to a project-
  owned translucent master (`M_Simple_Glass`) whenever the authored base
  collapses to the engine default, applies the corrected coastal colour +
  opacity + refraction under both the Datasmith and Simple_Glass parameter
  names, and `FitMesh` inflates the fit 4% so the water edge tucks under the
  shore instead of a hard seam. The layout test's pure `CalculateScale`/
  `CalculateLocation` math is unchanged.
- **New audit hook:** `-SprawlAuditWaterfront` points the existing
  `-SprawlVisualAudit` at the lake shore (derived from the live layout) so the
  ground/water repair is verifiable headlessly instead of the inland boulevard.
- **Validation:** `ConnectedSprawlEditor Mac Development` built clean.
  `ConnectedSprawl.World` automation passed 2/2 (`WaterfrontSceneryLayout`,
  `LightingContrastAndRoadMarkingFit`). A real-Metal `-SprawlVisualAudit
  -SprawlAuditWaterfront` logged `rebound 64 checker/default surface slot(s);
  remaining large checker/missing = 0`; three 1600x900 captures confirm the
  checker is gone (asphalt street, cobblestone kerbs, crosswalk, grass verge)
  and the lake reads as dark coastal water behind the promenade rail. The
  overlook capture passed the full visual gate (luma 116.9, red_blue 1.20); the
  shore-level frame reads cool on the warmth gate (red_blue 0.98) by design on
  a water-heavy view, crushed 0.01%.
- **Risk / remaining validation:** no cook, package, iPhone/device, or
  sustained performance profile was run. Vertex waves still come from the
  DatasmithContent path in-editor; a cooked iOS build will use the static
  translucent fallback (correct colour/refraction, no moving waves) until an
  engine-owned wave material is authored. The water reads slightly thin from the
  promenade angle — a shoreline foam/beach band is a sensible follow-up.
- **Status:** validated and uncommitted on `main`. New files
  `Public/World/SprawlSurfaceRepair.h` and `Private/World/SprawlSurfaceRepair.cpp`;
  edits to `SprawlGameMode.cpp`, `SprawlOceanSurface.{h,cpp}`, and
  `SprawlWaterfrontScenery.{h,cpp}`. No map, Blueprint, material, or other binary
  asset was authored; no branch, worktree, staging, commit, or remote operation
  was performed.
<!-- entry:waterfront-checker-water-repair-validated -->

### 2026-07-20 · main · codex (GPU ocean water and waves — validated)
- **New `FSprawlOceanSurface` module:** fits the existing tessellated water mesh
  to the live 9600x6400 waterfront despite its off-centre import bounds, then
  creates a dynamic instance of the authored translucent water shader with a
  coastal-blue colour, flow, dual-wave displacement, depth, and refraction.
  Wave motion remains GPU-driven; no tick, collision, overlap, navigation, or
  CPU simulation was added.
- **Integration and deterministic coverage:** `ASprawlWaterfrontScenery` now
  delegates mesh fitting and material configuration to the ocean module, with
  the existing engine plane and city water material retained as fallbacks.
  `ConnectedSprawl.World.WaterfrontSceneryLayout` additionally covers valid
  wave settings and the scale/translation needed for off-centre source meshes.
- **Validation:** a clean non-hot-reload `ConnectedSprawlEditor Mac
  Development` link succeeded in 4.76 seconds after the initial full compile
  succeeded in 27.56 seconds. The focused automation test passed 1/1. A final
  real-Metal 1600x900 capture logged `ocean=9600x6400`, `flow=0.15`,
  `wave=1100`, and `strength=2.0`; two water-region frames 2.5 seconds apart
  differed by 7.0% RMSE, confirming temporal GPU surface motion. The
  `ConnectedSprawl Mac Development` game target also built successfully in
  81.86 seconds.
- **Risk / remaining validation:** launching the raw Development app without a
  staged build failed as expected because no cooked global shader library was
  present; no cook, stage, package, iPhone/device, or sustained performance
  profile was run. The reused `SM_Water` source mesh reports a missing legacy
  material dependency, but the ocean module replaces that slot at runtime.
  Existing road-paint, mountain/rock ISM-usage, foliage, and car-detail warnings
  remain unrelated.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  or binary asset was authored, and no branch, worktree, staging, commit, or
  remote operation was performed.
<!-- entry:gpu-ocean-water-waves-validated -->

### 2026-07-20 · main · codex (waterfront and mountain backdrop — validated)
- **New runtime scenery module:** `ASprawlWaterfrontScenery` derives the exact
  9600x6400 lake footprint from the live city grid, adds an opaque blue water
  surface and two far banks, hides the two broken map placements of the legacy
  mountain mesh, and reuses that authored asset as four normalized HISM ranges
  with seven rock foothills along the south/east horizon. It is spawned once by
  `ASprawlGameMode`; it has no tick, collision, overlaps, navigation influence,
  shadows, decals, or custom-depth work.
- **Validation:** the final `ConnectedSprawlEditor Mac Development` build
  succeeded in 12.39 seconds and
  `ConnectedSprawl.World.WaterfrontSceneryLayout` passed 1/1. The broader World
  suite previously passed 2/2 after the module integration. A 30-second real-RHI
  traffic audit passed with all 14 traffic cars moving, visible contained
  drivers, and zero boundary, lane, wrong-way, intersection-entry, or missing-
  driver violations. A final real-Metal 1600x900 capture from the waterfront
  was inspected: the basin reads as blue water, the fragmented vista is gone,
  and the controlled authored ridges form the background.
- **Risk / remaining validation:** no cook, package, iPhone/device, or sustained
  performance profile was run. Existing missing foliage/car-detail dependency
  warnings and the `MI_RoadPaint` ISM-usage warning remain unrelated. The first
  compile exposed an iterator warning and early visual drafts looked too
  geometric; both were corrected before the final build/capture.
- **Status:** validated and uncommitted on `main`. No map, Blueprint, material,
  or binary asset was authored, and no branch, worktree, staging, commit, or
  remote operation was performed.
<!-- entry:waterfront-mountain-backdrop-validated -->

### 2026-07-20 · main · codex (waterfront and mountain backdrop — in progress)
- **New `ASprawlWaterfrontScenery` module:** derives the lake footprint from
  the live `USprawlCityGridSubsystem`, overlays it with a controlled opaque
  blue water surface, adds two non-colliding far banks, hides the fragmented
  marketplace mountain mesh, and replaces it with twelve instanced peaks plus
  seven foothills outside the playable boundary.
- **Performance/safety:** the scenery is spawned once by `ASprawlGameMode`,
  never ticks, never collides, never affects navigation, casts no shadows, and
  uses instancing for the background layers. No map, Blueprint, material, or
  other binary asset is rewritten.
- **Validation:** a deterministic grid/layout test is added; build, focused
  automation, runtime traffic regression, and visual capture are pending.
- **Status:** in progress and uncommitted on `main`; no branch, worktree,
  staging, commit, or remote operation has been performed.
<!-- entry:waterfront-mountain-backdrop-in-progress -->

### 2026-07-20 · main · codex (contained occupants, paint depth, drive logic — validated)
- **Integration complete:** `ASprawlCar` now seats visible AI occupants through
  `FSprawlVehicleOccupantPlacement`, uses `FSprawlVehicleDriveLogic` for both
  player and AI movement, and exposes a containment diagnostic. Road markings
  use `FSprawlRoadPaintOcclusion` to reject non-depth-writing paint and disable
  custom-depth rendering, so vehicle bodywork occludes painted lines.
- **Deterministic validation:** `ConnectedSprawl.Vehicles` passed 2/2 tests
  (`NamedFrontAxleDefinesVisibleNose` and
  `OccupantContainmentAndDriveLogic`); `ConnectedSprawl.World` passed 1/1
  (`LightingContrastAndRoadMarkingFit`).
- **Build/runtime validation:** `ConnectedSprawlEditor Mac Development`
  completed with `Result: Succeeded` in 25.84 seconds. The 30-second real-RHI
  `-SprawlTrafficAudit` passed with 14/14 traffic cars moved, 14 visible
  drivers, `outside_drivers=0`, `missing_drivers=0`, zero lane or wrong-way
  violators, and zero unauthorized intersection entries. `git diff --check`
  passes.
- **Remaining validation/risk:** no cook, package, iPhone/device, or sustained
  performance profile was run. Unreal reported pre-existing missing foliage
  dependencies and that `MI_RoadPaint` lacks its Instanced Static Mesh usage
  flag; Unreal substituted its opaque default for that instance in the audit,
  so resaving the authored material with that usage flag remains worthwhile.
- **Status:** validated and uncommitted on `main`; no map, Blueprint, material,
  or binary asset was authored. No branch, worktree, staging, commit, or remote
  operation was performed.
<!-- entry:contained-occupants-paint-depth-drive-logic-validated -->

### 2026-07-20 · main · codex (contained occupants, paint depth, drive logic — in progress)
- **New `FSprawlVehicleOccupantPlacement` module:** measures visible bodywork
  in actor space while excluding wheel and driver meshes, derives a conservative
  inner-cabin envelope, and clamps the driver pelvis inside it. An invalid body
  now suppresses the driver visual instead of displaying a person outside the
  car.
- **New `FSprawlRoadPaintOcclusion` module:** requires a depth-writing opaque or
  masked paint material, disables custom-depth rendering, and fixes paint at a
  background translucency priority so lines remain road surfaces and are
  occluded by vehicles. The existing opaque `MI_RoadPaint` remains preferred;
  `M_RoadWhite` is the safe authored fallback.
- **New `FSprawlVehicleDriveLogic` module:** centralizes forward force,
  brake-before-reverse engagement, reverse steering sign, AI planar velocity,
  gravity preservation, and the rule that stopped AI traffic cannot rotate in
  place. `ASprawlCar` uses it for both player and autonomous drive paths.
- **Validation:** deterministic module coverage has been added; build and
  runtime audit are pending.
- **Status:** in progress and uncommitted on `main`; no branch, worktree,
  staging, commit, or remote operation has been performed.
<!-- entry:contained-occupants-paint-depth-drive-logic -->

### 2026-07-20 · main · codex (contrast, visual forward, and road-marking fit — built)
- **Validation:** `ConnectedSprawlEditor Mac Development` completed with
  `Result: Succeeded` in 57.37 seconds; `git diff --check` passes. The rebuilt
  editor module is present at `Binaries/Mac/libUnrealEditor-ConnectedSprawl.dylib`.
  A focused `UnrealEditor-Cmd` automation invocation was attempted but exited
  during macOS application-services setup (`Connection invalid`) while the
  already-running game/editor session held the service. It produced no test
  report, so the new deterministic tests are compiled but **not claimed run**.
- **Risk / next:** restart the running game/editor so it loads the rebuilt
  module, then run `ConnectedSprawl.Vehicles` and `ConnectedSprawl.World`
  automation plus `-SprawlTrafficAudit` for runtime confirmation. The previous
  directed-traffic build and audit results remain the latest runtime evidence.
- **Status:** built and uncommitted on `main`; no map, Blueprint, material, or
  binary asset was authored; no branch, worktree, staging, commit, or remote
  operation was performed.
<!-- entry:contrast-visual-forward-road-marking-fit-built -->

### 2026-07-20 · main · codex (contrast, visual forward, and road-marking fit — in progress)
- **New runtime modules:** `FSprawlLightingContrast` establishes a conservative
  direct-to-ambient/fog profile for a more legible streetscape with
  auto-exposure still disabled. `FSprawlVehicleVisualForward` derives imported
  vehicle yaw from its named front/rear wheel axles, so physical hull +X is the
  visible nose-forward direction and reverse remains an intentional input.
  `FSprawlRoadMarkingLayout` supplies thin paint dimensions and centred,
  stop-line-clear road intervals so stripes no longer end arbitrarily inside
  an intersection approach.
- **Integration:** the environment controller now uses the contrast profile;
  road-marking construction consumes the common fit/interval model; runtime
  split vehicle kits use the named-axle resolver instead of a local-axis
  heuristic. No map, Blueprint, material, or binary asset has been modified.
- **Validation:** pending build plus focused Unreal automation/audit execution.
- **Status:** in progress and uncommitted on `main`; no branch, worktree,
  staging, commit, or remote operation has been performed.
- **Next:** compile, run the focused unit tests and traffic audit, then inspect
  the resulting diff before handoff.
<!-- entry:contrast-visual-forward-road-marking-fit -->

### 2026-07-20 · main · codex (directed traffic and lane discipline)
- **New `FSprawlTrafficRoute` module:** traffic now owns a directed route through
  each approach. Straight/right/left exits are topology-checked against the
  live grid and lake; a dead end executes a real U-turn only inside a reserved
  junction. A car with no legal route stops immediately for manager recycle
  instead of flipping its heading across an active lane. Runtime spawns must
  have a dry forward exit and try the opposite directed lane before rejection.
- **New `FSprawlTrafficLaneDiscipline` module:** grid-scaled pure-pursuit
  guidance, lane-error measurement, and wrong-way velocity detection share one
  allocation-free implementation. Stationary authored traffic with the
  historical small lane offset is collision-sweep canonicalized before it
  accelerates; blocked or large corrections stay on the controller path.
- **Integration and diagnostics:** `ASprawlCar` exposes its planned heading and
  road, retains junction leases through legal turns/U-turns, and reports route
  recycle/turn counters. `AProceduralTrafficManager` backfills invalid routes,
  audits the planned lane rather than actor yaw, and now fails on persistent
  wrong-way velocity independently of lane position. Four deterministic Unreal
  automation tests cover all heading/maneuver mappings, the lake-edge U-turn,
  forward-exit spawn policy, exact-lane guidance, historical offset repair, and
  wrong-way detection.
- **Validation:** `ConnectedSprawlEditor Mac Development` build succeeded;
  `ConnectedSprawl.Traffic` found and passed all 4 tests. Two real-Metal
  TrafficAudit runs passed: the focused run had 14/14 movers, 17 turns,
  `lane_violators=0`, `wrong_way=0`, `route_recycles=0`; the final combined
  takeover run had 23 movers, 11 turns, the same zero violation/recycle gates,
  one-car max junction occupancy, and 0 unauthorized entries. CarjackAudit
  passed possession, fleeing driver, lease release, retirement, and 14-car
  backfill. `git diff --check` passed. No map, Blueprint, or binary asset was
  changed.
- **Status:** validated and uncommitted on `main`; no branch, worktree, staging,
  commit, or remote operation was performed.
- **Next:** interactive visual traffic review and iPhone device profiling remain
  worthwhile. The displaced junction props, warmth gate, and stale legacy
  generation constants remain separate open ledger items and were not touched.
<!-- entry:directed-traffic-lane-discipline -->

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
