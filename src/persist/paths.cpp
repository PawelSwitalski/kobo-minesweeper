#include "persist/paths.h"

#include <cstdlib>
#include <filesystem>

namespace minesweeper::persist {

namespace fs = std::filesystem;

Paths resolveDataDir(const char* cliOverride) {
    std::string dir;
    if (cliOverride && *cliOverride) {
        dir = cliOverride;
    } else if (const char* env = std::getenv("MINESWEEPER_DATA_DIR"); env && *env) {
        dir = env;
    } else {
        std::error_code ec;
        const char* kobo = "/mnt/onboard/.adds/minesweeper";
        dir = fs::is_directory(kobo, ec) ? kobo : "minesweeper-data";
    }
    std::error_code ec;
    fs::create_directories(dir, ec);  // best effort; saves will fail loudly if unusable

    Paths p;
    p.dataDir = dir;
    p.game = (fs::path(dir) / "game.json").string();
    p.settings = (fs::path(dir) / "settings.json").string();
    return p;
}

}  // namespace minesweeper::persist
