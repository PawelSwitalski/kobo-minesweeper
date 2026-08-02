#pragma once
#include "ui/screens/screen.h"
#include "ui/widgets.h"

namespace minesweeper::ui {

// Color/Black-and-white switch. Always shown and toggleable regardless of hardware color
// capability (FR-021); Theme::color composes this with DisplayInfo::color (research.md #7).
class SettingsScreen : public Screen {
public:
    explicit SettingsScreen(App& app) : Screen(app) {}

    void draw() override;
    void onTap(Tap tap) override;

private:
    void layout();

    Button colorButton_{};
    Button blackWhiteButton_{};
    Button hideTimerButton_{};
    Button backButton_{};
};

}  // namespace minesweeper::ui
