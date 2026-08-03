#include "platform/sdl/mouse_touch.h"

#include <SDL.h>

namespace minesweeper {

namespace {
// A single simulated "finger": slot id 0 is the only slot the SDL backend ever reports, since a
// desktop mouse has only one pointer (research.md #7).
constexpr int kMouseSlot = 0;
}  // namespace

std::optional<std::variant<Tap, core::GestureEvent>> MouseTouch::waitForEvent(int timeoutMs) {
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
                    gestureRecognizer_.feed({{kMouseSlot, ev.button.x, ev.button.y}},
                                             ev.button.timestamp);
                }
                break;
            case SDL_MOUSEMOTION:
                if (mouseDown_) {
                    auto result = gestureRecognizer_.feed(
                        {{kMouseSlot, ev.motion.x, ev.motion.y}}, ev.motion.timestamp);
                    if (result) {
                        // A drag can only ever yield a mid-gesture PanStep here (a GestureTap is
                        // only ever produced on lift, when the points list is empty).
                        return *std::get_if<core::GestureEvent>(&*result);
                    }
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    mouseDown_ = false;
                    auto result = gestureRecognizer_.feed({}, ev.button.timestamp);
                    if (result) {
                        auto* tap = std::get_if<core::GestureTap>(&*result);
                        return Tap{tap->x, tap->y, tap->longPress};
                    }
                    // A drag that never produced a Tap (it was classified as a pan, not a tap) --
                    // no event on lift; keep waiting for the next input or the timeout.
                }
                break;
            case SDL_MOUSEWHEEL: {
                int dir = ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
                int y = ev.wheel.y * dir;
                if (y != 0) return core::GestureEvent{core::GestureKind::ZoomStep, y > 0 ? 1 : -1};
                break;
            }
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
