#pragma once
#include <memory>

#include "core/counter.h"
#include "platform/renderer.h"
#include "ui/screens/screen.h"
#include "ui/theme.h"

namespace minesweeper::ui {

// Semantic surface the screens talk to. Implemented by the app shell in
// main.cpp, which owns backends, persistence wiring and the screen stack.
//
// counter()/autosave() are this template's placeholder app-state hooks.
// Replace them with your own (session/settings/stats/whatever your project
// needs) when building a real project from this template — keep the shape:
// state accessors on App, one persist entrypoint the screens call after
// every mutation.
class App {
public:
    virtual ~App() = default;

    virtual Renderer& renderer() = 0;
    virtual const Theme& theme() const = 0;

    virtual core::Counter& counter() = 0;
    virtual void autosave() = 0;  // persist counter.json after every mutation

    // Navigation. Transitions trigger a full redraw + flushFull (Constitution II).
    virtual void push(std::unique_ptr<Screen> s) = 0;
    virtual void pop() = 0;
    virtual void requestExit() = 0;
};

}  // namespace minesweeper::ui
