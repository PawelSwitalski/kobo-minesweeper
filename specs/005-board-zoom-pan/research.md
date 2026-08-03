# Phase 0 Research: Board Zoom & Pan

**Revised 2026-08-03** for the gesture-based correction (two-finger pinch to zoom, one-finger drag
to pan; no on-screen buttons). Sections #1, #2, #4, #5 below carry over from the original
button-based research largely unchanged, since they concern zoom-level/viewport geometry, not the
input mechanism. Sections #3, #6, #7 are new or substantially rewritten to replace the button
design with real multi-touch gesture recognition.

No items in Technical Context are `NEEDS CLARIFICATION`. Research is grounded in direct inspection
of `src/platform/input.h`, `src/platform/kobo/evdev_touch.cpp`, `src/platform/sdl/mouse_touch.cpp`,
`src/ui/screens/board_screen.h/.cpp`, `src/ui/screens/screen.h`, `src/main.cpp`, `src/core/`, and
`CMakeLists.txt`, plus the app's own default target geometry (`src/main.cpp`: 1264×1680 @300dpi,
"Kobo Libra Colour geometry").

## 1. Zoom levels: fixed 2x/3x multipliers of the existing "Fit" baseline (unchanged from original)

**Decision**: `enum class ZoomLevel { Fit, Zoom2x, Zoom3x }` (UI-layer, `board_screen.h`), exactly as
originally researched. `Fit` keeps today's fit-to-screen cell-size formula as `baseCellSizePx_`;
`Zoom2x`/`Zoom3x` multiply it by 2/3.

**What changes in this revision**: only the *trigger*. Previously a "Zoom In"/"Zoom Out" button tap
advanced/retreated one level. Now a `GestureEvent{ZoomStep, delta=+1}` (pinch-out) advances one
level (clamped at `Zoom3x`) and `delta=-1` (pinch-in) retreats one level (clamped at `Fit`) — see
#3 for where `ZoomStep` comes from. The verified-against-target-geometry math (Expert's Fit cell is
40px; `Zoom3x` reaches 120px, clearing the 106px touch-target floor for SC-001) is unchanged.

