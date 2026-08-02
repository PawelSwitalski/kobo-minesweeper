# Implementation Plan: Core Minesweeper Gameplay & Display Settings

**Branch**: `001-core-gameplay-settings` | **Date**: 2026-08-01 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/001-core-gameplay-settings/spec.md`

## Summary

Implement the core Minesweeper game — board generation with deferred (post-first-click) mine
placement, cascading reveal, flagging (long-press and an explicit Flag Mode tap toggle), chording,
win/loss detection, and a mine-count/elapsed-time HUD — plus a Settings screen with a persisted
Color/Black-and-white switch, on top of the existing four-layer Kobo template architecture
(`core` / `persist` / `platform` / `ui`). All new gameplay/session/settings state lives in
`src/core/` with JSON persistence through the existing atomic file store; the `TouchInput`
platform contract gains long-press detection; three new screens (New Game, Board, Settings)
replace the placeholder Counter/About demo the template ships with. No new third-party
dependencies are required.

## Technical Context

**Language/Version**: C++17 (existing codebase; per constitution)

**Primary Dependencies**: nlohmann/json (vendored, header-only — persistence), doctest (vendored,
header-only — host tests), SDL2 (host-only desktop simulator backend), FBInk (vendored — Kobo
device backend). All already present in `third_party/`; no additions.

**Storage**: JSON files via the existing `persist::store` atomic write-then-rename
(`saveFileAtomic`/`loadFile`/`removeFile`), in the app's data directory resolved by
`persist::paths`. Two files, mirroring the existing `counter.json` precedent: `game.json`
(in-progress/ended session) and `settings.json` (color mode), kept separate so a corrupt one
cannot take the other down with it (FR-023).

**Testing**: doctest, host-run via `-DBUILD_TESTS=ON` + `ctest` (Constitution III,
non-negotiable). No device-only test path.

**Target Platform**: Kobo e-ink devices (FBInk render + evdev touch backend) and the SDL2 desktop
simulator, through the existing `Renderer`/`TouchInput` platform abstraction
(`docs/contracts/platform-abstraction.md`).

**Project Type**: Single project, existing layered structure — no new top-level directories.

**Performance Goals**: E-ink partial-refresh discipline (Constitution II) — a single cell open
uses `flushPartial` over the affected rect(s); a cascade covering many cells still issues one
`flushPartial` over the union rect, not one per cell; `flushFull` only on screen transitions and
win/loss. No per-second redraw anywhere (timer updates at minute granularity, FR-016).

**Constraints**: Boards up to 30×16 = 480 cells (Expert) must render and hit-test correctly within
the existing DPI-derived `Theme`; grid cell size is *not* held to the general 9mm touch-target
floor (`Theme::touchTargetPx`) — see research.md for why that's the correct call here, not a gap.

**Scale/Scope**: Single local player, no network, no concurrency; largest persisted state is one
480-cell board (well under any practical file-size concern for the existing JSON store).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design (see below).*

| Principle | Status | Notes |
|---|---|---|
| I. Portable Core, Thin Platform Layer | **PASS** | All game rules/state (`Board`, `GameSession`, `DifficultyConfig`, `Settings`) live in `src/core/`, host-buildable, no platform includes. The one platform-layer change (long-press timing) stays confined to `src/platform/*`, behind the existing `TouchInput` interface — UI/core never see raw input events. |
| II. E-ink-First, Grayscale-First UX | **PASS** | Timer display updates ≤ once/minute during play (FR-016). Every board state (unopened/opened/flagged, 1–8) is shape/contrast-distinguishable per FR-020/SC-003 — no color-only meaning. Color-mode setting is an accent toggle on top of an always-legible grayscale board, not a requirement to see color. Touch targets for real controls (buttons, HUD) still use `Theme::touchTargetPx`; only the game grid's per-cell hit area is exempt (inherent to Minesweeper — see research.md). |
| III. Host-Testable Correctness (NON-NEGOTIABLE) | **PASS** | `Board`/`GameSession`/`DifficultyConfig`/`Settings` are pure core types with doctest coverage: state-transition tests (open/flag/chord/win/loss), a many-seed property test that the first-opened cell is never a mine (SC-002), and JSON round-trip + corrupted-input-rejection tests for both persisted files, mirroring the existing `test_counter.cpp`/`test_persist.cpp` pattern. |
| IV. Firmware-Agnostic Device Integration | **PASS** | No new device API surface. Long-press timing is computed from evdev touch-down/up events the `EvdevTouch` backend already reads — no new firmware dependency. |
| V. Never Lose the User's Progress | **PASS** | `game.json` is written via `saveFileAtomic` after every mutating action (FR-022); a missing/corrupt file degrades to "start at new-game selection" without crashing (FR-023), exactly like the existing `counter.json` handling in `AppImpl`. `settings.json` is persisted and degrades independently. |
| VI. Simplicity and Minimal Dependencies | **PASS** | No new dependencies. New UI is built entirely from the existing `Button`/`Label`/`Dialog` widget vocabulary (custom board size uses +/- stepper `Button`s, not a new text-input widget; the abandon-game confirmation reuses `Dialog::confirm` as-is). |

No violations — Complexity Tracking table is empty (see below).

**Post-Phase-1 re-check**: research.md and data-model.md were reviewed against the same six rows
after design (Phase 0/1) completed. No new platform/OS dependency, no new third-party dependency,
no new color-only meaning, and no per-second redraw were introduced by any design decision (the
`TouchInput` long-press extension, the two-file persistence split, and the theme color-mode
composition were all evaluated against Principles I, V, and II respectively in research.md #1,
#3, and #7). Gate remains **PASS**, unchanged from the pre-research check above.

## Project Structure

### Documentation (this feature)

```text
specs/001-core-gameplay-settings/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md         # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
│   ├── app-interface.md
│   ├── platform-touch-input.md
│   └── persistence-schema.md
└── tasks.md             # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── core/
│   ├── cell.h                  # CellState enum + Cell struct (new)
│   ├── difficulty.h/.cpp       # DifficultyConfig: presets + custom bounds validation (new)
│   ├── board.h/.cpp            # Board: mine placement, open/cascade/flag/chord, win/loss (new)
│   ├── game_session.h/.cpp     # GameSession: Board + DifficultyConfig + elapsed time + JSON (new)
│   ├── settings.h/.cpp         # Settings: ColorMode + JSON (new)
│   ├── counter.h/.cpp          # REMOVED (placeholder demo, superseded)
│
├── persist/
│   ├── paths.h/.cpp            # Paths gains `game` and `settings` file paths (edit)
│   └── store.h/.cpp            # unchanged — reused as-is
│
├── platform/
│   ├── input.h                 # Tap gains `longPress`; shared kLongPressMs threshold (edit)
│   ├── renderer.h              # unchanged
│   ├── kobo/evdev_touch.h/.cpp # tracks down/up timing to set Tap::longPress (edit)
│   ├── sdl/mouse_touch.h/.cpp  # tracks mouse-down/up timing to set Tap::longPress (edit)
│
├── ui/
│   ├── app.h                   # App interface: session()/settings() + autosave hooks (edit)
│   ├── theme.h/.cpp            # gains setColorMode()-style recompute of Theme::color (edit)
│   ├── widgets.h/.cpp          # unchanged — reused as-is (Button/Label/Dialog/formatTime)
│   ├── screens/
│   │   ├── screen.h            # unchanged — onTick/countsPlayTime already fit this feature
│   │   ├── new_game_screen.h/.cpp   # presets + custom stepper + abandon-confirm (new)
│   │   ├── board_screen.h/.cpp      # grid, HUD, tap/long-press/flag-mode/chord (new)
│   │   ├── settings_screen.h/.cpp   # color-mode switch (new)
│   │   ├── counter_screen.h/.cpp    # REMOVED (placeholder demo, superseded)
│   │   ├── about_screen.h/.cpp      # REMOVED (placeholder demo, superseded)
│
├── main.cpp                    # AppImpl updated for the new App surface (edit)

tests/
├── test_board.cpp              # open/cascade/flag/chord/win/loss + first-click-safety (new)
├── test_game_session.cpp       # JSON round-trip + corrupted-input rejection (new)
├── test_settings.cpp           # JSON round-trip + corrupted-input rejection (new)
├── test_difficulty.cpp         # preset values + custom bounds validation (new)
├── test_counter.cpp            # REMOVED (placeholder demo, superseded)
├── test_persist.cpp            # unchanged — persist::store is reused as-is
├── test_smoke.cpp              # unchanged

CMakeLists.txt                  # source lists updated for the above adds/removals (edit)
```

**Structure Decision**: Single project, existing four-layer template structure retained
unchanged (`core` → `persist`/`ui` → `platform` at the edges, wired together in `main.cpp`). No
new directories; this feature only adds files within the existing `src/core/`, `src/ui/screens/`,
and touches `src/platform/*`, `src/persist/paths.*`, `src/ui/app.h`, `src/ui/theme.*`, and
`main.cpp`. The placeholder Counter/About demo (`src/core/counter.*`,
`src/ui/screens/counter_screen.*`, `src/ui/screens/about_screen.*`, `tests/test_counter.cpp`) is
removed, per `SETUP.md`'s own instruction to delete it once real state replaces it.

## Complexity Tracking

*No Constitution Check violations — table intentionally empty.*
