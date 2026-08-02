#include "doctest/doctest.h"

#include "core/settings.h"

using namespace minesweeper::core;

TEST_CASE("Settings defaults to BlackAndWhite") {
    Settings s;
    CHECK(s.colorMode == ColorMode::BlackAndWhite);
}

TEST_CASE("Settings defaults hideTimer to false") {
    Settings s;
    CHECK(s.hideTimer == false);
}

TEST_CASE("Settings JSON round-trip is lossless") {
    Settings a;
    a.colorMode = ColorMode::Color;
    Settings b = Settings::fromJson(a.toJson());
    CHECK(b.colorMode == a.colorMode);

    Settings c;
    c.colorMode = ColorMode::BlackAndWhite;
    Settings d = Settings::fromJson(c.toJson());
    CHECK(d.colorMode == c.colorMode);
}

TEST_CASE("hideTimer JSON round-trip is lossless for both values") {
    Settings a;
    a.hideTimer = true;
    Settings b = Settings::fromJson(a.toJson());
    CHECK(b.hideTimer == true);

    Settings c;
    c.hideTimer = false;
    Settings d = Settings::fromJson(c.toJson());
    CHECK(d.hideTimer == false);
}

TEST_CASE("settings.json without a hideTimer key defaults it to false (pre-existing install)") {
    // Simulates a settings.json written before this field existed.
    std::string oldFile = R"({"schemaVersion":1,"colorMode":"Color"})";
    Settings s = Settings::fromJson(oldFile);
    CHECK(s.hideTimer == false);
    CHECK(s.colorMode == ColorMode::Color);  // pre-existing field still loads correctly
}

TEST_CASE("malformed or invalid settings JSON is rejected") {
    CHECK_THROWS_AS(Settings::fromJson(""), std::exception);
    CHECK_THROWS_AS(Settings::fromJson("{"), std::exception);
    CHECK_THROWS_AS(Settings::fromJson("[]"), std::exception);
    CHECK_THROWS_AS(Settings::fromJson(R"({"schemaVersion":2,"colorMode":"Color"})"), std::exception);
    CHECK_THROWS_AS(Settings::fromJson(R"({"schemaVersion":1,"colorMode":"Purple"})"), std::exception);
}