**Alternatives considered**: unchanged from the original — a dynamically-derived max zoom level was
rejected (could make the top level identical to Fit on small boards), and a continuous zoom slider
was rejected as needless complexity layered on top of quantized pinch steps (see #6).

## 2. Pan/viewport geometry: `core::BoardViewport` (unchanged from original)

**Decision**: unchanged — `src/core/board_viewport.h/.cpp` remains exactly as originally designed:
pure pan-clamping and FR-006a cascade-recenter arithmetic, no OS calls, linked into
`minesweeper_core` for host-testability (Constitution III), mirroring `003`'s
`core::ActiveTimeTracker` precedent.

**What changes in this revision**: only *who calls* `panBy()`. Previously, four directional pan
button handlers called `panBy(±visibleCols_, 0)` / `panBy(0, ±visibleRows_)` — one full page per
tap. Now `BoardScreen::onGesture()` calls `panBy(dCols, dRows)` with a variable cell count derived
from the drag distance (see #6). `BoardViewport`'s public contract, invariants, and host tests
(`tests/test_board_viewport.cpp`) are untouched by this revision.

## 3. Gesture recognition: a new, host-tested `core::GestureRecognizer`, not per-backend logic

**Decision**: Add `src/core/gesture_recognizer.h/.cpp` — a pure state machine that takes a stream of
raw touch-point snapshots (one call per input-poll cycle, each carrying every currently-down
contact's `{slot, x, y}` in display coordinates) plus a caller-supplied monotonic timestamp, and
emits at most one of `Tap` or `GestureEvent` per call:

```cpp
namespace minesweeper::core {

struct TouchPoint { int slot; int x; int y; };  // one physical contact, this poll cycle

enum class GestureKind { ZoomStep, PanStep };
struct GestureEvent {
    GestureKind kind;
    int zoomDelta = 0;       // ZoomStep: +1 pinch-out (zoom in) / -1 pinch-in (zoom out)
    int dxPx = 0, dyPx = 0;  // PanStep: single-finger drag delta since the last emitted step
};

class GestureRecognizer {
public:
    // points: every contact currently down (empty = all fingers lifted this cycle).
    std::optional<std::variant<GestureTap, GestureEvent>> feed(const std::vector<TouchPoint>& points,
                                                                int64_t nowMs);
    // GestureTap, not platform::Tap -- keeps core/ free of any platform/ include; backends
    // convert 1:1 when constructing their waitForEvent() return value.
private: /* slot/gesture tracking state -- see contracts/gesture-recognizer.md */
};

}  // namespace minesweeper::core
```

Both backends (`evdev_touch.cpp` for Kobo, `mouse_touch.cpp` for the SDL simulator) become thin
adapters: parse their OS-specific raw input into a `vector<TouchPoint>` per poll cycle (applying the
existing swap/mirror/scale transform per-point, unchanged from today), feed it to an owned
`GestureRecognizer`, and return as soon as `feed()` yields something.

**Rationale**: this is the one piece of new logic with real, combinatorial edge cases —
distinguishing a stationary tap from the start of a drag, tracking which of up to two slots moved
and by how much, ignoring a third contact, and deciding when accumulated pinch/drag distance has
crossed a step threshold. `evdev_touch.cpp`/`mouse_touch.cpp` are platform-backend files, not linked
into `minesweeper_tests` (`CMakeLists.txt` only links `minesweeper_core` + `minesweeper_persist`
into the test binary — the same fact `003`'s `ActiveTimeTracker` precedent and this feature's own
`BoardViewport` (#2) were extracted to `core/` to satisfy). Leaving gesture classification inline in
the evdev/SDL files would put exactly the kind of state-machine logic Constitution III calls out
(non-obvious edge cases, needs exhaustive testing) in the one place it's least testable. This also
mirrors an existing precedent already in this codebase: `platform/input.h`'s own comment on
`kLongPressMs` — "shared by every TouchInput backend... so the 'what counts as long' policy lives
here rather than per-backend" — just taken one step further into a fully host-testable class instead
of a header-only constant, because pinch/drag classification has materially more edge cases than a
single duration comparison.

**A real control-flow change, not just a new class**: today, `waitForTap()` blocks until a complete
down→up cycle finishes (or times out) — it never returns mid-touch. Gesture steps must fire *while
fingers are still down* (FR-012's discrete-step-during-the-gesture requirement), so the backends'
poll loop now returns as soon as `feed()` produces an event — either a completed `Tap`, or a
`GestureEvent` fired mid-gesture — not only on lift. `TouchInput::waitForTap()` is renamed
`waitForEvent()` and its return type becomes `std::optional<std::variant<Tap, GestureEvent>>`.
`Screen` gains `virtual void onGesture(GestureEvent) {}` (default no-op) alongside the existing
`onTap(Tap)`; `main.cpp`'s loop dispatches to whichever alternative `waitForEvent()` returned. Only
`BoardScreen` overrides `onGesture()` — `NewGameScreen`/`SettingsScreen` are untouched, since they
have no zoom/pan surface.

**Alternatives considered**:
- *Classify gestures inline per-backend* (in `evdev_touch.cpp` and `mouse_touch.cpp` separately):
  rejected — duplicates the same tap/drag/pinch decision logic twice, untestable on host in either
  copy, and prone to the two backends silently drifting in behavior (e.g., different slop
  thresholds) since there'd be no shared source of truth.
- *Put the recognizer under `src/platform/` as a shared-but-not-per-backend file*: rejected in favor
  of `src/core/` specifically so it's linked into `minesweeper_tests` — `src/platform/` files are
  never part of the test binary today, and there is no existing precedent for treating some platform
  files as test-linked and others not.
- *Keep `waitForTap()`'s blocking-until-lift semantics and only emit gesture info once the gesture
  ends*: rejected outright — this directly contradicts FR-012 (discrete steps *during* the gesture,
  e.g. so a long continuous pinch can walk through Fit → Zoom2x → Zoom3x without needing to lift and
  re-place fingers between each step).

## 4. Zoom/pan state lives on `BoardScreen`, resetting "for free" on every new game (unchanged)

**Decision**: unchanged from the original — `zoomLevel_` and the `core::BoardViewport` member are
plain `BoardScreen` fields, default-initialized to `Fit`/`(0,0)`. Every code path that shows a
`BoardScreen` already constructs a fresh instance (research.md's original #4 citations of
`NewGameScreen::startGame()` and `App::returnToMainMenu()` still hold, unchanged by this revision),
so FR-008 is satisfied with zero explicit reset code, exactly as before.

## 5. Cascade recenter: center the changed-cell bounding box, clamped to board edges (unchanged)

**Decision**: unchanged — `BoardScreen::afterMutation()` computes the changed-cell bounding box from
the diff loop it already runs, and calls `viewport_.recenterOn(...)`; a `true` return triggers a full
redraw instead of the existing partial-redraw path. None of this depends on how zoom/pan are
triggered, so the gesture-based correction leaves it untouched.

## 6. Pan step size: proportional to drag distance, quantized to whole cells (replaces "one page per tap")

**Decision**: `GestureRecognizer` reports raw pixel deltas (`dxPx`, `dyPx`) for each `PanStep`,
measured in a fixed screen-pixel threshold (`kGestureStepPx`, see #3's contract) independent of
board/zoom specifics — the recognizer has no notion of cell size. `BoardScreen` accumulates these
into two small remainder counters, `panAccumPxX_`/`panAccumPxY_`; whenever the magnitude of an
accumulator reaches the *current* `cellSizePx_`, it calls `viewport_.panBy(wholeCells, 0)` (or the Y
equivalent) for however many whole cells fit, and keeps the signed remainder for the next step. This
makes roughly one cell's width of on-screen finger travel move the view by one cell — proportional,
map-like panning — rather than the original design's fixed one-page-per-tap jump.

**Rationale**: FR-012 rules out live, continuous finger-following, but a full-page jump per small
gesture-step threshold (the original button design's granularity) would feel jarring for a drag
gesture whose whole premise is "the view follows your finger, just not smoothly." Quantizing to
whole cells at a *cell-proportional* rate is the closest discrete approximation to a real drag-to-pan
map gesture available without continuous redraws, and it reuses `BoardViewport::panBy()` (#2) and
its edge-clamping unchanged — only the caller-supplied cell count differs from the original design.
The division/remainder arithmetic itself (`accum / cellSizePx_`, `accum % cellSizePx_`, sign-correct
for negative drags) is simple, has no correctness-critical edge case beyond what integer division
already guarantees, and stays in `board_screen.cpp` as ordinary UI-layer code — verified via
`quickstart.md`'s manual scenarios, matching the `002`/`004` precedent for this category of trivial,
non-`core::` arithmetic (the same category the original research.md #1 placed the
`ZoomLevel → multiplier` lookup in).

**Alternatives considered**:
- *Keep the fixed one-page-per-gesture-step*: rejected as a jarring mismatch with the "drag like a
  map" mental model the whole feature correction is about.
- *Make `GestureRecognizer` itself cell-size-aware and emit whole-cell deltas directly*: rejected —
  would require the recognizer to be reconfigured every time `cellSizePx_` changes (i.e., every zoom
  level change), coupling a portable, backend-agnostic input class to board-specific UI state for no
  real benefit over doing the trivial division in `BoardScreen`, which already owns `cellSizePx_`.

## 7. SDL desktop simulator: mouse wheel stands in for pinch, mouse-drag stands in for one-finger pan

**Decision**: The desktop simulator has one mouse pointer, not two fingers, so it cannot produce a
real pinch. `mouse_touch.cpp` maps: a mouse-button-down-drag-up cycle feeds a single-slot
`TouchPoint` stream to the same shared `GestureRecognizer` used by the real device (exercising
`Tap`/`PanStep` identically to a one-finger touch); a mouse-wheel notch is translated directly to one
`GestureEvent{ZoomStep, delta=sign(wheel)}`, bypassing `GestureRecognizer`'s pinch-distance math
entirely, since there is no way to simulate a second contact point from one pointer.

**Rationale**: This is an explicit, documented simulator-only convenience, exactly analogous to how
the simulator's mouse click already stands in for a finger tap today (`mouse_touch.cpp`'s existing
`SDL_MOUSEBUTTONDOWN`/`UP` → `Tap` mapping). It keeps the one-finger path (tap vs. drag
disambiguation, the harder-to-get-right half of `GestureRecognizer`) exercised identically on both
backends, and gives developers a fast, real way to test zoom without needing actual multi-touch
hardware at the desk — while the plan/quickstart are explicit that true two-finger pinch behavior is
only verified on real Kobo hardware (README's existing "not yet tested on real Kobo hardware"
caveat already covers this project-wide).

**Alternatives considered**:
- *Require holding a keyboard modifier + dragging vertically to simulate pinch*: rejected — more
  contrived than the wheel, and drag is already claimed for pan simulation.
- *Skip simulator zoom/pan gesture testing entirely, only test on device*: rejected — the simulator
  is this project's primary fast-iteration loop (`docs/building.md`); losing the ability to
  exercise zoom/pan at all on host would significantly slow development and defeat Constitution I's
  "core MUST run on the development host" intent as applied to this feature's UI layer.
