#pragma once

namespace minesweeper::core {

enum class CellState { Unopened, Opened, Flagged };

struct Cell {
    bool isMine = false;
    CellState state = CellState::Unopened;
    int adjacentMines = 0;  // valid only when isMine == false; computed at mine-placement time
};

}  // namespace minesweeper::core
