#include "persist/store.h"

#include <cstdio>
#include <filesystem>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace minesweeper::persist {

namespace fs = std::filesystem;

std::optional<std::string> loadFile(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return std::nullopt;
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0)
        out.append(buf, n);
    bool ok = !std::ferror(f);
    std::fclose(f);
    if (!ok) return std::nullopt;
    return out;
}

bool saveFileAtomic(const std::string& path, const std::string& contents) {
    const std::string tmp = path + ".tmp";
    std::FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;

    bool ok = std::fwrite(contents.data(), 1, contents.size(), f) == contents.size();
    ok = ok && std::fflush(f) == 0;
    if (ok) {
#ifdef _WIN32
        ok = _commit(_fileno(f)) == 0;
#else
        ok = fsync(fileno(f)) == 0;
#endif
    }
    ok = (std::fclose(f) == 0) && ok;

    std::error_code ec;
    if (ok) {
        fs::rename(tmp, path, ec);  // replaces existing target on POSIX and MSVC
        ok = !ec;
    }
    if (!ok) fs::remove(tmp, ec);
    return ok;
}

void removeFile(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
}

}  // namespace minesweeper::persist
