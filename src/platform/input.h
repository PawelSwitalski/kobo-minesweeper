#pragma once
#include <optional>

namespace minesweeper {

// Display coordinates, post-rotation (same space as Renderer::info()).
struct Tap {
    int x = 0, y = 0;
};

// See docs/contracts/platform-abstraction.md.
class TouchInput {
public:
    virtual ~TouchInput() = default;

    // Blocks up to timeoutMs; returns the tap, or nothing on timeout. The
    // timeout wakes the app loop for timer updates and housekeeping.
    virtual std::optional<Tap> waitForTap(int timeoutMs) = 0;
};

}  // namespace minesweeper
