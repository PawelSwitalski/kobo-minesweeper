#pragma once
#include "platform/renderer.h"
#include "platform/softcanvas.h"

namespace minesweeper {

// Renderer whose drawing ops all target a SoftCanvas; concrete backends only
// implement info() and the two flushes.
class CanvasRenderer : public Renderer {
public:
    void fillRect(Rect r, Gray shade, Color accent = Color::None) override {
        uint8_t v = static_cast<uint8_t>(shade);
        if (info().color && accent == Color::Red)
            canvas_.fillRect(r, 0xF2, 0xC8, 0xC8);
        else
            canvas_.fillRect(r, v, v, v);
    }
    void drawText(Rect r, std::string_view text, const TextStyle& style) override {
        canvas_.drawText(r, text, style, info().color);
    }
    void drawLine(Point a, Point b, int thicknessPx, Gray shade) override {
        canvas_.drawLine(a, b, thicknessPx, static_cast<uint8_t>(shade));
    }

    SoftCanvas& canvas() { return canvas_; }

protected:
    SoftCanvas canvas_;
};

}  // namespace minesweeper
