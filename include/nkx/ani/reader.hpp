#pragma once

#include "nkx/core/error.hpp"
#include "nkx/core/types.hpp"
#include <filesystem>

namespace nkx {

// Reads a Windows animated cursor (.ani) or static cursor (.cur) file.
// Supports multi-size cursors and correctly handles animation frames and delays.
auto read_cursor_file(const std::filesystem::path& path) -> Result<CursorAnimation>;

} // namespace nkx
