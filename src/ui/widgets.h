#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "platform/input.h"
#include "platform/renderer.h"
#include "ui/theme.h"

namespace minesweeper::ui {

// All widgets draw through Renderer only — no OS calls (Constitution I).

struct Button {
    Rect rect;
    std::string label;
    bool enabled = true;
    bool toggled = false;  // pressed-in look
    int textPx = 0;        // 0 = theme.textPx

    void draw(Renderer& r, const Theme& t) const;
    bool hit(Tap tap) const { return enabled && rect.contains({tap.x, tap.y}); }
};

struct Label {
    Rect rect;
    std::string text;
    int sizePx = 0;  // 0 = theme.textPx
    bool bold = false;
    TextStyle::Align align = TextStyle::Align::Center;

    void draw(Renderer& r, const Theme& t) const;
};

// Modal confirm/info dialog. The owning screen draws it last and routes taps
// to hitButton() while active.
class Dialog {
public:
    static Dialog info(std::string title, std::string message, std::string okLabel);
    static Dialog confirm(std::string title, std::string message, std::string cancelLabel,
                          std::string confirmLabel);

    void layout(const Theme& t, const DisplayInfo& d);
    void draw(Renderer& r, const Theme& t) const;
    // -1 = no button hit (taps outside are swallowed: modal).
    int hitButton(Tap tap) const;
    int buttonCount() const { return static_cast<int>(buttons_.size()); }
    Rect box() const { return box_; }

private:
    std::string title_, message_;
    std::vector<Button> buttons_;
    Rect box_{};
};

// Formats seconds as "M:SS" (or "H:MM:SS" past an hour).
std::string formatTime(uint32_t seconds);

}  // namespace minesweeper::ui
