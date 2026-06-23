#pragma once

#include "nkx/core/error.hpp"
#include <filesystem>
#include <string>

namespace nkx {

struct InstallOptions {
    std::filesystem::path theme_dir; // The already-built theme directory
    std::string theme_name;
};

auto install_theme(const InstallOptions& opts) -> Result<std::filesystem::path>;

// Check system readiness
struct DoctorResult {
    bool xcursor_available = false;
    bool icons_dir_writable = false;
    std::filesystem::path icons_dir;
};

auto run_doctor() -> DoctorResult;

} // namespace nkx
