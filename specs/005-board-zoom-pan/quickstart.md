# Quickstart: Validating Board Zoom & Pan

**Revised 2026-08-03** for the gesture-based correction (pinch-to-zoom, drag-to-pan; no buttons).

## Prerequisites

- Same as prior features: CMake ≥ 3.20, a C++17 toolchain. Steps 1–2 run on the host via the SDL2
  desktop simulator and the host unit test suite. Step 3 requires real Kobo hardware, since a
  two-finger pinch cannot be simulated with one desktop mouse pointer (research.md #7).

## 1. Host unit tests (new + regression)

```bash
cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON
cmake --build build/host --config Release
ctest --test-dir build/host -C Release --output-on-failure
```

**Expected**: all existing suites still pass unchanged, plus:
- `test_board_viewport.cpp` — clamping at all four board edges, `recenterOn()` cases (already
  visible, just-past-an-edge, larger-than-viewport), and `panBy()` overshoot clamping
  (contracts/board-viewport.md).
- `test_gesture_recognizer.cpp` (new) — the tap/drag/pinch classification cases in
  contracts/gesture-recognizer.md: stationary tap, long-press tap, drag-slop boundary, multi-step
  drag with correct per-step deltas, back-and-forth drag (cumulative not net movement), two-finger
  pinch-out/pinch-in step counts, sub-threshold pinch (no events), a third finger being ignored, and
  a drag that returns near its start before lifting (still no `Tap`).

## 2. Desktop simulator — manual scenario walkthrough

```bash
cmake -B build/sim -DMINESWEEPER_BACKEND=sdl
cmake --build build/sim --config Release
build/sim/Release/minesweeper --width 1264 --height 1680 --dpi 300
```

**Simulator gesture mapping** (research.md #7 — desktop-only stand-in, since a mouse has no second
finger): **mouse wheel** = pinch zoom (each notch = one zoom step, direction matching wheel
direction); **click-drag** (press, move, release) = one-finger pan; a plain **click** (no
significant movement before release) = tap, exactly as before this feature.

Run through each acceptance scenario from `spec.md` directly in the simulator window:

1. **US1 — Zoom in makes cells larger and stays tappable** (FR-001, FR-006, SC-001): Start an
   Expert game. Confirm no zoom/pan buttons are shown anywhere on screen. Scroll the mouse wheel
   "up" (zoom in) once; confirm cells render larger. Click a currently-visible cell; confirm the
   correct cell opens/flags. Continue scrolling up until the maximum zoom level; confirm cells look
   comfortably tap-sized (not cramped). Scroll up once more; confirm nothing further happens (no
   crash, no further growth).
2. **US1 — Cascade auto-recenter** (FR-006a): While zoomed in on an Expert board, click an unopened
   cell likely to trigger a large cascade (a corner cell early in the game is a good bet). Confirm
   the view automatically shifts so the revealed area becomes visible, without needing to pan
   manually afterward.
3. **US2 — Zoom back out** (FR-002, SC-003): From the maximum zoom level, scroll the wheel "down"
   (zoom out) repeatedly. Confirm the board shrinks back to showing the entire grid in a single
   continued scroll, and one more scroll-down at the fit level does nothing further.
4. **US3 — Pan via drag** (FR-004, FR-007): Zoom in until part of the board is off-screen. Click and
   drag across the board; confirm the visible portion shifts in the dragged direction (content
   follows the drag, like a map) and that a drag of only a few pixels doesn't accidentally open a
   cell (FR-007a — the tap-vs-drag disambiguation).
5. **US3 — Pan reaches every edge, never shows blank space** (FR-004, FR-005, SC-002): While zoomed
   in, drag repeatedly toward each edge of the board; confirm the view stops exactly at the edge
   with no blank space beyond the grid ever shown, and that you can reach and successfully
   open/flag a cell in each corner this way.
6. **US3 — Drag does nothing when nothing needs panning** (FR-009): At the default Fit view (or any
   zoom level where the whole board still fits), click-drag across the board; confirm nothing pans
   and no cell is accidentally triggered.
7. **Regression — Beginner never needs panning** (Edge Cases): Start a Beginner game. Zoom in to the
   maximum level; if the board still fits entirely on your test display size, confirm dragging does
   nothing (nothing to pan to) — this is expected, not a bug.
8. **Regression — outcome banner stays usable regardless of zoom/pan** (FR-011): Zoom in and pan
   away from the board's center, then finish the game (open a mine, or complete it). Confirm the
   win/loss banner is fully visible and its buttons are tappable exactly as before.
9. **Regression — zoom/pan never affects game state** (FR-003, SC-004): Zoom in, pan around, and
   zoom back out without opening or flagging anything. Confirm the mine count, elapsed time, and
   every cell's state are unchanged from before you started.
10. **Regression — new game resets the view** (FR-008): While zoomed/panned, finish or abandon the
    game and start a new one. Confirm the new game starts at the default Fit view.

## 3. Real device — two-finger pinch and one-finger drag

The simulator's mouse-wheel/click-drag mapping (step 2) is a desktop stand-in only. The actual
two-finger pinch gesture, and drag responsiveness/`kDragSlopPx`/`kGestureStepPx` tuning (research.md
#3, contracts/gesture-recognizer.md), can only be verified on real Kobo touch hardware. Re-run
scenarios 1–6 above on-device using an actual pinch (two fingers, spread to zoom in / pinch together
to zoom out) and an actual one-finger drag, and confirm the gesture feels responsive without
accidentally triggering cell taps. This is the one part of this feature the host + simulator loop
cannot fully substitute for — flag any tuning-constant adjustments found here in the PR description.

## 4. Regression: existing flows still work

Quickly re-run a couple of earlier quickstart scenarios (long-press flagging, chording, the
Exit/Return-to-Menu controls, the hide-timer toggle, per-digit colors) to confirm this feature's
changes — confined to `BoardScreen`, `core::BoardViewport`, `core::GestureRecognizer`, and the
`TouchInput`/`Screen` interface rename/extension — didn't disturb anything else. In particular,
confirm a game played entirely at the default Fit zoom level (plain taps only, no drag/pinch)
behaves byte-for-byte identically to before this feature.
