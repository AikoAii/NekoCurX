#include "nkx/theme/installer.hpp"
#include <cstdlib>
#include <iostream>

namespace nkx {

auto install_theme(const InstallOptions& opts) -> Result<std::filesystem::path> {
    // Determine install destination
    const char* home = std::getenv("HOME");
    if (!home) {
        return tl::unexpected(Error{ErrorCode::InstallError, "HOME environment variable not set"});
    }

    auto icons_dir = std::filesystem::path(home) / ".local" / "share" / "icons";
    auto dest = icons_dir / opts.theme_name;

    // Remove existing theme if present
    if (std::filesystem::exists(dest)) {
        std::filesystem::remove_all(dest);
    }

    // Copy the theme directory
    try {
        std::filesystem::create_directories(icons_dir);
        std::filesystem::copy(opts.theme_dir, dest,
            std::filesystem::copy_options::recursive |
            std::filesystem::copy_options::copy_symlinks);
    } catch (const std::filesystem::filesystem_error& e) {
        return tl::unexpected(Error{ErrorCode::InstallError, e.what(), dest.string()});
    }

    return dest;
}

auto run_doctor() -> DoctorResult {
    DoctorResult result;

    // Check xcursor
    // If we linked successfully, it's available
    result.xcursor_available = true;

    // Check icons dir
    const char* home = std::getenv("HOME");
    if (home) {
        result.icons_dir = std::filesystem::path(home) / ".local" / "share" / "icons";
        try {
            std::filesystem::create_directories(result.icons_dir);
            result.icons_dir_writable = true;
        } catch (...) {
            result.icons_dir_writable = false;
        }
    }

    return result;
}

} // namespace nkx
