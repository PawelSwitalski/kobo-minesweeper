#include "core/board_viewport.h"

#include <algorithm>

namespace minesweeper::core {

BoardViewport::BoardViewport(int boardWidth, int boardHeight)
    : boardWidth_(boardWidth), boardHeight_(boardHeight) {
    clampPan();
}

void BoardViewport::configure(int boardWidth, int boardHeight) {
    boardWidth_ = boardWidth;
    boardHeight_ = boardHeight;
    clampPan();
}

void BoardViewport::setVisibleSize(int visibleCols, int visibleRows) {
    visibleCols_ = std::max(1, visibleCols);
    visibleRows_ = std::max(1, visibleRows);
    clampPan();
}

void BoardViewport::panBy(int dCols, int dRows) {
    panX_ += dCols;
    panY_ += dRows;
    clampPan();
}

bool BoardViewport::recenterOn(int minX, int minY, int maxX, int maxY) {
    bool alreadyVisible = minX >= panX_ && maxX < panX_ + visibleCols_ && minY >= panY_ &&
                          maxY < panY_ + visibleRows_;
    if (alreadyVisible) return false;

    int oldPanX = panX_, oldPanY = panY_;
    // Centers on the box's midpoint even if only one axis needed it -- simpler than a per-axis
    // decision, and the box is always contained in a single cascade/mutation so both axes moving
    // together stays visually coherent (research.md #5).
    panX_ = (minX + maxX) / 2 - visibleCols_ / 2;
    panY_ = (minY + maxY) / 2 - visibleRows_ / 2;
    clampPan();
    return panX_ != oldPanX || panY_ != oldPanY;
}

void BoardViewport::clampPan() {
    int maxPanX = std::max(0, boardWidth_ - visibleCols_);
    int maxPanY = std::max(0, boardHeight_ - visibleRows_);
    panX_ = std::clamp(panX_, 0, maxPanX);
    panY_ = std::clamp(panY_, 0, maxPanY);
}

}  // namespace minesweeper::core
