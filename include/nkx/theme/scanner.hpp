#pragma once

#include "nkx/core/error.hpp"
#include <filesystem>
#include <optional>
#include <vector>

namespace nkx {

struct ScanResult {
    std::vector<std::filesystem::path> cursor_files;
    std::optional<std::filesystem::path> inf_file;
};

class ThemeScanner {
public:
    static auto scan_directory(const std::filesystem::path& dir) -> Result<ScanResult>;
};

} // namespace nkx
