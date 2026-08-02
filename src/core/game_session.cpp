#include "core/game_session.h"

#include <stdexcept>

#include "nlohmann/json.hpp"

namespace minesweeper::core {

using nlohmann::json;

namespace {

std::string presetToString(DifficultyPreset p) {
    switch (p) {
        case DifficultyPreset::Beginner: return "Beginner";
        case DifficultyPreset::Intermediate: return "Intermediate";
        case DifficultyPreset::Expert: return "Expert";
        case DifficultyPreset::Custom: return "Custom";
    }
    throw std::runtime_error("invalid difficulty preset");
}

DifficultyPreset presetFromString(const std::string& s) {
    if (s == "Beginner") return DifficultyPreset::Beginner;
    if (s == "Intermediate") return DifficultyPreset::Intermediate;
    if (s == "Expert") return DifficultyPreset::Expert;
    if (s == "Custom") return DifficultyPreset::Custom;
    throw std::runtime_error("invalid game session: difficulty.preset");
}

std::string statusToString(Board::Status s) {
    switch (s) {
        case Board::Status::NotStarted: return "NotStarted";
        case Board::Status::InProgress: return "InProgress";
        case Board::Status::Won: return "Won";
        case Board::Status::Lost: return "Lost";
    }
    throw std::runtime_error("invalid board status");
}

Board::Status statusFromString(const std::string& s) {
    if (s == "NotStarted") return Board::Status::NotStarted;
    if (s == "InProgress") return Board::Status::InProgress;
    if (s == "Won") return Board::Status::Won;
    if (s == "Lost") return Board::Status::Lost;
    throw std::runtime_error("invalid game session: status");
}

std::string cellStateToString(CellState s) {
    switch (s) {
        case CellState::Unopened: return "Unopened";
        case CellState::Opened: return "Opened";
        case CellState::Flagged: return "Flagged";
    }
    throw std::runtime_error("invalid cell state");
}

CellState cellStateFromString(const std::string& s) {
    if (s == "Unopened") return CellState::Unopened;
    if (s == "Opened") return CellState::Opened;
    if (s == "Flagged") return CellState::Flagged;
    throw std::runtime_error("invalid game session: cell state");
}

}  // namespace

GameSession::GameSession() : GameSession(DifficultyConfig::beginner()) {}

GameSession::GameSession(DifficultyConfig config)
    : config_(config), board_(config.width, config.height, config.mineCount) {}

void GameSession::addActiveSeconds(uint32_t seconds) {
    if (status() == Board::Status::Won || status() == Board::Status::Lost) return;
    elapsedSeconds_ += seconds;
}

std::string GameSession::toJson() const {
    json j;
    j["schemaVersion"] = 1;
    j["difficulty"] = {
        {"preset", presetToString(config_.preset)},
        {"width", config_.width},
        {"height", config_.height},
        {"mineCount", config_.mineCount},
    };
    j["status"] = statusToString(board_.status());
    j["elapsedSeconds"] = elapsedSeconds_;

    json jc = json::array();
    if (board_.status() != Board::Status::NotStarted) {
        for (const Cell& c : board_.cells())
            jc.push_back({{"isMine", c.isMine}, {"state", cellStateToString(c.state)}});
    }
    j["cells"] = jc;

    return j.dump();
}

GameSession GameSession::fromJson(const std::string& text) {
    json j = json::parse(text);  // throws on malformed JSON
    if (!j.is_object() || j.value("schemaVersion", 0) != 1)
        throw std::runtime_error("invalid game session: schemaVersion");

    const json& jd = j.at("difficulty");
    DifficultyConfig config;
    config.preset = presetFromString(jd.at("preset").get<std::string>());
    config.width = jd.at("width").get<int>();
    config.height = jd.at("height").get<int>();
    config.mineCount = jd.at("mineCount").get<int>();
    if (!config.isValid()) throw std::runtime_error("invalid game session: difficulty bounds");

    Board::Status status = statusFromString(j.at("status").get<std::string>());
    uint32_t elapsed = j.at("elapsedSeconds").get<uint32_t>();

    GameSession session(config);

    const json& jc = j.at("cells");
    if (!jc.is_array()) throw std::runtime_error("invalid game session: cells");

    if (status != Board::Status::NotStarted) {
        size_t expected =
            static_cast<size_t>(config.width) * static_cast<size_t>(config.height);
        if (jc.size() != expected) throw std::runtime_error("invalid game session: cells length");

        std::vector<Cell> cells;
        cells.reserve(expected);
        int mineCount = 0;
        for (const json& jcell : jc) {
            Cell c;
            c.isMine = jcell.at("isMine").get<bool>();
            c.state = cellStateFromString(jcell.at("state").get<std::string>());
            if (c.isMine) ++mineCount;
            cells.push_back(c);
        }
        if (mineCount != config.mineCount)
            throw std::runtime_error("invalid game session: mine count mismatch");

        session.board_.loadCells(std::move(cells), status);
    }

    session.elapsedSeconds_ = elapsed;
    return session;
}

}  // namespace minesweeper::core
