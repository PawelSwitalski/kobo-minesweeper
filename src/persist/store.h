#pragma once
#include <optional>
#include <string>

namespace minesweeper::persist {

// Whole-file read. Missing/unreadable file => nullopt (never throws).
std::optional<std::string> loadFile(const std::string& path);

// Atomic write: <path>.tmp -> fsync -> rename over <path> (SC-005,
// Constitution V). Returns false on any I/O failure; never leaves a partially
// written <path> behind.
bool saveFileAtomic(const std::string& path, const std::string& contents);

// Deletes the file if present (used when a save is corrupt or reset).
void removeFile(const std::string& path);

}  // namespace minesweeper::persist
