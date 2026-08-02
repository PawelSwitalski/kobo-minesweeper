# Contract: `ui::App` interface (replaces the Counter-demo surface)

`src/ui/app.h` is the seam every `Screen` talks to. This feature replaces its placeholder
`counter()`/`autosave()` pair with the shape below, following the exact pattern `SETUP.md`
describes ("extend `App`'s interface with your own state accessors/persist hooks... following the
shape `counter()`/`autosave()` already show").

```cpp
class App {
public:
    virtual ~App() = default;

    virtual Renderer& renderer() = 0;
    virtual const Theme& theme() const = 0;   // Theme::color reflects Settings x hardware (see below)

    // Game session (the one in-progress-or-ended game, if any).
    virtual core::GameSession& session() = 0;
    virtual void autosaveSession() = 0;        // persist game.json after every mutation
    virtual bool hasInProgressGame() const = 0;// session().status() == InProgress
    virtual void startNewGame(core::DifficultyConfig cfg) = 0;
        // Discards the current session unconditionally and starts a fresh one at `cfg`.
        // Callers (NewGameScreen) are responsible for the FR-024 confirmation *before*
        // calling this when hasInProgressGame() is true — App itself does not prompt.

    // Player-level settings.
    virtual core::Settings& settings() = 0;
    virtual void autosaveSettings() = 0;       // persist settings.json after every mutation
        // Also responsible for re-deriving Theme::color via ui::applyColorMode() so the
        // change is visible on the very next draw.

    // Navigation. Transitions trigger a full redraw + flushFull (Constitution II).
    virtual void push(std::unique_ptr<Screen> s) = 0;
    virtual void pop() = 0;
    virtual void requestExit() = 0;
};
```

## Startup behavior (`AppImpl` in `main.cpp`)

1. Load `settings.json` → `Settings` (default `BlackAndWhite` on missing/corrupt, per
   data-model.md); call `ui::applyColorMode(theme, rendererInfo, settings.colorMode)`.
2. Load `game.json` → `GameSession` (on missing/corrupt: discard file, fall back to a
   `NotStarted` session with no difficulty chosen yet — settings load in step 1 is unaffected,
   per FR-023).
3. Push the initial screen: `BoardScreen` if `session().status() != NotStarted` (resumes
   in-progress or shows the last ended game's outcome, per research.md #6's resume behavior);
   otherwise `NewGameScreen`.

## Persistence obligations (Constitution V)

- Every `Board`/`GameSession` mutation a screen performs (`openCell`, `toggleFlag`, `chord`,
  `startNewGame`) is immediately followed by `app_.autosaveSession()` — same call shape
  `CounterScreen` used for `autosave()` today.
- Every `Settings` mutation (`SettingsScreen` toggling color mode) is immediately followed by
  `app_.autosaveSettings()`.
- `main.cpp`'s existing "persist on every exit path" call (SIGTERM/SIGINT/normal exit) becomes
  `app.autosaveSession(); app.autosaveSettings();` — belt-and-suspenders on top of the
  per-mutation autosaves above, matching the existing `app.autosave()` exit-path call.
