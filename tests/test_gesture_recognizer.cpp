#include "doctest/doctest.h"

#include "core/gesture_recognizer.h"

using namespace minesweeper::core;

namespace {
bool isNullopt(const std::optional<std::variant<GestureTap, GestureEvent>>& r) {
    return !r.has_value();
}
const GestureTap* asTap(const std::optional<std::variant<GestureTap, GestureEvent>>& r) {
    return r ? std::get_if<GestureTap>(&*r) : nullptr;
}
const GestureEvent* asEvent(const std::optional<std::variant<GestureTap, GestureEvent>>& r) {
    return r ? std::get_if<GestureEvent>(&*r) : nullptr;
}
}  // namespace

TEST_CASE("a stationary touch under the long-press duration yields a short Tap on lift") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 100, 100}}, 0)));
    auto r = gr.feed({}, 200);
    const GestureTap* tap = asTap(r);
    REQUIRE(tap != nullptr);
    CHECK(tap->x == 100);
    CHECK(tap->y == 100);
    CHECK_FALSE(tap->longPress);
}

TEST_CASE("a stationary touch held past the long-press duration yields a long-press Tap") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 100, 100}}, 0)));
    auto r = gr.feed({}, 600);
    const GestureTap* tap = asTap(r);
    REQUIRE(tap != nullptr);
    CHECK(tap->longPress);
}

TEST_CASE("movement exactly at the drag-slop threshold counts as exceeded -- no Tap on lift") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}}, 0)));
    CHECK(isNullopt(gr.feed({{1, kDragSlopPx, 0}}, 50)));  // distance == kDragSlopPx
    CHECK(isNullopt(gr.feed({}, 100)));                    // lift: Dragging never taps
}

TEST_CASE("movement just under the drag-slop threshold still yields a Tap on lift") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}}, 0)));
    CHECK(isNullopt(gr.feed({{1, kDragSlopPx - 1, 0}}, 50)));
    auto r = gr.feed({}, 100);
    REQUIRE(asTap(r) != nullptr);
}

TEST_CASE("a multi-step drag yields one PanStep per step threshold crossed, then no trailing Tap") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}}, 0)));
    CHECK(isNullopt(gr.feed({{1, 15, 0}}, 10)));  // exceeds slop; no step yet (reference resets here)

    auto r1 = gr.feed({{1, 40, 0}}, 20);  // 25px since slop-exceed point >= kGestureStepPx (24)
    const GestureEvent* e1 = asEvent(r1);
    REQUIRE(e1 != nullptr);
    CHECK(e1->kind == GestureKind::PanStep);
    CHECK(e1->dxPx == 25);
    CHECK(e1->dyPx == 0);

    auto r2 = gr.feed({{1, 65, 0}}, 30);  // another 25px
    const GestureEvent* e2 = asEvent(r2);
    REQUIRE(e2 != nullptr);
    CHECK(e2->dxPx == 25);

    CHECK(isNullopt(gr.feed({}, 40)));  // lift: never a Tap once dragging
}

TEST_CASE("a back-and-forth drag fires steps from cumulative movement, not net displacement") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}}, 0)));
    CHECK(isNullopt(gr.feed({{1, 30, 0}}, 10)));  // exceeds slop, reference set to (30,0)

    auto rBack = gr.feed({{1, 0, 0}}, 20);  // 30px back
    const GestureEvent* eBack = asEvent(rBack);
    REQUIRE(eBack != nullptr);
    CHECK(eBack->dxPx == -30);

    auto rForward = gr.feed({{1, 30, 0}}, 30);  // 30px forward again -- net displacement is 0
    const GestureEvent* eForward = asEvent(rForward);
    REQUIRE(eForward != nullptr);
    CHECK(eForward->dxPx == 30);
}

TEST_CASE("two fingers spreading apart yield ZoomStep(+1) pinch-out events, no trailing event") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}, {2, 100, 0}}, 0)));  // dist 100, pinch begins

    auto r1 = gr.feed({{1, 0, 0}, {2, 130, 0}}, 10);  // dist 130, +30
    const GestureEvent* e1 = asEvent(r1);
    REQUIRE(e1 != nullptr);
    CHECK(e1->kind == GestureKind::ZoomStep);
    CHECK(e1->zoomDelta == 1);

    auto r2 = gr.feed({{1, 0, 0}, {2, 160, 0}}, 20);  // dist 160, +30
    const GestureEvent* e2 = asEvent(r2);
    REQUIRE(e2 != nullptr);
    CHECK(e2->zoomDelta == 1);

    CHECK(isNullopt(gr.feed({}, 30)));  // both lift: no trailing event
}

TEST_CASE("two fingers moving together yield ZoomStep(-1) pinch-in events") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}, {2, 200, 0}}, 0)));  // dist 200

    auto r1 = gr.feed({{1, 0, 0}, {2, 170, 0}}, 10);  // dist 170, -30
    const GestureEvent* e1 = asEvent(r1);
    REQUIRE(e1 != nullptr);
    CHECK(e1->zoomDelta == -1);

    auto r2 = gr.feed({{1, 0, 0}, {2, 140, 0}}, 20);  // dist 140, -30
    const GestureEvent* e2 = asEvent(r2);
    REQUIRE(e2 != nullptr);
    CHECK(e2->zoomDelta == -1);
}

TEST_CASE("a pinch that never crosses the step threshold yields zero events") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}, {2, 100, 0}}, 0)));
    CHECK(isNullopt(gr.feed({{1, 0, 0}, {2, 110, 0}}, 10)));  // only +10, below threshold
    CHECK(isNullopt(gr.feed({}, 20)));                        // lift: still nothing
}

TEST_CASE("a third simultaneous contact during an active pinch is ignored") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}, {2, 100, 0}}, 0)));

    // A third finger (slot 3) appears alongside the tracked pinch pair.
    auto r1 = gr.feed({{1, 0, 0}, {2, 130, 0}, {3, 500, 500}}, 10);
    const GestureEvent* e1 = asEvent(r1);
    REQUIRE(e1 != nullptr);
    CHECK(e1->zoomDelta == 1);

    // Third finger lifts; the original pinch continues exactly as if it had never appeared.
    auto r2 = gr.feed({{1, 0, 0}, {2, 160, 0}}, 20);
    const GestureEvent* e2 = asEvent(r2);
    REQUIRE(e2 != nullptr);
    CHECK(e2->zoomDelta == 1);
}

TEST_CASE("a drag that returns near its start before lifting still yields no Tap") {
    GestureRecognizer gr;
    CHECK(isNullopt(gr.feed({{1, 0, 0}}, 0)));
    CHECK(isNullopt(gr.feed({{1, 20, 0}}, 10)));  // exceeds slop -> Dragging
    CHECK(isNullopt(gr.feed({{1, 2, 0}}, 20)));   // back near start, but under step threshold
    CHECK(isNullopt(gr.feed({}, 30)));            // lift: Dragging never taps, regardless of position
}
