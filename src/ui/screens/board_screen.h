#pragma once
#include <cstdint>
#include <vector>

#include "core/cell.h"
#include "platform/renderer.h"
#include "ui/screens/screen.h"
#include "ui/widgets.h"

namespace minesweeper::ui {

// The main gameplay screen: grid, HUD (mine count + timer), tap/long-press/flag-mode/chord
// routing, and the win/loss outcome banner.
class BoardScreen : public Screen {
public:
    explicit BoardScreen(App& app) : Screen(app) {}

    void draw() override;
    void onTap(Tap tap) override;
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
    void afterMutation(const std::vector<core::Cell>& before);

    Rect gridRect_{};
    int cellSizePx_ = 0;
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
