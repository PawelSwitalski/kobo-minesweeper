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

std::string screenRefreshIntervalToString(ScreenRefreshInterval interval) {
    switch (interval) {
        case ScreenRefreshInterval::Every5: return "Every5";
        case ScreenRefreshInterval::Every10: return "Every10";
        case ScreenRefreshInterval::Every25: return "Every25";
        case ScreenRefreshInterval::Never: return "Never";
    }
    throw std::runtime_error("invalid settings: screenRefreshInterval");
}

ScreenRefreshInterval screenRefreshIntervalFromString(const std::string& s) {
    if (s == "Every5") return ScreenRefreshInterval::Every5;
    if (s == "Every10") return ScreenRefreshInterval::Every10;
    if (s == "Every25") return ScreenRefreshInterval::Every25;
    if (s == "Never") return ScreenRefreshInterval::Never;
    throw std::runtime_error("invalid settings: screenRefreshInterval");
}
}  // namespace

std::string Settings::toJson() const {
    return json{{"schemaVersion", 1},
                {"colorMode", colorModeToString(colorMode)},
                {"hideTimer", hideTimer},
                {"screenRefreshInterval", screenRefreshIntervalToString(screenRefreshInterval)}}
        .dump();
}

Settings Settings::fromJson(const std::string& text) {
    json j = json::parse(text);  // throws on malformed JSON
    if (!j.is_object() || j.value("schemaVersion", 0) != 1)
        throw std::runtime_error("invalid settings: schemaVersion");

    Settings s;
    s.colorMode = colorModeFromString(j.at("colorMode").get<std::string>());
    // Absent means false: an already-existing settings.json written before this field existed
    // must still load successfully with hideTimer defaulted, not be rejected outright.
    s.hideTimer = j.value("hideTimer", false);
    // Same backward-compatibility rule: absent means the default ("Every10"), not rejection.
    s.screenRefreshInterval =
        screenRefreshIntervalFromString(j.value("screenRefreshInterval", std::string("Every10")));
    return s;
}

}  // namespace minesweeper::core
