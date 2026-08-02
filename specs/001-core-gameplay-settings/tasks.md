---

description: "Task list for Core Minesweeper Gameplay & Display Settings"
---

# Tasks: Core Minesweeper Gameplay & Display Settings

**Input**: Design documents from `/specs/001-core-gameplay-settings/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md (all present)

**Tests**: Constitution III (NON-NEGOTIABLE) requires host-run unit tests for correctness-critical
core logic and persistence round-trip/corrupted-input recovery — these are included below as part
of the Foundational phase, where all new core logic lives (see rationale in that phase's intro).
User-story phases beyond Foundational are UI-wiring over already-tested core logic, so their
"Independent Test" is the corresponding manual scenario in `quickstart.md` (this template has no
automated UI/simulator test framework — only doctest host unit tests for `core`/`persist`).

**Organization**: Tasks are grouped by user story (spec.md priorities P1/P2/P2/P2/P3) to enable
independent implementation and testing of each story, on top of a Foundational phase that holds
the complete `Board`/`GameSession`/`Settings` core engine — these are tightly-coupled methods on a
few small classes (`chord()` reuses `openCell()`; `remainingMineCount()` reads flag state) that
don't decompose cleanly along the UI-facing story boundary, so all core logic and its tests are
built and verified once, up front; each story phase then wires one interaction affordance
(open, flag, chord, settings, custom sizing) onto that already-correct engine.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on another incomplete task in the
  same batch)
- **[Story]**: Which user story this task belongs to (US1–US5, per spec.md)
- File paths are relative to the repository root

## Path Conventions

Single project, existing layout: `src/core/`, `src/persist/`, `src/platform/`, `src/ui/`,
`tests/`, `CMakeLists.txt` at repository root — see plan.md's Project Structure.

---

## Phase 1: Setup

**Purpose**: Clear out the template's placeholder demo so its names/files don't collide with the
real feature.

- [X] T001 Delete the placeholder Counter/About demo: `src/core/counter.h`, `src/core/counter.cpp`,
      `src/ui/screens/counter_screen.h`, `src/ui/screens/counter_screen.cpp`,
      `src/ui/screens/about_screen.h`, `src/ui/screens/about_screen.cpp`, `tests/test_counter.cpp`
- [X] T002 Remove the deleted files' entries from `CMakeLists.txt` (the `minesweeper_core`,
      `minesweeper_ui`, and `minesweeper_tests` source lists) so the tree builds clean before new
      sources are added (depends on T001)

**Checkpoint**: `cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON && cmake --build
build/host` succeeds with `minesweeper_core`/`minesweeper_ui` reduced to zero sources (about to
be filled in by Phase 2) — if the toolchain rejects a zero-source `STATIC` library at this
checkpoint, add a placeholder `.cpp` temporarily rather than treating it as a real failure.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The complete, host-tested Minesweeper engine (`Cell`, `DifficultyConfig`, `Board`,
`GameSession`, `Settings`), its persistence, the platform long-press extension every gesture
depends on, and the `App`/`AppImpl` rewiring every screen depends on. Per data-model.md and
contracts/.

**⚠️ CRITICAL**: No user story screen can be built until this phase compiles and its tests pass.

- [X] T003 [P] Create `CellState` enum (`Unopened`/`Opened`/`Flagged`) and `Cell` struct
      (`isMine`, `state`, `adjacentMines`) in `src/core/cell.h`, per data-model.md §Cell (FR-004)
- [X] T004 [P] Create `DifficultyPreset` enum and `DifficultyConfig` struct with `beginner()`/
      `intermediate()`/`expert()`/`custom(w,h,mines)` factories and `isValidCustom()` bounds
      checking (5–16/5–16/1..(w×h−9)) in `src/core/difficulty.h` and `src/core/difficulty.cpp`,
      per data-model.md §DifficultyConfig (FR-001, FR-002, FR-003)
- [X] T005 [P] Create `ColorMode` enum and `Settings` class (`colorMode`, default
      `BlackAndWhite`) with `toJson()`/`fromJson()` per contracts/persistence-schema.md's
      `settings.json` shape in `src/core/settings.h` and `src/core/settings.cpp` (FR-019, FR-021)
- [X] T006 Create `Board` class in `src/core/board.h` and `src/core/board.cpp`: lazy mine
      placement excluding the first-opened cell (FR-005), `openCell(x,y)` with iterative
      cascade flood-fill on zero-adjacency cells (FR-006, FR-007), win detection (FR-017),
      `toggleFlag(x,y)` (FR-009-adjacent core rule: no-op on opened cells or after game end,
      FR-011, FR-014), `chord(x,y)` (FR-012, FR-013), `flaggedCount()`/`remainingMineCount()`
      (FR-015), `cellAt(x,y)` read accessor, `Status` enum (`NotStarted`/`InProgress`/`Won`/
      `Lost`) — per data-model.md §Board (depends on T003)
- [X] T007 Create `GameSession` class in `src/core/game_session.h` and
      `src/core/game_session.cpp`: wraps `DifficultyConfig` + `Board` + `elapsedSeconds`,
      delegates `openCell`/`toggleFlag`/`chord`/`status()` to `Board`, `addActiveSeconds(uint32_t)`
      (no-op once ended), `toJson()`/`fromJson()` per contracts/persistence-schema.md's
      `game.json` shape (cells' `adjacentMines` recomputed on load, not stored; full rejection
      rules from data-model.md §game.json shape) (depends on T004, T006)
- [X] T008 [P] doctest: `DifficultyConfig` preset values and custom-bounds validation
      (accepts 5–16/1..(w×h−9), rejects outside them) in `tests/test_difficulty.cpp`
      (depends on T004)
- [X] T009 [P] doctest: `Board` state transitions — open reveals number/blank, cascade opens all
      connected zero-cells, opening a mine ends in `Lost`, opening the last non-mine cell ends in
      `Won`, flag/unflag no-ops on opened cells and after game end, chord opens neighbors only
      when flagged-count matches (and no-ops otherwise), a mis-flagged chord ends in `Lost`; plus
      a many-seed loop (all three presets + boundary custom configs) asserting the first-opened
      cell is never a mine (SC-002) — in `tests/test_board.cpp` (depends on T006)
- [X] T010 [P] doctest: `GameSession` JSON round-trip is lossless (including a `NotStarted`
      session with no `cells`), and malformed/invalid JSON (bad `schemaVersion`, invalid `status`/
      `state` strings, `cells.length` mismatch, difficulty bounds violation) is rejected via a
      thrown exception, in `tests/test_game_session.cpp` (depends on T007)
- [X] T011 [P] doctest: `Settings` JSON round-trip is lossless and malformed/invalid JSON (bad
      `schemaVersion`, invalid `colorMode`) is rejected via a thrown exception, in
      `tests/test_settings.cpp` (depends on T005)
- [X] T012 Extend `persist::Paths` with `game` and `settings` file paths (alongside/replacing
      `counter`) in `src/persist/paths.h` and `src/persist/paths.cpp`
- [X] T013 Add `bool longPress` to `Tap` and a shared `constexpr int kLongPressMs = 500;` in
      `src/platform/input.h`, per contracts/platform-touch-input.md (FR-009 prerequisite)
- [X] T014 [P] Track touch-down→up duration in `EvdevTouch::waitForTap` and set
      `Tap::longPress` against `kLongPressMs` in `src/platform/kobo/evdev_touch.h` and
      `src/platform/kobo/evdev_touch.cpp` (depends on T013)
- [X] T015 [P] Track mouse-down→up duration in `MouseTouch::waitForTap` and set
      `Tap::longPress` against `kLongPressMs` in `src/platform/sdl/mouse_touch.h` and
      `src/platform/sdl/mouse_touch.cpp` (depends on T013)
- [X] T016 Add an `applyColorMode(Theme&, const DisplayInfo&, ColorMode)` helper (sets
      `Theme::color = display.color && colorMode == ColorMode::Color`) in `src/ui/theme.h` and
      `src/ui/theme.cpp`, per research.md #7 (depends on T005)
- [X] T017 Replace the `App` interface in `src/ui/app.h` with `session()`/`autosaveSession()`/
      `hasInProgressGame()`/`startNewGame(DifficultyConfig)`/`settings()`/`autosaveSettings()`
      (navigation methods unchanged), per contracts/app-interface.md (depends on T005, T007)
- [X] T018 Rewrite `AppImpl` in `src/main.cpp`: constructor loads `game.json`/`settings.json` via
      `persist::loadFile`, on parse/validation failure logs to stderr and discards just that file
      (mirroring the existing `Counter` corrupt-file handling; a bad `game.json` must never touch
      `settings.json` or vice versa, FR-023), applies `applyColorMode` at startup, implements all
      of the T017 `App` interface's non-navigation methods; leave the initial `app.push(...)` call
      and the exit-path autosave calls in `main()` itself for T024 (depends on T012, T016, T017)
- [X] T019 Register the new sources in `CMakeLists.txt`: `src/core/difficulty.cpp`,
      `src/core/board.cpp`, `src/core/game_session.cpp`, `src/core/settings.cpp` in
      `minesweeper_core`; `tests/test_difficulty.cpp`, `tests/test_board.cpp`,
      `tests/test_game_session.cpp`, `tests/test_settings.cpp` in `minesweeper_tests`
      (depends on T004, T005, T006, T007, T008, T009, T010, T011)

**Checkpoint**: `cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON && cmake --build
build/host --config Release && ctest --test-dir build/host -C Release --output-on-failure` passes
in full (Constitution III gate). The `minesweeper` executable itself does not yet link/run a full
session — that lands with User Story 1, the first phase to add concrete `Screen` subclasses.

---

## Phase 3: User Story 1 - Play a game to a win or a loss (Priority: P1) 🎯 MVP

**Goal**: A player can pick a difficulty preset, open cells, and reach a correctly-detected win
or loss with the outcome, difficulty, and elapsed time shown.

**Independent Test**: quickstart.md §2 scenario 1 — start Beginner, open cells until win or loss,
confirm the first-opened cell is never a mine and the end-of-game banner is correct.

### Implementation for User Story 1

- [X] T020 [P] [US1] Create `BoardScreen` in `src/ui/screens/board_screen.h` and
      `src/ui/screens/board_screen.cpp`: `draw()` renders the grid (unopened/opened/flagged
      cells, adjacency numbers 1–8, blank for zero, shape/contrast-only per FR-020) sized to fit
      `session().board` within the display (research.md #8), plus a HUD (remaining-mine count via
      `remainingMineCount()`, elapsed time via `ui::formatTime`), plus a win/loss outcome banner
      (outcome, difficulty, elapsed time, FR-018) (depends on T007, T017)
- [X] T021 [US1] Wire `BoardScreen::onTap` to hit-test the grid and call `session().openCell(x,y)`
      on an unopened cell, `app_.autosaveSession()`, partial-flush the union rect of all cells the
      call revealed (single or cascaded), and full-flush plus draw the outcome banner when
      `status()` transitions to `Won`/`Lost`; taps are ignored once the game has ended
      (depends on T020)
- [X] T022 [US1] Implement `BoardScreen::onTick(activeSeconds)` (calls
      `session().addActiveSeconds(activeSeconds)`, `app_.autosaveSession()`, and partial-flushes
      the timer label only when its displayed minute value changes) and
      `BoardScreen::countsPlayTime()` (`true` only while `status() == InProgress`), per
      research.md #6 (depends on T020)
- [X] T023 [P] [US1] Create `NewGameScreen` in `src/ui/screens/new_game_screen.h` and
      `src/ui/screens/new_game_screen.cpp`: three preset buttons (Beginner/Intermediate/Expert)
      that call `app_.startNewGame(cfg)` and `app_.push(BoardScreen)`; if
      `app_.hasInProgressGame()` is true, first show `ui::Dialog::confirm("Abandon current
      game?", ...)` and only proceed on confirmation, leaving the session untouched on cancel
      (FR-024) (depends on T017)
- [X] T024 [US1] Wire `main()` in `src/main.cpp`: push `BoardScreen` if
      `app.hasInProgressGame()` or the session already ended (`Won`/`Lost`), else push
      `NewGameScreen`; change the exit-path persistence call to
      `app.autosaveSession(); app.autosaveSettings();` (depends on T018, T020, T023)
- [X] T025 [US1] Register `src/ui/screens/board_screen.cpp` and
      `src/ui/screens/new_game_screen.cpp` in `CMakeLists.txt`'s `minesweeper_ui` sources
      (depends on T020, T023)

**Checkpoint**: quickstart.md §2 scenarios 1, 6 (resume across restart), and 7 (abandon
confirmation) all pass in the SDL simulator. This is the MVP — a complete, persistent,
resumable, single-difficulty game loop.

---

## Phase 4: User Story 2 - Flag suspected mines (Priority: P2)

**Goal**: A player can flag/unflag cells via long-press or an explicit Flag Mode toggle, and
flagged cells are protected from accidental opening.

**Independent Test**: quickstart.md §2 scenario 2 — long-press flags/unflags; Flag Mode makes a
plain tap flag instead of open; a flagged cell never opens via a normal tap; the mine counter
reflects flags (including negative).

### Implementation for User Story 2

- [X] T026 [US2] Add a Flag Mode `Button` to `BoardScreen`'s HUD and extend
      `BoardScreen::onTap` per contracts/platform-touch-input.md's routing table: `tap.longPress`
      on an unopened cell → `session().toggleFlag` regardless of Flag Mode; plain tap on an
      unopened cell → `toggleFlag` when Flag Mode is on, else `openCell`; a flagged cell is never
      opened by a plain tap. Each flag/unflag calls `app_.autosaveSession()` and partial-flushes
      the cell + the mine-count HUD label. `flagModeOn_` is a plain ephemeral `bool` member, not
      persisted (per spec Assumptions) — in `src/ui/screens/board_screen.h` and
      `src/ui/screens/board_screen.cpp` (depends on T020, T021, T014, T015)

**Checkpoint**: quickstart.md §2 scenario 2 passes; User Stories 1 and 2 both work.

---

## Phase 5: User Story 3 - Chord an opened cell to clear neighbors quickly (Priority: P2)

**Goal**: Tapping an opened numbered cell whose adjacent flag count matches its number opens all
remaining unflagged neighbors at once.

**Independent Test**: quickstart.md §2 scenario 3 — flag exactly the mines around a numbered
cell, tap it, confirm the rest open at once; a deliberate mis-flag ends the game as a loss.

### Implementation for User Story 3

- [X] T027 [US3] Extend `BoardScreen::onTap` so a tap on an already-opened numbered cell calls
      `session().chord(x,y)`, following the same autosave/partial-flush-union-rect/win-loss
      handling path T021 established, in `src/ui/screens/board_screen.cpp`
      (depends on T021)

**Checkpoint**: quickstart.md §2 scenario 3 passes; User Stories 1–3 all work together.

---

## Phase 6: User Story 4 - Switch and keep a color or black-and-white display (Priority: P2)

**Goal**: A player can switch Color/Black-and-white in Settings, the board stays fully legible
either way, and the choice survives an app restart.

**Independent Test**: quickstart.md §2 scenario 4 — switch to Black-and-white, confirm every
cell state/number is still distinguishable by shape/contrast; restart the app, confirm the mode
persisted.

### Implementation for User Story 4

- [X] T028 [US4] Create `SettingsScreen` in `src/ui/screens/settings_screen.h` and
      `src/ui/screens/settings_screen.cpp`: two mutually-exclusive `Button`s ("Color" /
      "Black-and-white", using `Button::toggled` for the active one) that set
      `app_.settings().colorMode`, call `app_.autosaveSettings()`, re-run `applyColorMode` on the
      live `Theme`, and redraw (depends on T016, T017)
- [X] T029 [US4] Add a "Settings" navigation button (pushes `SettingsScreen`) to `BoardScreen`
      and `NewGameScreen` in `src/ui/screens/board_screen.cpp` and
      `src/ui/screens/new_game_screen.cpp` (depends on T028, T020, T023)
- [X] T030 [US4] Register `src/ui/screens/settings_screen.cpp` in `CMakeLists.txt`'s
      `minesweeper_ui` sources (depends on T028)

**Checkpoint**: quickstart.md §2 scenario 4 passes; User Stories 1–4 all work together.

---

## Phase 7: User Story 5 - Start a game with a custom board size (Priority: P3)

**Goal**: A player can choose a custom width/height/mine count within the supported bounds and
start a game with it.

**Independent Test**: quickstart.md §2 scenario 5 — pick a custom size via the steppers, confirm
it starts and behaves like any other board; confirm the steppers clamp at the FR-002 bounds.

### Implementation for User Story 5

- [X] T031 [US5] Add a custom-size section to `NewGameScreen`: `+`/`-` stepper `Button`s for
      width, height, and mine count (each clamped live to 5–16/5–16/1..(w×h−9) as the other
      fields change, per research.md #9), a `Label` showing current values, and a "Start Custom
      Game" action calling `DifficultyConfig::custom(w,h,mines)` through the same
      `app_.startNewGame`/abandon-confirmation path T023 built, in
      `src/ui/screens/new_game_screen.h` and `src/ui/screens/new_game_screen.cpp`
      (depends on T023, T004)

**Checkpoint**: quickstart.md §2 scenario 5 passes; all five user stories work together.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Bring repo documentation in line with the shipped feature and do a full end-to-end
validation pass.

- [X] T032 [P] Update `docs/contracts/platform-abstraction.md`'s `TouchInput` section to include
      `Tap::longPress`/`kLongPressMs` and each backend's new obligation, per
      contracts/platform-touch-input.md
- [X] T033 [P] Update `docs/settings.md`: describe the new in-app Settings screen (color mode) as
      a first-class settings surface alongside the existing launcher env vars, and update the
      "Files on the device" table (`counter.json` → `game.json`, `settings.json`)
- [ ] T034 Run the full `quickstart.md` validation guide end-to-end — host tests, all 8 simulator
      scenarios, and the corrupted-save recovery check — and fix any discrepancy found
      (depends on all preceding tasks)
      **Partial**: §1 (host `ctest`, all suites) and §3 (corrupted `game.json` recovery) verified
      directly. §2's 8 scenarios need real mouse/touch interaction with the SDL window, which
      isn't available in this CLI environment — process-lifecycle and persisted-JSON checks
      (launch, default screen selection by session status, clean exit-persistence) were used as
      a proxy, but the actual click-through of each acceptance scenario is still open and should
      be run manually before release.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately.
- **Foundational (Phase 2)**: Depends on Setup. BLOCKS every user story — no screen can be built
  until `Board`/`GameSession`/`Settings`/`App` exist and their tests pass.
- **User Stories (Phase 3–7)**: All depend on Foundational. **User Story 1 additionally gates the
  others in practice** (not by requirement, but because it's the phase that creates
  `BoardScreen`/`NewGameScreen` and wires `main()` to actually push a screen and run — US2–US5 all
  edit files US1 creates). Recommended order: US1 → US2 → US3 → US4 → US5 (P1 → P2 → P2 → P2 →
  P3), matching spec.md priority order.
- **Polish (Phase 8)**: Depends on whichever user stories are in scope for the release.

### Within Each Phase

- Foundational: type definitions (T003–T005) → `Board`/`GameSession` (T006–T007) → their tests
  (T008–T011, can be written alongside T006/T007 per Constitution III's "with, or before" rule) →
  persistence/platform/theme plumbing (T012–T016) → `App`/`AppImpl` (T017–T018) → build wiring
  (T019).
- User Story 1: `BoardScreen` skeleton (T020) → tap wiring (T021) → tick wiring (T022);
  `NewGameScreen` (T023) can be built in parallel with T020–T022 (different file); both feed into
  `main()` wiring (T024) and the CMake registration (T025).

### Parallel Opportunities

- T003, T004, T005 (Foundational type definitions — different files, no shared dependency).
- T008, T009, T010, T011 (Foundational tests — different files, each depending only on its own
  already-listed prerequisite).
- T014, T015 (platform backends — different files, both depending only on T013).
- T020 and T023 (`BoardScreen` vs. `NewGameScreen` — different files, both depend only on
  Foundational, not on each other).
- T032, T033 (docs — different files).

---

## Parallel Example: Phase 2 (Foundational)

```bash
# Type definitions — launch together:
Task: "Create CellState enum and Cell struct in src/core/cell.h"
Task: "Create DifficultyConfig (presets + custom validation) in src/core/difficulty.h/.cpp"
Task: "Create Settings (ColorMode + JSON) in src/core/settings.h/.cpp"

# Once Board/GameSession/Settings exist, their tests — launch together:
Task: "doctest DifficultyConfig in tests/test_difficulty.cpp"
Task: "doctest Board in tests/test_board.cpp"
Task: "doctest GameSession in tests/test_game_session.cpp"
Task: "doctest Settings in tests/test_settings.cpp"
```

## Parallel Example: Phase 3 (User Story 1)

```bash
Task: "Create BoardScreen (grid + HUD + outcome banner) in src/ui/screens/board_screen.h/.cpp"
Task: "Create NewGameScreen (presets + abandon-confirm) in src/ui/screens/new_game_screen.h/.cpp"
```

---

## Implementation Strategy

### MVP First (Setup + Foundational + User Story 1)

1. Phase 1: Setup.
2. Phase 2: Foundational — **stop and run `ctest`** (Constitution III gate) before touching UI.
3. Phase 3: User Story 1 — **stop and validate** against quickstart.md §2 scenarios 1, 6, 7 in
   the SDL simulator.
4. This is a shippable MVP: a single-difficulty, persistent, resumable Minesweeper game with no
   flagging, chording, custom sizing, or settings UI yet.

### Incremental Delivery

1. Setup + Foundational → engine ready, fully unit-tested.
2. + User Story 1 → MVP playable game. Demo-able.
3. + User Story 2 → flagging. Demo-able.
4. + User Story 3 → chording. Demo-able.
5. + User Story 4 → color-mode settings. Demo-able.
6. + User Story 5 → custom board sizes. Feature-complete against spec.md.
7. Phase 8 → docs + full validation pass.

### Notes

- [P] tasks touch different files and have no unmet dependency on each other.
- Every core-logic task (T003–T011) is covered by Constitution III's non-negotiable host-test
  gate — do not defer T008–T011 past their corresponding implementation task.
- Commit after each task or logical group; stop at any checkpoint to validate a story
  independently in the SDL simulator before moving on.
- Avoid: adding gameplay logic inside `board_screen.cpp` that belongs in `Board`/`GameSession`
  (Constitution I — keep rules in `core`, keep `ui/` about drawing and input routing only).
