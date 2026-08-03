#include "doctest/doctest.h"

#include "core/board_viewport.h"

using namespace minesweeper::core;

TEST_CASE("viewport exactly as large as the board stays pinned at (0,0) regardless of panBy") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(10, 8);
    vp.panBy(100, 100);
    CHECK(vp.panX() == 0);
    CHECK(vp.panY() == 0);
    vp.panBy(-100, -100);
    CHECK(vp.panX() == 0);
    CHECK(vp.panY() == 0);
}

TEST_CASE("panBy clamps at the left/top edge") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);
    vp.panBy(-100, -100);
    CHECK(vp.panX() == 0);
    CHECK(vp.panY() == 0);
}

TEST_CASE("panBy clamps at the right/bottom edge") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);
    vp.panBy(100, 100);
    CHECK(vp.panX() == 10 - 4);
    CHECK(vp.panY() == 8 - 3);
}

TEST_CASE("panBy overshoot from a mid-board position lands exactly at the edge, not beyond") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);
    vp.panBy(2, 2);
    CHECK(vp.panX() == 2);
    CHECK(vp.panY() == 2);
    vp.panBy(-100, -100);
    CHECK(vp.panX() == 0);
    CHECK(vp.panY() == 0);
}

TEST_CASE("setVisibleSize defends against a degenerate zero/negative viewport") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(0, -5);
    CHECK(vp.visibleCols() == 1);
    CHECK(vp.visibleRows() == 1);
}

TEST_CASE("recenterOn is a no-op when the box is already fully visible") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);  // visible x:[0,4), y:[0,3)
    bool changed = vp.recenterOn(0, 0, 3, 2);
    CHECK_FALSE(changed);
    CHECK(vp.panX() == 0);
    CHECK(vp.panY() == 0);
}

TEST_CASE("recenterOn brings a box just past the right edge into view") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);  // visible x:[0,4), y:[0,3); cell (4,1) is just off-screen right
    bool changed = vp.recenterOn(4, 1, 4, 1);
    CHECK(changed);
    // Box now within [panX, panX+4) x [panY, panY+3)
    CHECK(vp.panX() <= 4);
    CHECK(4 < vp.panX() + vp.visibleCols());
    CHECK(vp.panY() <= 1);
    CHECK(1 < vp.panY() + vp.visibleRows());
}

TEST_CASE("recenterOn centers a box larger than the viewport, clamped to board edges") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);
    bool changed = vp.recenterOn(0, 0, 9, 0);  // spans the entire board width
    CHECK(changed);
    // Formula: newPanX = (0+9)/2 - 4/2 = 2; newPanY = (0+0)/2 - 3/2 -> clamped to 0.
    CHECK(vp.panX() == 2);
    CHECK(vp.panY() == 0);
}

TEST_CASE("recenterOn never moves pan beyond the board edges even for an edge-hugging box") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);
    bool changed = vp.recenterOn(9, 7, 9, 7);  // bottom-right corner cell
    CHECK(changed);
    CHECK(vp.panX() == 10 - 4);
    CHECK(vp.panY() == 8 - 3);
}

TEST_CASE("configure re-clamps an out-of-range pan when board dimensions shrink") {
    BoardViewport vp(10, 8);
    vp.setVisibleSize(4, 3);
    vp.panBy(6, 5);  // now at the bottom-right (6,5)
    vp.configure(5, 8);  // board shrinks in width; maxPanX becomes 5-4=1
    CHECK(vp.panX() == 1);
    CHECK(vp.panY() == 5);
}
