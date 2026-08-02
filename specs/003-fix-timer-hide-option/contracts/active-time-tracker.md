# Contract: `core::ActiveTimeTracker` and its `main.cpp` call site

Governs the new `src/core/active_time_tracker.h/.cpp` type and how `main.cpp`'s event loop uses it
to fix the elapsed-play-time undercount bug (research.md #1, #2; data-model.md).

## `core::ActiveTimeTracker`

```cpp
namespace minesweeper::core {

class ActiveTimeTracker {
public:
    explicit ActiveTimeTracker(std::chrono::steady_clock::time_point start);

    // Called once per main-loop iteration, whether or not that iteration produced a tap.
    // Returns whole seconds elapsed since the previous call (0 if `counts` is false or the
    // interval is negative/zero), and unconditionally advances the internal reference point
    // to `now`.
    uint32_t tick(std::chrono::steady_clock::time_point now, bool counts);
};

}  // namespace minesweeper::core
```

**Invariants**:
1. **A counted call never loses time — it carries the remainder forward.** When `counts` is true,
   `tick()` returns `deltaMs / 1000` (whole seconds since the reference point) but advances the
   reference point by only that many whole seconds, not all the way to `now`. Any sub-second
   remainder stays represented as the gap between the (partially advanced) reference point and the
   next call's `now`, so a run of closely-spaced calls (e.g. rapid tapping, each individually under
   1 second apart) still sums to the correct total once enough of them cross a whole-second
   boundary — this is what closes the bug: the old `main.cpp` code reset its reference point to
   `now` on *every* iteration but only *used* the interval on some of them, silently discarding the
   rest, and a naive "always reset to `now`" fix would still lose time on every individual
   sub-second call even while calling `onTick` every iteration (caught by
   `tests/test_active_time_tracker.cpp`'s tap-heavy case during implementation).
2. **`counts == false` returns 0 and resets fully.** A screen that doesn't count play time (e.g.
   `NewGameScreen`, `SettingsScreen`, or `BoardScreen` once the game has ended) or a `slept`
   iteration contributes 0 seconds for that interval, and — unlike the counted case — the
   reference point resets all the way to `now`, so a paused/excluded gap is never later
   reconstituted as bonus active seconds once counting resumes.
3. **Cumulative accuracy, not per-call truncation.** Because whole seconds are only "spent" from
   the reference point once actually consumed, the running total across many calls converges to
   the true elapsed counted time (off by at most the final, still-pending sub-second remainder at
   any given instant) rather than accumulating a per-call rounding loss. Given the loop's
   tap-driven iteration rate, this stays well under the 5-second tolerance from spec.md
   SC-001/SC-002 (see quickstart.md for the scenario that verifies this empirically).

## `main.cpp` call site (delta)

```cpp
minesweeper::core::ActiveTimeTracker activeTimeTracker(lastTickSteady);
// ...
while (!app.exitRequested() && !g_signalled && !sdlQuit && app.top()) {
    std::optional<minesweeper::Tap> tap = touch.waitForTap(kTimeoutMs);
    // ... nowSteady / wallMs / slept computed exactly as today ...

    if (slept || sdlRedraw) { screen->draw(); renderer.flushFull(); }

    bool counts = !slept && screen->countsPlayTime();
    uint32_t activeSeconds = activeTimeTracker.tick(nowSteady, counts);
    screen->onTick(activeSeconds);              // NOW: every iteration, not just no-tap ones

    if (tap) {
        lastTapSteady = nowSteady;
        screen->onTap(*tap);
    } else {
        // idle-exit watchdog logic unchanged, still no-tap-only
    }

    if (app.consumeNavDirty()) { ... }           // unchanged
}
```

**Behavior change summary**:
- `screen->onTick(activeSeconds)` moves out of the `if (tap) {...} else {...}` branching and runs
  unconditionally, once per loop iteration, immediately after the sleep/redraw check.
- The bare `lastTickSteady` local and its unconditional `lastTickSteady = nowSteady;` line
  (old `main.cpp:270`) are removed — `ActiveTimeTracker` owns that state now.
- The idle-exit watchdog (`main.cpp:254-268`) and the `if (tap) {...}` tap-dispatch stay exactly
  where they are today; only the timing/`onTick` call moves.

## Call sites

- `main.cpp`'s event loop is the only caller of `ActiveTimeTracker::tick()`.
- `BoardScreen::onTick(uint32_t activeSeconds)` is the only override that does anything with the
  value (`GameSession::addActiveSeconds`, unchanged); every other screen's `onTick` is the `Screen`
  base class's no-op default, unaffected by being called more often.
