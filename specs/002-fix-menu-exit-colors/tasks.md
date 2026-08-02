---

description: "Task list for Menu Layout, Exit Controls, and Mine-Count Colors Fixes"
---

# Tasks: Menu Layout, Exit Controls, and Mine-Count Colors Fixes

**Input**: Design documents from `/specs/002-fix-menu-exit-colors/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md (all present)

**Tests**: This feature adds no new `core::` logic (plan.md's Constitution Check, row III) — every
change lives in `src/ui/`, the app shell (`AppImpl` in `main.cpp`), or the shared
`platform/renderer.h`/`platform/softcanvas.cpp` rendering layer, none of which this template
host-unit-tests today (only `src/core/`/`src/persist/` have doctest coverage). No new automated
test tasks are generated; each story's "Independent Test" is its corresponding manual scenario in
`quickstart.md`, exactly as `001`'s UI-facing stories were verified.

**Organization**: Tasks are grouped by user story (spec.md priorities P1/P1/P3/P4). There is no
Setup or Foundational phase in the usual sense — see the note below — so numbering starts directly
at the first user story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on another incomplete task in the
  same batch)
- **[Story]**: Which user story this task belongs to (US1–US4, per spec.md)
- File paths are relative to the repository root

## Path Conventions

Single project, existing layout: `src/platform/`, `src/ui/`, `src/main.cpp` — see plan.md's
Project Structure. This feature adds **zero new files**; every task edits a file that already
exists from `001-core-gameplay-settings`.

---

## Phase 1: Setup — not applicable

No project initialization, new dependencies, or new directories are needed. Every task below edits
an existing file in place.

## Phase 2: Foundational — not applicable

There is no cross-story shared prerequisite: `App::requestExit()` (used by US1) already exists,
and `App::returnToMainMenu()` (needed only by US2) is added within US2's own phase, not shared by
any other story. The one real cross-story consideration is **file-level overlap, not a blocking
prerequisite** — `src/ui/screens/board_screen.h`/`.cpp` is edited by US1, US2, and US4 (different
functions each time: the HUD nav row, the outcome banner, and `drawCell()`, respectively), and
`src/ui/screens/new_game_screen.h`/`.cpp` is edited by both US1 and US3. Each affected task below
notes this explicitly as a same-file dependency rather than marking those tasks `[P]` against each
other.

---

## Phase 3: User Story 1 - Exit the application from anywhere (Priority: P1) 🎯 MVP

**Goal**: A visible Exit control on the main menu (New Game screen) and on the board screen closes
the application immediately, with no confirmation, and never loses in-progress state.

**Independent Test**: quickstart.md §2 scenarios 1–3 — Exit from the New Game screen closes the
app; Exit mid-game closes the app and the game resumes exactly as left on relaunch; Settings shows
only Back (no Exit there), with Exit still reachable one tap after Back.

### Implementation for User Story 1

- [X] T001 [P] [US1] Add an `exitButton_` `Button` field to `NewGameScreen`, lay it out grouped
      immediately after `settingsButton_` (same per-item `t.gap` spacing as the other preset
      buttons, ahead of the existing doubled gap before Custom — research.md #4), draw it, and
      route its tap to `app_.requestExit()` (no confirmation, per FR-001/FR-014) in
      `src/ui/screens/new_game_screen.h` and `src/ui/screens/new_game_screen.cpp`
- [X] T002 [P] [US1] Change `BoardScreen`'s HUD nav row from two buttons (Flag Mode, Settings) to
      three equal-width buttons (Flag Mode, Settings, Exit); route the new button's tap to
      `app_.requestExit()` (no confirmation, per FR-002/FR-014) in
      `src/ui/screens/board_screen.h` and `src/ui/screens/board_screen.cpp`

**Checkpoint**: quickstart.md §2 scenarios 1–3 pass in the SDL simulator — Exit is reachable and
immediate from both screens, and mid-game progress survives an exit (relying on the existing
always-on autosave-on-exit path in `main.cpp`, unchanged by this feature).

---

## Phase 4: User Story 2 - Recover after winning or losing a game (Priority: P1)

**Goal**: The win/loss outcome banner offers both a "Return to Menu" and an "Exit" control;
returning to the menu clears the finished game so a later relaunch shows the menu, not the old
board.

**Independent Test**: quickstart.md §2 scenarios 4–5 — from both a win and a loss banner, "Return
to Menu" goes to the New Game screen and a subsequent relaunch opens on the New Game screen (not
the finished board); "Exit" from either banner closes the app immediately.

### Implementation for User Story 2

- [X] T003 [US2] Add `virtual void returnToMainMenu() = 0;` to the `App` interface in
      `src/ui/app.h`, per contracts/app-interface.md
- [X] T004 [US2] Implement `AppImpl::returnToMainMenu()` in `src/main.cpp`: reset `session_` to a
      fresh default `core::GameSession()` (`NotStarted`, clearing resume-on-launch state per
      FR-006), call `autosaveSession()` immediately to persist the reset, then `stack_.clear()`
      and `push()` a fresh `NewGameScreen` (not a bare `pop()` — `BoardScreen` can be the sole
      stack root when a finished game was resumed at launch, per research.md #2 and
      contracts/app-interface.md) (depends on T003)
- [X] T005 [US2] Extend `BoardScreen::drawOutcomeBanner()` with two new buttons ("Return to Menu",
      "Exit"), growing the banner's box height to fit them; extend `BoardScreen::onTap()` to route
      taps to these two buttons *before* the existing "board is inert once the game has ended"
      early return (FR-004/FR-005) — "Return to Menu" calls `app_.returnToMainMenu()`, "Exit" calls
      `app_.requestExit()` (no confirmation, per FR-014) — in `src/ui/screens/board_screen.h` and
      `src/ui/screens/board_screen.cpp` (depends on T002, T003 — same file as T002, and needs the
      T003 interface method declared)

**Checkpoint**: quickstart.md §2 scenarios 4–5 pass — both outcome banners offer working
Return-to-Menu and Exit controls, and a relaunch after "Return to Menu" opens on the New Game
screen.

---

## Phase 5: User Story 3 - Clearer New Game menu layout (Priority: P3)

**Goal**: Beginner/Intermediate/Expert/Settings/Exit read as one grouped block, followed by a gap
twice the normal inter-item spacing, followed by the visually distinct Custom section.

**Independent Test**: quickstart.md §2 scenario 6 — the preset+Exit group is visually clustered,
followed by a clearly larger gap, followed by the Custom section; every preset button, the Exit
button, and every Custom adjuster/start button still work.

### Implementation for User Story 3

- [X] T006 [US3] Confirm and, if needed, adjust `NewGameScreen::layout()` so the gap immediately
      above the "Custom" header remains exactly `2 * t.gap` (already present in the pre-existing
      code per research.md #4) with the new `exitButton_` from T001 now included in the grouped
      block above that gap — i.e. the doubled-gap advance happens once, immediately after the last
      button in the Beginner/Intermediate/Expert/Settings/Exit group, per FR-007/FR-008/FR-009 —
      in `src/ui/screens/new_game_screen.cpp` (depends on T001 — same file)
      **Found and fixed a real bug while confirming this**: the "Custom" header label was
      positioned by working backward from `widthMinus_.rect.y` (`- t.textPx - t.gap`, ~64px), but
      the gap it was squeezed into was only `2 * t.gap` (~28px) — so the bold "Custom" label was
      actually drawn overlapping the last button in the group above it (this pre-dates this
      feature; it likely contributed directly to the user's "Custom looks like it's not in this
      place" complaint). Fixed by giving the header its own reserved `customTitleRect_` row,
      advanced past explicitly in `layout()` after the doubled gap, instead of computing it
      backward from the steppers' position.

**Checkpoint**: quickstart.md §2 scenario 6 passes — the grouping reads clearly as two blocks, and
all existing New Game interactions are unaffected.

---

## Phase 6: User Story 4 - Distinct colors per mine-count number in Color mode (Priority: P4)

**Goal**: In Color mode on a color-capable device, each mine-count digit (1–8) renders in its own
distinct color per FR-010; Black & White mode / monochrome devices are completely unaffected.

**Independent Test**: quickstart.md §2 scenario 7 (plus the monochrome regression, scenario 8) — a
board exposing digits 1–8 in Color mode shows each in its assigned color; switching back to Black
& White reverts every digit to plain black, unchanged from `001`.

### Implementation for User Story 4

- [X] T007 [P] [US4] Extend `enum class Color` in `src/platform/renderer.h` from `{ None, Red }` to
      `{ None, Red, Blue, Green, Navy, Crimson, Cyan }`, per data-model.md and
      contracts/digit-color-mapping.md
- [X] T008 [US4] Add an RGB mapping for the five new `Color` values inside
      `SoftCanvas::resolveColor()`'s `colorDisplay` branch (leaving the `colorDisplay == false`
      grayscale fallback path untouched, per FR-011) in `src/platform/softcanvas.cpp`, using the
      RGB table in contracts/digit-color-mapping.md (depends on T007)
- [X] T009 [US4] Replace `BoardScreen::drawCell()`'s single
      `if (app_.theme().color) accent = Color::Red;` with the full per-count mapping from
      contracts/digit-color-mapping.md: 1→`Color::Blue`, 2→`Color::Green`, 3→`Color::Red`,
      4→`Color::Navy`, 5→`Color::Crimson`, 6→`Color::Cyan`, 7→`Color::None` (unchanged
      `Gray::Black` shade), 8→`Color::None` with `glyphShade` set to `Gray::Mid` — in
      `src/ui/screens/board_screen.cpp` (depends on T007, T008; touches the same file as T002/T005
      but a different function, `drawCell()`, so sequence after those complete rather than editing
      concurrently)

**Checkpoint**: quickstart.md §2 scenario 7 and the `--gray` monochrome regression (scenario 8)
both pass — all 8 digits are mutually distinguishable by color in Color mode, and Black & White /
monochrome rendering is byte-for-byte unchanged from `001`.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Full end-to-end validation now that all four fixes are in place together.

- [X] T010 Run the full `quickstart.md` validation guide end-to-end: the host `ctest` regression
      pass (§1), all 8 simulator scenarios (§2), and the `001` regression spot-check (§3) — fix any
      discrepancy found (depends on T001–T009)
      **Partial**: §1 verified directly — `cmake --build build/host` (both `minesweeper_ui` and
      `minesweeper_tests`) and `ctest --test-dir build/host` pass (1/1 suites, unchanged from
      before this feature, confirming no `src/core/` regression). The full SDL simulator build
      (`build/sim`, `MINESWEEPER_BACKEND=sdl`) also compiles and links clean, and the resulting
      `minesweeper.exe` launches and runs its event loop without crashing (smoke-tested via a
      timed run). §2's 8 scenarios require real mouse-click interaction with the SDL window (Exit
      buttons, Return-to-Menu, Settings back-only, the New Game menu grouping, and the 8 digit
      colors), which isn't available in this CLI environment — same limitation `001`'s own
      `tasks.md` (T034) recorded. **These should be run manually before release**; see
      quickstart.md §2 for the exact steps.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup / Foundational (Phase 1–2)**: Not applicable — see notes above.
- **User Story 1 (Phase 3)**: No dependencies beyond the existing `001` codebase. Can start
  immediately.
- **User Story 2 (Phase 4)**: T003/T004 have no dependency on US1; T005 depends on T002 (same
  file, `board_screen.h`/`.cpp`) and T003 (needs the new interface method declared).
- **User Story 3 (Phase 5)**: Depends on T001 (same file, `new_game_screen.cpp`).
- **User Story 4 (Phase 6)**: No dependency on US1–US3's changes at all (different files/functions
  entirely: `renderer.h`, `softcanvas.cpp`, and `drawCell()` vs. the layout/onTap/banner code the
  other stories touch) — could be implemented in full isolation and merged last, or first; ordered
  last here only because it's the lowest spec.md priority (P4).
- **Polish (Phase 7)**: Depends on all of Phases 3–6.

### Within Each Story

- US1: T001 and T002 touch different files — genuinely parallel.
- US2: T003 → T004 (interface before implementation) and T003 → T005 (banner needs the method to
  call); T005 also waits on T002 finishing in the shared `board_screen.*` files.
- US3: T006 alone, gated on T001 finishing in the shared `new_game_screen.cpp`.
- US4: T007 → T008 → T009 (enum before its RGB resolution before its call site).

### Parallel Opportunities

- T001, T002 (US1 — different files).
- T003, T007 (different stories, different files — `app.h` vs `renderer.h`; both can start
  immediately with no dependency on anything).
- T007 can run in parallel with all of Phase 3/4/5's tasks (touches only `renderer.h`, which no
  other story edits).

---

## Parallel Example: User Story 1

```bash
Task: "Add Exit button to NewGameScreen in src/ui/screens/new_game_screen.h/.cpp"
Task: "Add Exit button to BoardScreen's HUD row in src/ui/screens/board_screen.h/.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 3: User Story 1 — a working Exit control from the main menu and mid-game is, by itself,
   the single highest-value fix (the app currently has no way to quit at all).
2. **STOP and VALIDATE**: quickstart.md §2 scenarios 1–3.
3. This alone is a shippable improvement even before US2–US4 land.

### Incremental Delivery

1. + User Story 1 → Exit works everywhere. Demo-able, ships the most-blocking fix.
2. + User Story 2 → win/loss screens are no longer a dead end. Demo-able.
3. + User Story 3 → New Game menu reads cleanly. Demo-able.
4. + User Story 4 → classic per-digit colors in Color mode. Feature-complete against spec.md.
5. Phase 7 → full validation pass.

### Notes

- [P] tasks touch different files and have no unmet dependency on each other.
- Commit after each task or logical group; stop at any checkpoint to validate a story
  independently in the SDL simulator before moving on.
- Avoid: adding any new `core::` state or persisted field for this feature — `returnToMainMenu()`'s
  reset reuses the existing `GameSession()` default constructor and existing `game.json` shape
  exactly as-is (data-model.md); no schema version bump, no new file.
