#include "core/settings.h"

#include <stdexcept>

#include "nlohmann/json.hpp"

namespace minesweeper::core {

using nlohmann::json;

namespace {
std::string colorModeToString(ColorMode m) {
    return m == ColorMode::Color ? "Color" : "BlackAndWhite";
}

ColorMode colorModeFromString(const std::string& s) {
    if (s == "Color") return ColorMode::Color;
    if (s == "BlackAndWhite") return ColorMode::BlackAndWhite;
    throw std::runtime_error("invalid settings: colorMode");
}
}  // namespace

std::string Settings::toJson() const {
    return json{{"schemaVersion", 1}, {"colorMode", colorModeToString(colorMode)}}.dump();
}

Settings Settings::fromJson(const std::string& text) {
    json j = json::parse(text);  // throws on malformed JSON
    if (!j.is_object() || j.value("schemaVersion", 0) != 1)
        throw std::runtime_error("invalid settings: schemaVersion");

    Settings s;
    s.colorMode = colorModeFromString(j.at("colorMode").get<std::string>());
    return s;
}

}  // namespace minesweeper::core
