#include "core/board.h"

#include <algorithm>
#include <random>

namespace minesweeper::core {

Board::Board(int width, int height, int mineCount)
    : width_(width),
      height_(height),
      mineCount_(mineCount),
      cells_(static_cast<size_t>(width) * static_cast<size_t>(height)) {}

bool Board::inBounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

std::vector<std::pair<int, int>> Board::neighbors(int x, int y) const {
    std::vector<std::pair<int, int>> out;
    out.reserve(8);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (inBounds(nx, ny)) out.emplace_back(nx, ny);
        }
    }
    return out;
}

void Board::placeMines(int excludeX, int excludeY) {
    int total = width_ * height_;
    int excludeIdx = index(excludeX, excludeY);

    std::vector<int> candidates;
    candidates.reserve(total - 1);
    for (int i = 0; i < total; ++i)
        if (i != excludeIdx) candidates.push_back(i);

    static thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(candidates.begin(), candidates.end(), rng);

    int n = std::min(mineCount_, static_cast<int>(candidates.size()));
    for (int i = 0; i < n; ++i) cells_[candidates[i]].isMine = true;
}

void Board::computeAdjacency() {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            Cell& c = cells_[index(x, y)];
            if (c.isMine) {
                c.adjacentMines = 0;
                continue;
            }
            int n = 0;
            for (auto [nx, ny] : neighbors(x, y))
                if (cells_[index(nx, ny)].isMine) ++n;
            c.adjacentMines = n;
        }
    }
}

void Board::revealFrom(int startIdx) {
    std::vector<int> stack{startIdx};
    while (!stack.empty()) {
        int idx = stack.back();
        stack.pop_back();
        Cell& c = cells_[idx];
        if (c.state != CellState::Unopened) continue;  // already Opened, or protected by a flag
        c.state = CellState::Opened;
        if (c.adjacentMines == 0) {
            int x = idx % width_, y = idx / width_;
            for (auto [nx, ny] : neighbors(x, y)) {
                int nidx = index(nx, ny);
                if (cells_[nidx].state == CellState::Unopened) stack.push_back(nidx);
            }
        }
    }
}

void Board::checkWin() {
    if (status_ != Status::InProgress) return;
    for (const Cell& c : cells_)
        if (!c.isMine && c.state != CellState::Opened) return;
    status_ = Status::Won;
}

void Board::openCell(int x, int y) {
    if (!inBounds(x, y)) return;
    if (status_ != Status::NotStarted && status_ != Status::InProgress) return;

    int idx = index(x, y);
    if (cells_[idx].state != CellState::Unopened) return;

    if (!minesPlaced_) {
        placeMines(x, y);
        computeAdjacency();
        minesPlaced_ = true;
        status_ = Status::InProgress;
    }

    if (cells_[idx].isMine) {
        cells_[idx].state = CellState::Opened;
        status_ = Status::Lost;
        return;
    }

    revealFrom(idx);
    checkWin();
}

void Board::toggleFlag(int x, int y) {
    if (!inBounds(x, y)) return;
    if (status_ == Status::Won || status_ == Status::Lost) return;

    Cell& c = cells_[index(x, y)];
    if (c.state == CellState::Opened) return;
    c.state = (c.state == CellState::Flagged) ? CellState::Unopened : CellState::Flagged;
}

void Board::chord(int x, int y) {
    if (!inBounds(x, y)) return;
    if (status_ != Status::InProgress) return;

    const Cell& c = cells_[index(x, y)];
    if (c.state != CellState::Opened || c.adjacentMines <= 0) return;

    auto nbrs = neighbors(x, y);
    int flagged = 0;
    for (auto [nx, ny] : nbrs)
        if (cells_[index(nx, ny)].state == CellState::Flagged) ++flagged;
    if (flagged != c.adjacentMines) return;

    for (auto [nx, ny] : nbrs) {
        int nidx = index(nx, ny);
        if (cells_[nidx].state != CellState::Unopened) continue;
        if (cells_[nidx].isMine) {
            cells_[nidx].state = CellState::Opened;
            status_ = Status::Lost;
            break;
        }
        revealFrom(nidx);
    }

    checkWin();
}

int Board::flaggedCount() const {
    int n = 0;
    for (const Cell& c : cells_)
        if (c.state == CellState::Flagged) ++n;
    return n;
}

int Board::remainingMineCount() const { return mineCount_ - flaggedCount(); }

void Board::loadCells(std::vector<Cell> cells, Status status) {
    cells_ = std::move(cells);
    minesPlaced_ = true;
    computeAdjacency();
    status_ = status;
}

}  // namespace minesweeper::core
