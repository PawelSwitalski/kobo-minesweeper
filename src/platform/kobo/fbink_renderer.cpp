#include "platform/kobo/fbink_renderer.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "fbink.h"

namespace minesweeper {

namespace {
FBInkConfig baseConfig() {
    FBInkConfig cfg{};
    cfg.is_quiet = true;
    return cfg;
}
}  // namespace

FbinkRenderer::~FbinkRenderer() {
    if (fbfd_ >= 0) {
        // Leave the framebuffer as Nickel expects it. Nickel is frozen while
        // we run and does NOT repaint on SIGCONT, so restore the screen we
        // captured in init() with a flashing refresh; without this our last
        // frame stays on the panel until the user pokes a menu.
        if (haveNickelDump_) {
            FBInkConfig cfg = baseConfig();
            cfg.is_flashing = true;
            cfg.wfm_mode = WFM_GC16;
            if (fbink_restore(fbfd_, &cfg, &nickelDump_) != EXIT_SUCCESS)
                std::fprintf(stderr, "fbink_restore failed\n");
        } else {
            // No snapshot (dump failed, or sunxi): a blank page still beats a
            // stuck app frame.
            canvas_.fillRect({0, 0, canvas_.width(), canvas_.height()}, 0xFF, 0xFF, 0xFF);
            pushRegion({0, 0, canvas_.width(), canvas_.height()}, true);
        }
        fbink_close(fbfd_);
    }
    if (haveNickelDump_) fbink_free_dump_data(&nickelDump_);
}

bool FbinkRenderer::init(const std::string& assetsDir) {
    FBInkConfig cfg = baseConfig();
    fbfd_ = fbink_open();
    if (fbfd_ < 0) {
        std::fprintf(stderr, "fbink_open failed\n");
        return false;
    }
    if (fbink_init(fbfd_, &cfg) != EXIT_SUCCESS) {
        std::fprintf(stderr, "fbink_init failed\n");
        return false;
    }

    FBInkState state{};
    fbink_get_state(&cfg, &state);
    info_.width = static_cast<int>(state.view_width);
    info_.height = static_cast<int>(state.view_height);
    info_.dpi = static_cast<int>(state.screen_dpi);
    // Color panels (Kaleido: Libra/Clara Colour) run a 32bpp fb under Nickel;
    // mono devices run 8bpp.
    info_.color = state.bpp >= 24;

    // Snapshot Nickel's screen before we draw anything, so the destructor can
    // put it back. Skipped on sunxi SoCs (Elipsa/Sage): the fb there is
    // per-process, so a dump would only capture solid black.
    if (!state.is_sunxi) {
        if (fbink_dump(fbfd_, &nickelDump_) == EXIT_SUCCESS)
            haveNickelDump_ = true;
        else
            std::fprintf(stderr, "fbink_dump failed; will blank screen on exit instead\n");
    }

    return canvas_.init(info_.width, info_.height, assetsDir + "/DejaVuSans.ttf",
                        assetsDir + "/DejaVuSans-Bold.ttf");
}

void FbinkRenderer::pushRegion(Rect r, bool flash) {
    // Clamp to the canvas.
    if (r.x < 0) { r.w += r.x; r.x = 0; }
    if (r.y < 0) { r.h += r.y; r.y = 0; }
    if (r.x + r.w > canvas_.width()) r.w = canvas_.width() - r.x;
    if (r.y + r.h > canvas_.height()) r.h = canvas_.height() - r.y;
    if (r.w <= 0 || r.h <= 0) return;

    // Sub-image copy of the dirty region (RGBA).
    std::vector<unsigned char> sub(static_cast<size_t>(r.w) * r.h * 4);
    const uint8_t* src = canvas_.pixels();
    for (int y = 0; y < r.h; ++y)
        std::memcpy(sub.data() + static_cast<size_t>(y) * r.w * 4,
                    src + ((static_cast<size_t>(r.y + y) * canvas_.width()) + r.x) * 4,
                    static_cast<size_t>(r.w) * 4);

    FBInkConfig cfg = baseConfig();
    cfg.no_refresh = true;  // draw first, refresh explicitly below
    cfg.ignore_alpha = true;
    if (fbink_print_raw_data(fbfd_, sub.data(), r.w, r.h, sub.size(),
                             static_cast<short>(r.x), static_cast<short>(r.y),
                             &cfg) != EXIT_SUCCESS) {
        std::fprintf(stderr, "fbink_print_raw_data failed\n");
        return;
    }

    cfg.no_refresh = false;
    cfg.is_flashing = flash;
    cfg.wfm_mode = flash ? WFM_GC16 : WFM_AUTO;
    fbink_refresh(fbfd_, static_cast<uint32_t>(r.y), static_cast<uint32_t>(r.x),
                  static_cast<uint32_t>(r.w), static_cast<uint32_t>(r.h), &cfg);
}

void FbinkRenderer::flushPartial(Rect r) {
    if (++partialCount_ >= ghostingPartials_) {
        // Ghosting policy: promote to a full flashing refresh (Constitution II).
        flushFull();
        return;
    }
    pushRegion(r, false);
}

void FbinkRenderer::flushFull() {
    partialCount_ = 0;
    pushRegion({0, 0, canvas_.width(), canvas_.height()}, true);
}

void FbinkRenderer::setGhostingInterval(int n) {
    ghostingPartials_ = n > 0 ? n : INT_MAX;
}

}  // namespace minesweeper
