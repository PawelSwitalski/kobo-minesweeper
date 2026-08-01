#include "ui/theme.h"

namespace minesweeper::ui {

Theme makeTheme(const DisplayInfo& d, const std::string& assetsDir) {
    Theme t;
    t.dpi = d.dpi > 0 ? d.dpi : 300;
    t.color = d.color;

    t.touchTargetPx = t.mm(9.0);  // FR-015 / SC-003 touch-target floor
    t.pad = t.mm(2.0);
    t.gap = t.mm(1.2);

    t.titlePx = t.mm(7.0);
    t.textPx = t.mm(4.2);
    t.smallPx = t.mm(3.2);

    t.thinLine = t.mm(0.25);
    t.thickLine = t.mm(0.7);

    t.fontPath = assetsDir + "/DejaVuSans.ttf";
    t.fontBoldPath = assetsDir + "/DejaVuSans-Bold.ttf";
    return t;
}

}  // namespace minesweeper::ui
