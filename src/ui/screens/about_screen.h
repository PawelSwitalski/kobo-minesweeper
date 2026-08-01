#pragma once
#include "ui/screens/screen.h"
#include "ui/widgets.h"

namespace minesweeper::ui {

// Trivial second screen; exists only to exercise App::push/pop navigation.
class AboutScreen : public Screen {
public:
    explicit AboutScreen(App& app) : Screen(app) {}

    void draw() override;
    void onTap(Tap tap) override;

private:
    Button backButton_{};
};

}  // namespace minesweeper::ui
