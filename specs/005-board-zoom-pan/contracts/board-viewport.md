# Contract: `core::BoardViewport`

**Revised 2026-08-03**: unchanged from the original design except the "Call sites" section below —
zoom/pan are now triggered by pinch/drag gestures rather than buttons (see
contracts/board-screen-integration.md, contracts/gesture-recognizer.md). Governs the
`src/core/board_viewport.h/.cpp` type (research.md #2, #5, #6; data-model.md).

## Interface

```cpp
namespace minesweeper::core {

class BoardViewport {
public:
    explicit BoardViewport(int boardWidth = 0, int boardHeight = 0);

    void configure(int boardWidth, int boardHeight);
    void setVisibleSize(int visibleCols, int visibleRows);

    int visibleCols() const;
    int visibleRows() const;
    int panX() const;
    int panY() const;

    void panBy(int dCols, int dRows);
    bool recenterOn(int minX, int minY, int maxX, int maxY);

private:
    void clampPan();
    // ...
};

}  // namespace minesweeper::core
```

## Invariants

1. **Pan never shows space beyond the grid (FR-005).** After any of `configure()`,
   `setVisibleSize()`, `panBy()`, or `recenterOn()` returns, `0 <= panX() <=
   max(0, boardWidth - visibleCols())` and the equivalent for `panY()`/`boardHeight`/`visibleRows()`
   always hold.
2. **`visibleCols()`/`visibleRows()` are always at least 1**, even if `setVisibleSize()` is called
   with a smaller or non-positive value (defensive clamp — a degenerate 0-cell viewport is never
   constructible).
3. **`recenterOn()` is a no-op when the box is already fully visible.** Given
   `minX >= panX() && maxX < panX()+visibleCols() && minY >= panY() && maxY < panY()+visibleRows()`,
   `recenterOn()` returns `false` and leaves `panX()`/`panY()` unchanged.
4. **`recenterOn()` centers on the box's midpoint when the box is not already fully visible**,
   computing `newPanX = (minX+maxX)/2 - visibleCols()/2` (and the `Y` equivalent), then clamping via
   the same rule as Invariant 1. Returns `true` iff the resulting `panX()`/`panY()` differ from
   their values before the call.
5. **A box larger than the viewport is centered as best as possible**, not rejected or clamped to
   only the box's top-left corner — `recenterOn()` never fails or throws for any board-cell-range
   input; the viewport simply shows as much of the box as it can hold, centered.
6. **`panBy()` clamps exactly like `recenterOn()`/`configure()`/`setVisibleSize()`** — panning past
   an edge lands exactly at that edge, never beyond it, and never throws.

## Call sites (`src/ui/screens/board_screen.cpp`)

- `layout()`: `viewport_.configure(board.width(), board.height())` then
  `viewport_.setVisibleSize(visibleCols, visibleRows)` every call (idempotent; board dimensions
  never change during a `BoardScreen`'s lifetime, but re-deriving is simpler than caching a "did
  this change" flag — Constitution VI).
- `onGesture()`, for a `PanStep`: `viewport_.panBy(-dCols, -dRows)`, where `dCols`/`dRows` are
  however many whole cells the accumulated drag-pixel remainder has crossed since the last call
  (research.md #6; contracts/board-screen-integration.md) — a variable, drag-proportional amount,
  not the original design's fixed one-page-per-tap.
- `afterMutation()`: `viewport_.recenterOn(minX, minY, maxX, maxY)` over the bounding box of every
  cell whose state changed in the mutation just applied (research.md #5).

## Host tests (`tests/test_board_viewport.cpp`)

At minimum: `clampPan()` at each of the four board edges (including a viewport exactly as large as
the board — `panX()`/`panY()` always `0`); `recenterOn()` for a box already fully visible (no
change); a box just past one edge (pans minimally to include it); a box spanning more than the
viewport (centers, clamped); and `panBy()` overshooting past an edge (lands exactly at the edge, not
beyond).
