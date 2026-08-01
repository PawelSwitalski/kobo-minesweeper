#include "core/counter.h"

#include <stdexcept>

#include "nlohmann/json.hpp"

namespace minesweeper::core {

using nlohmann::json;

std::string Counter::toJson() const {
    return json{{"schemaVersion", 1}, {"count", value_}}.dump();
}

Counter Counter::fromJson(const std::string& text) {
    json j = json::parse(text);  // throws on malformed JSON
    if (!j.is_object() || j.value("schemaVersion", 0) != 1)
        throw std::runtime_error("invalid counter: schemaVersion");
    int64_t count = j.at("count").get<int64_t>();
    if (count < 0) throw std::runtime_error("invalid counter: negative count");

    Counter c;
    c.value_ = static_cast<int32_t>(count);
    return c;
}

}  // namespace minesweeper::core
