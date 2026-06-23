#include "nkx/theme/detector.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>

namespace nkx {

namespace {

std::string to_lower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

// Maps standard Windows .inf cursor roles to our internal "stem" aliases
// This allows us to use the same logic for inf mapping and filename mapping
std::string inf_role_to_alias(const std::string& role) {
    auto r = to_lower(role);
    if (r == "pointer" || r == "arrow") return "normal";
    if (r == "help") return "help";
    if (r == "work") return "working";
    if (r == "busy") return "busy";
    if (r == "cross") return "precision";
    if (r == "text") return "text";
    if (r == "hand" || r == "pen") return "handwriting";
    if (r == "unavailiable" || r == "unavailable") return "unavailable";
    if (r == "vert") return "vertical";
    if (r == "horz") return "horizontal";
    if (r == "dgn1") return "diagonal1";
    if (r == "dgn2") return "diagonal2";
    if (r == "move") return "move";
    if (r == "alternate") return "alternate";
    if (r == "link") return "link";
    return r; // fallback
}

} // namespace

auto CursorDetector::create(const std::string& compat_aliases_json_content) -> Result<CursorDetector> {
    CursorDetector detector;

    try {
        auto j = nlohmann::json::parse(compat_aliases_json_content);
        for (auto& [linux_name, data] : j.items()) {
            if (data.contains("windows_names") && data["windows_names"].is_array()) {
                for (auto& alias : data["windows_names"]) {
                    detector._stem_to_linux_name[to_lower(alias.get<std::string>())] = linux_name;
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        return tl::unexpected(Error{ErrorCode::AliasLoadError, "Failed to parse aliases JSON", e.what()});
    }

    return detector;
}

auto CursorDetector::parse_inf(const std::filesystem::path& inf_path) const -> std::map<std::string, std::string> {
    std::map<std::string, std::string> file_to_role;
    std::ifstream file(inf_path);
    if (!file.is_open()) return file_to_role;

    std::string line;
    std::regex strings_section(R"(^\[Strings\])", std::regex_constants::icase);
    std::regex key_val{"^\\s*([^=]+?)\\s*=\\s*\"?([^\"]+?)\"?\\s*$"};
    bool in_strings = false;

    // A very simple .inf parser. Usually the files are listed under [Strings]
    // like:
    // pointer = "Arrow.cur"
    // help    = "Help.cur"
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';') continue; // comment or empty

        if (line[0] == '[') {
            in_strings = std::regex_search(line, strings_section);
            continue;
        }

        if (in_strings) {
            std::smatch match;
            if (std::regex_match(line, match, key_val)) {
                std::string role = match[1].str();
                std::string filename = to_lower(match[2].str());
                file_to_role[filename] = inf_role_to_alias(role);
            }
        }
    }
    return file_to_role;
}

auto CursorDetector::detect(const std::vector<std::filesystem::path>& files,
                            const std::optional<std::filesystem::path>& inf_file) const -> DetectionResult {
    DetectionResult result;
    
    std::map<std::string, std::string> inf_mapping;
    if (inf_file) {
        inf_mapping = parse_inf(*inf_file);
    }

    for (const auto& path : files) {
        std::string filename = to_lower(path.filename().string());
        std::string stem = to_lower(path.stem().string());

        std::string target_linux_name;

        // 1. Try inf mapping first
        if (auto it = inf_mapping.find(filename); it != inf_mapping.end()) {
            if (auto alias_it = _stem_to_linux_name.find(it->second); alias_it != _stem_to_linux_name.end()) {
                target_linux_name = alias_it->second;
            }
        }

        // 2. Fallback to filename heuristics
        if (target_linux_name.empty()) {
            if (auto alias_it = _stem_to_linux_name.find(stem); alias_it != _stem_to_linux_name.end()) {
                target_linux_name = alias_it->second;
            }
        }

        // 3. Record result
        if (!target_linux_name.empty()) {
            result.mapped[path] = target_linux_name;
        } else {
            result.unmapped.push_back(path);
        }
    }

    return result;
}

} // namespace nkx
