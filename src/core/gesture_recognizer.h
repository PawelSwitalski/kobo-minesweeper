#pragma once
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace minesweeper::core {

// One physical contact, this poll cycle. `slot` is a backend-assigned, stable id for the
// contact's lifetime (e.g. the Linux MT-B ABS_MT_SLOT index, or a synthetic id the SDL backend
// assigns to its one simulated "finger") -- never persisted, never meaningful across gestures.
struct TouchPoint {
    int slot;
    int x, y;  // display coordinates, post-rotation (same space as Tap)
};

// A completed, classified tap -- deliberately not named `Tap` (platform/input.h's type) so this
// header has no dependency on platform/ (Constitution I: core stays portable/host-only). Backends
// convert this 1:1 into a platform::Tap when constructing their waitForEvent() return value.
struct GestureTap {
    int x = 0, y = 0;
    bool longPress = false;
};

enum class GestureKind { ZoomStep, PanStep };

struct GestureEvent {
    GestureKind kind;
    int zoomDelta = 0;       // ZoomStep only: +1 pinch-out (zoom in) / -1 pinch-in (zoom out)
    int dxPx = 0, dyPx = 0;  // PanStep only: signed drag delta (display px) since the last step
};

// Tuning constants, not correctness invariants -- reasonable starting values, adjustable after
// on-device testing (specs/005-board-zoom-pan/contracts/gesture-recognizer.md).
inline constexpr int kDragSlopPx = 12;
inline constexpr int64_t kGestureStepPx = 24;

// Moved here from platform/input.h (005-board-zoom-pan): tap/long-press classification now
// happens once, centrally, in GestureRecognizer rather than per-backend, so the "what counts as
// long" policy belongs alongside the rest of the classification logic it's part of.
inline constexpr int64_t kLongPressMs = 500;

// Classifies a stream of raw touch-point snapshots into taps, pinch-zoom steps, and pan-drag
// steps. Pure arithmetic on caller-supplied data (points + a caller-supplied monotonic clock, the
// same caller-clock pattern as ActiveTimeTracker) -- no OS calls -- so it stays host-testable per
// Constitution I/III, and is shared by every TouchInput backend
// (specs/005-board-zoom-pan/contracts/gesture-recognizer.md, research.md #3).
class GestureRecognizer {
public:
    // points: every contact currently down this poll cycle (empty => all fingers lifted).
    // nowMs: caller-supplied monotonic clock, used only for GestureTap::longPress.
    std::optional<std::variant<GestureTap, GestureEvent>> feed(const std::vector<TouchPoint>& points,
                                                                int64_t nowMs);

private:
    enum class Mode { Idle, TapCandidate, Dragging, Pinching };

    static const TouchPoint* find(const std::vector<TouchPoint>& points, int slot);
    static const TouchPoint* firstOtherThan(const std::vector<TouchPoint>& points, int slot);
    static int64_t distance(int x1, int y1, int x2, int y2);

    std::optional<std::variant<GestureTap, GestureEvent>> beginFromIdle(
        const std::vector<TouchPoint>& points, int64_t nowMs);
    std::optional<std::variant<GestureTap, GestureEvent>> continueTapCandidate(
        const std::vector<TouchPoint>& points, int64_t nowMs);
    std::optional<std::variant<GestureTap, GestureEvent>> continueDragging(
        const std::vector<TouchPoint>& points);
    std::optional<std::variant<GestureTap, GestureEvent>> continuePinching(
        const std::vector<TouchPoint>& points);

    Mode mode_ = Mode::Idle;

    // One-finger (TapCandidate/Dragging) state.
    int slot1_ = -1;
    int downX_ = 0, downY_ = 0;
    int64_t downMs_ = 0;
    int lastKnownX_ = 0, lastKnownY_ = 0;  // most recent position, for a Tap emitted on lift
    int lastStepX_ = 0, lastStepY_ = 0;    // position at which the last PanStep was emitted

    // Two-finger (Pinching) state.
    int slot2_ = -1;
    int64_t lastDist_ = 0;  // inter-point distance at which the last ZoomStep was emitted
};

}  // namespace minesweeper::core
