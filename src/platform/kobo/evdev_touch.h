#pragma once
#include <chrono>

#include "platform/input.h"
#include "platform/renderer.h"

namespace minesweeper {

// Raw evdev multitouch (type B, with type A/BTN_TOUCH fallback) on
// /dev/input/event*. Collapses one touch-down..up into a single Tap in the
// renderer's coordinate space (docs/contracts/platform-abstraction.md).
//
// Panel-to-display mapping differs per Kobo model; it is controlled by env
// vars so it can be calibrated in the field without a rebuild:
//   MINESWEEPER_TOUCH_SWAP_XY=1   swap raw x/y first
//   MINESWEEPER_TOUCH_MIRROR_X=1  mirror x after swap
//   MINESWEEPER_TOUCH_MIRROR_Y=1  mirror y after swap
//   MINESWEEPER_TOUCH_DEBUG=1     log raw+mapped taps to stderr (-> crash.log)
class EvdevTouch : public TouchInput {
public:
    ~EvdevTouch() override;

    bool init(const DisplayInfo& display);
    std::optional<Tap> waitForTap(int timeoutMs) override;

    // Timestamp of the most recent raw input activity, even if it never
    // formed a complete tap (a stray touch, a drag, sensor noise while
    // resting a finger). Idle-exit uses this so any touch counts, not just
    // ones that land as a clean down+up cycle.
    std::chrono::steady_clock::time_point lastActivity() const { return lastActivity_; }

private:
    int fd_ = -1;
    int rawMinX_ = 0, rawMaxX_ = 0, rawMinY_ = 0, rawMaxY_ = 0;
    int viewW_ = 0, viewH_ = 0;
    bool swapXY_ = false, mirrorX_ = false, mirrorY_ = false, debug_ = false;
    std::chrono::steady_clock::time_point lastActivity_ = std::chrono::steady_clock::now();
};

}  // namespace minesweeper
