# Contract: Platform Abstraction (`src/platform`)

Two interfaces isolate everything device-specific. The UI layer may use **only** these; `core` uses neither.

## Renderer (`renderer.h`)

```cpp
struct DisplayInfo { int width, height, dpi; bool color; };

class Renderer {
public:
  virtual DisplayInfo info() const = 0;

  // Drawing (grayscale-first; color is accent-only)
  virtual void fillRect(Rect, Gray shade, Color accent = Color::None) = 0;
  virtual void drawText(Rect, std::string_view, const TextStyle&) = 0;   // sizes in px derived from dpi
  virtual void drawLine(Point, Point, int thicknessPx, Gray) = 0;

  // E-ink refresh discipline
  virtual void flushPartial(Rect) = 0;   // fast, may ghost
  virtual void flushFull() = 0;          // flashing, clears ghosting
};
```

**Backend obligations**:
- `kobo/FbinkRenderer`: maps to FBInk; owns waveform choice; honors device rotation; tracks partial-refresh count and MAY promote a `flushPartial` to `flushFull` after N partials (ghosting policy lives here, not in UI code).
- `sdl/SdlRenderer`: 1:1 window; `flushFull` may simulate the e-ink flash (invert blink) to make refresh behavior visible during development.
- Both must render all information without `color`; UI code must never encode meaning in `accent` alone.

## TouchInput (`input.h`)

```cpp
struct Tap { int x, y; };   // display coordinates, post-rotation

class TouchInput {
public:
  // Blocks up to timeoutMs; returns tap, or nothing on timeout.
  // Timeout wakes the app loop for periodic housekeeping (autosave, idle-exit).
  virtual std::optional<Tap> waitForTap(int timeoutMs) = 0;
};
```

**Backend obligations**:
- `kobo/EvdevTouch`: reads `/dev/input/event*` multitouch type-B; translates raw coordinates into the same rotated space `Renderer::info()` reports; collapses a touch-down/up pair into one `Tap` (no gestures needed).
- `sdl/MouseTouch`: mouse click → `Tap`.

## App loop contract (`main.cpp`)

- Select backend at compile time (device) or flag (host).
- On any tap: hit-test UI → call app-state mutator → partial-flush the changed rect(s); full-flush on screen transitions.
- On exit action: persist, restore framebuffer state, return 0 so the launcher hands control back to Nickel.
- On SIGTERM/SIGINT: persist and exit cleanly (device sleep/power events).
