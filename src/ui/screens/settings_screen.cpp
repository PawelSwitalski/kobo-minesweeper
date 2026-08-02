#include "ui/screens/settings_screen.h"

#include "core/settings.h"
#include "ui/app.h"

namespace minesweeper::ui {

void SettingsScreen::layout() {
    const Theme& t = app_.theme();
    DisplayInfo d = app_.renderer().info();

    int y = t.pad * 2 + t.titlePx + t.gap;
    int btnW = (d.width - 3 * t.pad) / 2;
    colorButton_.rect = {t.pad, y, btnW, t.touchTargetPx};
    colorButton_.label = "Color";
    blackWhiteButton_.rect = {t.pad * 2 + btnW, y, btnW, t.touchTargetPx};
    blackWhiteButton_.label = "Black-and-white";

    int hideTimerY = y + t.touchTargetPx + t.gap;
    hideTimerButton_.rect = {t.pad, hideTimerY, d.width - 2 * t.pad, t.touchTargetPx};
    hideTimerButton_.label = "Hide Timer";

    backButton_.rect = {t.pad, d.height - t.pad - t.touchTargetPx, d.width - 2 * t.pad,
                        t.touchTargetPx};
    backButton_.label = "Back";
}

void SettingsScreen::draw() {
    layout();
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    DisplayInfo d = r.info();

    r.fillRect({0, 0, d.width, d.height}, Gray::White);

    Label title;
    title.rect = {t.pad, t.pad, d.width - 2 * t.pad, t.titlePx};
    title.text = "Settings";
    title.bold = true;
    title.sizePx = t.titlePx;
    title.draw(r, t);

    core::ColorMode mode = app_.settings().colorMode;
    colorButton_.toggled = mode == core::ColorMode::Color;
    blackWhiteButton_.toggled = mode == core::ColorMode::BlackAndWhite;
    colorButton_.draw(r, t);
    blackWhiteButton_.draw(r, t);

    hideTimerButton_.toggled = app_.settings().hideTimer;
    hideTimerButton_.draw(r, t);

    backButton_.draw(r, t);
}

void SettingsScreen::onTap(Tap tap) {
    if (colorButton_.hit(tap)) {
        app_.settings().colorMode = core::ColorMode::Color;
        app_.autosaveSettings();  // also re-derives Theme::color (contracts/app-interface.md)
        draw();
        app_.renderer().flushFull();
        return;
    }
    if (blackWhiteButton_.hit(tap)) {
        app_.settings().colorMode = core::ColorMode::BlackAndWhite;
        app_.autosaveSettings();
        draw();
        app_.renderer().flushFull();
        return;
    }
    if (hideTimerButton_.hit(tap)) {
        app_.settings().hideTimer = !app_.settings().hideTimer;
        app_.autosaveSettings();
        draw();
        app_.renderer().flushFull();
        return;
    }
    if (backButton_.hit(tap)) {
        app_.pop();
        return;
    }
}

}  // namespace minesweeper::ui
