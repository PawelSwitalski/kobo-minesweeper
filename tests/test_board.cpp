#include "doctest/doctest.h"

#include "core/board.h"

using namespace minesweeper::core;

namespace {
// Builds a board with an explicit mine layout for deterministic testing, bypassing the
// random first-click-safe placement (that placement is separately covered by SC-002 below).
Board makeBoard(int w, int h, const std::vector<std::pair<int, int>>& minePositions) {
    Board b(w, h, static_cast<int>(minePositions.size()));
    std::vector<Cell> cells(static_cast<size_t>(w) * static_cast<size_t>(h));
    for (auto [mx, my] : minePositions) cells[static_cast<size_t>(my * w + mx)].isMine = true;
    b.loadCells(std::move(cells), Board::Status::InProgress);
    return b;
}
}  // namespace

TEST_CASE("open reveals a number or a blank") {
    // Mine adjacent to (0,0) gives it a nonzero adjacentMines count, so opening it reveals
    // just a number with no cascade.
    Board b = makeBoard(3, 3, {{1, 0}, {2, 2}});
    b.openCell(0, 0);
    CHECK(b.cellAt(0, 0).state == CellState::Opened);
    CHECK(b.cellAt(0, 0).adjacentMines == 1);
    CHECK(b.status() == Board::Status::InProgress);
}

TEST_CASE("cascade opens all connected zero-adjacency cells") {
    // Single mine in the far corner: opening the opposite corner cascades the whole
    // zero-region, opening every non-mine cell -- clearing the whole board ends in Won.
    Board b = makeBoard(5, 5, {{4, 4}});
    b.openCell(0, 0);
    int openedCount = 0;
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            if (b.cellAt(x, y).state == CellState::Opened) ++openedCount;
    CHECK(openedCount == 24);
    CHECK(b.status() == Board::Status::Won);
}

TEST_CASE("opening a mine ends the game in Lost") {
    Board b = makeBoard(3, 3, {{1, 1}});
    b.openCell(1, 1);
    CHECK(b.status() == Board::Status::Lost);
    CHECK(b.cellAt(1, 1).state == CellState::Opened);
}

TEST_CASE("opening the last non-mine cell ends the game in Won") {
    Board b = makeBoard(2, 2, {{1, 1}});
    b.openCell(0, 0);
    b.openCell(1, 0);
    b.openCell(0, 1);
    CHECK(b.status() == Board::Status::Won);
}

TEST_CASE("flag/unflag toggles on unopened cells, no-ops on opened cells and after game end") {
    // Mine adjacent to (0,0) keeps its open from cascading (so the board isn't cleared in one
    // move); the second mine at (2,2) is opened later to end the game in Lost.
    Board b = makeBoard(3, 3, {{1, 0}, {2, 2}});
    b.toggleFlag(0, 0);
    CHECK(b.cellAt(0, 0).state == CellState::Flagged);
    b.toggleFlag(0, 0);
    CHECK(b.cellAt(0, 0).state == CellState::Unopened);

    b.openCell(0, 0);
    b.toggleFlag(0, 0);  // opened cell: no-op
    CHECK(b.cellAt(0, 0).state == CellState::Opened);
    REQUIRE(b.status() == Board::Status::InProgress);

    b.openCell(2, 2);  // hits the second mine -> Lost
    REQUIRE(b.status() == Board::Status::Lost);
    b.toggleFlag(1, 1);  // after game end: no-op
    CHECK(b.cellAt(1, 1).state == CellState::Unopened);
}

TEST_CASE("chord opens remaining neighbors only when flagged count matches adjacentMines") {
    // Center cell surrounded by 4 corner mines; the 4 edge-center cells are safe.
    Board b = makeBoard(3, 3, {{0, 0}, {2, 0}, {0, 2}, {2, 2}});
    b.openCell(1, 1);
    REQUIRE(b.cellAt(1, 1).state == CellState::Opened);
    REQUIRE(b.cellAt(1, 1).adjacentMines == 4);

    b.chord(1, 1);  // no flags yet -> no-op
    CHECK(b.cellAt(1, 0).state == CellState::Unopened);

    b.toggleFlag(0, 0);
    b.toggleFlag(2, 0);
    b.toggleFlag(0, 2);
    b.chord(1, 1);  // only 3 of 4 flagged -> no-op
    CHECK(b.cellAt(1, 0).state == CellState::Unopened);
    CHECK(b.status() == Board::Status::InProgress);

    b.toggleFlag(2, 2);
    b.chord(1, 1);  // all 4 correctly flagged -> opens the remaining safe neighbors
    CHECK(b.cellAt(1, 0).state == CellState::Opened);
    CHECK(b.cellAt(0, 1).state == CellState::Opened);
    CHECK(b.cellAt(2, 1).state == CellState::Opened);
    CHECK(b.status() == Board::Status::Won);
}

