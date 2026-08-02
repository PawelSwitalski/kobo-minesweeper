#include "doctest/doctest.h"

#include "core/active_time_tracker.h"

using namespace minesweeper::core;
using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

TEST_CASE("tap-heavy sequence: summed active seconds match the total real interval") {
    // Reproduces the bug in research.md #1: a burst of frequent, closely-spaced ticks (as
    // happens when the player taps rapidly) must not lose any of the elapsed wall-clock time.
    Clock::time_point start{};
    ActiveTimeTracker tracker(start);

    Clock::time_point now = start;
    uint32_t totalSeconds = 0;
    const int iterations = 500;
    const auto step = milliseconds(120);  // frequent taps, well under 1s apart
    for (int i = 0; i < iterations; ++i) {
        now += step;
        totalSeconds += tracker.tick(now, /*counts=*/true);
    }

    uint32_t expected = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() / 1000);
    CHECK(totalSeconds == expected);
    CHECK(totalSeconds > 0);
}

TEST_CASE("counts=false contributes zero but still advances the reference point") {
    Clock::time_point start{};
    ActiveTimeTracker tracker(start);

    Clock::time_point pausedUntil = start + std::chrono::seconds(30);
    CHECK(tracker.tick(pausedUntil, /*counts=*/false) == 0);

    // The next counted interval should be measured from `pausedUntil`, not `start` -- the
    // paused 30s must not be attributed to active play once counting resumes.
    Clock::time_point resumed = pausedUntil + std::chrono::seconds(5);
    CHECK(tracker.tick(resumed, /*counts=*/true) == 5);
}

TEST_CASE("sub-second remainders are truncated per call without compounding drift") {
    Clock::time_point start{};
    ActiveTimeTracker tracker(start);

    // Ten 900ms ticks: 9000ms of real time, but each individual call truncates to 0.
    Clock::time_point now = start;
    uint32_t totalSeconds = 0;
    for (int i = 0; i < 10; ++i) {
        now += milliseconds(900);
        totalSeconds += tracker.tick(now, /*counts=*/true);
    }
    CHECK(totalSeconds == 9);  // 9000ms / 1000, not inflated or deflated by per-call rounding
}

TEST_CASE("a single long idle interval is attributed in full") {
    Clock::time_point start{};
    ActiveTimeTracker tracker(start);

    Clock::time_point now = start + std::chrono::seconds(20);
    CHECK(tracker.tick(now, /*counts=*/true) == 20);
}

TEST_CASE("zero or negative interval contributes zero") {
    Clock::time_point start{};
    ActiveTimeTracker tracker(start);

    CHECK(tracker.tick(start, /*counts=*/true) == 0);  // same instant
}
