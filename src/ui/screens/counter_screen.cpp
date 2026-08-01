#include "ui/screens/counter_screen.h"

#include <memory>

#include "ui/app.h"
#include "ui/screens/about_screen.h"

namespace minesweeper::ui {

void CounterScreen::layout() {
    const Theme& t = app_.theme();
    DisplayInfo d = app_.renderer().info();

    countRect_ = {t.pad, t.pad * 2 + t.titlePx, d.width - 2 * t.pad, t.mm(20.0)};

    int buttonW = d.width - 2 * t.pad;
    int buttonH = t.touchTargetPx * 2;
    incrementButton_.rect = {t.pad, d.height - t.pad - buttonH - t.gap - t.touchTargetPx,
                             buttonW, buttonH};
    incrementButton_.label = "Tap (+1)";
    incrementButton_.textPx = t.titlePx;

    aboutButton_.rect = {t.pad, d.height - t.pad - t.touchTargetPx, buttonW, t.touchTargetPx};
    aboutButton_.label = "About";
}

void CounterScreen::draw() {
    layout();
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    DisplayInfo d = r.info();

    r.fillRect({0, 0, d.width, d.height}, Gray::White);

    Label title;
    title.rect = {t.pad, t.pad, d.width - 2 * t.pad, t.titlePx};
    title.text = "Minesweeper";
    title.bold = true;
    title.sizePx = t.titlePx;
    title.draw(r, t);

    drawCount();
    incrementButton_.draw(r, t);
    aboutButton_.draw(r, t);
}

void CounterScreen::drawCount() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(countRect_, Gray::White);
    Label count;
    count.rect = countRect_;
    count.text = std::to_string(app_.counter().value());
    count.bold = true;
    count.sizePx = t.mm(16.0);
    count.draw(r, t);
}

void CounterScreen::onTap(Tap tap) {
    if (incrementButton_.hit(tap)) {
        app_.counter().increment();
        app_.autosave();
        drawCount();
        app_.renderer().flushPartial(countRect_);
        return;
    }
    if (aboutButton_.hit(tap)) {
        app_.push(std::make_unique<AboutScreen>(app_));
    }
}

}  // namespace minesweeper::ui
