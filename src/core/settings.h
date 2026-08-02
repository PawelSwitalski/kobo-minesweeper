#pragma once
#include <string>

namespace minesweeper::core {

enum class ColorMode { Color, BlackAndWhite };

// Ghosting-cleanup cadence: how many screen updates before an automatic full clearing refresh.
// Maps onto Renderer::setGhostingInterval's existing count contract (5/10/25/0 for Never).
enum class ScreenRefreshInterval { Every5, Every10, Every25, Never };

class Settings {
public:
    ColorMode colorMode = ColorMode::BlackAndWhite;  // default, per clarification
    bool hideTimer = false;  // default off; hides the live board timer, not the outcome time
    ScreenRefreshInterval screenRefreshInterval = ScreenRefreshInterval::Every10;  // default

    std::string toJson() const;
    static Settings fromJson(const std::string& text);  // throws on invalid input
};

}  // namespace minesweeper::core
