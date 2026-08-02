#include "ui/screens/board_screen.h"

#include <memory>
#include <string>

#include "ui/app.h"
#include "ui/screens/settings_screen.h"

namespace minesweeper::ui {

void BoardScreen::layout() {
    const Theme& t = app_.theme();
    DisplayInfo d = app_.renderer().info();
    const core::Board& board = app_.session().board();

    int hudY = t.pad;
    int hudH = t.touchTargetPx;
    mineCountRect_ = {t.pad, hudY, d.width / 2 - t.pad, hudH};
    timerRect_ = {d.width / 2, hudY, d.width / 2 - t.pad, hudH};

    int navY = hudY + hudH + t.gap;
    int btnW = (d.width - 4 * t.pad) / 3;
    flagModeButton_.rect = {t.pad, navY, btnW, t.touchTargetPx};
    flagModeButton_.label = "Flag Mode";
    settingsButton_.rect = {t.pad * 2 + btnW, navY, btnW, t.touchTargetPx};
    settingsButton_.label = "Settings";
    exitButton_.rect = {t.pad * 3 + btnW * 2, navY, btnW, t.touchTargetPx};
    exitButton_.label = "Exit";

    int gridTop = navY + t.touchTargetPx + t.gap;
    int availW = d.width - 2 * t.pad;
    int availH = d.height - gridTop - t.pad;
    int cw = board.width() > 0 ? availW / board.width() : availW;
    int ch = board.height() > 0 ? availH / board.height() : availH;
    cellSizePx_ = cw < ch ? cw : ch;
    if (cellSizePx_ < 1) cellSizePx_ = 1;

    int gridW = cellSizePx_ * board.width();
    int gridH = cellSizePx_ * board.height();
    gridRect_ = {(d.width - gridW) / 2, gridTop, gridW, gridH};
}

Rect BoardScreen::cellRect(int x, int y) const {
    return {gridRect_.x + x * cellSizePx_, gridRect_.y + y * cellSizePx_, cellSizePx_,
            cellSizePx_};
}

void BoardScreen::drawMineCount() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(mineCountRect_, Gray::White);
    Label mineLabel;
    mineLabel.rect = mineCountRect_;
    mineLabel.text = "Mines: " + std::to_string(app_.session().board().remainingMineCount());
    mineLabel.align = TextStyle::Align::Left;
    mineLabel.draw(r, t);
}

void BoardScreen::drawTimer() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    r.fillRect(timerRect_, Gray::White);
    if (app_.settings().hideTimer) return;  // FR-007: blank slot, no elapsed-time text
    Label timerLabel;
    timerLabel.rect = timerRect_;
    timerLabel.text = formatTime(app_.session().elapsedSeconds());
    timerLabel.align = TextStyle::Align::Right;
    timerLabel.draw(r, t);
}

void BoardScreen::drawHud() {
    drawMineCount();
    drawTimer();
}

void BoardScreen::drawCell(int x, int y) {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    const core::Cell& c = app_.session().board().cellAt(x, y);
    Rect rect = cellRect(x, y);

    Gray bg = Gray::White;
    std::string glyph;
    Gray glyphShade = Gray::Black;
    Color accent = Color::None;

    switch (c.state) {
        case core::CellState::Unopened:
            bg = Gray::Mid;
            break;
        case core::CellState::Flagged:
            bg = Gray::Mid;
            glyph = "F";
            break;
        case core::CellState::Opened:
            if (c.isMine) {
                bg = Gray::Black;
                glyph = "*";
                glyphShade = Gray::White;
            } else if (c.adjacentMines > 0) {
                glyph = std::to_string(c.adjacentMines);
                // Per-count color in Color mode only (accent/shade, never the sole meaning --
                // the printed digit itself is unchanged either way). See
                // specs/002-fix-menu-exit-colors/contracts/digit-color-mapping.md.
                if (app_.theme().color) {
                    switch (c.adjacentMines) {
                        case 1: accent = Color::Blue; break;
                        case 2: accent = Color::Green; break;
                        case 3: accent = Color::Red; break;
                        case 4: accent = Color::Navy; break;
                        case 5: accent = Color::Crimson; break;
                        case 6: accent = Color::Cyan; break;
                        case 8: glyphShade = Gray::Mid; break;
                        default: break;  // 7 stays plain black (Color::None, Gray::Black)
                    }
                }
            }
            break;
    }

    r.fillRect(rect, bg);
    // Thin cell border so adjacent same-shade cells stay visually distinct.
    r.fillRect({rect.x, rect.y, rect.w, t.thinLine}, Gray::Black);
    r.fillRect({rect.x, rect.y, t.thinLine, rect.h}, Gray::Black);

    if (!glyph.empty()) {
        TextStyle st;
        st.sizePx = cellSizePx_ * 6 / 10;
        if (st.sizePx < 6) st.sizePx = 6;
        st.shade = glyphShade;
        st.accent = accent;
        st.bold = true;
        r.drawText(rect, glyph, st);
    }
}

