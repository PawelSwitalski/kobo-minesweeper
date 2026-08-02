# Implementation Plan: Fix Game Timer & Add Hide-Timer Setting

**Branch**: `003-fix-timer-hide-option` | **Date**: 2026-08-02 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/003-fix-timer-hide-option/spec.md`

## Summary

Two changes on top of the existing `001-core-gameplay-settings` / `002-fix-menu-exit-colors`
implementation: (1) a correctness fix for the elapsed-play-time bug, root-caused to
`main.cpp`'s event loop resetting its tick reference (`lastTickSteady`) on *every* iteration
(`main.cpp:270`) while only ever accumulating active seconds (`screen->onTick(...)`) on iterations
that did **not** produce a tap (`main.cpp:242-253`) — so every interval between two taps is
silently dropped from `GameSession::elapsedSeconds_` during active, tap-heavy play, the exact
opposite of when the timer should be most reliable. The fix extracts the tick-timing arithmetic out
of `main.cpp`'s untestable raw loop into a new small, portable type, `core::ActiveTimeTracker`
(`src/core/active_time_tracker.h/.cpp`), and calls `screen->onTick(...)` on *every* loop iteration
(tap or not) using that tracker, so no interval is ever dropped. (2) A new "Hide Timer" setting
(`core::Settings::hideTimer`, default `false`) toggled from `SettingsScreen` via one new `Button`
(mirroring the existing color-mode buttons and `BoardScreen`'s own toggle-style `flagModeButton_`),
which `BoardScreen::drawTimer()`/`onTick()` check to skip drawing/redrawing the live HUD timer —
the win/loss outcome banner's elapsed-time label is untouched and keeps showing the final time
regardless (per the 2026-08-02 clarification), since it never read `hideTimer` to begin with. No
new files beyond `active_time_tracker.h/.cpp` and their test; every other change is an edit to a
file that already exists.

## Technical Context

**Language/Version**: C++17 (existing codebase; per constitution)

**Primary Dependencies**: None added. Reuses the existing `Button`/`Label` widgets
(`src/ui/widgets.h/.cpp`), `core::Settings` persistence pattern (`src/core/settings.h/.cpp`), and
`std::chrono::steady_clock` (already used in `main.cpp`).

**Storage**: `settings.json` gains one optional field, `hideTimer` (bool, default `false` when
absent so existing installs' `settings.json` files — which predate this field — still parse
without error). `schemaVersion` stays `1` (additive, backward-compatible field; see research.md
#4). `game.json`'s schema is completely unchanged — the timer fix only changes *how reliably*
`elapsedSeconds` is accumulated, never its shape.

**Testing**: doctest, host-run via `-DBUILD_TESTS=ON` + `ctest` (Constitution III). New:
`tests/test_active_time_tracker.cpp` (synthetic timestamps reproducing the tap-heavy undercount
bug and proving the fix, plus the existing pause/sleep-exclusion behavior) and an extended
`tests/test_settings.cpp` (round-trip + default + old-file-without-`hideTimer` backward
compatibility). `core::ActiveTimeTracker` is added to the `minesweeper_core` static library
specifically so it links into `minesweeper_tests` (today only `minesweeper_core` +
`minesweeper_persist` do, per `CMakeLists.txt:99-111`) — see research.md #2 for why this placement,
not `main.cpp` itself, is what makes the fix host-testable at all.

**Target Platform**: Kobo e-ink devices (FBInk backend) and the SDL2 desktop simulator —
unchanged. The loop restructuring in `main.cpp` is backend-agnostic (it sits above the
`Renderer`/`TouchInput` interfaces), so both backends get the fix identically.

**Project Type**: Single project, existing layered structure. One new file pair
(`src/core/active_time_tracker.h/.cpp`) plus its test; every other touched file already exists.

**Performance Goals**: No change to e-ink refresh discipline (Constitution II). The hide-timer
setting *reduces* redraw/partial-flush work when on (`BoardScreen::onTick` skips the
minute-boundary `drawTimer()` + `flushPartial()` call entirely when `hideTimer` is true, since
there is nothing on screen to update). The timer fix adds no new redraw — it only changes which
loop iterations feed `addActiveSeconds`, not how often the HUD repaints (still capped at once per
displayed minute, per FR-003, unchanged from `001`).

**Constraints**: `screen->onTick(...)` now runs on every loop iteration instead of only on
no-tap iterations — including on tap iterations that also call `BoardScreen::afterMutation()`,
which independently calls `app_.autosaveSession()`. This means a tap-and-mutate iteration now
autosaves twice in a row (once from `onTick`, once from `afterMutation`) instead of once. This is
accepted as harmless: `autosaveSession()` is an idempotent atomic write-then-rename of the current
in-memory state (Constitution V), so the extra call costs one redundant file write, never
correctness or data loss.

**Scale/Scope**: Same single-local-player scope as `001`/`002`; no new persisted entities, no new
concurrency, no new device API surface.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design (see below).*

| Principle | Status | Notes |
|---|---|---|
| I. Portable Core, Thin Platform Layer | **PASS** | The new `core::ActiveTimeTracker` takes `std::chrono::steady_clock::time_point` values as plain parameters and does no OS/rendering/input calls itself (`main.cpp` alone still calls `steady_clock::now()`) — it is pure arithmetic on given timestamps, same category as `GameSession::addActiveSeconds(uint32_t)` already is. `main.cpp`'s loop restructuring stays entirely in the platform-shell layer it already lives in. `Settings::hideTimer` follows the exact pattern `Settings::colorMode` already established. |
| II. E-ink-First, Grayscale-First UX | **PASS** | No new per-tick redraw source: the HUD timer still updates at most once per displayed minute (FR-003), unchanged. Hiding the timer *removes* a redraw trigger rather than adding one. `hideTimer` is a plain on/off preference with no color dependency — fully legible/usable in Black & White mode. |
| III. Host-Testable Correctness (NON-NEGOTIABLE) | **PASS** | This is the crux of the design: the timing bug lives in `main.cpp`, which is not linked into `minesweeper_tests` (`CMakeLists.txt:99-111` links only `minesweeper_core` + `minesweeper_persist`), so leaving the fix inline in `main.cpp` would leave it exactly as untestable as the code that shipped the bug in the first place. Extracting the tick-timing bookkeeping into `core::ActiveTimeTracker` (added to `minesweeper_core`) makes the fixed behavior — including the specific tap-heavy scenario that exposed the bug — directly host-testable with synthetic timestamps, no event loop or real clock needed. `Settings::hideTimer`'s round-trip and backward-compatible-default behavior are host-tested exactly like `colorMode` already is. |
| IV. Firmware-Agnostic Device Integration | **PASS** | No new device API surface; `EvdevTouch`/`FBInk` usage is untouched. The loop restructuring only changes when `onTick`/`onTap` are called relative to each other, not any device call. |
| V. Never Lose the User's Progress | **PASS** | `onTick`'s existing `app_.autosaveSession()` call now simply runs more often (every loop iteration a screen counts play time, instead of only no-tap iterations) — strictly more frequent persistence, never less (see Constraints above for the one accepted harmless side effect: an occasional redundant double-save on the same iteration). `hideTimer` persists via the existing `autosaveSettings()` path unchanged. |
| VI. Simplicity and Minimal Dependencies | **PASS** | No new dependencies. `ActiveTimeTracker` is one small class with a single `tick()` method — no new abstraction layer, no event system. The hide-timer toggle reuses the existing `Button` widget exactly as `flagModeButton_` already demonstrates for a single on/off toggle (simpler than the two-button mutually-exclusive pattern `colorMode` uses, since `hideTimer` is a plain boolean, not a 2-way choice). |

No violations — Complexity Tracking table is empty (see below).

**Post-Phase-1 re-check**: research.md and data-model.md were reviewed against the same six rows
after design. The `ActiveTimeTracker` placement decision (research.md #2) and the
`onTick`-runs-every-iteration restructuring (research.md #1) are the two decisions with real design
weight; neither introduces a platform dependency, a new persisted schema shape (only one additive,
backward-compatible field), or a new per-tick redraw source. Gate remains **PASS**, unchanged from
the pre-research check above.

## Project Structure

### Documentation (this feature)

```text
specs/003-fix-timer-hide-option/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md         # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/            # Phase 1 output (/speckit-plan command)
│   ├── active-time-tracker.md
│   └── persistence-schema.md
└── tasks.md              # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── core/
│   ├── active_time_tracker.h/.cpp  # NEW — extracted, host-testable tick-timing bookkeeping
│   ├── settings.h/.cpp              # Settings gains `bool hideTimer = false;` + JSON round-trip (edit)
│   ├── game_session.h/.cpp          # UNCHANGED — addActiveSeconds()'s own logic is not the bug
│   ├── board.cpp / difficulty.cpp   # UNCHANGED
│
├── ui/
│   ├── screens/
│   │   ├── board_screen.h/.cpp      # drawTimer() and onTick() check settings().hideTimer;
│   │   │                            # drawOutcomeBanner() intentionally untouched (edit)
│   │   ├── settings_screen.h/.cpp   # new hideTimerButton_ toggle, below the color-mode row (edit)
│
├── main.cpp                         # loop restructured to use ActiveTimeTracker and call
│                                     # screen->onTick(...) every iteration, not just no-tap ones (edit)

tests/
├── test_active_time_tracker.cpp     # NEW — reproduces the tap-heavy undercount scenario + fix
├── test_settings.cpp                # extended: hideTimer default, round-trip, old-file compat (edit)

CMakeLists.txt                       # active_time_tracker.cpp added to minesweeper_core sources;
                                      # test_active_time_tracker.cpp added to minesweeper_tests (edit)
```

**Structure Decision**: Single project, existing four-layer template structure retained unchanged.
One new file pair (plus its test) is added to the existing `src/core/` layer specifically so the
timer fix is host-testable (Principle III); every other change is an edit to a file that already
exists from `001`/`002`. No new top-level directories, no new build targets.

## Complexity Tracking

*No Constitution Check violations — table intentionally empty.*
