#include "platform/sdl/sdl_renderer.h"

#include <SDL.h>

namespace minesweeper {

SdlRenderer::~SdlRenderer() {
    if (win_) SDL_DestroyWindow(win_);
    SDL_Quit();
}

bool SdlRenderer::init(int width, int height, int dpi, bool color,
                       const std::string& fontPath, const std::string& boldFontPath) {
    SDL_SetMainReady();  // plain main(); no SDL_main indirection
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    win_ = SDL_CreateWindow("Minesweeper (simulator)", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, width, height, 0);
    if (!win_) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    info_ = {width, height, dpi, color};
    return canvas_.init(width, height, fontPath, boldFontPath);
}

void SdlRenderer::blit(Rect r, bool invert) {
    SDL_Surface* ws = SDL_GetWindowSurface(win_);
    if (!ws) return;

    if (r.w <= 0 || r.h <= 0) r = {0, 0, canvas_.width(), canvas_.height()};

    SDL_Surface* src = SDL_CreateRGBSurfaceWithFormatFrom(
        const_cast<uint8_t*>(canvas_.pixels()), canvas_.width(), canvas_.height(), 32,
        canvas_.stride(), SDL_PIXELFORMAT_RGBA32);
    if (!src) return;

    SDL_Rect rect{r.x, r.y, r.w, r.h};
    if (invert) {
        SDL_FillRect(ws, &rect, SDL_MapRGB(ws->format, 0, 0, 0));
    } else {
        SDL_BlitSurface(src, &rect, ws, &rect);
    }
    SDL_FreeSurface(src);
    SDL_UpdateWindowSurfaceRects(win_, &rect, 1);
}

void SdlRenderer::flushPartial(Rect r) {
    blit(r, false);
}

void SdlRenderer::flushFull() {
    // Simulated e-ink flash: whole screen to black, then the real content.
    blit({}, true);
    SDL_Delay(90);
    blit({}, false);
}

}  // namespace minesweeper
