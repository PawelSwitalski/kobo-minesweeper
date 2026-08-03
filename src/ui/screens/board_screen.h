#pragma once
#include <cstdint>
#include <vector>

#include "core/board_viewport.h"
#include "core/cell.h"
#include "core/gesture_recognizer.h"
#include "platform/renderer.h"
#include "ui/screens/screen.h"
#include "ui/widgets.h"

namespace minesweeper::ui {

// Fit == today's automatic fit-to-screen size; Zoom2x/Zoom3x multiply it (specs/005-board-zoom-pan,
// research.md #1). Triggered by a two-finger pinch, not a button (FR-007).
enum class ZoomLevel { Fit, Zoom2x, Zoom3x };

// The main gameplay screen: grid, HUD (mine count + timer), tap/long-press/flag-mode/chord
// routing, pinch-zoom/drag-pan (specs/005-board-zoom-pan), and the win/loss outcome banner.
class BoardScreen : public Screen {
public:
    explicit BoardScreen(App& app) : Screen(app) {}

    void draw() override;
    void onTap(Tap tap) override;
    void onGesture(core::GestureEvent gesture) override;
    void onTick(uint32_t activeSeconds) override;
    bool countsPlayTime() const override;

private:
    void layout();
    void drawHud();
    void drawMineCount();
    void drawTimer();
    void drawCell(int x, int y);
    void drawOutcomeBanner();
    Rect cellRect(int x, int y) const;
    bool cellVisible(int x, int y) const;
    void afterMutation(const std::vector<core::Cell>& before);

    Rect gridRect_{};
    int baseCellSizePx_ = 0;  // the Fit-level size; cellSizePx_ = baseCellSizePx_ * multiplier
    int cellSizePx_ = 0;
    ZoomLevel zoomLevel_ = ZoomLevel::Fit;
    core::BoardViewport viewport_;
    int panAccumPxX_ = 0, panAccumPxY_ = 0;  // sub-cell drag remainder (research.md #6)
    Rect mineCountRect_{};
    Rect timerRect_{};
    Button flagModeButton_{};
    Button settingsButton_{};
    Button exitButton_{};
    Button returnToMenuButton_{};
    Button outcomeExitButton_{};
    bool flagModeOn_ = false;
    uint32_t lastDisplayedMinute_ = 0xFFFFFFFFu;
};

}  // namespace minesweeper::ui