TEST_CASE("openCell and chord are no-ops once the game has ended") {
    // Mine adjacent to (0,0) keeps its open from cascading, so (0,0) stays a plain numbered
    // opened cell after the game ends via the second mine at (2,2).
    Board b = makeBoard(3, 3, {{1, 0}, {2, 2}});
    b.openCell(0, 0);
    b.openCell(2, 2);  // hits the mine -> Lost
    REQUIRE(b.status() == Board::Status::Lost);

    b.openCell(1, 1);  // openCell no-op after game end
    CHECK(b.cellAt(1, 1).state == CellState::Unopened);

    b.chord(0, 0);  // (0,0) is opened & numbered, but chord no-op after game end
    CHECK(b.cellAt(0, 1).state == CellState::Unopened);
}

TEST_CASE("chording opens all N remaining safe neighbors in one action, for every achievable N") {
    // An interior cell has exactly 8 neighbors. Chording requires adjacentMines > 0 (a cell
    // with 0 adjacent mines never needs chording -- it is already opened by the cascade in
    // FR-007), so at least one neighbor must be a mine. That caps the number of safe neighbors
    // a single chord can open at 7, not 8: SC-006's "every value of N from 1 to 8" is only
    // achievable for N = 1..7 for a standard 8-neighbor cell. This covers the full achievable
    // range.
    const std::vector<std::pair<int, int>> offsets = {
        {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1},
    };
    for (int mineCount = 1; mineCount <= 7; ++mineCount) {
        int expectedSafeOpened = 8 - mineCount;

        std::vector<std::pair<int, int>> mines;
        for (int i = 0; i < mineCount; ++i)
            mines.push_back({3 + offsets[static_cast<size_t>(i)].first,
                             3 + offsets[static_cast<size_t>(i)].second});

        Board b = makeBoard(7, 7, mines);
        b.openCell(3, 3);
        REQUIRE(b.cellAt(3, 3).state == CellState::Opened);
        REQUIRE(b.cellAt(3, 3).adjacentMines == mineCount);

        for (auto [mx, my] : mines) b.toggleFlag(mx, my);
        b.chord(3, 3);

        int safeOpened = 0;
        for (int i = 0; i < 8; ++i) {
            bool isMine = i < mineCount;
            if (isMine) continue;
            int nx = 3 + offsets[static_cast<size_t>(i)].first;
            int ny = 3 + offsets[static_cast<size_t>(i)].second;
            if (b.cellAt(nx, ny).state == CellState::Opened) ++safeOpened;
        }
        CAPTURE(mineCount);
        CHECK(safeOpened == expectedSafeOpened);
    }
}

TEST_CASE("a mis-flagged chord that opens a mine ends in Lost") {
    Board b = makeBoard(3, 3, {{0, 0}, {2, 0}});
    b.openCell(1, 1);
    REQUIRE(b.cellAt(1, 1).adjacentMines == 2);
    // Flag two safe cells instead of the two real mines -- count matches (2) but the
    // flags are wrong, so chording still tries to open the actual mines.
    b.toggleFlag(1, 0);
    b.toggleFlag(0, 1);
    b.chord(1, 1);
    CHECK(b.status() == Board::Status::Lost);
}

TEST_CASE("the first-opened cell is never a mine, across presets and boundary custom configs") {
    struct Cfg { int w, h, mines; };
    std::vector<Cfg> configs = {
        {9, 9, 10},              // Beginner
        {16, 16, 40},            // Intermediate
        {30, 16, 99},            // Expert
        {5, 5, 1},                // smallest custom
        {16, 16, 16 * 16 - 9},   // largest custom mine density
    };
    for (const Cfg& cfg : configs) {
        for (int trial = 0; trial < 50; ++trial) {
            Board b(cfg.w, cfg.h, cfg.mines);
            b.openCell(0, 0);
            CHECK(b.cellAt(0, 0).isMine == false);
        }
    }
}
