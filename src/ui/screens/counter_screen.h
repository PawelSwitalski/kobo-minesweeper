#pragma once
#include "platform/renderer.h"
#include "ui/screens/screen.h"
#include "ui/widgets.h"

namespace minesweeper::ui {

// Default/home screen of the placeholder demo. Shows the persisted count and
// a big tap-to-increment button, plus a way to reach AboutScreen — enough to
// exercise core (Counter), persist (autosave), platform (partial-refresh
// draw), and ui (navigation) end to end. Replace with your own screens.
class CounterScreen : public Screen {
public:
    explicit CounterScreen(App& app) : Screen(app) {}

    void draw() override;
    void onTap(Tap tap) override;

private:
    void layout();
    void drawCount();

    Rect countRect_{};
    Button incrementButton_{};
    Button aboutButton_{};
};

}  // namespace minesweeper::ui
