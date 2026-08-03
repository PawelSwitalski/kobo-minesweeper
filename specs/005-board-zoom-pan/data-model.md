# Phase 1 Data Model: Board Zoom & Pan

**Revised 2026-08-03** for the gesture-based correction. This feature introduces **no new persisted
entities and no changes to any existing JSON schema** — `game.json` and `settings.json` are both
completely unchanged; zoom/pan is transient, in-memory state only (research.md #4). What follows are
the new non-persisted types.

## `Tap` (`src/platform/input.h`) — unchanged

```cpp
struct Tap { int x = 0, y = 0; bool longPress = false; };
```

## `GestureTap` (`src/core/gesture_recognizer.h`) — NEW

```cpp
struct GestureTap { int x = 0, y = 0; bool longPress = false; };
```

Deliberately not named `Tap` (platform/input.h's type): `core::GestureRecognizer` emits this
instead of `platform::Tap` so `core/` has no `#include "platform/..."` dependency (Constitution I
— no existing `core/` file has ever included a `platform/` header, and this preserves that).
Backends convert it 1:1 into a `Tap` when constructing their `waitForEvent()` return value.

## `GestureEvent` / `GestureKind` (`src/core/gesture_recognizer.h`) — NEW

```cpp
enum class GestureKind { ZoomStep, PanStep };

struct GestureEvent {
    GestureKind kind;
    int zoomDelta = 0;       // ZoomStep only: +1 pinch-out (zoom in) / -1 pinch-in (zoom out)
    int dxPx = 0, dyPx = 0;  // PanStep only: signed drag delta (display px) since the last step
};
```

Not persisted. See research.md #3 for why this replaces the original design's `Button` fields, and
#6/#7 for how `PanStep`/`ZoomStep` are produced on each backend.

## `TouchPoint` (`src/core/gesture_recognizer.h`) — NEW

```cpp
struct TouchPoint { int slot; int x; int y; };  // one physical contact, this poll cycle
```

Input to `GestureRecognizer::feed()`; never stored, never crosses the `TouchInput` interface itself
(backends consume it internally to drive their owned recognizer).

## `core::GestureRecognizer` (`src/core/gesture_recognizer.h/.cpp`) — NEW, not persisted

Pure gesture-classification state machine. No OS/rendering calls (Constitution I); lives in `core/`
specifically for host-testability (Constitution III, research.md #3).

```cpp
namespace minesweeper::core {

class GestureRecognizer {
public:
    // points: every contact currently down this poll cycle (empty => all fingers lifted).
    // nowMs: caller-supplied monotonic clock (mirrors ActiveTimeTracker's caller-clock pattern),
    // used only for Tap's longPress determination.
    std::optional<std::variant<GestureTap, GestureEvent>> feed(const std::vector<TouchPoint>& points,
                                                                int64_t nowMs);

private:
    // One-finger state: down position/time, whether the drag-slop threshold has been exceeded
    // yet, and the position last reported as a PanStep (for computing the next step's delta).
    // Two-finger state: the two tracked slot ids, and the inter-point distance last reported as
    // a ZoomStep (for computing the next step's delta). A third simultaneous contact is ignored.
};

}  // namespace minesweeper::core
```

**Invariants** (see contracts/gesture-recognizer.md for the full list and host test plan):

- Exactly one event is emitted per `feed()` call at most (never both a `Tap` and a `GestureEvent`).
- A single contact that never moves beyond `kDragSlopPx` before lifting yields exactly one `Tap` on
  the lift call, with `longPress` set iff `nowMs(lift) - nowMs(down) >= kLongPressMs`.
- A single contact that moves beyond `kDragSlopPx` before lifting yields zero or more `PanStep`
  events (one per `kGestureStepPx` of cumulative movement) and **no** `Tap` at any point, including
  on lift.
- Exactly two simultaneous contacts yield zero or more `ZoomStep` events (one per `kGestureStepPx` of
  cumulative inter-point-distance change) and never a `Tap` or `PanStep`.
- A third simultaneous contact is ignored entirely — only the first two tracked slots affect
  classification.

## `TouchInput` (`src/platform/input.h`) — CHANGED

```cpp
class TouchInput {
public:
    virtual ~TouchInput() = default;
    // Blocks up to timeoutMs; returns a completed Tap, a mid-gesture GestureEvent, or nothing on
    // timeout. Unlike the pre-005 waitForTap(), this can return while fingers are still down, so
    // FR-012's discrete-step-during-the-gesture requirement can be met.
    virtual std::optional<std::variant<Tap, core::GestureEvent>> waitForEvent(int timeoutMs) = 0;
};
```

Renamed from `waitForTap()`. Both backends (`EvdevTouch`, `MouseTouch`) own a
`core::GestureRecognizer` and implement `waitForEvent()` by parsing their OS-specific raw input into
`core::TouchPoint`s each poll cycle and feeding the recognizer (research.md #3, #7).

## `ui::Screen` (`src/ui/screens/screen.h`) — CHANGED

```cpp
virtual void onTap(Tap tap) = 0;                       // unchanged
virtual void onGesture(core::GestureEvent) {}          // NEW, default no-op
```

Only `BoardScreen` overrides `onGesture()`. `NewGameScreen` and `SettingsScreen` are unchanged —
they have no zoom/pan surface and never receive gesture events in practice (nothing feeds them a
two-finger pinch or a drag long enough to cross `kGestureStepPx` while those screens are on top,
but the default no-op override exists so every `Screen` subclass compiles without one).

## `ZoomLevel` (enum, `src/ui/screens/board_screen.h`) — NEW, unchanged from original design

```cpp
enum class ZoomLevel { Fit, Zoom2x, Zoom3x };
```

Same as originally designed (research.md #1) — only the trigger changed, from a button tap to a
`GestureEvent{ZoomStep, ...}` handled in `BoardScreen::onGesture()`.

## `core::BoardViewport` (`src/core/board_viewport.h/.cpp`) — NEW, unchanged from original design

Public API, invariants, and host tests are exactly as originally designed (research.md #2) — see
contracts/board-viewport.md, only its caller changed (a `panBy()` call driven by accumulated drag
pixels via `BoardScreen::onGesture()`, instead of a directional button handler).

## `BoardScreen` (`src/ui/screens/board_screen.h/.cpp`)

New non-persisted members (screen-local UI/view state, same category as the existing
`flagModeOn_`):

| Field | Type | Purpose |
|---|---|---|
| `zoomLevel_` | `ZoomLevel` | Current zoom level; defaults to `Fit` (research.md #4: resets "for free" via fresh construction on every new game). |
| `baseCellSizePx_` | `int` | The Fit-level cell size, recomputed every `layout()` exactly as `cellSizePx_` was before this feature. |
| `viewport_` | `core::BoardViewport` | Pan position + visible-cell-window size, reconfigured every `layout()`. |
| `panAccumPxX_`, `panAccumPxY_` | `int` | Sub-cell drag-pixel remainder carried between `PanStep` events (research.md #6); reset to `0` whenever `layout()` changes `cellSizePx_` (a mid-drag zoom is out of scope — `Screen`s only ever receive one input source of truth at a time). |

Removed from the original (button-based) design: `panControlsActive_` and all six `Button` fields
(`zoomInButton_`, `zoomOutButton_`, `panLeftButton_/panUpButton_/panDownButton_/panRightButton_`) —
there are no on-screen zoom/pan buttons in the corrected design, so `BoardScreen`'s layout is
actually *simpler* than the original button-based plan: no reserved Zoom row, no conditionally
reserved Pan row, and no two-step layout pass (research.md's original #3 is gone entirely). The
board reclaims that vertical space in every case.

`cellSizePx_` (existing field) becomes `baseCellSizePx_ × multiplier(zoomLevel_)` instead of always
equalling the fit-to-screen size — the one existing field whose *meaning* changes; every other
existing field (`gridRect_`, `mineCountRect_`, `timerRect_`, the HUD/nav buttons) keeps its prior
meaning unchanged.

## Relationships (delta over `001`–`004`)

```text
BoardScreen
├─ zoomLevel_: ZoomLevel                  — NEW; drives cellSizePx_ = baseCellSizePx_ × multiplier
├─ viewport_: core::BoardViewport         — NEW; drives which board cells draw()/cellRect() map to
│                                            screen coordinates, and what onTap() maps a tap back to
├─ panAccumPxX_/panAccumPxY_: int         — NEW; sub-cell drag remainder (research.md #6)
├─ draw()                                 — iterates [panY_, panY_+visibleRows_) × [panX_, panX_+
│                                            visibleCols_) instead of the full board (identical at
│                                            Fit, since visibleCols_==boardWidth there)
├─ onGesture(GestureEvent)                — NEW; ZoomStep advances/retreats zoomLevel_ (clamped);
│                                            PanStep accumulates into panAccumPxX_/Y_ and calls
│                                            viewport_.panBy() for each whole cell crossed
└─ afterMutation()                        — gains a changed-cell bounding-box pass feeding
                                             viewport_.recenterOn(); a true return switches that
                                             mutation's redraw from partial to full (research.md #5)

core::Board / core::GameSession           — UNCHANGED; BoardScreen still only calls the same
                                             openCell()/toggleFlag()/chord() and reads the same
                                             Board::cells() it already did

TouchInput::waitForEvent()                — CHANGED (was waitForTap()); returns Tap | GestureEvent
core::GestureRecognizer                   — NEW; owned by EvdevTouch and MouseTouch, feeds Tap/
                                             GestureEvent classification from raw touch points
```
