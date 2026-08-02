#include "core/active_time_tracker.h"

namespace minesweeper::core {

ActiveTimeTracker::ActiveTimeTracker(std::chrono::steady_clock::time_point start)
    : last_(start) {}

uint32_t ActiveTimeTracker::tick(std::chrono::steady_clock::time_point now, bool counts) {
    int64_t deltaMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_).count();
    if (!counts || deltaMs <= 0) {
        // Nothing carries across a non-counted (paused/asleep) gap or a non-positive interval --
        // reset the reference point fully so it isn't later attributed as active time.
        last_ = now;
        return 0;
    }
    // Advance only by the whole seconds actually consumed, keeping the sub-second remainder in
    // place so repeated sub-second calls (e.g. rapid tapping) still accumulate correctly instead
    // of truncating every single call to zero and losing the remainder for good.
    uint32_t wholeSeconds = static_cast<uint32_t>(deltaMs / 1000);
    last_ += std::chrono::milliseconds(static_cast<int64_t>(wholeSeconds) * 1000);
    return wholeSeconds;
}

}  // namespace minesweeper::core
