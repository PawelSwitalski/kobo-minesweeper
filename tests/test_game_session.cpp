#include "doctest/doctest.h"

#include "core/game_session.h"
#include "nlohmann/json.hpp"

using namespace minesweeper::core;
using nlohmann::json;

namespace {
json validNotStarted() {
    return json{
        {"schemaVersion", 1},
        {"difficulty", {{"preset", "Beginner"}, {"width", 9}, {"height", 9}, {"mineCount", 10}}},
        {"status", "NotStarted"},
        {"elapsedSeconds", 0},
        {"cells", json::array()},
    };
}

json validInProgress5x5() {
    json cells = json::array();
    for (int i = 0; i < 25; ++i) cells.push_back({{"isMine", i == 0}, {"state", "Unopened"}});
    return json{
        {"schemaVersion", 1},
        {"difficulty", {{"preset", "Custom"}, {"width", 5}, {"height", 5}, {"mineCount", 1}}},
        {"status", "InProgress"},
        {"elapsedSeconds", 5},
        {"cells", cells},
    };
}
}  // namespace

TEST_CASE("GameSession JSON round-trip is lossless for a NotStarted session") {
    GameSession a;
    GameSession b = GameSession::fromJson(a.toJson());
    CHECK(b.status() == Board::Status::NotStarted);
    CHECK(b.status() == a.status());
    CHECK(b.config().width == a.config().width);
    CHECK(b.config().height == a.config().height);
    CHECK(b.config().mineCount == a.config().mineCount);
    CHECK(b.elapsedSeconds() == a.elapsedSeconds());
    // A NotStarted Board pre-allocates its cell array (all default/Unopened) regardless of
    // whether mines have been placed yet; only the persisted JSON's "cells" is empty/omitted
    // for NotStarted (contracts/persistence-schema.md). Round-trip should preserve that shape.
    CHECK(b.board().cells().size() == a.board().cells().size());
}

TEST_CASE("GameSession JSON round-trip is lossless for an in-progress session") {
    GameSession a(DifficultyConfig::beginner());
    a.openCell(0, 0);
    a.toggleFlag(a.config().width - 1, a.config().height - 1);
    a.addActiveSeconds(42);

    GameSession b = GameSession::fromJson(a.toJson());
    CHECK(b.status() == a.status());
    CHECK(b.elapsedSeconds() == a.elapsedSeconds());
    CHECK(b.config().preset == a.config().preset);
    REQUIRE(b.board().cells().size() == a.board().cells().size());
    for (size_t i = 0; i < a.board().cells().size(); ++i) {
        CHECK(b.board().cells()[i].isMine == a.board().cells()[i].isMine);
        CHECK(b.board().cells()[i].state == a.board().cells()[i].state);
        CHECK(b.board().cells()[i].adjacentMines == a.board().cells()[i].adjacentMines);
    }
}

TEST_CASE("addActiveSeconds is a no-op once the game has ended") {
    json lost = validInProgress5x5();
    lost["status"] = "Lost";
    lost["cells"][0]["state"] = "Opened";  // the mine, opened
    lost["elapsedSeconds"] = 10;

    GameSession session = GameSession::fromJson(lost.dump());
    REQUIRE(session.status() == Board::Status::Lost);
    REQUIRE(session.elapsedSeconds() == 10);

    session.addActiveSeconds(5);
    CHECK(session.elapsedSeconds() == 10);  // unchanged: elapsed time freezes at game end
}

TEST_CASE("malformed or invalid game session JSON is rejected") {
    CHECK_THROWS_AS(GameSession::fromJson(""), std::exception);
    CHECK_THROWS_AS(GameSession::fromJson("{"), std::exception);
    CHECK_THROWS_AS(GameSession::fromJson("[]"), std::exception);

    json badVersion = validNotStarted();
    badVersion["schemaVersion"] = 2;
    CHECK_THROWS_AS(GameSession::fromJson(badVersion.dump()), std::exception);

    json badStatus = validNotStarted();
    badStatus["status"] = "Sideways";
    CHECK_THROWS_AS(GameSession::fromJson(badStatus.dump()), std::exception);

    json badDifficulty = validNotStarted();
    badDifficulty["difficulty"]["preset"] = "Custom";
    badDifficulty["difficulty"]["width"] = 2;  // below the 5-16 custom bound
    CHECK_THROWS_AS(GameSession::fromJson(badDifficulty.dump()), std::exception);

    json lengthMismatch = validInProgress5x5();
    lengthMismatch["cells"].erase(lengthMismatch["cells"].size() - 1);
    CHECK_THROWS_AS(GameSession::fromJson(lengthMismatch.dump()), std::exception);

    json badCellState = validInProgress5x5();
    badCellState["cells"][0]["state"] = "Exploded";
    CHECK_THROWS_AS(GameSession::fromJson(badCellState.dump()), std::exception);

    json mineMismatch = validInProgress5x5();
    mineMismatch["cells"][1]["isMine"] = true;  // now 2 mines but difficulty says 1
    CHECK_THROWS_AS(GameSession::fromJson(mineMismatch.dump()), std::exception);
}
