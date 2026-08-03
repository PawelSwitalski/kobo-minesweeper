#include "ui/screens/board_screen.h"

#include <algorithm>
#include <memory>
#include <string>

#include "ui/app.h"
#include "ui/screens/settings_screen.h"

namespace minesweeper::ui {

namespace {
int zoomMultiplier(ZoomLevel z) {
    switch (z) {
        case ZoomLevel::Fit: return 1;
        case ZoomLevel::Zoom2x: return 2;
        case ZoomLevel::Zoom3x: return 3;
    }
    return 1;
}
}  // namespace

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

    // No reserved zoom/pan button rows in the gesture-based design (FR-007) -- the grid keeps the
    // full space below the nav row, exactly as before 005 (specs/005-board-zoom-pan,
    // contracts/board-screen-integration.md).
    int gridTop = navY + t.touchTargetPx + t.gap;
    int availW = d.width - 2 * t.pad;
    int availH = d.height - gridTop - t.pad;
    int cw = board.width() > 0 ? availW / board.width() : availW;
    int ch = board.height() > 0 ? availH / board.height() : availH;
    baseCellSizePx_ = cw < ch ? cw : ch;
    if (baseCellSizePx_ < 1) baseCellSizePx_ = 1;
    cellSizePx_ = baseCellSizePx_ * zoomMultiplier(zoomLevel_);

    int visibleCols = zoomLevel_ == ZoomLevel::Fit
                           ? board.width()
                           : std::min(board.width(), std::max(1, availW / cellSizePx_));
    int visibleRows = zoomLevel_ == ZoomLevel::Fit
                           ? board.height()
                           : std::min(board.height(), std::max(1, availH / cellSizePx_));

    viewport_.configure(board.width(), board.height());
    viewport_.setVisibleSize(visibleCols, visibleRows);

    int gridW = cellSizePx_ * viewport_.visibleCols();
    int gridH = cellSizePx_ * viewport_.visibleRows();
    gridRect_ = {(d.width - gridW) / 2, gridTop, gridW, gridH};
}

Rect BoardScreen::cellRect(int x, int y) const {
    return {gridRect_.x + (x - viewport_.panX()) * cellSizePx_,
            gridRect_.y + (y - viewport_.panY()) * cellSizePx_, cellSizePx_, cellSizePx_};
}

bool BoardScreen::cellVisible(int x, int y) const {
    return x >= viewport_.panX() && x < viewport_.panX() + viewport_.visibleCols() &&
           y >= viewport_.panY() && y < viewport_.panY() + viewport_.visibleRows();
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

    for (int y = viewport_.panY(); y < viewport_.panY() + viewport_.visibleRows(); ++y)
        for (int x = viewport_.panX(); x < viewport_.panX() + viewport_.visibleCols(); ++x)
            drawCell(x, y);

    if (app_.session().status() == core::Board::Status::Won ||
        app_.session().status() == core::Board::Status::Lost)
        drawOutcomeBanner();
}

void BoardScreen::afterMutation(const std::vector<core::Cell>& before) {
    app_.autosaveSession();

    const core::Board& board = app_.session().board();
    const std::vector<core::Cell>& after = board.cells();
    Rect dirty{};
    bool anyChanged = false;
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (size_t i = 0; i < after.size(); ++i) {
        if (after[i].state != before[i].state) {
            int x = static_cast<int>(i) % board.width();
            int y = static_cast<int>(i) / board.width();
            if (!anyChanged) {
                minX = maxX = x;
                minY = maxY = y;
            } else {
                minX = std::min(minX, x); maxX = std::max(maxX, x);
                minY = std::min(minY, y); maxY = std::max(maxY, y);
            }
            anyChanged = true;
            if (cellVisible(x, y)) {
                drawCell(x, y);
                dirty = dirty.unite(cellRect(x, y));
            }
        }
    }

    // FR-006a: a cascade (or chord) that reveals cells beyond the current viewport auto-recenters
    // rather than leaving the player to discover them manually (research.md #5).
    if (anyChanged && viewport_.recenterOn(minX, minY, maxX, maxY)) {
        draw();
        app_.renderer().flushFull();
        return;
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
    int cx = viewport_.panX() + (tap.x - gridRect_.x) / cellSizePx_;
    int cy = viewport_.panY() + (tap.y - gridRect_.y) / cellSizePx_;
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

void BoardScreen::onGesture(core::GestureEvent gesture) {
    if (gesture.kind == core::GestureKind::ZoomStep) {
        ZoomLevel next = zoomLevel_;
        if (gesture.zoomDelta > 0) {  // pinch-out: zoom in one step (US1)
            if (zoomLevel_ == ZoomLevel::Fit) next = ZoomLevel::Zoom2x;
            else if (zoomLevel_ == ZoomLevel::Zoom2x) next = ZoomLevel::Zoom3x;
            // already Zoom3x: next stays Zoom3x (clamped)
        } else if (gesture.zoomDelta < 0) {  // pinch-in: zoom out one step (US2)
            if (zoomLevel_ == ZoomLevel::Zoom3x) next = ZoomLevel::Zoom2x;
            else if (zoomLevel_ == ZoomLevel::Zoom2x) next = ZoomLevel::Fit;
            // already Fit: next stays Fit (clamped)
        }
        // A no-op step (already at the clamped end) does nothing further -- "further pinch has no
        // additional effect" (US1 Scenario 3 / US2 Scenario 2) with no extra guard state.
        if (next == zoomLevel_) return;
        zoomLevel_ = next;
        panAccumPxX_ = panAccumPxY_ = 0;  // stale remainder from the old cell size
        layout();
        draw();
        app_.renderer().flushFull();
        return;
    }

    // PanStep (US3): proportional drag-to-pan, quantized to whole cells (research.md #6).
    if (viewport_.visibleCols() == app_.session().board().width() &&
        viewport_.visibleRows() == app_.session().board().height()) {
        return;  // nothing to pan to (FR-009) -- board already fully visible
    }
    panAccumPxX_ += gesture.dxPx;
    panAccumPxY_ += gesture.dyPx;
    int dCols = panAccumPxX_ / cellSizePx_;  // truncates toward zero; sign-correct for either drag
    int dRows = panAccumPxY_ / cellSizePx_;  // direction
    if (dCols == 0 && dRows == 0) return;    // hasn't accumulated a whole cell yet
    panAccumPxX_ -= dCols * cellSizePx_;
    panAccumPxY_ -= dRows * cellSizePx_;
    // Dragging right (+dxPx) reveals cells to the right, i.e. the viewport moves left -- the
    // standard drag-to-pan/scroll convention (content follows the finger).
    viewport_.panBy(-dCols, -dRows);
    draw();
    app_.renderer().flushFull();
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
