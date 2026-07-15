# Repository Agent Instructions

This root `AGENTS.md` is the sole canonical, provider-neutral authority for AI
workflow in this repository. Provider discovery files may point here, but must
not duplicate these rules. Live Git state and the latest user instruction take
precedence over any historical Markdown snapshot.

## Mandatory Preflight

Before analysis, planning, edits, tests, generated files, or Git operations, run:

```sh
git rev-parse --show-toplevel
git status --short --branch
git branch --show-current
git rev-parse HEAD
git worktree list --porcelain
```

Use that output to identify the current repository, branch, commit, dirty state,
and linked worktrees. Never infer current state from this file, assume
`origin/main`, or assume that a remote or upstream exists.

## Worktree Ownership And Git Safety

- Use one writer per worktree. A writer is any person or agent that edits,
  formats, generates, stages, or commits files.
- Concurrent writers require separate user-authorized worktrees and branches,
  with one writer assigned to each worktree. Do not create either merely for
  convenience.
- Stay in the current branch and worktree by default. Preserve its complete
  dirty state, including modified, deleted, staged, and untracked files.
- Treat every pre-existing or newly appearing change outside the active task as
  user-owned. Never discard, overwrite, normalize, or absorb it into the task.
- Do not create, switch, rename, merge, rebase, or delete branches; create,
  move, repair, prune, or delete worktrees; stage, commit, amend, tag, stash,
  restore, reset, clean, fetch, pull, or push unless the user explicitly asks
  for that operation.
- If the user requests a branch and supplies a name, use that name. If the user
  requests a branch but supplies no name, use `<assistant>/<task>` with a short,
  lowercase, hyphenated task slug.
- Before any authorized branch or worktree operation, refresh the mandatory
  preflight, inspect the status of every linked worktree, and ensure the target
  branch is not already checked out elsewhere.
- Never assume another writer committed, pushed, staged, or finished work. Check
  live state and report uncertainty rather than guessing.

## Safe Editing

- Re-read each target file immediately before editing it, and refresh
  `git status --short` before and after each edit batch.
- Keep changes narrow and task-scoped. Do not run blanket formatting, generated
  file refreshes, or dependency updates unless they are part of the request.
- Inspect the relevant diff after editing. If a target file changes concurrently,
  re-read it and preserve both intents when unambiguous; otherwise stop and ask.
- Never expose or store credentials, tokens, private keys, cookies, signing
  material, or environment-variable values in source, logs, or Markdown.

## Project Contract

- Read `README.md` for setup and architecture, `Design/GDD.md` for product and
  gameplay intent, and `Design/REALISTIC_CITY_NOTES.md` when changing the city.
- Treat `ConnectedSprawl.uproject`, `Source/`, `Config/`, `Content/`, and
  `Design/` as authored project inputs. Treat `Binaries/`, `Build/`,
  `DerivedDataCache/`, `Intermediate/`, `Saved/`, and `staging/` as generated,
  cached, runtime, or raw-import output; do not edit or regenerate them unless
  requested.
- Do not change the Unreal Engine association, regenerate IDE projects, upgrade
  engine APIs, or rewrite binary assets as a side effect of an unrelated task.
- Preserve the iPhone-first performance target and macOS support described in
  the project documentation. Validate performance-sensitive changes in the
  editor or target build when that environment is available.
- Keep C++ and Blueprint-facing contracts synchronized. When changing an exposed
  type or property, check its declarations, implementation, assets, and design
  documentation for affected consumers.

## Validation

- Run the smallest relevant validation first, then broaden it in proportion to
  risk. Use the documented Unreal/Xcode workflow that is available locally.
- Do not claim editor, cook, package, device, or performance validation unless it
  actually ran. Record unavailable validation explicitly.
- Before handoff, re-run the mandatory preflight, inspect only task-scoped diffs,
  and run `git diff --check` for tracked edits.
- Validation must not deploy, publish, rewrite assets, or mutate external state
  unless the user requested that action.

<!-- AI-SYNC-LEDGER:BEGIN (managed section; edit the template + re-run rollout) -->
## Project State Ledger (Cross-Agent Sync)

`AI_HANDOFF.md` is the shared project ledger for in-flight work across every
worktree, branch, and agent (claude · gemini · chatgpt · copilot). It holds
**state**; this file holds **rules**.

- Before starting any code task, read `AI_HANDOFF.md` to see what other branches
  and worktrees are doing. Do not duplicate, overwrite, or strand their work.
- Whenever a change touches code, update `AI_HANDOFF.md` in the **same change**:
  append a Log entry (date · branch · agent · files · validation · status ·
  next) and refresh your branch's row in Active Work.
- Stage and commit `AI_HANDOFF.md` **together with** the code, so the state flows
  through the branch and survives merges. A merge conflict in the Log means two
  agents diverged — resolve by keeping both entries, never by dropping one.
- Never delete or rewrite past Log entries. When a branch is merged or abandoned,
  note it in the Log and remove its Active Work row.
- Run `bash scripts/ai-sync-status.sh` to see live cross-worktree state and spot
  stranded (unmerged / uncommitted) work. Run `bash scripts/ai-sync-install.sh`
  once per clone to enable the pre-commit reminder.

Enforcement is layered:

- **Local:** `.githooks/pre-commit` blocks a commit that stages code without
  staging `AI_HANDOFF.md`. Bypass a docs-only commit with `git commit --no-verify`.
- **CI:** `.github/workflows/ai-sync.yml` runs the same check on every pull
  request and fails if code changed without a ledger update. For a legitimate
  docs-only PR, apply the **`skip-ledger`** label (auditable) instead of bypassing.
<!-- AI-SYNC-LEDGER:END -->

## Required Handoff

Report all of the following exactly:

- Files changed and the purpose of each change.
- Validation commands run and their results.
- Checks not run, failures, and remaining risks.
- Current branch, commit, and worktree path from the final live preflight.
- Final staged, modified, deleted, and untracked state relevant to the task,
  including pre-existing changes intentionally left untouched.
- Any authorized branch, worktree, commit, or remote operation performed; if
  none, say so explicitly.
