#pragma once
#include <utility>
#include <vector>

#include "core/cell.h"

namespace minesweeper::core {

// Pure game-rules engine for one Minesweeper board. No OS/platform calls
// (Constitution I). Mines are placed lazily on the first openCell() call,
// excluding that exact cell (FR-005).
class Board {
public:
    enum class Status { NotStarted, InProgress, Won, Lost };

    Board(int width, int height, int mineCount);

    int width() const { return width_; }
    int height() const { return height_; }
    int mineCount() const { return mineCount_; }
    Status status() const { return status_; }
    bool minesPlaced() const { return minesPlaced_; }

    const Cell& cellAt(int x, int y) const { return cells_[index(x, y)]; }
    const std::vector<Cell>& cells() const { return cells_; }

    // No-op if the cell is Flagged/Opened, or the game has already ended. Places mines
    // (excluding this cell) on the very first call. Opening a mine ends in Lost; opening a
    // zero-adjacency cell cascades open all connected safe neighbors (FR-006, FR-007, FR-008).
    void openCell(int x, int y);

    // No-op if the cell is Opened, or the game has already ended (FR-011, FR-014).
    void toggleFlag(int x, int y);

    // No-op unless the cell is Opened with adjacentMines > 0, the game is InProgress, and the
    // count of Flagged neighbors equals adjacentMines. Otherwise opens every remaining
    // Unopened neighbor via the same path as openCell (FR-012, FR-013).
    void chord(int x, int y);

    int flaggedCount() const;
    int remainingMineCount() const;  // mineCount() - flaggedCount(); may be negative (FR-015)

    // Persistence support: replace the cell layout/status from an already-validated
    // deserialized board (isMine + state per cell). adjacentMines is recomputed from the
    // isMine layout rather than trusted from storage (data-model.md §GameSession).
    void loadCells(std::vector<Cell> cells, Status status);

private:
    int index(int x, int y) const { return y * width_ + x; }
    bool inBounds(int x, int y) const;
    std::vector<std::pair<int, int>> neighbors(int x, int y) const;

    void placeMines(int excludeX, int excludeY);
    void computeAdjacency();
    void revealFrom(int startIdx);  // iterative cascade flood-fill (research.md #5)
    void checkWin();

    int width_, height_, mineCount_;
    std::vector<Cell> cells_;
    bool minesPlaced_ = false;
    Status status_ = Status::NotStarted;
};

}  // namespace minesweeper::core
