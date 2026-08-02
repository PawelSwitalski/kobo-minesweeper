#pragma once
#include <cstdint>

#include "platform/input.h"

namespace minesweeper::ui {

class App;

// A full-screen view. draw() renders everything into the canvas; the app shell
// full-flushes after screen transitions. Handlers do their own partial flushes.
class Screen {
public:
    explicit Screen(App& app) : app_(app) {}
    virtual ~Screen() = default;

    virtual void draw() = 0;
    virtual void onTap(Tap tap) = 0;
    // Called on input timeout with the active seconds since the last call.
    virtual void onTick(uint32_t /*activeSeconds*/) {}
    // Only time spent on screens that return true counts toward any "active
    // time" tracking the app may keep (device sleep and menu time excluded).
    virtual bool countsPlayTime() const { return false; }

protected:
    App& app_;
};

}  // namespace minesweeper::ui
