#pragma once
#include <cstdint>
#include <string>

#include "core/board.h"
#include "core/difficulty.h"

namespace minesweeper::core {

// Wraps one Minesweeper game: the chosen difficulty, the Board engine, and accumulated
// active-play time. Thin delegation to Board for gameplay methods (data-model.md §GameSession).
class GameSession {
public:
    GameSession();                          // NotStarted, no difficulty chosen yet
    explicit GameSession(DifficultyConfig config);

    const DifficultyConfig& config() const { return config_; }
    const Board& board() const { return board_; }
    Board::Status status() const { return board_.status(); }
    uint32_t elapsedSeconds() const { return elapsedSeconds_; }

    void openCell(int x, int y) { board_.openCell(x, y); }
    void toggleFlag(int x, int y) { board_.toggleFlag(x, y); }
    void chord(int x, int y) { board_.chord(x, y); }

    // Accumulates active-play time; no-op once the game has ended (Won/Lost) so elapsed time
    // is frozen at the outcome shown in FR-018.
    void addActiveSeconds(uint32_t seconds);

    std::string toJson() const;
    static GameSession fromJson(const std::string& text);  // throws on invalid input

private:
    DifficultyConfig config_;
    Board board_;
    uint32_t elapsedSeconds_ = 0;
};

}  // namespace minesweeper::core
