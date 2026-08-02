#include "platform/softcanvas.h"

#include <cstdio>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb/stb_truetype.h"

namespace minesweeper {

struct SoftCanvas::FontData {
    std::vector<uint8_t> bytes;
    stbtt_fontinfo info{};
    int ascent = 0, descent = 0, lineGap = 0;
};

namespace {

// Maps the accent to RGB on color displays; falls back to the gray shade
// otherwise so no meaning is ever lost without color (FR-014).
void resolveColor(Gray shade, Color accent, bool colorDisplay, uint8_t& r, uint8_t& g,
                  uint8_t& b) {
    r = g = b = static_cast<uint8_t>(shade);
    if (!colorDisplay) return;
    switch (accent) {
        case Color::Red:     r = 0xB4; g = 0x20; b = 0x20; break;  // mine-count 3
        case Color::Blue:    r = 0x10; g = 0x30; b = 0xC0; break;  // mine-count 1
        case Color::Green:   r = 0x12; g = 0x7A; b = 0x12; break;  // mine-count 2
        case Color::Navy:    r = 0x0A; g = 0x18; b = 0x68; break;  // mine-count 4 ("deep blue")
        case Color::Crimson: r = 0x8B; g = 0x0A; b = 0x2A; break;  // mine-count 5 ("cherry red")
        case Color::Cyan:    r = 0x0A; g = 0x7A; b = 0x82; break;  // mine-count 6
        case Color::None:    break;  // keep the grayscale shade already set above
    }
}

// Decodes one UTF-8 codepoint; ASCII fast-path, everything else best-effort.
int nextCodepoint(std::string_view s, size_t& i) {
    uint8_t c = static_cast<uint8_t>(s[i]);
    if (c < 0x80) { ++i; return c; }
    int len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : 2;
    int cp = c & (0x3F >> len);
    for (int k = 1; k < len && i + k < s.size(); ++k)
        cp = (cp << 6) | (static_cast<uint8_t>(s[i + k]) & 0x3F);
    i += len;
    return cp;
}

std::shared_ptr<SoftCanvas::FontData> loadFont(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return nullptr;
    auto fd = std::make_shared<SoftCanvas::FontData>();
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size <= 0) { std::fclose(f); return nullptr; }
    fd->bytes.resize(static_cast<size_t>(size));
    size_t got = std::fread(fd->bytes.data(), 1, fd->bytes.size(), f);
    std::fclose(f);
    if (got != fd->bytes.size()) return nullptr;
    if (!stbtt_InitFont(&fd->info, fd->bytes.data(),
                        stbtt_GetFontOffsetForIndex(fd->bytes.data(), 0)))
        return nullptr;
    stbtt_GetFontVMetrics(&fd->info, &fd->ascent, &fd->descent, &fd->lineGap);
    return fd;
}

}  // namespace

bool SoftCanvas::init(int width, int height, const std::string& fontPath,
                      const std::string& boldFontPath) {
    w_ = width;
    h_ = height;
    px_.assign(static_cast<size_t>(w_) * h_ * 4, 0xFF);
    font_ = loadFont(fontPath);
    fontBold_ = loadFont(boldFontPath);
    if (!fontBold_) fontBold_ = font_;
    return w_ > 0 && h_ > 0 && font_ != nullptr;
}

void SoftCanvas::fillRect(Rect r, uint8_t red, uint8_t green, uint8_t blue) {
    int x0 = r.x < 0 ? 0 : r.x;
    int y0 = r.y < 0 ? 0 : r.y;
    int x1 = r.x + r.w > w_ ? w_ : r.x + r.w;
    int y1 = r.y + r.h > h_ ? h_ : r.y + r.h;
    for (int y = y0; y < y1; ++y) {
        uint8_t* p = px_.data() + (static_cast<size_t>(y) * w_ + x0) * 4;
        for (int x = x0; x < x1; ++x) {
            p[0] = red; p[1] = green; p[2] = blue; p[3] = 0xFF;
            p += 4;
        }
    }
}

