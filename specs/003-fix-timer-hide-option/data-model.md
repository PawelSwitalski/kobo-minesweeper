# Phase 1 Data Model: Fix Game Timer & Add Hide-Timer Setting

This feature makes **one additive, backward-compatible change** to `settings.json`
(`schemaVersion` stays `1`) and introduces **one new non-persisted core type**. `game.json` is
completely unchanged — the timer fix changes only *how reliably* `elapsedSeconds` accumulates, not
its shape or meaning.

## `Settings` (`src/core/settings.h/.cpp`)

| Field | Type | Default | Introduced by |
|---|---|---|---|
| `colorMode` | `ColorMode` (`Color` \| `BlackAndWhite`) | `BlackAndWhite` | `001` |
| `hideTimer` | `bool` | `false` | `003` (this feature) |

**Validation rule**: `fromJson` reads `hideTimer` via `j.value("hideTimer", false)` — absent in an
already-existing `settings.json` (written before this feature) is treated as `false`, not a parse
error, so upgrading an existing install never discards a player's saved `colorMode` (research.md
#4). `toJson` always writes the field explicitly (`{"hideTimer": true|false}`), so every file
written by this version onward round-trips losslessly.

**Persisted shape** (delta over `001`'s `contracts/persistence-schema.md`):

```json
{ "schemaVersion": 1, "colorMode": "BlackAndWhite", "hideTimer": false }
```

## `core::ActiveTimeTracker` (`src/core/active_time_tracker.h/.cpp`) — NEW, not persisted

A small, stateful, host-testable helper that replaces the tick-timing arithmetic previously inlined
in `main.cpp`'s event loop (research.md #1, #2).

```cpp
class ActiveTimeTracker {
public:
    explicit ActiveTimeTracker(std::chrono::steady_clock::time_point start);

    // Called once per main-loop iteration, whether or not that iteration produced a tap.
    // Returns whole seconds to attribute as active play time for the interval since the
    // previous call (0 if `counts` is false), and unconditionally advances the internal
    // reference point to `now` -- so no iteration's interval is ever silently dropped.
    uint32_t tick(std::chrono::steady_clock::time_point now, bool counts);

private:
    std::chrono::steady_clock::time_point last_;
};
```

- **State**: one `time_point`, `last_`, initialized from the constructor's `start` and updated by
  every `tick()` call regardless of `counts`.
- **No OS calls**: never calls `steady_clock::now()` itself; the caller (`main.cpp`) supplies `now`.
  Pure arithmetic on given timestamps, per Constitution I.
- **Not persisted**: purely an in-memory main-loop helper, analogous to `main.cpp`'s existing
  `lastWall`/`lastTapSteady` locals, just extracted for testability.

## `BoardScreen` (`src/ui/screens/board_screen.h/.cpp`)

No new fields. Two existing methods change behavior (not shape):

- `drawTimer()`: always blanks `timerRect_` (`fillRect(..., Gray::White)`), but only constructs and
  draws the elapsed-time `Label` when `!app_.settings().hideTimer`.
- `onTick(uint32_t)`: only performs the minute-boundary redraw + `flushPartial(timerRect_)` when
  `!app_.settings().hideTimer`. The underlying `addActiveSeconds(activeSeconds)` + `autosaveSession()`
  calls are unconditional either way (FR-011: hiding never affects tracking/persistence).
- `drawOutcomeBanner()`: **unchanged** — its own elapsed-time `Label` never reads `hideTimer`
  (research.md #3), satisfying FR-008 by construction.

## `SettingsScreen` (`src/ui/screens/settings_screen.h/.cpp`)

New non-persisted member (screen-local UI state, same category as `BoardScreen::flagModeOn_`):

| Field | Type | Purpose |
|---|---|---|
| `hideTimerButton_` | `Button` | Single toggle (not a mutually-exclusive pair like the color buttons); `toggled` mirrors `app_.settings().hideTimer`. |

## Relationships (delta over `001`/`002`)

```text
App
├─ settings(): Settings          — gains hideTimer; read by BoardScreen::drawTimer()/onTick(),
│                                   written by SettingsScreen::onTap() via the new toggle button
└─ session(): GameSession        — unchanged type/shape; elapsedSeconds_ now accumulates correctly
                                    during tap-heavy play because main.cpp's loop calls
                                    screen->onTick(...) every iteration via ActiveTimeTracker,
                                    not just no-tap ones

main.cpp (AppImpl + event loop)
└─ ActiveTimeTracker              — NEW, in-memory only; replaces the loop's bare lastTickSteady
                                    bookkeeping (research.md #1, #2)
```
