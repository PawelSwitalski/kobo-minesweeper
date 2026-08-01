#pragma once
#include <string>

#include "platform/canvas_renderer.h"

struct SDL_Window;

namespace minesweeper {

// Desktop simulator backend: 1:1 window over the shared canvas; flushFull
// simulates the e-ink flash (brief invert) so refresh behavior is visible
// during development.
class SdlRenderer : public CanvasRenderer {
public:
    ~SdlRenderer() override;

    bool init(int width, int height, int dpi, bool color, const std::string& fontPath,
              const std::string& boldFontPath);

    DisplayInfo info() const override { return info_; }
    void flushPartial(Rect r) override;
    void flushFull() override;

private:
    void blit(Rect r, bool invert);

    SDL_Window* win_ = nullptr;
    DisplayInfo info_{};
};

}  // namespace minesweeper
