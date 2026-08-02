#include "ui/screens/new_game_screen.h"

#include <algorithm>
#include <memory>
#include <string>

#include "ui/app.h"
#include "ui/screens/board_screen.h"
#include "ui/screens/settings_screen.h"

namespace minesweeper::ui {

namespace {
constexpr int kMinSide = 5, kMaxSide = 16;
}

void NewGameScreen::layout() {
    const Theme& t = app_.theme();
    DisplayInfo d = app_.renderer().info();
    int fullW = d.width - 2 * t.pad;
    int y = t.pad * 2 + t.titlePx;

    beginnerButton_.rect = {t.pad, y, fullW, t.touchTargetPx};
    beginnerButton_.label = "Beginner (9x9, 10 mines)";
    y += t.touchTargetPx + t.gap;

    intermediateButton_.rect = {t.pad, y, fullW, t.touchTargetPx};
    intermediateButton_.label = "Intermediate (16x16, 40 mines)";
    y += t.touchTargetPx + t.gap;

    expertButton_.rect = {t.pad, y, fullW, t.touchTargetPx};
    expertButton_.label = "Expert (30x16, 99 mines)";
    y += t.touchTargetPx + t.gap;

    settingsButton_.rect = {t.pad, y, fullW, t.touchTargetPx};
    settingsButton_.label = "Settings";
    y += t.touchTargetPx + t.gap * 2;

    int stepW = t.touchTargetPx;
    int valueW = fullW - 2 * stepW - 2 * t.gap;

    widthMinus_.rect = {t.pad, y, stepW, t.touchTargetPx};
    widthMinus_.label = "-";
    widthValueRect_ = {t.pad + stepW + t.gap, y, valueW, t.touchTargetPx};
    widthPlus_.rect = {t.pad + stepW + t.gap + valueW + t.gap, y, stepW, t.touchTargetPx};
    widthPlus_.label = "+";
    y += t.touchTargetPx + t.gap;

    heightMinus_.rect = {t.pad, y, stepW, t.touchTargetPx};
    heightMinus_.label = "-";
    heightValueRect_ = {t.pad + stepW + t.gap, y, valueW, t.touchTargetPx};
    heightPlus_.rect = {t.pad + stepW + t.gap + valueW + t.gap, y, stepW, t.touchTargetPx};
    heightPlus_.label = "+";
    y += t.touchTargetPx + t.gap;

    minesMinus_.rect = {t.pad, y, stepW, t.touchTargetPx};
    minesMinus_.label = "-";
    minesValueRect_ = {t.pad + stepW + t.gap, y, valueW, t.touchTargetPx};
    minesPlus_.rect = {t.pad + stepW + t.gap + valueW + t.gap, y, stepW, t.touchTargetPx};
    minesPlus_.label = "+";
    y += t.touchTargetPx + t.gap;

    reasonRect_ = {t.pad, y, fullW, t.smallPx};
    y += t.smallPx + t.gap;

    startCustomButton_.rect = {t.pad, y, fullW, t.touchTargetPx};
    startCustomButton_.label = "Start Custom Game";
}

void NewGameScreen::drawWidthValue() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(widthValueRect_, Gray::White);
    Label l;
    l.rect = widthValueRect_;
    l.text = "Width: " + std::to_string(customWidth_);
    l.draw(r, t);
}

void NewGameScreen::drawHeightValue() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(heightValueRect_, Gray::White);
    Label l;
    l.rect = heightValueRect_;
    l.text = "Height: " + std::to_string(customHeight_);
    l.draw(r, t);
}

void NewGameScreen::drawMinesValue() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(minesValueRect_, Gray::White);
    Label l;
    l.rect = minesValueRect_;
    l.text = "Mines: " + std::to_string(customMines_);
    l.draw(r, t);
}

void NewGameScreen::drawReason() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(reasonRect_, Gray::White);
    if (reasonText_.empty()) return;
    Label l;
    l.rect = reasonRect_;
    l.text = reasonText_;
    l.sizePx = t.smallPx;
    l.align = TextStyle::Align::Left;
    l.draw(r, t);
}

void NewGameScreen::clampCustom() {
    customWidth_ = std::clamp(customWidth_, kMinSide, kMaxSide);
    customHeight_ = std::clamp(customHeight_, kMinSide, kMaxSide);
    int maxMines = customWidth_ * customHeight_ - 9;
    customMines_ = std::clamp(customMines_, 1, maxMines);
}

void NewGameScreen::adjustWidth(int delta) {
    int before = customWidth_;
    customWidth_ += delta;
    clampCustom();
    reasonText_ = (customWidth_ == before)
                      ? "Width must be between " + std::to_string(kMinSide) + " and " +
                            std::to_string(kMaxSide)
                      : std::string();
    drawWidthValue();
    drawMinesValue();  // the mine ceiling may have shifted too
    drawReason();
    app_.renderer().flushPartial(widthValueRect_.unite(minesValueRect_).unite(reasonRect_));
}

