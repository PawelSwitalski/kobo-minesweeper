---

description: "Task list for Screen Refresh Frequency Setting"
---

# Tasks: Screen Refresh Frequency Setting

**Input**: Design documents from `/specs/004-screen-refresh-setting/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md (all present)

**Tests**: This feature extends the existing `core::Settings` type, which Constitution Principle
III (NON-NEGOTIABLE) requires host-run doctest coverage for — a test task is included for it,
extending `tests/test_settings.cpp` exactly as `colorMode`/`hideTimer` already are. The
`ScreenRefreshInterval → int` mapping (`ui::applyScreenRefreshInterval`) and the `SettingsScreen`
UI changes live in `src/ui/`, which is not linked into `minesweeper_tests` (`CMakeLists.txt`) — per
plan.md's Constitution Check (row III) and the `002` precedent for exactly this category of
trivial, non-`core::` lookup, these are verified via `quickstart.md`'s manual scenarios instead.

**Organization**: Tasks are grouped by user story (spec.md has a single P1 story — this is a
single, self-contained settings control). There is no Setup or Foundational phase in the usual
sense — see the note below — so numbering starts directly at the user story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on another incomplete task in the
  same batch)
- **[Story]**: Which user story this task belongs to (US1, per spec.md)
- File paths are relative to the repository root

## Path Conventions

Single project, existing layout: `src/core/`, `src/ui/`, `src/main.cpp`, `tests/` — see plan.md's
Project Structure. This feature adds **zero new files**; every task edits a file that already
exists.

---

## Phase 1: Setup — not applicable

No project initialization, new dependencies, or new directories are needed; no `CMakeLists.txt`
change either, since no files are added or removed.

## Phase 2: Foundational — not applicable

There is only one user story, so there is no cross-story prerequisite to isolate. Within the story,
`T001` (the `Settings` field itself) is the one task everything else in this feature depends on —
see Dependencies below.

---

## Phase 3: User Story 1 - Control how often the screen fully refreshes (Priority: P1) 🎯 MVP

**Goal**: A "Screen Refresh" control in Settings lets the player choose Every 5 / Every 10 (default)
/ Every 25 / Never for the existing-but-previously-unused ghosting-driven auto-full-refresh policy,
persisted across restarts and taking effect immediately.

**Independent Test**: quickstart.md §1 (host tests) plus §2 (UI presence, persistence, no-op on
simulator) and, where a device is available, §3 (actual on-device refresh cadence per value).

### Implementation for User Story 1

- [X] T001 [US1] Add `enum class ScreenRefreshInterval { Every5, Every10, Every25, Never };` and a
      `ScreenRefreshInterval screenRefreshInterval = ScreenRefreshInterval::Every10;` field to
      `core::Settings`; update `toJson()` to always write the field (as one of `"Every5"`/
      `"Every10"`/`"Every25"`/`"Never"`) and `fromJson()` to read it via
      `j.value("screenRefreshInterval", "Every10")` (absent means `"Every10"`, so an
      already-existing `settings.json` still parses successfully) then parse to the enum, throwing
      on any other unrecognized string — in `src/core/settings.h` and `src/core/settings.cpp`, per
      contracts/persistence-schema.md and research.md #2
- [X] T002 [P] [US1] Extend `tests/test_settings.cpp` with cases: `Settings` defaults to
      `screenRefreshInterval == Every10`; `toJson`/`fromJson` round-trips all four values
      losslessly; a hand-built `settings.json` string with valid `schemaVersion`/`colorMode` but
      **no** `screenRefreshInterval` key still parses successfully with `Every10` (simulating an
      install that predates this feature); an unrecognized `screenRefreshInterval` string is
      rejected (throws), matching the existing invalid-`colorMode` case (depends on T001)
- [X] T003 [P] [US1] Add `void applyScreenRefreshInterval(Renderer& renderer,
      core::ScreenRefreshInterval interval);` to `src/ui/theme.h` and `src/ui/theme.cpp`, alongside
      the existing `applyColorMode` — maps `Every5/Every10/Every25/Never` to
      `renderer.setGhostingInterval(5/10/25/0)` per contracts/screen-refresh-application.md
      (depends on T001 for the enum type)
- [X] T004 [US1] In `src/main.cpp`'s `AppImpl`, call `applyScreenRefreshInterval(renderer_,
      settings_.screenRefreshInterval)` once in the constructor (immediately after the existing
      `applyColorMode(...)` call, right after `settings_` is loaded) and again inside
      `autosaveSettings()` (immediately after its existing `applyColorMode(...)` call) — per
      contracts/screen-refresh-application.md's call-site contract (depends on T003)
- [X] T005 [P] [US1] Add a `screenRefreshLabelRect_` `Rect` field and four mutually-exclusive
      `Button` fields (`refresh5Button_`, `refresh10Button_`, `refresh25Button_`,
      `refreshNeverButton_`, labeled "5"/"10"/"25"/"Never") to `SettingsScreen`; extend `layout()`
      to position a new "Screen Refresh" section row below the existing Hide Timer row (above
      Back); extend `draw()` to set exactly one button's `toggled = true` matching
      `app_.settings().screenRefreshInterval` and draw the section label + all four buttons; extend
      `onTap()` so each button sets `app_.settings().screenRefreshInterval` to its corresponding
      value, calls `app_.autosaveSettings()`, redraws, and `flushFull()`s — mirroring the existing
      `colorButton_`/`blackWhiteButton_` handler shape exactly — in
      `src/ui/screens/settings_screen.h` and `src/ui/screens/settings_screen.cpp` (depends on T001
      for the enum type)

**Checkpoint**: `ctest` passes including the extended `test_settings.cpp` cases; quickstart.md §2
passes in the SDL simulator (control visible, selectable, persists, no simulator-visible effect);
quickstart.md §3 passes on a Kobo device where available (actual refresh cadence matches the
selected value, "Never" suppresses only the ghosting-driven auto-refresh).

---

## Phase 4: Polish & Cross-Cutting Concerns

**Purpose**: Full end-to-end validation now that the feature is complete.

- [X] T006 Run the full `quickstart.md` validation guide end-to-end: the host `ctest` run (§1,
      including the extended test suite), the SDL simulator checks (§2), the on-device cadence
      checks where a Kobo is available (§3), and the regression spot-check (§4) — fix any
      discrepancy found (depends on T001–T005)
      **Partial**: §1 verified directly — `cmake --build build/host` (both `minesweeper_ui` and
      `minesweeper_tests`) and `ctest --test-dir build/host` pass (1/1 suites), including all new
      `screenRefreshInterval` cases in `test_settings.cpp` (default, round-trip for all 4 values,
      missing-key backward compatibility, invalid-value rejection). The full SDL simulator build
      (`build/sim`, `MINESWEEPER_BACKEND=sdl`) also compiles and links clean, and the resulting
      `minesweeper.exe` launches and stays running (smoke-tested via a backgrounded run, confirmed
      still alive after 2s). §2's interactive checks (button visibility/selection/persistence in
      the SDL window) and §3's on-device refresh-cadence checks (which need real Kobo hardware,
      unavailable here) both require manual verification — same limitation `002`'s and `003`'s own
      `tasks.md` recorded. **These should be run manually before release**; see quickstart.md §2–3
      for the exact steps.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup / Foundational (Phase 1–2)**: Not applicable — see notes above.
- **User Story 1 (Phase 3)**: The only story. Can start immediately.
- **Polish (Phase 4)**: Depends on Phase 3.

### Within the Story

- T001 (the `Settings` field) blocks T002, T003, and T005 — each of which then has no further
  dependency on one another (different files: `tests/test_settings.cpp`, `src/ui/theme.*`,
  `src/ui/screens/settings_screen.*` respectively) — all three can proceed in parallel once T001
  lands.
- T004 depends on T003 (the function must exist before `main.cpp` can call it).

### Parallel Opportunities

- Once T001 lands: T002, T003, and T005 can all proceed in parallel.
- T004 must wait for T003 specifically (not just T001), since it calls the new function directly.

---

## Parallel Example: User Story 1

```bash
# After T001 (Settings::screenRefreshInterval) lands:
Task: "Extend tests/test_settings.cpp for screenRefreshInterval"
Task: "Add ui::applyScreenRefreshInterval in src/ui/theme.h/.cpp"
Task: "Add the four Screen Refresh buttons to SettingsScreen"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 3: User Story 1 — the entire feature is this one story; there is nothing to sequence
   around it.
2. **STOP and VALIDATE**: `ctest` (extended suite) + quickstart.md §2, and §3 where a device is
   available.
3. Phase 4 → full validation pass.

### Notes

- [P] tasks touch different files and have no unmet dependency on each other.
- Commit after each task or logical group; stop at the checkpoint to validate in the SDL simulator
  (and, ideally, on-device) before considering the feature done.
- Avoid: bumping `settings.json`'s `schemaVersion` — `screenRefreshInterval` is additive with a
  spec-mandated default, handled via `j.value("screenRefreshInterval", "Every10")`, not a breaking
  schema change (research.md #2).
- Avoid: adding a new `Renderer` method or a new "ghosting" mechanism — `setGhostingInterval`
  already exists and already does exactly what this feature needs (research.md #1).
