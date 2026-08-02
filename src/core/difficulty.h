#pragma once

namespace minesweeper::core {

enum class DifficultyPreset { Beginner, Intermediate, Expert, Custom };

struct DifficultyConfig {
    DifficultyPreset preset = DifficultyPreset::Beginner;
    int width = 9;
    int height = 9;
    int mineCount = 10;

    static DifficultyConfig beginner();
    static DifficultyConfig intermediate();
    static DifficultyConfig expert();
    // Caller (NewGameScreen) is expected to already clamp width/height/mineCount via its
    // stepper UI; this is a defensive check, not the primary UX gate.
    static DifficultyConfig custom(int width, int height, int mineCount);

    // Bounds check for Custom configs only: 5 <= width, height <= 16,
    // 1 <= mineCount <= width*height - 9. Named presets are exact, fixed values and are
    // never subject to this check.
    bool isValidCustom() const;

    // Named presets: width/height/mineCount must match the fixed factory values exactly.
    // Custom: delegates to isValidCustom(). Used to validate a deserialized game.json.
    bool isValid() const;
};

}  // namespace minesweeper::core
