# Contract: `ui::App` interface (adds `returnToMainMenu()`)

`src/ui/app.h` gains one method; every other member is unchanged from `001-core-gameplay-settings`
(see that feature's `contracts/app-interface.md` for the full pre-existing surface). This document
covers only the delta.

```cpp
class App {
public:
    // ... all 001 members unchanged (renderer(), theme(), session(), autosaveSession(),
    //     hasInProgressGame(), startNewGame(cfg), settings(), autosaveSettings(),
    //     push(s), pop(), requestExit()) ...

    // Resets the current game to a fresh, unstarted state and returns to the main menu
    // unconditionally — no confirmation (FR-014), regardless of whether the current session was
    // Won, Lost, or (in principle) still InProgress. Only ever called from a screen the player
    // reached *after* a game has already ended (BoardScreen's outcome banner), so an in-progress
    // game is never silently discarded by this path in practice.
    virtual void returnToMainMenu() = 0;
};
```

## `AppImpl::returnToMainMenu()` behavior (`main.cpp`)

```cpp
void returnToMainMenu() override {
    session_ = minesweeper::core::GameSession();  // NotStarted, Beginner config (FR-006)
    autosaveSession();                             // persist the reset immediately (Constitution V)
    stack_.clear();
    push(std::make_unique<minesweeper::ui::NewGameScreen>(*this));  // sets navDirty_ = true
}
```

1. **Reset**: `session_` becomes a fresh default `GameSession` — `NotStarted`, no board cells.
   This is the exact same construction `main.cpp`'s own startup logic already treats as "nothing to
   resume" (`AppImpl` ctor / the `hasInProgressGame() || Won || Lost` check at
   `main.cpp:182-188`), so the very next launch shows `NewGameScreen`, not the old finished board —
   satisfying FR-006 with no new persisted field.
2. **Persist immediately**: `autosaveSession()` writes this reset state to `game.json` right away,
   not deferred to the next mutation or the final exit-path autosave — so even if the player then
   exits before doing anything else, the reset has already landed on disk.
3. **Screen stack**: `stack_.clear()` followed by pushing one fresh `NewGameScreen` guarantees the
   player always lands on the menu regardless of how deep/shallow the stack was beforehand
   (research.md #2) — whether `BoardScreen` was the sole root (resumed a finished game at launch)
   or sat on top of an existing `NewGameScreen` (started this session).
4. **Redraw**: reuses the existing `push()` path, which already sets `navDirty_ = true`; the main
   loop's existing `if (app.consumeNavDirty()) { app.top()->draw(); renderer.flushFull(); }`
   (`main.cpp:266-270`) handles the full-screen transition redraw exactly as every other
   navigation call already does — no new redraw plumbing needed.

## Call sites

- `BoardScreen`'s new "Return to Menu" outcome-banner button (only reachable once
  `status()` is `Won` or `Lost`) calls `app_.returnToMainMenu()`.
- No other screen calls this method. `NewGameScreen` never needs to "return to the menu" — it
  already *is* the menu.
