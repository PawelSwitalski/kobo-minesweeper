# Contract: `core::GestureRecognizer`

**New 2026-08-03** for the gesture-based correction. Governs the new
`src/core/gesture_recognizer.h/.cpp` type (research.md #3, #6, #7; data-model.md) — the pure,
host-tested state machine that turns a stream of raw touch-point snapshots into `Tap` or
`GestureEvent` classifications, shared by both `EvdevTouch` (Kobo) and `MouseTouch` (SDL simulator).

## Interface

```cpp
namespace minesweeper::core {

struct TouchPoint { int slot; int x; int y; };

enum class GestureKind { ZoomStep, PanStep };
struct GestureEvent {
    GestureKind kind;
    int zoomDelta = 0;
    int dxPx = 0, dyPx = 0;
};

struct GestureTap { int x = 0, y = 0; bool longPress = false; };  // not platform::Tap -- see below

inline constexpr int kDragSlopPx = 12;      // movement beyond this => no longer a tap candidate
inline constexpr int kGestureStepPx = 24;   // cumulative pinch/drag distance per emitted step

class GestureRecognizer {
public:
    std::optional<std::variant<GestureTap, GestureEvent>> feed(const std::vector<TouchPoint>& points,
                                                                int64_t nowMs);
private:
    // ...
};

}  // namespace minesweeper::core
```

`kDragSlopPx`/`kGestureStepPx` are tuning constants, not correctness invariants — reasonable
starting values to be adjusted after on-device testing (README's existing "not yet tested on real
Kobo hardware" caveat applies here as much as anywhere else in this feature). `GestureTap` is a
separate type from `platform::Tap` purely to keep `core/` free of any `platform/` include —
backends convert it 1:1 into a `Tap` when constructing their `waitForEvent()` return value.

## Invariants

1. **At most one event per `feed()` call.** Never both a `Tap` and a `GestureEvent` from the same
   call; most calls (no threshold crossed, or an ongoing steady press) return nothing.
2. **A one-finger touch that stays within `kDragSlopPx` of its down position for its entire duration
   yields exactly one `Tap`, on the call where `points` becomes empty** (the lift). `longPress` is
   `true` iff `nowMs(lift) − nowMs(down) >= kLongPressMs` (`platform/input.h`'s existing constant,
   unchanged).
3. **A one-finger touch that moves beyond `kDragSlopPx` before lifting never yields a `Tap`, at any
   point in its lifetime** — including on lift. Once slop is exceeded, the contact is permanently a
   drag candidate for the rest of its lifetime (it cannot revert to tap candidacy even if the finger
   comes back to rest before lifting).
4. **A one-finger drag yields one `PanStep{dxPx, dyPx}` per `kGestureStepPx` of cumulative
   movement since the drag's last emitted step** (or since slop was first exceeded, for the first
   step) — not per raw input sample. `dxPx`/`dyPx` are the signed delta *for that step only*, not
   the cumulative distance since the drag began.
5. **Exactly two simultaneous contacts are tracked as a pinch**, identified by their two slot ids at
   the moment the second contact appears. `ZoomStep{zoomDelta}` is emitted once per `kGestureStepPx`
   of cumulative change in the distance between those two slots' positions since the pinch's last
   emitted step (or since the second contact appeared, for the first step); `zoomDelta` is `+1` when
   the distance is increasing (pinch-out) and `-1` when decreasing (pinch-in).
6. **A third simultaneous contact is ignored** — if a slot beyond the first two (for a pinch) or the
   first one (for a tap/drag candidate) appears, it has no effect on classification; only the
   original tracked slot(s) matter until they lift.
7. **Two contacts never yield a `Tap` or a `PanStep`**, only `ZoomStep` (possibly none, if movement
   stays under threshold) — even if both fingers lift simultaneously without crossing
   `kGestureStepPx`, no event is emitted for that pinch at all (not misclassified as two taps).
8. **An empty `points` list with no prior tracked contact yields nothing** (not a spurious `Tap`) —
   only a transition from "one tracked contact, slop never exceeded" to "no contacts" produces a
   `Tap`.

## Call sites

- `src/platform/kobo/evdev_touch.cpp`: on every `SYN_REPORT`, assembles the current
  `vector<TouchPoint>` from all active `ABS_MT_SLOT` entries (applying the existing swap/mirror/
  raw-to-display scale transform per point, unchanged from pre-`005`), calls `feed()`, and returns
  from `waitForEvent()` immediately if it yields a result.
- `src/platform/sdl/mouse_touch.cpp`: a mouse-down/motion/up sequence feeds a single-slot
  `TouchPoint` stream to the same recognizer (exercising `Tap`/`PanStep` identically to a real
  one-finger touch); a mouse-wheel event bypasses the recognizer and constructs
  `GestureEvent{ZoomStep, delta=sign(wheel)}` directly (research.md #7 — there is no second pointer
  to simulate a real pinch with).
- `src/ui/screens/board_screen.cpp`'s `onGesture()` (contracts/board-screen-integration.md) consumes
  the resulting `GestureEvent`s; `Tap`s continue to flow through `onTap()` exactly as before.

## Host tests (`tests/test_gesture_recognizer.cpp`)

At minimum, using synthetic `nowMs` and hand-built `TouchPoint` sequences (no real clock or OS input
needed — the whole point of extracting this into `core::`):

- A one-slot touch that never moves, held under `kLongPressMs`, then lifts → one `Tap{longPress:
  false}`.
- The same, held past `kLongPressMs` before lifting → one `Tap{longPress: true}`.
- A one-slot touch that moves exactly `kDragSlopPx` (boundary) then lifts without further movement →
  confirms which side of the boundary counts as "exceeded" (documented, tested precisely).
- A one-slot touch that moves several multiples of `kGestureStepPx` in one direction, then lifts →
  the expected count of `PanStep`s, each with the correct per-step delta, and no trailing `Tap`.
- A one-slot touch that moves back and forth (net displacement near zero, but real cumulative
  movement past `kGestureStepPx` in each direction) → confirms steps fire based on cumulative
  movement, not net displacement, and confirms sign correctness for a reversal.
- Two slots appearing together, moving apart by several `kGestureStepPx` multiples, then both
  lifting → the expected count of `ZoomStep{+1}`s and no `Tap`/`PanStep`.
- Two slots moving together (pinch-in) → `ZoomStep{-1}`s.
- Two slots that move apart by less than `kGestureStepPx` total, then both lift → zero events.
- A third slot appearing during an active two-slot pinch → confirms it's ignored (same `ZoomStep`
  sequence as if the third slot never appeared).
- A one-slot touch that exceeds `kDragSlopPx`, moves for a while, then returns to near its down
  position before lifting → still no `Tap` on lift (Invariant 3).
