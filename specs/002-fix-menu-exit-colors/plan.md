# Implementation Plan: Menu Layout, Exit Controls, and Mine-Count Colors Fixes

**Branch**: `002-fix-menu-exit-colors` | **Date**: 2026-08-02 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/002-fix-menu-exit-colors/spec.md`

## Summary

Four UI fixes on top of the existing `001-core-gameplay-settings` implementation, all confined to
`src/ui/` plus one small `platform/renderer.h`+`platform/softcanvas.cpp` extension: (1) an `Exit`
control on `NewGameScreen` and `BoardScreen` that calls the already-existing but never-invoked
`App::requestExit()`, closing immediately with no confirmation; (2) two new buttons — "Return to
Menu" and "Exit" — added to `BoardScreen`'s win/loss outcome banner, backed by a new
`App::returnToMainMenu()` that resets the session to `NotStarted` (clearing resume-on-launch
state) and replaces the screen stack with a fresh `NewGameScreen`; (3) a `NewGameScreen` relayout
that groups Beginner/Intermediate/Expert/Settings/Exit together, then doubles the gap before the
"Custom" section; (4) extending the renderer's `Color` accent enum from `{None, Red}` to include
`Blue`, `Green`, `Navy` (deep blue), `Crimson` (cherry red), and `Cyan`, mapped in the shared
`SoftCanvas::resolveColor()` used by both backends, with digits 7 and 8 handled via the existing
grayscale `Gray::Black`/`Gray::Mid` shades rather than new accent colors (black and gray are
already points on the existing gray axis). No new files, no new third-party dependencies, no
`CMakeLists.txt` changes — every change is an edit to an existing file.

## Technical Context

**Language/Version**: C++17 (existing codebase; per constitution)

**Primary Dependencies**: None added. Reuses the existing `Button`/`Label` widgets
(`src/ui/widgets.h/.cpp`), the existing `Renderer`/`Color`/`Gray` contract
(`src/platform/renderer.h`), and the existing shared `SoftCanvas` rasterizer
(`src/platform/softcanvas.h/.cpp`) already used by both the SDL and FBInk backends.

**Storage**: No schema change. `returnToMainMenu()` persists via the existing
`persist::saveFileAtomic`/`GameSession::toJson()` path (`game.json`, schemaVersion 1, unchanged
shape) — it just resets the in-memory `GameSession` to a fresh default (`NotStarted`) before that
existing save call, exactly as `startNewGame()` already does for a chosen difficulty.

**Testing**: doctest, host-run via `-DBUILD_TESTS=ON` + `ctest` (Constitution III). No new core
logic is introduced (see Constitution Check row III below), so no new test files are added;
existing `test_game_session.cpp` coverage of the default `GameSession()` constructor and
`toJson`/`fromJson` round-trip already exercises the state `returnToMainMenu()` resets into. UI/
layout/color changes are verified manually via `quickstart.md`, matching how `001`'s own UI screens
(never host-unit-tested) were validated.

**Target Platform**: Kobo e-ink devices (FBInk backend) and the SDL2 desktop simulator — unchanged;
this feature touches only `src/ui/` and the shared `SoftCanvas` layer both backends already run
through, so both remain pixel-identical to each other exactly as they are today.

**Project Type**: Single project, existing layered structure — no new top-level directories, no
new files at all.

**Performance Goals**: No change to e-ink refresh discipline (Constitution II). The outcome banner
already triggers `flushFull()` when a game ends (`BoardScreen::afterMutation`); adding two buttons
to that same banner draw doesn't add any new redraw trigger. The new Exit/Return buttons are static
once drawn (no per-tick redraw). Digit color is resolved at draw time inside the same
`drawText`/`SoftCanvas` call path already used for every cell — no extra draw pass.

**Constraints**: The new HUD button row on `BoardScreen` goes from 2 buttons (Flag Mode, Settings)
to 3 (Flag Mode, Settings, Exit) sharing the same width, still each `>= Theme::touchTargetPx` tall
per FR-013; on the narrowest supported width this yields a smaller per-button width than today, but
button height (the touch-target-relevant dimension) is unchanged.

**Scale/Scope**: Same single-local-player scope as `001`; no new persisted entities, no new
concurrency, no new device API surface.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design (see below).*

| Principle | Status | Notes |
|---|---|---|
| I. Portable Core, Thin Platform Layer | **PASS** | All new/changed logic lives in `src/ui/` (screens) and the app shell (`AppImpl` in `main.cpp`), or in the shared, backend-agnostic `SoftCanvas`/`Color` value types in `src/platform/`. Nothing new touches `src/platform/kobo/*` or `src/platform/sdl/*` — both backends automatically pick up the new colors and buttons through the same shared rendering path they already share, so no divergent per-backend code is added. |
| II. E-ink-First, Grayscale-First UX | **PASS** | Mine-count digits remain distinguishable in Black & White mode purely by their printed glyph (unchanged — no color was ever the sole signal for digit identity; see research.md #1). New buttons are static UI, not a new per-tick redraw source. The outcome banner's existing `flushFull()` trigger is unchanged; it now just draws two more static buttons in the same pass. |
| III. Host-Testable Correctness (NON-NEGOTIABLE) | **PASS** | No new core state-transition logic: `returnToMainMenu()`'s reset is the existing `GameSession()` default constructor (already covered by `test_game_session.cpp`), and the digit→color mapping is a pure, static lookup with no branching logic worth a doctest beyond what a visual quickstart check already verifies (there is no core/ file touched by this feature at all — `Board`/`GameSession`/`DifficultyConfig`/`Settings` are all unchanged). |
| IV. Firmware-Agnostic Device Integration | **PASS** | No new device API surface; no changes to `EvdevTouch`, `FBInk` usage, or launch mechanism. |
| V. Never Lose the User's Progress | **PASS** | Exit controls call the existing `App::requestExit()`, and `main.cpp`'s loop already unconditionally calls `autosaveSession()`/`autosaveSettings()` after the loop ends regardless of exit cause — so no new persistence code is needed for FR-003/SC-004. `returnToMainMenu()` explicitly calls `autosaveSession()` right after resetting the session, so the cleared (`NotStarted`) state is durably persisted before the screen switch (FR-006). |
| VI. Simplicity and Minimal Dependencies | **PASS** | No new dependencies. New buttons reuse the existing `Button` widget verbatim. The color mapping is one `switch`-style extension of the existing `resolveColor()` function, not a new palette/theming system. One new `App` method (`returnToMainMenu()`) mirrors the existing `push`/`pop`/`requestExit` navigation vocabulary rather than introducing a new concept. |

No violations — Complexity Tracking table is empty (see below).

**Post-Phase-1 re-check**: research.md and data-model.md were reviewed against the same six rows
after design. The screen-stack-reset design for `returnToMainMenu()` (research.md #2) and the
digit-color mapping (research.md #4) were the two decisions with any real design weight; neither
introduces a platform dependency, a new persisted schema, a new color-only meaning, or per-tick
redraw. Gate remains **PASS**, unchanged from the pre-research check above.

## Project Structure

### Documentation (this feature)

```text
specs/002-fix-menu-exit-colors/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md        # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
│   ├── app-interface.md
│   └── digit-color-mapping.md
└── tasks.md             # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── core/                        # UNCHANGED — no file in this feature touches src/core/
│
├── platform/
│   ├── renderer.h                # Color enum gains Blue/Green/Navy/Crimson/Cyan (edit)
│   └── softcanvas.cpp             # resolveColor() gains RGB mapping for the new values (edit)
│
├── ui/
│   ├── app.h                     # App gains `virtual void returnToMainMenu() = 0;` (edit)
│   ├── screens/
│   │   ├── new_game_screen.h/.cpp # Exit button; presets+Settings+Exit group, then a doubled
│   │   │                          # gap, then Custom (edit)
│   │   ├── board_screen.h/.cpp    # 3rd HUD button (Exit); outcome banner gains "Return to
│   │   │                          # Menu"/"Exit" buttons + hit-testing; drawCell's digit accent
│   │   │                          # becomes a per-count lookup instead of a single Color::Red (edit)
│   │
├── main.cpp                      # AppImpl::returnToMainMenu() implementation (edit)

tests/                            # UNCHANGED — existing test_game_session.cpp coverage already
                                   # exercises the GameSession() default state this feature reuses

CMakeLists.txt                    # UNCHANGED — no files added or removed
```

**Structure Decision**: Single project, existing four-layer template structure retained unchanged.
This feature adds zero new files — every change is an edit to a file that already exists from
`001-core-gameplay-settings`. No new directories, no new build targets.

## Complexity Tracking

*No Constitution Check violations — table intentionally empty.*
