#include "core/difficulty.h"

namespace minesweeper::core {

DifficultyConfig DifficultyConfig::beginner() {
    return {DifficultyPreset::Beginner, 9, 9, 10};
}

DifficultyConfig DifficultyConfig::intermediate() {
    return {DifficultyPreset::Intermediate, 16, 16, 40};
}

DifficultyConfig DifficultyConfig::expert() {
    return {DifficultyPreset::Expert, 30, 16, 99};
}

DifficultyConfig DifficultyConfig::custom(int width, int height, int mineCount) {
    return {DifficultyPreset::Custom, width, height, mineCount};
}

bool DifficultyConfig::isValidCustom() const {
    if (preset != DifficultyPreset::Custom) return false;
    if (width < 5 || width > 16) return false;
    if (height < 5 || height > 16) return false;
    int maxMines = width * height - 9;
    if (mineCount < 1 || mineCount > maxMines) return false;
    return true;
}

bool DifficultyConfig::isValid() const {
    switch (preset) {
        case DifficultyPreset::Beginner: {
            DifficultyConfig b = beginner();
            return width == b.width && height == b.height && mineCount == b.mineCount;
        }
        case DifficultyPreset::Intermediate: {
            DifficultyConfig i = intermediate();
            return width == i.width && height == i.height && mineCount == i.mineCount;
        }
        case DifficultyPreset::Expert: {
            DifficultyConfig e = expert();
            return width == e.width && height == e.height && mineCount == e.mineCount;
        }
        case DifficultyPreset::Custom:
            return isValidCustom();
    }
    return false;
}

}  // namespace minesweeper::core
