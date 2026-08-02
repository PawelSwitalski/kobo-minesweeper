#pragma once
#include <string>

#include "core/difficulty.h"
#include "ui/screens/screen.h"
#include "ui/widgets.h"

namespace minesweeper::ui {

// Difficulty selection: three presets plus a custom width/height/mine-count stepper section
// (FR-001, FR-002). Guards starting a new game behind an abandon-confirmation dialog when a
// game is already in progress (FR-024).
class NewGameScreen : public Screen {
public:
    explicit NewGameScreen(App& app) : Screen(app) {}

    void draw() override;
    void onTap(Tap tap) override;

private:
    void layout();
    void drawWidthValue();
    void drawHeightValue();
    void drawMinesValue();
    void drawReason();
    void clampCustom();
    // Applies delta to width/height/mines, re-clamps, redraws the affected value(s) and the
    // reason line, and flushes. Shows a reason message (FR-003) whenever the requested change
    // was clamped away from what the player asked for.
    void adjustWidth(int delta);
    void adjustHeight(int delta);
    void adjustMines(int delta);
    void requestStart(core::DifficultyConfig cfg);
    void startGame(core::DifficultyConfig cfg);

    Button beginnerButton_{}, intermediateButton_{}, expertButton_{};
    Button settingsButton_{};

    Button widthMinus_{}, widthPlus_{};
    Button heightMinus_{}, heightPlus_{};
    Button minesMinus_{}, minesPlus_{};
    Rect widthValueRect_{}, heightValueRect_{}, minesValueRect_{};
    Rect reasonRect_{};
    std::string reasonText_;
    Button startCustomButton_{};
    int customWidth_ = 9, customHeight_ = 9, customMines_ = 10;

    Dialog abandonDialog_{};
    bool abandonDialogActive_ = false;
    core::DifficultyConfig pendingConfig_{};
};

}  // namespace minesweeper::ui