void BoardScreen::drawOutcomeBanner() {
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    DisplayInfo d = r.info();
    const core::GameSession& session = app_.session();

    bool won = session.status() == core::Board::Status::Won;
    int bannerH = t.pad * 2 + t.titlePx + t.textPx + t.gap + t.touchTargetPx;
    Rect banner = {t.pad, gridRect_.y + gridRect_.h / 2 - bannerH / 2,
                   d.width - 2 * t.pad, bannerH};
    r.fillRect(banner, Gray::White);
    Gray frameShade = Gray::Black;
    r.fillRect({banner.x, banner.y, banner.w, t.thickLine}, frameShade);
    r.fillRect({banner.x, banner.y + banner.h - t.thickLine, banner.w, t.thickLine}, frameShade);
    r.fillRect({banner.x, banner.y, t.thickLine, banner.h}, frameShade);
    r.fillRect({banner.x + banner.w - t.thickLine, banner.y, t.thickLine, banner.h}, frameShade);

    Label outcome;
    outcome.rect = {banner.x, banner.y + t.pad, banner.w, t.titlePx};
    outcome.text = won ? "You Win!" : "Game Over";
    outcome.bold = true;
    outcome.sizePx = t.titlePx;
    outcome.draw(r, t);

    Label detail;
    detail.rect = {banner.x, banner.y + t.pad + t.titlePx, banner.w, t.textPx};
    detail.text = formatTime(session.elapsedSeconds());
    detail.sizePx = t.textPx;
    detail.draw(r, t);

    int buttonY = banner.y + t.pad + t.titlePx + t.textPx + t.gap;
    int btnW = (banner.w - 3 * t.pad) / 2;
    returnToMenuButton_.rect = {banner.x + t.pad, buttonY, btnW, t.touchTargetPx};
    returnToMenuButton_.label = "Return to Menu";
    outcomeExitButton_.rect = {banner.x + t.pad * 2 + btnW, buttonY, btnW, t.touchTargetPx};
    outcomeExitButton_.label = "Exit";
    returnToMenuButton_.draw(r, t);
    outcomeExitButton_.draw(r, t);
}

void BoardScreen::draw() {
    layout();
    Renderer& r = app_.renderer();
    const Theme& t = app_.theme();
    DisplayInfo d = r.info();

    r.fillRect({0, 0, d.width, d.height}, Gray::White);

    drawHud();
    flagModeButton_.toggled = flagModeOn_;
    flagModeButton_.draw(r, t);
    settingsButton_.draw(r, t);
    exitButton_.draw(r, t);

    const core::Board& board = app_.session().board();
    for (int y = 0; y < board.height(); ++y)
        for (int x = 0; x < board.width(); ++x) drawCell(x, y);

    if (app_.session().status() == core::Board::Status::Won ||
        app_.session().status() == core::Board::Status::Lost)
        drawOutcomeBanner();
}

void BoardScreen::afterMutation(const std::vector<core::Cell>& before) {
    app_.autosaveSession();

    const core::Board& board = app_.session().board();
    const std::vector<core::Cell>& after = board.cells();
    Rect dirty{};
    for (size_t i = 0; i < after.size(); ++i) {
        if (after[i].state != before[i].state) {
            int x = static_cast<int>(i) % board.width();
            int y = static_cast<int>(i) / board.width();
            drawCell(x, y);
            dirty = dirty.unite(cellRect(x, y));
        }
    }

    drawMineCount();

    core::Board::Status status = board.status();
    if (status == core::Board::Status::Won || status == core::Board::Status::Lost) {
        drawOutcomeBanner();
        app_.renderer().flushFull();
    } else if (dirty.w > 0 && dirty.h > 0) {
        app_.renderer().flushPartial(dirty.unite(mineCountRect_));
    }
}

void BoardScreen::onTap(Tap tap) {
    if (flagModeButton_.hit(tap)) {
        flagModeOn_ = !flagModeOn_;
        flagModeButton_.toggled = flagModeOn_;
        flagModeButton_.draw(app_.renderer(), app_.theme());
        app_.renderer().flushPartial(flagModeButton_.rect);
        return;
    }
    if (settingsButton_.hit(tap)) {
        app_.push(std::make_unique<SettingsScreen>(app_));
        return;
    }
    if (exitButton_.hit(tap)) {
        app_.requestExit();
        return;
    }

    core::Board::Status status = app_.session().status();
    if (status == core::Board::Status::Won || status == core::Board::Status::Lost) {
        if (returnToMenuButton_.hit(tap)) { app_.returnToMainMenu(); return; }
        if (outcomeExitButton_.hit(tap)) { app_.requestExit(); return; }
        return;  // board actions are inert once the game has ended (FR-018)
    }

    if (!gridRect_.contains({tap.x, tap.y})) return;
    const core::Board& board = app_.session().board();
    int cx = (tap.x - gridRect_.x) / cellSizePx_;
    int cy = (tap.y - gridRect_.y) / cellSizePx_;
    if (cx < 0 || cx >= board.width() || cy < 0 || cy >= board.height()) return;

    std::vector<core::Cell> before = board.cells();
    const core::Cell& cell = board.cellAt(cx, cy);

    if (cell.state == core::CellState::Opened) {
        if (cell.adjacentMines <= 0) return;  // nothing to chord on a blank opened cell
        app_.session().chord(cx, cy);
    } else if (tap.longPress) {
        app_.session().toggleFlag(cx, cy);  // long-press always flags/unflags (FR-009)
    } else if (flagModeOn_) {
        app_.session().toggleFlag(cx, cy);
    } else if (cell.state == core::CellState::Flagged) {
        return;  // flagged cells never open via a plain tap
    } else {
        app_.session().openCell(cx, cy);
    }

    afterMutation(before);
}

void BoardScreen::onTick(uint32_t activeSeconds) {
    app_.session().addActiveSeconds(activeSeconds);
    app_.autosaveSession();

    if (app_.settings().hideTimer) return;  // nothing on screen to update (FR-011: tracking above
                                             // stays unconditional; only the redraw is skipped)
    uint32_t minute = app_.session().elapsedSeconds() / 60;
    if (minute != lastDisplayedMinute_) {
        lastDisplayedMinute_ = minute;
        drawTimer();
        app_.renderer().flushPartial(timerRect_);
    }
}

bool BoardScreen::countsPlayTime() const {
    return app_.session().status() == core::Board::Status::InProgress;
}

}  // namespace minesweeper::ui
