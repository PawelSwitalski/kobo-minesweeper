#pragma once
#include <string>

namespace minesweeper::persist {

struct Paths {
    std::string dataDir;
    std::string game;      // dataDir/game.json
    std::string settings;  // dataDir/settings.json
};

// Resolution order: cliOverride > $MINESWEEPER_DATA_DIR > /mnt/onboard/.adds/minesweeper
// (when it exists, i.e. on a Kobo) > ./minesweeper-data. Creates the directory.
Paths resolveDataDir(const char* cliOverride);

}  // namespace minesweeper::persist