void NewGameScreen::adjustHeight(int delta) {
    int before = customHeight_;
    customHeight_ += delta;
    clampCustom();
    reasonText_ = (customHeight_ == before)
                      ? "Height must be between " + std::to_string(kMinSide) + " and " +
                            std::to_string(kMaxSide)
                      : std::string();
    drawHeightValue();
    drawMinesValue();
    drawReason();
    app_.renderer().flushPartial(heightValueRect_.unite(minesValueRect_).unite(reasonRect_));
}

void NewGameScreen::adjustMines(int delta) {
    int before = customMines_;
    customMines_ += delta;
    clampCustom();
    if (customMines_ == before) {
        int maxMines = customWidth_ * customHeight_ - 9;
        reasonText_ = "Mines must be between 1 and " + std::to_string(maxMines);
    } else {
        reasonText_.clear();
    }
    drawMinesValue();
    drawReason();
    app_.renderer().flushPartial(minesValueRect_.unite(reasonRect_));
}

void NewGameScreen::draw() {
    layout();
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    DisplayInfo d = r.info();

    r.fillRect({0, 0, d.width, d.height}, Gray::White);

    Label title;
    title.rect = {t.pad, t.pad, d.width - 2 * t.pad, t.titlePx};
    title.text = "New Game";
    title.bold = true;
    title.sizePx = t.titlePx;
    title.draw(r, t);

    beginnerButton_.draw(r, t);
    intermediateButton_.draw(r, t);
    expertButton_.draw(r, t);
    settingsButton_.draw(r, t);

    Label customTitle;
    customTitle.rect = {t.pad, widthMinus_.rect.y - t.textPx - t.gap, d.width - 2 * t.pad,
                        t.textPx};
    customTitle.text = "Custom";
    customTitle.bold = true;
    customTitle.align = TextStyle::Align::Left;
    customTitle.draw(r, t);

    widthMinus_.draw(r, t);
    widthPlus_.draw(r, t);
    drawWidthValue();
    heightMinus_.draw(r, t);
    heightPlus_.draw(r, t);
    drawHeightValue();
    minesMinus_.draw(r, t);
    minesPlus_.draw(r, t);
    drawMinesValue();
    drawReason();
    startCustomButton_.draw(r, t);

    if (abandonDialogActive_) {
        abandonDialog_.layout(t, d);
        abandonDialog_.draw(r, t);
    }
}

void NewGameScreen::requestStart(core::DifficultyConfig cfg) {
    if (app_.hasInProgressGame()) {
        pendingConfig_ = cfg;
        abandonDialog_ = Dialog::confirm(
            "Abandon current game?",
            "Starting a new game will discard your current in-progress game.", "Cancel",
            "Abandon");
        abandonDialogActive_ = true;
        draw();
        app_.renderer().flushFull();
        return;
    }
    startGame(cfg);
}

void NewGameScreen::startGame(core::DifficultyConfig cfg) {
    app_.startNewGame(cfg);
    app_.push(std::make_unique<BoardScreen>(app_));
}

void NewGameScreen::onTap(Tap tap) {
    if (abandonDialogActive_) {
        int hit = abandonDialog_.hitButton(tap);
        abandonDialogActive_ = false;
        if (hit == 1) {
            startGame(pendingConfig_);
        } else {
            draw();
            app_.renderer().flushFull();
        }
        return;  // modal: every tap while active is handled here, never falls through
    }

    if (beginnerButton_.hit(tap)) { requestStart(core::DifficultyConfig::beginner()); return; }
    if (intermediateButton_.hit(tap)) {
        requestStart(core::DifficultyConfig::intermediate());
        return;
    }
    if (expertButton_.hit(tap)) { requestStart(core::DifficultyConfig::expert()); return; }
    if (settingsButton_.hit(tap)) {
        app_.push(std::make_unique<SettingsScreen>(app_));
        return;
    }

    if (widthMinus_.hit(tap)) { adjustWidth(-1); return; }
    if (widthPlus_.hit(tap)) { adjustWidth(1); return; }
    if (heightMinus_.hit(tap)) { adjustHeight(-1); return; }
    if (heightPlus_.hit(tap)) { adjustHeight(1); return; }
    if (minesMinus_.hit(tap)) { adjustMines(-1); return; }
    if (minesPlus_.hit(tap)) { adjustMines(1); return; }
    if (startCustomButton_.hit(tap)) {
        requestStart(core::DifficultyConfig::custom(customWidth_, customHeight_, customMines_));
        return;
    }
}

}  // namespace minesweeper::ui
