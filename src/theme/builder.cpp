#include "nkx/theme/builder.hpp"
#include "nkx/theme/scanner.hpp"
#include "nkx/theme/detector.hpp"
#include "nkx/ani/reader.hpp"
#include "nkx/xcursor/writer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace nkx {

namespace {

std::string derive_theme_name(const std::filesystem::path& input_dir) {
    auto name = input_dir.stem().string();
    if (name.empty()) name = "ConvertedTheme";
    return name;
}

void write_index_theme(const std::filesystem::path& path, const std::string& name, const std::string& comment, const std::string& author) {
    std::ofstream out(path);
    std::string final_name = name.empty() ? "ConvertedTheme" : name;
    std::string final_comment = comment.empty() ? "Converted using NekoCurX" : comment;

    out << "[Icon Theme]\n";
    out << "Name=" << final_name << "\n";
    out << "Comment=" << final_comment << "\n\n";
    out << "[X-NekoCurX]\n";
    if (!author.empty() && author != "Unknown") {
        out << "Author=" << author << "\n";
    }
    out << "Generator=NekoCurX\n";
}

void write_metadata_json(const std::filesystem::path& path, const BuildResult& result, const std::string& comment, const std::string& author) {
    nlohmann::json j;
    std::string final_name = result.theme.theme_name.empty() ? "ConvertedTheme" : result.theme.theme_name;
    std::string final_comment = comment.empty() ? "Converted using NekoCurX" : comment;
    std::string final_author = author.empty() ? "Unknown" : author;

    j["theme_name"] = final_name;
    j["comment"] = final_comment;
    j["author"] = final_author;
    j["generator"] = "NekoCurX";
    j["version"] = "0.1.0";
    j["cursor_count"] = result.cursor_count;
    j["extra_count"] = result.extra_count;
    j["symlink_count"] = result.symlink_count;

    nlohmann::json cursors_arr = nlohmann::json::array();
    for (const auto& [name, _] : result.theme.cursors) {
        cursors_arr.push_back(name);
    }
    j["cursors"] = cursors_arr;

    nlohmann::json extras_arr = nlohmann::json::array();
    for (const auto& [name, _] : result.theme.extras) {
        extras_arr.push_back(name);
    }
    j["extras"] = extras_arr;

    std::ofstream out(path);
    out << j.dump(2) << "\n";
}

uint32_t create_symlinks(const std::filesystem::path& cursors_dir,
                         const std::string& compat_aliases_json_content,
                         const CursorTheme& theme) {
    uint32_t count = 0;
    try {
        auto j = nlohmann::json::parse(compat_aliases_json_content);
        for (auto& [primary, data] : j.items()) {
            // Only create symlinks for cursors we actually have
            if (theme.cursors.find(primary) == theme.cursors.end()) continue;

            if (data.contains("symlinks") && data["symlinks"].is_array()) {
                for (auto& alias : data["symlinks"]) {
                    std::string alias_name = alias.get<std::string>();
                    auto link_path = cursors_dir / alias_name;
                    if (!std::filesystem::exists(link_path)) {
                        std::filesystem::create_symlink(primary, link_path);
                        ++count;
                    }
                }
            }
        }
    } catch (...) {
        // Non-fatal: symlinks are nice-to-have
    }
    return count;
}

} // namespace

auto build_theme(const BuildOptions& opts) -> Result<BuildResult> {
    // 1. Scan
    auto scan_result = ThemeScanner::scan_directory(opts.input_dir);
    if (!scan_result) return tl::unexpected(scan_result.error());

    if (scan_result->cursor_files.empty()) {
        return tl::unexpected(Error{ErrorCode::IoError,
            "No cursor files found", opts.input_dir.string()});
    }

    // 2. Detect
    auto detector_result = CursorDetector::create(opts.compat_aliases_json);
    if (!detector_result) return tl::unexpected(detector_result.error());

    auto detection = detector_result->detect(scan_result->cursor_files, scan_result->inf_file);

    // 3. Setup output dirs
    std::string theme_name = opts.theme_name.empty()
        ? derive_theme_name(opts.input_dir)
        : opts.theme_name;

    auto output_dir = opts.output_dir.empty()
        ? opts.input_dir.parent_path() / (theme_name + "-Linux")
        : opts.output_dir;

    auto cursors_dir = output_dir / "cursors";
    auto extras_dir = output_dir / "extras";
    std::filesystem::create_directories(cursors_dir);

    BuildResult result;
    result.output_path = output_dir;
    result.theme.theme_name = theme_name;

    // 4. Convert mapped cursors
    for (const auto& [file, linux_name] : detection.mapped) {
        std::cout << "  " << file.filename().string() << " -> " << linux_name;
        auto anim = read_cursor_file(file);
        if (!anim) {
            std::cout << " ✗ (" << anim.error().message << ")\n";
            continue;
        }

        auto xcursor_path = cursors_dir / linux_name;
        auto write_result = write_xcursor(xcursor_path, *anim);
        if (!write_result) {
            std::cout << " ✗ (" << write_result.error().message << ")\n";
            continue;
        }

        result.theme.cursors[linux_name] = std::move(*anim);
        result.cursor_count++;
        std::cout << " ✓\n";
    }

    // 5. Convert unmapped cursors -> extras
    if (!detection.unmapped.empty()) {
        std::filesystem::create_directories(extras_dir);
        for (const auto& file : detection.unmapped) {
            std::string extra_name = file.stem().string();
            // lowercase the extra name
            std::transform(extra_name.begin(), extra_name.end(), extra_name.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            std::cout << "  " << file.filename().string() << " -> extras/" << extra_name;
            auto anim = read_cursor_file(file);
            if (!anim) {
                std::cout << " ✗ (" << anim.error().message << ")\n";
                continue;
            }

            auto xcursor_path = extras_dir / extra_name;
            auto write_result = write_xcursor(xcursor_path, *anim);
            if (!write_result) {
                std::cout << " ✗ (" << write_result.error().message << ")\n";
                continue;
            }

            result.theme.extras[extra_name] = std::move(*anim);
            result.extra_count++;
            std::cout << " ✓\n";
        }
    }

    // 6. Create symlinks
    result.symlink_count = create_symlinks(cursors_dir, opts.compat_aliases_json, result.theme);

    // 7. Generate metadata files
    write_index_theme(output_dir / "index.theme", theme_name, opts.comment, opts.author);
    write_metadata_json(output_dir / "metadata.json", result, opts.comment, opts.author);

    return result;
}

} // namespace nkx
