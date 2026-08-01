#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "platform/renderer.h"

namespace minesweeper {

// Shared software rasterizer: both backends draw into this 32-bit RGBA buffer
// and only differ in how they flush it (SDL window blit vs. FBInk raw print).
// This guarantees pixel-identical rendering between simulator and device.
//
// Text uses stb_truetype (vendored, public domain). Justification per
// Constitution VI: FBInk already bundles stb_truetype internally; using the
// same single header directly gives one text path for all backends instead of
// two divergent ones. No new kind of dependency is introduced.
class SoftCanvas {
public:
    struct FontData;  // stbtt_fontinfo + file bytes (definition in .cpp)

    bool init(int width, int height, const std::string& fontPath,
              const std::string& boldFontPath);

    int width() const { return w_; }
    int height() const { return h_; }
    const uint8_t* pixels() const { return px_.data(); }  // RGBA, row-major
    int stride() const { return w_ * 4; }

    void fillRect(Rect r, uint8_t red, uint8_t green, uint8_t blue);
    void drawLine(Point a, Point b, int thicknessPx, uint8_t gray);  // axis-aligned
    void drawText(Rect r, std::string_view text, const TextStyle& style, bool colorDisplay);
    int measureText(std::string_view text, int sizePx, bool bold);

private:
    struct Glyph {
        int w = 0, h = 0, xoff = 0, yoff = 0;
        int advance = 0;  // pixels (rounded)
        std::vector<uint8_t> alpha;
    };

    const Glyph& glyph(int codepoint, int sizePx, bool bold);
    void blendGlyph(const Glyph& g, int penX, int baselineY, const Rect& clip,
                    uint8_t red, uint8_t green, uint8_t blue);

    int w_ = 0, h_ = 0;
    std::vector<uint8_t> px_;
    std::shared_ptr<FontData> font_, fontBold_;
    std::map<uint64_t, Glyph> glyphCache_;
};

}  // namespace minesweeper
