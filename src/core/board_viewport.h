#pragma once

namespace minesweeper::core {

// Pan position + visible-cell-window size over a board grid (contracts/board-viewport.md,
// specs/005-board-zoom-pan). Pure integer arithmetic -- no OS/rendering calls -- so it stays
// host-testable per Constitution I, mirroring the ActiveTimeTracker precedent.
class BoardViewport {
public:
    explicit BoardViewport(int boardWidth = 0, int boardHeight = 0);

    void configure(int boardWidth, int boardHeight);        // idempotent; re-clamps pan
    void setVisibleSize(int visibleCols, int visibleRows);  // viewport capacity, in cells; re-clamps

    int visibleCols() const { return visibleCols_; }
    int visibleRows() const { return visibleRows_; }
    int panX() const { return panX_; }
    int panY() const { return panY_; }

    void panBy(int dCols, int dRows);  // clamped to board edges (FR-005)

    // Adjusts pan (if needed) so the inclusive bounding box [minX,minY]-[maxX,maxY] is centered
    // and as fully visible as possible; clamped to board edges. Returns true iff pan changed.
    bool recenterOn(int minX, int minY, int maxX, int maxY);  // FR-006a

private:
    void clampPan();

    int boardWidth_, boardHeight_;
    int visibleCols_ = 1, visibleRows_ = 1;
    int panX_ = 0, panY_ = 0;
};

}  // namespace minesweeper::core
