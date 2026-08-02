#pragma once
#include <memory>

#include "core/difficulty.h"
#include "core/game_session.h"
#include "core/settings.h"
#include "platform/renderer.h"
#include "ui/screens/screen.h"
#include "ui/theme.h"

namespace minesweeper::ui {

// Semantic surface the screens talk to. Implemented by the app shell in
// main.cpp, which owns backends, persistence wiring and the screen stack.
class App {
public:
    virtual ~App() = default;

    virtual Renderer& renderer() = 0;
    virtual const Theme& theme() const = 0;  // Theme::color reflects Settings x hardware

    // Game session (the one in-progress-or-ended game, if any).
    virtual core::GameSession& session() = 0;
    virtual void autosaveSession() = 0;         // persist game.json after every mutation
    virtual bool hasInProgressGame() const = 0;  // session().status() == InProgress
    virtual void startNewGame(core::DifficultyConfig cfg) = 0;
        // Discards the current session unconditionally and starts a fresh one at `cfg`.
        // Callers (NewGameScreen) are responsible for the FR-024 confirmation *before*
        // calling this when hasInProgressGame() is true -- App itself does not prompt.

    // Player-level settings.
    virtual core::Settings& settings() = 0;
    virtual void autosaveSettings() = 0;  // persist settings.json after every mutation
        // Also re-derives Theme::color via ui::applyColorMode() so the change is visible
        // on the very next draw.

    // Navigation. Transitions trigger a full redraw + flushFull (Constitution II).
    virtual void push(std::unique_ptr<Screen> s) = 0;
    virtual void pop() = 0;
    virtual void requestExit() = 0;
    // Resets the session to a fresh, unstarted game (clearing any finished game's
    // resume-on-launch state) and replaces the whole screen stack with a fresh NewGameScreen.
    // Unlike pop(), this never depends on stack depth: BoardScreen may be the sole stack root
    // when a finished game was resumed at launch, so a plain pop() could empty the stack instead
    // of reaching the menu.
    virtual void returnToMainMenu() = 0;
};

}  // namespace minesweeper::ui
