#pragma once

#include "nkx/core/error.hpp"
#include "nkx/core/types.hpp"
#include <filesystem>
#include <string>

namespace nkx {

struct BuildOptions {
    std::filesystem::path input_dir;
    std::filesystem::path output_dir;  // auto-generated if empty
    std::string theme_name;            // auto-generated from dir name if empty
    std::string comment;
    std::string author;
    std::string compat_aliases_json;   // content of compat_aliases.json
};

struct BuildResult {
    CursorTheme theme;
    std::filesystem::path output_path;
    uint32_t cursor_count = 0;
    uint32_t extra_count = 0;
    uint32_t symlink_count = 0;
};

auto build_theme(const BuildOptions& opts) -> Result<BuildResult>;

} // namespace nkx
