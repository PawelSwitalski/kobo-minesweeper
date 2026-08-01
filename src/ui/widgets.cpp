#include "ui/widgets.h"

#include <cstdio>

namespace minesweeper::ui {

namespace {
void drawFrame(Renderer& r, Rect box, int thickness, Gray shade) {
    r.fillRect({box.x, box.y, box.w, thickness}, shade);
    r.fillRect({box.x, box.y + box.h - thickness, box.w, thickness}, shade);
    r.fillRect({box.x, box.y, thickness, box.h}, shade);
    r.fillRect({box.x + box.w - thickness, box.y, thickness, box.h}, shade);
}

std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        size_t nl = s.find('\n', start);
        if (nl == std::string::npos) {
            out.push_back(s.substr(start));
            return out;
        }
        out.push_back(s.substr(start, nl - start));
        start = nl + 1;
    }
}
}  // namespace

void Button::draw(Renderer& r, const Theme& t) const {
    Gray bg = toggled ? Gray::Light : Gray::White;
    r.fillRect(rect, bg);
    drawFrame(r, rect, toggled ? t.mm(0.8) : t.mm(0.35), enabled ? Gray::Black : Gray::Mid);
    TextStyle st;
    st.sizePx = textPx > 0 ? textPx : t.textPx;
    st.shade = enabled ? Gray::Black : Gray::Mid;
    st.bold = toggled;
    r.drawText(rect, label, st);
}

void Label::draw(Renderer& r, const Theme& t) const {
    TextStyle st;
    st.sizePx = sizePx > 0 ? sizePx : t.textPx;
    st.bold = bold;
    st.align = align;
    r.drawText(rect, text, st);
}

Dialog Dialog::info(std::string title, std::string message, std::string okLabel) {
    Dialog d;
    d.title_ = std::move(title);
    d.message_ = std::move(message);
    d.buttons_.push_back({{}, std::move(okLabel)});
    return d;
}

Dialog Dialog::confirm(std::string title, std::string message, std::string cancelLabel,
                       std::string confirmLabel) {
    Dialog d;
    d.title_ = std::move(title);
    d.message_ = std::move(message);
    d.buttons_.push_back({{}, std::move(cancelLabel)});
    d.buttons_.push_back({{}, std::move(confirmLabel)});
    return d;
}

void Dialog::layout(const Theme& t, const DisplayInfo& d) {
    int lines = static_cast<int>(splitLines(message_).size());
    int lineH = t.textPx + t.textPx / 2;
    int w = d.width - 4 * t.touchTargetPx / 2;
    int maxW = t.mm(95.0);
    if (w > maxW) w = maxW;
    int h = t.pad * 2 + t.touchTargetPx            // title band
            + lines * lineH + t.gap                // message
            + t.touchTargetPx + t.pad;             // buttons
    box_ = {(d.width - w) / 2, (d.height - h) / 2, w, h};

    int n = static_cast<int>(buttons_.size());
    int bw = (w - (n + 1) * t.pad) / n;
    int by = box_.y + h - t.pad - t.touchTargetPx;
    for (int i = 0; i < n; ++i)
        buttons_[i].rect = {box_.x + t.pad + i * (bw + t.pad), by, bw, t.touchTargetPx};
}

void Dialog::draw(Renderer& r, const Theme& t) const {
    r.fillRect(box_, Gray::White);
    drawFrame(r, box_, t.mm(0.6), Gray::Black);

    Rect title = {box_.x + t.pad, box_.y + t.pad, box_.w - 2 * t.pad, t.touchTargetPx};
    TextStyle st;
    st.sizePx = t.textPx;
    st.bold = true;
    r.drawText(title, title_, st);

    int lineH = t.textPx + t.textPx / 2;
    int y = title.y + title.h + t.gap / 2;
    st.bold = false;
    for (const std::string& line : splitLines(message_)) {
        r.drawText({box_.x + t.pad, y, box_.w - 2 * t.pad, lineH}, line, st);
        y += lineH;
    }
    for (const Button& b : buttons_)
        b.draw(r, t);
}

int Dialog::hitButton(Tap tap) const {
    for (size_t i = 0; i < buttons_.size(); ++i)
        if (buttons_[i].hit(tap)) return static_cast<int>(i);
    return -1;
}

std::string formatTime(uint32_t seconds) {
    char buf[16];
    uint32_t h = seconds / 3600, m = (seconds / 60) % 60, s = seconds % 60;
    if (h > 0)
        std::snprintf(buf, sizeof buf, "%u:%02u:%02u", h, m, s);
    else
        std::snprintf(buf, sizeof buf, "%u:%02u", m, s);
    return buf;
}

}  // namespace minesweeper::ui
