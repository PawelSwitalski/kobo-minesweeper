# Contract: `BoardScreen` zoom/pan integration

**Revised 2026-08-03** for the gesture-based correction. Governs how
`src/ui/screens/board_screen.h/.cpp` (existing file) integrates `ZoomLevel` and
`core::BoardViewport` into `layout()`, `draw()`, `afterMutation()`, and the new `onGesture()`
(research.md #1, #2, #5, #6; data-model.md). `onTap()` is unchanged from before this feature except
for the coordinate-mapping math it already needed for a zoomed/panned viewport.

## `layout()` — simpler than the original button-based design

No button rows are reserved at all — there are no on-screen zoom/pan controls in the corrected
design (FR-007). `layout()` keeps its pre-`005` shape (HUD row, nav row, then the grid) with only
the cell-size formula changed:

```text
1. Lay out HUD row, nav row (Flag Mode / Settings / Exit) exactly as today -- unchanged from before
   this feature entirely.
2. gridTop = (bottom of nav row) + gap                       # unchanged from pre-005
   availW = displayWidth - 2*pad
   availH = displayHeight - gridTop - pad
   baseCellSizePx_ = min(availW / board.width(), availH / board.height())   # unchanged formula
   cellSizePx_ = baseCellSizePx_ * multiplier(zoomLevel_)
   visibleCols_ = zoomLevel_ == Fit ? board.width()  : min(board.width(),  availW  / cellSizePx_)
   visibleRows_ = zoomLevel_ == Fit ? board.height() : min(board.height(), availH / cellSizePx_)
3. viewport_.configure(board.width(), board.height())
   viewport_.setVisibleSize(visibleCols_, visibleRows_)
4. gridDrawW = viewport_.visibleCols() * cellSizePx_
   gridDrawH = viewport_.visibleRows() * cellSizePx_
   gridRect_ = { (displayWidth - gridDrawW) / 2, gridTop, gridDrawW, gridDrawH }   # horizontally
                                                                                     # centered,
                                                                                     # top-anchored
                                                                                     # -- same
                                                                                     # convention
                                                                                     # as before
```

At `ZoomLevel::Fit`, `visibleCols_ == board.width()` and `visibleRows_ == board.height()` directly
(not derived from pixel division), so the board is always fully visible and `viewport_`'s pan stays
`(0, 0)`, matching pre-`005` behavior exactly.

## `cellRect(x, y)` / `draw()` / `afterMutation()` — unchanged from the original design

These three are identical to the original (button-based) plan — none of them cared *how* zoom/pan
were triggered, only the resulting `viewport_`/`cellSizePx_` state:

```cpp
Rect BoardScreen::cellRect(int x, int y) const {
    return {gridRect_.x + (x - viewport_.panX()) * cellSizePx_,
            gridRect_.y + (y - viewport_.panY()) * cellSizePx_,
            cellSizePx_, cellSizePx_};
}
```

```cpp
for (int y = viewport_.panY(); y < viewport_.panY() + viewport_.visibleRows(); ++y)
    for (int x = viewport_.panX(); x < viewport_.panX() + viewport_.visibleCols(); ++x)
        drawCell(x, y);
```

`afterMutation()`'s cascade bounding-box + `viewport_.recenterOn()` call (FR-006a, research.md #5)
is unchanged.

## `onTap()` — tap-to-cell mapping, unchanged

```cpp
if (!gridRect_.contains({tap.x, tap.y})) return;
int cx = viewport_.panX() + (tap.x - gridRect_.x) / cellSizePx_;
int cy = viewport_.panY() + (tap.y - gridRect_.y) / cellSizePx_;
if (cx < 0 || cx >= board.width() || cy < 0 || cy >= board.height()) return;
```

Satisfies FR-006 by construction, exactly as originally designed. Because
`core::GestureRecognizer` (contracts/gesture-recognizer.md) never emits a `Tap` for a touch that
moved past the drag-slop threshold, `onTap()` never has to guard against a drag-that-should-have-
been-a-pan reaching it as a false tap (FR-007a is satisfied upstream, not here).

## `onGesture(GestureEvent)` — NEW, replaces all button wiring

```cpp
void BoardScreen::onGesture(core::GestureEvent g) {
    if (g.kind == core::GestureKind::ZoomStep) {
        ZoomLevel next = zoomLevel_;
        if (g.zoomDelta > 0) {                                     // pinch-out: zoom in one step
            if (zoomLevel_ == ZoomLevel::Fit) next = ZoomLevel::Zoom2x;
            else if (zoomLevel_ == ZoomLevel::Zoom2x) next = ZoomLevel::Zoom3x;
            // already Zoom3x: next stays Zoom3x (clamped)
        } else if (g.zoomDelta < 0) {                              // pinch-in: zoom out one step
            if (zoomLevel_ == ZoomLevel::Zoom3x) next = ZoomLevel::Zoom2x;
            else if (zoomLevel_ == ZoomLevel::Zoom2x) next = ZoomLevel::Fit;
            // already Fit: next stays Fit (clamped)
        }
        // A no-op step (already at the clamped end) returns without a redraw, satisfying
        // "further pinch has no additional effect" (US1 Scenario 3 / US2 Scenario 2) with no
        // extra guard state.
        if (next == zoomLevel_) return;
        zoomLevel_ = next;
        panAccumPxX_ = panAccumPxY_ = 0;   // stale sub-cell remainder from the old cell size
        layout();                          // recompute cellSizePx_/viewport_ for the new level
        draw();
        app_.renderer().flushFull();
        return;
    }
    // PanStep
    if (viewport_.visibleCols() == board.width() && viewport_.visibleRows() == board.height())
        return;  // nothing to pan to (FR-009) -- board already fully visible
    panAccumPxX_ += g.dxPx;
    panAccumPxY_ += g.dyPx;
    int dCols = panAccumPxX_ / cellSizePx_;   // truncates toward zero; sign-correct for drags
    int dRows = panAccumPxY_ / cellSizePx_;   // in either direction (research.md #6)
    if (dCols == 0 && dRows == 0) return;     // hasn't accumulated a whole cell yet -- no redraw
    panAccumPxX_ -= dCols * cellSizePx_;
    panAccumPxY_ -= dRows * cellSizePx_;
    viewport_.panBy(-dCols, -dRows);  // drag right (+dxPx) reveals cells to the right => pan left
    draw();
    app_.renderer().flushFull();
}
```

The `panBy(-dCols, -dRows)` sign is deliberate: dragging a finger right (positive `dxPx`) is the
same "push the content right" motion as panning a map — the *visible window* moves left (toward
lower board-cell x), revealing board content that was previously off-screen to the left. This
matches every standard drag-to-pan / drag-to-scroll convention (the content follows the finger, the
viewport moves opposite to it).

No explicit "already at max/min zoom" or "already at a board edge" guard is needed beyond what's
shown above — `ZoomLevel` clamps at its own type's ends, and `BoardViewport::panBy()` already clamps
pan to the board edges internally (contracts/board-viewport.md, Invariant 1), so an edge-hitting
`PanStep` simply results in `viewport_`'s pan position not changing further, satisfying US3
Scenario 2 with no additional state.
