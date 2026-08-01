#pragma once
#include <cstdint>
#include <string_view>

namespace minesweeper {

struct Point {
    int x = 0, y = 0;
};

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(Point p) const {
        return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
    }
    // Smallest rect covering both (treats empty rects as identity).
    Rect unite(const Rect& o) const;
};

// Grayscale-first palette (Constitution II). Values are 0=black..255=white;
// backends may quantize to the device's gray levels.
enum class Gray : uint8_t {
    Black = 0x00,
    Dark = 0x55,
    Mid = 0xAA,
    Light = 0xD8,
    Lighter = 0xEE,
    White = 0xFF,
};

// Accent color: optional, meaning must never be encoded in color alone (FR-014).
enum class Color : uint8_t { None, Red };

struct TextStyle {
    int sizePx = 24;
    bool bold = false;
    Gray shade = Gray::Black;
    Color accent = Color::None;
    enum class Align { Left, Center, Right } align = Align::Center;
};

struct DisplayInfo {
    int width = 0, height = 0, dpi = 0;
    bool color = false;
};

// See docs/contracts/platform-abstraction.md. The UI layer uses only this
// interface; core uses nothing from platform.
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual DisplayInfo info() const = 0;

    virtual void fillRect(Rect r, Gray shade, Color accent = Color::None) = 0;
    virtual void drawText(Rect r, std::string_view text, const TextStyle& style) = 0;
    virtual void drawLine(Point a, Point b, int thicknessPx, Gray shade) = 0;

    // E-ink refresh discipline (FR-016): partial is fast and may ghost; full
    // flashes and clears ghosting. Ghosting policy (auto-promotion after N
    // partials) lives in the device backend, not in UI code.
    virtual void flushPartial(Rect r) = 0;
    virtual void flushFull() = 0;

    // User-tunable ghosting policy (Settings screen): n <= 0 disables
    // auto-promotion. No-op on backends without a ghosting concept (e.g.
    // the desktop simulator).
    virtual void setGhostingInterval(int /*n*/) {}
};

inline Rect Rect::unite(const Rect& o) const {
    if (w <= 0 || h <= 0) return o;
    if (o.w <= 0 || o.h <= 0) return *this;
    int x0 = x < o.x ? x : o.x;
    int y0 = y < o.y ? y : o.y;
    int x1 = (x + w > o.x + o.w) ? x + w : o.x + o.w;
    int y1 = (y + h > o.y + o.h) ? y + h : o.y + o.h;
    return {x0, y0, x1 - x0, y1 - y0};
}

}  // namespace minesweeper
