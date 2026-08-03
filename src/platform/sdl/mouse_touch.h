#pragma once
#include "core/gesture_recognizer.h"
#include "platform/input.h"

namespace minesweeper {

// Mouse click-drag -> Tap/PanStep (via the shared core::GestureRecognizer, exactly like a single
// finger); mouse wheel -> ZoomStep directly, standing in for a two-finger pinch, since a desktop
// mouse has no second pointer to simulate one with (specs/005-board-zoom-pan/research.md #7).
// Window-close sets *quitFlag; window-exposed sets *redrawFlag so the app shell can repaint.
class MouseTouch : public TouchInput {
public:
    MouseTouch(bool* quitFlag, bool* redrawFlag) : quit_(quitFlag), redraw_(redrawFlag) {}

    std::optional<std::variant<Tap, core::GestureEvent>> waitForEvent(int timeoutMs) override;

private:
    bool* quit_;
    bool* redraw_;
    bool mouseDown_ = false;
    core::GestureRecognizer gestureRecognizer_;
};

}  // namespace minesweeper
