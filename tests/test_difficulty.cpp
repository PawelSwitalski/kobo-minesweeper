#include "doctest/doctest.h"

#include "core/difficulty.h"

using namespace minesweeper::core;

TEST_CASE("preset factory values match spec FR-001") {
    DifficultyConfig b = DifficultyConfig::beginner();
    CHECK(b.preset == DifficultyPreset::Beginner);
    CHECK(b.width == 9);
    CHECK(b.height == 9);
    CHECK(b.mineCount == 10);

    DifficultyConfig i = DifficultyConfig::intermediate();
    CHECK(i.preset == DifficultyPreset::Intermediate);
    CHECK(i.width == 16);
    CHECK(i.height == 16);
    CHECK(i.mineCount == 40);

    DifficultyConfig e = DifficultyConfig::expert();
    CHECK(e.preset == DifficultyPreset::Expert);
    CHECK(e.width == 30);
    CHECK(e.height == 16);
    CHECK(e.mineCount == 99);
}

TEST_CASE("custom bounds accept the full 5-16 / 1..(w*h-9) range") {
    CHECK(DifficultyConfig::custom(5, 5, 1).isValidCustom());
    CHECK(DifficultyConfig::custom(16, 16, 16 * 16 - 9).isValidCustom());
    CHECK(DifficultyConfig::custom(10, 10, 50).isValidCustom());
}

TEST_CASE("custom bounds reject values outside 5-16 or the mine-count ceiling") {
    CHECK(!DifficultyConfig::custom(4, 9, 1).isValidCustom());               // width too small
    CHECK(!DifficultyConfig::custom(17, 9, 1).isValidCustom());              // width too large
    CHECK(!DifficultyConfig::custom(9, 4, 1).isValidCustom());               // height too small
    CHECK(!DifficultyConfig::custom(9, 17, 1).isValidCustom());              // height too large
    CHECK(!DifficultyConfig::custom(5, 5, 0).isValidCustom());               // mineCount too low
    CHECK(!DifficultyConfig::custom(5, 5, 5 * 5 - 9 + 1).isValidCustom());   // mineCount too high
}

TEST_CASE("named presets are never subject to the custom-bounds check, but are valid overall") {
    CHECK(!DifficultyConfig::beginner().isValidCustom());
    CHECK(DifficultyConfig::beginner().isValid());
    CHECK(DifficultyConfig::intermediate().isValid());
    CHECK(DifficultyConfig::expert().isValid());
}
