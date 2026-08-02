#pragma once
#include <chrono>
#include <cstdint>

namespace minesweeper::core {

// Replaces the main loop's own tick-timing bookkeeping (contracts/active-time-tracker.md,
// specs/003-fix-timer-hide-option). Pure arithmetic on given timestamps -- never calls
// steady_clock::now() itself -- so it stays host-testable per Constitution I.
class ActiveTimeTracker {
public:
    explicit ActiveTimeTracker(std::chrono::steady_clock::time_point start);

    // Called once per main-loop iteration, whether or not that iteration produced a tap.
    // Returns whole seconds elapsed since the last time whole seconds were consumed (0 if
    // `counts` is false or the interval is non-positive). When counting, the reference point
    // only advances by the whole seconds just returned -- any sub-second remainder is kept in
    // place so a run of sub-second calls (e.g. rapid tapping) still accumulates correctly
    // instead of truncating away on every call. When not counting (paused/asleep), the
    // reference point resets fully to `now` so that excluded time is never later attributed.
    uint32_t tick(std::chrono::steady_clock::time_point now, bool counts);

private:
    std::chrono::steady_clock::time_point last_;
};

}  // namespace minesweeper::core
