# Implementation Plan: Board Zoom & Pan

**Branch**: `005-board-zoom-pan` | **Date**: 2026-08-02 (plan revised 2026-08-03) | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/005-board-zoom-pan/spec.md`

**Revision note (2026-08-03)**: The spec was corrected from tap-driven buttons to real touch
gestures — two-finger pinch to zoom, one-finger drag to pan, no on-screen buttons. This plan is
rewritten accordingly. The zoom-level/viewport geometry (`ZoomLevel`, `core::BoardViewport`) carries
over from the original plan largely unchanged; everything about *how* zoom/pan are triggered is new.

## Summary

Adds gesture-driven zoom (Fit / 2x / 3x, via two-finger pinch) and pan (one-finger drag) to
`BoardScreen` — no on-screen buttons. The hard part is that today's touch input layer only ever
reports a single, completed tap (or long-press) per touch, with no multi-touch or motion tracking on
either backend (Kobo evdev or the SDL simulator). This plan adds a new, pure, host-tested
`core::GestureRecognizer` that classifies a stream of raw touch-point snapshots into `Tap` or
`GestureEvent{ZoomStep|PanStep}`, shared by both backends — each backend becomes a thin adapter that
parses its OS-specific raw input into touch points and feeds this one recognizer (research.md #3).
`TouchInput::waitForTap()` is renamed `waitForEvent()` and can now return mid-gesture (while fingers
are still down), which is what lets zoom/pan update in discrete steps *during* a pinch/drag rather
than only on lift, per FR-012's explicit e-ink decision (no live/continuous redraws). The zoom-level
enum and the pan/viewport clamping+cascade-recenter arithmetic (`core::BoardViewport`) are unchanged
from the original design (research.md #1, #2, #5) — only their trigger changed, from button taps to
gesture events consumed by a new `BoardScreen::onGesture()`. Pan is now proportional to drag distance
(quantized to whole cells via a small pixel-remainder accumulator) rather than the original design's
fixed one-page-per-tap (research.md #6). The SDL simulator can't produce a real two-finger pinch
from one mouse pointer, so it maps mouse-wheel to zoom steps and click-drag to pan, an explicitly
documented simulator-only convention (research.md #7) — real pinch behavior is verified on device
(quickstart.md §3). Net effect on `BoardScreen`'s layout: *simpler* than the original button-based
plan, since no button rows are reserved at all.

## Technical Context

**Language/Version**: C++17 (existing codebase; per constitution)

**Primary Dependencies**: None added. New use of `<variant>` (standard library, C++17) for
`TouchInput::waitForEvent()`'s return type — no new third-party dependency (Constitution VI).

**Storage**: No schema change. Zoom/pan state is intentionally *not* persisted — `game.json` and
`settings.json` are both untouched (spec.md's "Board View" Key Entity is explicitly transient,
resetting to Fit/no-pan whenever a fresh `BoardScreen` is constructed, which already happens on
every new game — research.md #4, unchanged from the original design).

**Testing**: doctest, host-run via `-DBUILD_TESTS=ON` + `ctest` (Constitution III). Two new test
files: `tests/test_board_viewport.cpp` (unchanged scope from the original plan — clamping and
cascade-recenter arithmetic) and `tests/test_gesture_recognizer.cpp` (new — the tap/drag/pinch
classification state machine, contracts/gesture-recognizer.md's full case list). Both are pure
`core::` logic with no OS dependency, so both run identically on every host. The `ZoomLevel →
multiplier` mapping and the pixel-to-whole-cell pan accumulator division remain trivial UI-layer
code (`src/ui/`-only, not linked into `minesweeper_tests`), verified via `quickstart.md`'s manual
scenarios — matching the `002`/`004` precedent for this category of code (research.md #1, #6).

**Target Platform**: Kobo e-ink devices (FBInk backend) and the SDL2 desktop simulator — unchanged.
Every zoom/pan/recenter step still triggers one `flushFull()`, the same discrete, player-triggered
full-refresh pattern every other state-changing action in this app already uses — FR-012 makes this
an explicit requirement now, not just an implementation choice: the view must *not* live-track
fingers, only step at defined thresholds (Constitution II).

**Project Type**: Single project, existing layered structure. One new file pair
(`src/core/gesture_recognizer.h/.cpp`, plus its test) in addition to the original plan's
`src/core/board_viewport.h/.cpp`; every other change is an edit to a file that already exists —
`platform/input.h`, `ui/screens/screen.h`, `ui/screens/board_screen.h/.cpp`, `main.cpp`,
`platform/kobo/evdev_touch.h/.cpp`, `platform/sdl/mouse_touch.h/.cpp`, `CMakeLists.txt`.

**Performance Goals**: No change to e-ink refresh discipline beyond the discrete, threshold-stepped
redraws FR-012 now specifies explicitly. Redraw cost per zoom/pan step is bounded by the viewport's
visible cell count, unchanged from the original plan's analysis — zooming in never increases
per-refresh redraw cost, since fewer cells are visible at a time.

**Constraints**: `BoardScreen`'s layout gains *no* new reserved rows (the corrected, button-free
design is simpler here than the original plan) — the grid keeps the full space below the existing
HUD/nav row. The new constraint is instead architectural: `TouchInput::waitForEvent()` must be able
to return before a touch lifts (to satisfy FR-012's "step during the gesture" requirement), which is
a real behavioral change from today's "blocks until a complete down→up cycle" `waitForTap()`.

**Scale/Scope**: Same single-local-player scope as prior features. No new persisted entities, no new
concurrency, no new device API surface beyond reading more `ABS_MT_*` axes than before on the evdev
backend (still the same `/dev/input/eventN` device, no new device file).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design (see below).*

| Principle | Status | Notes |
|---|---|---|
| I. Portable Core, Thin Platform Layer | **PASS** | `core::GestureRecognizer` (new) and `core::BoardViewport` (unchanged from original plan) are pure integer arithmetic over touch points / board-cell coordinates — no OS, filesystem, rendering, or input calls. `EvdevTouch`/`MouseTouch` remain the only OS-specific code, and this revision *shrinks* their responsibility to "parse raw input into `TouchPoint`s, feed the shared recognizer" rather than growing bespoke per-backend gesture logic — a thinner platform layer than a naive multi-touch implementation would produce, better satisfying this principle's letter than the alternative (research.md #3). `ZoomLevel` and `cellSizePx_` derivation stay in `src/ui/screens/board_screen.h/.cpp`, unchanged in kind from the original plan. |
| II. E-ink-First, Grayscale-First UX | **PASS** | FR-012 (new) makes the constitution's "no frequent/non-deliberate redraws" requirement an explicit functional requirement for this feature: gesture steps redraw in discrete jumps at threshold crossings, never continuously tracking a finger, never with a smooth/animated transition. Every zoom/pan step is still one discrete, gesture-triggered `flushFull()` — the same category as an existing Settings change or screen transition, just triggered by a pinch/drag step instead of a button tap. Touch targets are unaffected (no buttons were ever a factor for grayscale/color meaning here). |
| III. Host-Testable Correctness (NON-NEGOTIABLE) | **PASS** | This revision *adds* a host-tested unit specifically because gesture classification is real, non-obvious, edge-case-bearing logic (tap vs. drag vs. pinch, slot tracking, threshold crossings) — exactly what this principle targets. Extracting it into `core::GestureRecognizer` (rather than leaving it inline in `evdev_touch.cpp`/`mouse_touch.cpp`, which aren't linked into `minesweeper_tests`) is a stronger application of this principle than the original button-based plan needed to make, since that plan had no comparable input-classification logic at all. `core::BoardViewport`'s testing scope is unchanged from the original plan. |
| IV. Firmware-Agnostic Device Integration | **PASS** | Still no new device API surface — `EvdevTouch` reads more `ABS_MT_*` axes (`ABS_MT_SLOT`, `ABS_MT_TRACKING_ID` per slot) from the same `/dev/input/eventN` evdev device it already opens; these are standard Linux multi-touch protocol (MT-B) axes already queried defensively today (`hasAbsAxis()`), not a firmware-private API. |
| V. Never Lose the User's Progress | **PASS** | Zoom/pan state is still deliberately *not* persisted (spec.md's Key Entity + Assumptions, unchanged) and never touches `GameSession`/`Settings` — `autosaveSession()`'s call sites and `game.json`'s shape are completely unchanged by this feature. |
| VI. Simplicity and Minimal Dependencies | **PASS** | No new third-party dependency (`<variant>` is standard library). The corrected design is *simpler* on the UI-layout side than the original plan (no button rows, no two-step layout, no new `Button` fields) — the added complexity is concentrated entirely in one new, narrowly-scoped, host-tested class (`GestureRecognizer`) that both backends share, rather than duplicated per-backend logic. No new generic "input event bus" or speculative gesture types beyond `ZoomStep`/`PanStep`, which are exactly what this feature's two gestures need. |

No violations — Complexity Tracking table is empty (see below).

**Post-Phase-1 re-check**: research.md, data-model.md, and both contracts were reviewed against the
same six rows after design. The two decisions with real design weight are `GestureRecognizer`'s
placement in `core/` (research.md #3) and the change to `TouchInput`'s blocking semantics (return
mid-gesture, not just on lift) — neither introduces a platform dependency, a new persisted field, or
an *automatic* (non-gesture-triggered) redraw source. Gate remains **PASS**, unchanged from the
pre-research check above.

## Project Structure

### Documentation (this feature)

```text
specs/005-board-zoom-pan/
├── plan.md                        # This file (/speckit-plan command output)
├── research.md                    # Phase 0 output (/speckit-plan command)
├── data-model.md                  # Phase 1 output (/speckit-plan command)
├── quickstart.md                  # Phase 1 output (/speckit-plan command)
├── contracts/                      # Phase 1 output (/speckit-plan command)
│   ├── board-viewport.md
│   ├── board-screen-integration.md
│   └── gesture-recognizer.md      # NEW in this revision
└── tasks.md                        # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── core/
│   ├── board_viewport.h/.cpp        # NEW (unchanged from original plan) — host-testable pan/
│   │                                 # viewport geometry (clamp, recenter)
│   ├── gesture_recognizer.h/.cpp    # NEW in this revision — host-testable tap/drag/pinch
│   │                                 # classification (TouchPoint in, Tap|GestureEvent out)
│   ├── board.h/.cpp                  # UNCHANGED — openCell()/chord()/toggleFlag() gameplay logic
│   │                                 # is not touched; BoardScreen only reads Board::cells() as
│   │                                 # it already does
│
├── platform/
│   ├── input.h                       # EDIT — Tap unchanged; TouchInput::waitForTap() renamed
│   │                                 # waitForEvent(), returns optional<variant<Tap,
│   │                                 # core::GestureEvent>>
│   ├── kobo/evdev_touch.h/.cpp       # EDIT — tracks multiple ABS_MT_SLOT contacts per SYN_REPORT,
│   │                                 # feeds core::GestureRecognizer, returns as soon as it yields
│   │                                 # a result (not only on lift)
│   ├── sdl/mouse_touch.h/.cpp        # EDIT — click-drag feeds core::GestureRecognizer as a single
│   │                                 # touch point; mouse wheel constructs GestureEvent::ZoomStep
│   │                                 # directly (research.md #7)
│
├── ui/
│   ├── screens/
│   │   ├── screen.h                  # EDIT — new virtual onGesture(core::GestureEvent) {}
│   │   │                             # (default no-op) alongside existing onTap(Tap)
│   │   ├── board_screen.h/.cpp       # ZoomLevel enum, baseCellSizePx_/cellSizePx_ split, a
│   │                                 # core::BoardViewport member, panAccumPxX_/panAccumPxY_,
│   │                                 # onGesture() override; layout()/draw()/afterMutation()/
│   │                                 # onTap() updated for viewport-relative coordinates and the
│   │                                 # cascade-recenter path (edit) — no new Button fields, no new
│   │                                 # layout rows
│   │   ├── new_game_screen.h/.cpp    # UNCHANGED — inherits the new onGesture() default no-op
│   │   ├── settings_screen.h/.cpp    # UNCHANGED — same
│
main.cpp                              # EDIT — waitForTap() → waitForEvent() call site; dispatches
                                       # to onTap() or onGesture() depending on which alternative
                                       # the variant holds

tests/
├── test_board_viewport.cpp           # NEW (unchanged scope) — clamping + recenterOn() edge cases
├── test_gesture_recognizer.cpp       # NEW in this revision — tap/drag/pinch classification cases
                                       # (contracts/gesture-recognizer.md)

CMakeLists.txt                        # board_viewport.cpp + gesture_recognizer.cpp added to
                                       # minesweeper_core; test_board_viewport.cpp +
                                       # test_gesture_recognizer.cpp added to minesweeper_tests (edit)
```

**Structure Decision**: Single project, existing four-layer template structure retained unchanged.
Two new file pairs (plus their tests) are added to `src/core/` specifically so both the pan/viewport
geometry and the gesture-classification logic are host-testable (Principle III); every other change
is an edit to a file that already exists. No new widget types, no new screens.

## Complexity Tracking

*No Constitution Check violations — table intentionally empty.*
