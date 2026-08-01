#pragma once
#include <string>

namespace minesweeper::persist {

struct Paths {
    std::string dataDir;
    std::string counter;  // dataDir/counter.json — replace/extend with your own state files
};

// Resolution order: cliOverride > $MINESWEEPER_DATA_DIR > /mnt/onboard/.adds/minesweeper
// (when it exists, i.e. on a Kobo) > ./minesweeper-data. Creates the directory.
Paths resolveDataDir(const char* cliOverride);

}  // namespace minesweeper::persist