void SoftCanvas::drawLine(Point a, Point b, int thicknessPx, uint8_t gray) {
    // Grid rendering only needs axis-aligned lines; centered on the segment.
    int t = thicknessPx < 1 ? 1 : thicknessPx;
    Rect r;
    if (a.y == b.y) {
        int x0 = a.x < b.x ? a.x : b.x;
        int x1 = a.x < b.x ? b.x : a.x;
        r = {x0, a.y - t / 2, x1 - x0, t};
    } else {
        int y0 = a.y < b.y ? a.y : b.y;
        int y1 = a.y < b.y ? b.y : a.y;
        r = {a.x - t / 2, y0, t, y1 - y0};
    }
    fillRect(r, gray, gray, gray);
}

int SoftCanvas::measureText(std::string_view text, int sizePx, bool bold) {
    int w = 0;
    for (size_t i = 0; i < text.size();)
        w += glyph(nextCodepoint(text, i), sizePx, bold).advance;
    return w;
}

void SoftCanvas::drawText(Rect r, std::string_view text, const TextStyle& style,
                          bool colorDisplay) {
    if (!font_ || text.empty()) return;
    uint8_t red, green, blue;
    resolveColor(style.shade, style.accent, colorDisplay, red, green, blue);

    const FontData& fd = *(style.bold ? fontBold_ : font_);
    float scale = stbtt_ScaleForPixelHeight(const_cast<stbtt_fontinfo*>(&fd.info),
                                            static_cast<float>(style.sizePx));
    int textW = measureText(text, style.sizePx, style.bold);
    int penX = r.x;
    if (style.align == TextStyle::Align::Center)
        penX = r.x + (r.w - textW) / 2;
    else if (style.align == TextStyle::Align::Right)
        penX = r.x + r.w - textW;
    int fontH = static_cast<int>((fd.ascent - fd.descent) * scale + 0.5f);
    int baselineY = r.y + (r.h - fontH) / 2 + static_cast<int>(fd.ascent * scale + 0.5f);

    for (size_t i = 0; i < text.size();) {
        const Glyph& g = glyph(nextCodepoint(text, i), style.sizePx, style.bold);
        blendGlyph(g, penX, baselineY, r, red, green, blue);
        penX += g.advance;
    }
}

const SoftCanvas::Glyph& SoftCanvas::glyph(int codepoint, int sizePx, bool bold) {
    uint64_t key = (static_cast<uint64_t>(codepoint) << 24) |
                   (static_cast<uint64_t>(sizePx & 0x7FFFFF) << 1) | (bold ? 1 : 0);
    auto it = glyphCache_.find(key);
    if (it != glyphCache_.end()) return it->second;

    Glyph g;
    FontData& fd = *(bold ? fontBold_ : font_);
    float scale = stbtt_ScaleForPixelHeight(&fd.info, static_cast<float>(sizePx));
    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&fd.info, codepoint, &adv, &lsb);
    g.advance = static_cast<int>(adv * scale + 0.5f);

    int w = 0, h = 0, xoff = 0, yoff = 0;
    unsigned char* bmp =
        stbtt_GetCodepointBitmap(&fd.info, scale, scale, codepoint, &w, &h, &xoff, &yoff);
    if (bmp) {
        g.w = w; g.h = h; g.xoff = xoff; g.yoff = yoff;
        g.alpha.assign(bmp, bmp + static_cast<size_t>(w) * h);
        stbtt_FreeBitmap(bmp, nullptr);
    }
    return glyphCache_.emplace(key, std::move(g)).first->second;
}

void SoftCanvas::blendGlyph(const Glyph& g, int penX, int baselineY, const Rect& clip,
                            uint8_t red, uint8_t green, uint8_t blue) {
    for (int gy = 0; gy < g.h; ++gy) {
        int y = baselineY + g.yoff + gy;
        if (y < 0 || y >= h_) continue;
        for (int gx = 0; gx < g.w; ++gx) {
            int x = penX + g.xoff + gx;
            if (x < 0 || x >= w_) continue;
            uint8_t a = g.alpha[static_cast<size_t>(gy) * g.w + gx];
            if (!a) continue;
            uint8_t* p = px_.data() + (static_cast<size_t>(y) * w_ + x) * 4;
            p[0] = static_cast<uint8_t>((p[0] * (255 - a) + red * a) / 255);
            p[1] = static_cast<uint8_t>((p[1] * (255 - a) + green * a) / 255);
            p[2] = static_cast<uint8_t>((p[2] * (255 - a) + blue * a) / 255);
        }
    }
    (void)clip;  // glyphs may slightly overhang their layout rect by design
}

}  // namespace minesweeper
