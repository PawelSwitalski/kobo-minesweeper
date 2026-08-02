# Contract: `TouchInput` long-press extension

Extends the existing contract in `docs/contracts/platform-abstraction.md` (unchanged otherwise).
This feature's implementation must update that repo doc to match once merged — the two must not
drift apart.

## Interface change (`src/platform/input.h`)

```cpp
struct Tap {
    int x = 0, y = 0;
    bool longPress = false;   // NEW: true if press-to-release duration >= kLongPressMs
};

// Shared by every TouchInput backend; not OS-specific, so it lives here rather than
// per-backend, keeping the "what counts as long" policy in one portable place.
inline constexpr int kLongPressMs = 500;

class TouchInput {
public:
    virtual ~TouchInput() = default;
    virtual std::optional<Tap> waitForTap(int timeoutMs) = 0;  // unchanged signature
};
```

`waitForTap`'s signature and blocking/timeout contract are **unchanged** — this is purely an
additional field on the returned `Tap`, so every existing call site keeps compiling; only sites
that need to distinguish the gesture read `tap.longPress`.

## Backend obligations (new)

- **`kobo::EvdevTouch`**: already tracks a down→up cycle via `BTN_TOUCH`/`ABS_MT_TRACKING_ID` to
  emit one `Tap`. Additionally record the `steady_clock` timestamp at first-contact and at lift;
  set `longPress = (lift - firstContact) >= kLongPressMs`.
- **`sdl::MouseTouch`**: already emits a `Tap` on `SDL_MOUSEBUTTONUP`. Additionally record the
  tick count at `SDL_MOUSEBUTTONDOWN` and compare against the tick count at button-up using the
  same `kLongPressMs` threshold.
- Both backends must still collapse the whole gesture into exactly one `Tap` return (no new
  gesture types, no change to "no gestures needed" scope beyond this one bool) — a drag or a
  cancelled touch (lifted off-target) is unaffected by this change and continues to be handled
  however it is today.

## Consumers

- `ui::screens::BoardScreen::onTap(Tap)` is the only consumer that reads `tap.longPress`:
  - Unopened cell + `tap.longPress` → `toggleFlag` (regardless of Flag Mode state).
  - Unopened cell + `!tap.longPress` + Flag Mode on → `toggleFlag`.
  - Unopened cell + `!tap.longPress` + Flag Mode off → `openCell`.
  - Opened numbered cell (either gesture) → `chord` attempt.
- No other screen (`NewGameScreen`, `SettingsScreen`) needs to read `longPress`; ordinary button
  hit-testing (`Button::hit`) is unaffected since it only looks at `{x, y}`.
