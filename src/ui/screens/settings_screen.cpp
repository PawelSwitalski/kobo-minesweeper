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

    int screenRefreshLabelY = hideTimerY + t.touchTargetPx + t.gap;
    screenRefreshLabelRect_ = {t.pad, screenRefreshLabelY, d.width - 2 * t.pad, t.textPx};

    int screenRefreshY = screenRefreshLabelY + t.textPx + t.gap;
    int btn4W = (d.width - 5 * t.pad) / 4;
    refresh5Button_.rect = {t.pad, screenRefreshY, btn4W, t.touchTargetPx};
    refresh5Button_.label = "5";
    refresh10Button_.rect = {t.pad * 2 + btn4W, screenRefreshY, btn4W, t.touchTargetPx};
    refresh10Button_.label = "10";
    refresh25Button_.rect = {t.pad * 3 + btn4W * 2, screenRefreshY, btn4W, t.touchTargetPx};
    refresh25Button_.label = "25";
    refreshNeverButton_.rect = {t.pad * 4 + btn4W * 3, screenRefreshY, btn4W, t.touchTargetPx};
    refreshNeverButton_.label = "Never";

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

    Label screenRefreshLabel;
    screenRefreshLabel.rect = screenRefreshLabelRect_;
    screenRefreshLabel.text = "Screen Refresh";
    screenRefreshLabel.align = TextStyle::Align::Left;
    screenRefreshLabel.draw(r, t);

    core::ScreenRefreshInterval interval = app_.settings().screenRefreshInterval;
    refresh5Button_.toggled = interval == core::ScreenRefreshInterval::Every5;
    refresh10Button_.toggled = interval == core::ScreenRefreshInterval::Every10;
    refresh25Button_.toggled = interval == core::ScreenRefreshInterval::Every25;
    refreshNeverButton_.toggled = interval == core::ScreenRefreshInterval::Never;
    refresh5Button_.draw(r, t);
    refresh10Button_.draw(r, t);
    refresh25Button_.draw(r, t);
    refreshNeverButton_.draw(r, t);

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
    if (refresh5Button_.hit(tap)) {
        app_.settings().screenRefreshInterval = core::ScreenRefreshInterval::Every5;
        app_.autosaveSettings();
        draw();
        app_.renderer().flushFull();
        return;
    }
    if (refresh10Button_.hit(tap)) {
        app_.settings().screenRefreshInterval = core::ScreenRefreshInterval::Every10;
        app_.autosaveSettings();
        draw();
        app_.renderer().flushFull();
        return;
    }
    if (refresh25Button_.hit(tap)) {
        app_.settings().screenRefreshInterval = core::ScreenRefreshInterval::Every25;
        app_.autosaveSettings();
        draw();
        app_.renderer().flushFull();
        return;
    }
    if (refreshNeverButton_.hit(tap)) {
        app_.settings().screenRefreshInterval = core::ScreenRefreshInterval::Never;
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
