#include "ui/screens/about_screen.h"

#include "ui/app.h"

namespace minesweeper::ui {

void AboutScreen::draw() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    DisplayInfo d = r.info();

    r.fillRect({0, 0, d.width, d.height}, Gray::White);

    Label title;
    title.rect = {t.pad, t.pad, d.width - 2 * t.pad, t.titlePx};
    title.text = "About";
    title.bold = true;
    title.sizePx = t.titlePx;
    title.draw(r, t);

    // Label/drawText render a single line each — no embedded '\n' support
    // (only Dialog splits multi-line messages internally) — so each line of
    // body text gets its own Label rect.
    static const char* kBodyLines[] = {
        "Minesweeper template placeholder demo.",
        "Layers: core / persist / platform / ui.",
        "See SETUP.md to start a real project.",
    };
    int lineH = t.textPx + t.textPx / 2;
    int y = t.pad * 2 + t.titlePx;
    for (const char* line : kBodyLines) {
        Label body;
        body.rect = {t.pad, y, d.width - 2 * t.pad, lineH};
        body.text = line;
        body.sizePx = t.textPx;
        body.align = TextStyle::Align::Left;
        body.draw(r, t);
        y += lineH;
    }

    backButton_.rect = {t.pad, d.height - t.pad - t.touchTargetPx, d.width - 2 * t.pad,
                        t.touchTargetPx};
    backButton_.label = "Back";
    backButton_.draw(r, t);
}

void AboutScreen::onTap(Tap tap) {
    if (backButton_.hit(tap)) app_.pop();
}

}  // namespace minesweeper::ui
