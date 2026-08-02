#pragma once
#include <string>

namespace minesweeper::core {

enum class ColorMode { Color, BlackAndWhite };

class Settings {
public:
    ColorMode colorMode = ColorMode::BlackAndWhite;  // default, per clarification

    std::string toJson() const;
    static Settings fromJson(const std::string& text);  // throws on invalid input
};

}  // namespace minesweeper::core
