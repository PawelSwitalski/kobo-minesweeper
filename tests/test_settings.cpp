#include "doctest/doctest.h"

#include "core/settings.h"

using namespace minesweeper::core;

TEST_CASE("Settings defaults to BlackAndWhite") {
    Settings s;
    CHECK(s.colorMode == ColorMode::BlackAndWhite);
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

TEST_CASE("malformed or invalid settings JSON is rejected") {
    CHECK_THROWS_AS(Settings::fromJson(""), std::exception);
    CHECK_THROWS_AS(Settings::fromJson("{"), std::exception);
    CHECK_THROWS_AS(Settings::fromJson("[]"), std::exception);
    CHECK_THROWS_AS(Settings::fromJson(R"({"schemaVersion":2,"colorMode":"Color"})"), std::exception);
    CHECK_THROWS_AS(Settings::fromJson(R"({"schemaVersion":1,"colorMode":"Purple"})"), std::exception);
}
