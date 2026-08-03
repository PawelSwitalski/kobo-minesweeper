#include "core/gesture_recognizer.h"

#include <cmath>

namespace minesweeper::core {

const TouchPoint* GestureRecognizer::find(const std::vector<TouchPoint>& points, int slot) {
    for (const auto& p : points) {
        if (p.slot == slot) return &p;
    }
    return nullptr;
}

const TouchPoint* GestureRecognizer::firstOtherThan(const std::vector<TouchPoint>& points,
                                                     int slot) {
    for (const auto& p : points) {
        if (p.slot != slot) return &p;
    }
    return nullptr;
}

int64_t GestureRecognizer::distance(int x1, int y1, int x2, int y2) {
    double dx = static_cast<double>(x1 - x2);
    double dy = static_cast<double>(y1 - y2);
    return static_cast<int64_t>(std::sqrt(dx * dx + dy * dy));
}

std::optional<std::variant<GestureTap, GestureEvent>> GestureRecognizer::feed(
    const std::vector<TouchPoint>& points, int64_t nowMs) {
    switch (mode_) {
        case Mode::Idle:
            return beginFromIdle(points, nowMs);
        case Mode::TapCandidate:
            return continueTapCandidate(points, nowMs);
        case Mode::Dragging:
            return continueDragging(points);
        case Mode::Pinching:
            return continuePinching(points);
    }
    return std::nullopt;  // unreachable; silences -Wreturn-type on some compilers
}

std::optional<std::variant<GestureTap, GestureEvent>> GestureRecognizer::beginFromIdle(
    const std::vector<TouchPoint>& points, int64_t nowMs) {
    if (points.empty()) return std::nullopt;

    if (points.size() == 1) {
        slot1_ = points[0].slot;
        downX_ = lastKnownX_ = points[0].x;
        downY_ = lastKnownY_ = points[0].y;
        downMs_ = nowMs;
        mode_ = Mode::TapCandidate;
        return std::nullopt;
    }

    // Two or more simultaneous contacts: track the first two, ignore the rest (Invariant 6).
    slot1_ = points[0].slot;
    slot2_ = points[1].slot;
    lastDist_ = distance(points[0].x, points[0].y, points[1].x, points[1].y);
    mode_ = Mode::Pinching;
    return std::nullopt;
}

std::optional<std::variant<GestureTap, GestureEvent>> GestureRecognizer::continueTapCandidate(
    const std::vector<TouchPoint>& points, int64_t nowMs) {
    const TouchPoint* p = find(points, slot1_);

    if (!p) {
        // Lifted (or vanished) before ever exceeding slop -- a completed tap.
        mode_ = Mode::Idle;
        return GestureTap{lastKnownX_, lastKnownY_, nowMs - downMs_ >= kLongPressMs};
    }

    lastKnownX_ = p->x;
    lastKnownY_ = p->y;

    if (points.size() >= 2) {
        // A second finger joined before slop was exceeded: abandon single-finger tracking and
        // start a pinch fresh from the current positions -- no Tap either way, since we can no
        // longer tell if the player meant a tap.
        const TouchPoint* other = firstOtherThan(points, slot1_);
        slot2_ = other->slot;
        lastDist_ = distance(p->x, p->y, other->x, other->y);
        mode_ = Mode::Pinching;
        return std::nullopt;
    }

    if (distance(downX_, downY_, p->x, p->y) >= kDragSlopPx) {
        // Slop exceeded: this contact can never become a Tap again, even if it later returns
        // near its down position before lifting (Invariant 3).
        lastStepX_ = p->x;
        lastStepY_ = p->y;
        mode_ = Mode::Dragging;
    }
    return std::nullopt;
}

std::optional<std::variant<GestureTap, GestureEvent>> GestureRecognizer::continueDragging(
    const std::vector<TouchPoint>& points) {
    const TouchPoint* p = find(points, slot1_);

    if (!p) {
        // Lifted -- a drag never yields a Tap, even here.
        mode_ = Mode::Idle;
        return std::nullopt;
    }

    if (points.size() >= 2) {
        // A second finger joined mid-drag: abandon pan tracking, start a pinch fresh.
        const TouchPoint* other = firstOtherThan(points, slot1_);
        slot2_ = other->slot;
        lastDist_ = distance(p->x, p->y, other->x, other->y);
        mode_ = Mode::Pinching;
        return std::nullopt;
    }

    if (distance(lastStepX_, lastStepY_, p->x, p->y) >= kGestureStepPx) {
        GestureEvent ev{GestureKind::PanStep, 0, p->x - lastStepX_, p->y - lastStepY_};
        lastStepX_ = p->x;
        lastStepY_ = p->y;
        return ev;
    }
    return std::nullopt;
}

std::optional<std::variant<GestureTap, GestureEvent>> GestureRecognizer::continuePinching(
    const std::vector<TouchPoint>& points) {
    const TouchPoint* p1 = find(points, slot1_);
    const TouchPoint* p2 = find(points, slot2_);

    if (!p1 || !p2) {
        // Either tracked contact lifted (or vanished): the pinch is over. Two contacts never
        // yield a Tap or PanStep (Invariant 7) -- even a sub-threshold pinch produces nothing.
        mode_ = Mode::Idle;
        return std::nullopt;
    }

    // Any other simultaneous slot besides slot1_/slot2_ is ignored entirely (Invariant 6) --
    // find() above only ever looks up the two tracked slots.
    int64_t curDist = distance(p1->x, p1->y, p2->x, p2->y);
    int64_t delta = curDist - lastDist_;
    if (delta >= kGestureStepPx || -delta >= kGestureStepPx) {
        GestureEvent ev{GestureKind::ZoomStep, delta > 0 ? 1 : -1, 0, 0};
        lastDist_ = curDist;
        return ev;
    }
    return std::nullopt;
}

}  // namespace minesweeper::core
