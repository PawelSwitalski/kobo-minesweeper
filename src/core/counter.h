#pragma once
#include <cstdint>
#include <string>

namespace minesweeper::core {

// Trivial demo domain object: exercises the same load/mutate/persist shape a
// real project's session/game state would use, without any actual game
// logic. Delete this once you add your own core/ types.
class Counter {
public:
    int32_t value() const { return value_; }
    void increment() { ++value_; }

    std::string toJson() const;
    static Counter fromJson(const std::string& text);  // throws on invalid input

private:
    int32_t value_ = 0;
};

}  // namespace minesweeper::core
