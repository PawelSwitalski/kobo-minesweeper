#pragma once
#include <optional>
#include <variant>

#include "core/gesture_recognizer.h"

namespace minesweeper {

// Display coordinates, post-rotation (same space as Renderer::info()).
struct Tap {
    int x = 0, y = 0;
    bool longPress = false;
};

// See docs/contracts/platform-abstraction.md.
class TouchInput {
public:
    virtual ~TouchInput() = default;

    // Blocks up to timeoutMs; returns a completed Tap, a mid-gesture GestureEvent (a pinch/drag
    // step -- possibly while fingers are still down, per FR-012), or nothing on timeout. The
    // timeout wakes the app loop for timer updates and housekeeping. Every backend classifies its
    // raw touch points via a shared core::GestureRecognizer (specs/005-board-zoom-pan) rather than
    // doing tap/drag/pinch classification itself.
    virtual std::optional<std::variant<Tap, core::GestureEvent>> waitForEvent(int timeoutMs) = 0;
};

}  // namespace minesweeper
