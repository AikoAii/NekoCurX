#include "nkx/theme/scanner.hpp"
#include <algorithm>
#include <cctype>

namespace nkx {

namespace {

std::string to_lower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

bool is_cursor_file(const std::filesystem::path& path) {
    auto ext = to_lower(path.extension().string());
    return ext == ".ani" || ext == ".cur";
}

bool is_inf_file(const std::filesystem::path& path) {
    return to_lower(path.extension().string()) == ".inf";
}

} // namespace

auto ThemeScanner::scan_directory(const std::filesystem::path& dir) -> Result<ScanResult> {
    if (!std::filesystem::exists(dir)) {
        return tl::unexpected(Error{ErrorCode::IoError, "Directory does not exist", dir.string()});
    }

    if (!std::filesystem::is_directory(dir)) {
        return tl::unexpected(Error{ErrorCode::IoError, "Path is not a directory", dir.string()});
    }

    ScanResult result;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                if (is_cursor_file(entry.path())) {
                    result.cursor_files.push_back(entry.path());
                } else if (is_inf_file(entry.path())) {
                    // Just take the first .inf file we find
                    if (!result.inf_file) {
                        result.inf_file = entry.path();
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return tl::unexpected(Error{ErrorCode::IoError, e.what(), dir.string()});
    }

    return result;
}

} // namespace nkx
