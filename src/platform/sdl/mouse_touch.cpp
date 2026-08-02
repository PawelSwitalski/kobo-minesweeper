#include "platform/sdl/mouse_touch.h"

#include <SDL.h>

namespace minesweeper {

std::optional<Tap> MouseTouch::waitForTap(int timeoutMs) {
    Uint32 deadline = SDL_GetTicks() + static_cast<Uint32>(timeoutMs);
    while (true) {
        Uint32 now = SDL_GetTicks();
        if (now >= deadline) return std::nullopt;
        SDL_Event ev;
        if (!SDL_WaitEventTimeout(&ev, static_cast<int>(deadline - now))) return std::nullopt;
        switch (ev.type) {
            case SDL_QUIT:
                *quit_ = true;
                return std::nullopt;
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    mouseDown_ = true;
                    downTick_ = ev.button.timestamp;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    bool longPress = mouseDown_ &&
                        (ev.button.timestamp - downTick_) >= static_cast<unsigned int>(kLongPressMs);
                    mouseDown_ = false;
                    return Tap{ev.button.x, ev.button.y, longPress};
                }
                break;
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_EXPOSED ||
                    ev.window.event == SDL_WINDOWEVENT_RESTORED) {
                    *redraw_ = true;
                    return std::nullopt;
                }
                break;
            default:
                break;
        }
    }
}

}  // namespace minesweeper
