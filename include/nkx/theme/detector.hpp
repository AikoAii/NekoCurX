#pragma once

#include "nkx/core/error.hpp"
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace nkx {

struct DetectionResult {
    std::map<std::filesystem::path, std::string> mapped; // file → linux_name
    std::vector<std::filesystem::path> unmapped;
};

class CursorDetector {
public:
    static auto create(const std::string& aliases_json_content) -> Result<CursorDetector>;

    auto detect(const std::vector<std::filesystem::path>& files,
                const std::optional<std::filesystem::path>& inf_file) const -> DetectionResult;

private:
    std::map<std::string, std::string> _stem_to_linux_name;
    
    // Internal helper to parse inf
    auto parse_inf(const std::filesystem::path& inf_path) const -> std::map<std::string, std::string>;
};

} // namespace nkx
