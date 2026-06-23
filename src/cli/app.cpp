#include "nkx/cli/app.hpp"
#include "nkx/theme/scanner.hpp"
#include "nkx/theme/detector.hpp"
#include "nkx/theme/builder.hpp"
#include "nkx/theme/installer.hpp"
#include "nkx/ani/reader.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace nkx {

namespace {

// Helper to read a file's entire content as string
std::string read_file_content(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

// Try to locate the assets directory relative to the executable or cwd
std::filesystem::path find_assets_dir() {
    // Try relative to cwd first
    if (std::filesystem::exists("assets/compat_aliases.json")) {
        return "assets";
    }
    // Try relative to executable (Linux /proc/self/exe)
    try {
        auto exe = std::filesystem::read_symlink("/proc/self/exe");
        auto dir = exe.parent_path() / "assets";
        if (std::filesystem::exists(dir / "compat_aliases.json")) return dir;
        // Also try one level up (e.g. build/nkx -> project root)
        dir = exe.parent_path().parent_path() / "assets";
        if (std::filesystem::exists(dir / "compat_aliases.json")) return dir;
        // Try two levels up (e.g. build/src/nkx)
        dir = exe.parent_path().parent_path().parent_path() / "assets";
        if (std::filesystem::exists(dir / "compat_aliases.json")) return dir;
    } catch (...) {}
    return "assets"; // fallback
}

} // namespace

App::App() : _app(std::make_unique<CLI::App>("NekoCurX - Windows to Linux Cursor Converter")) {
    _app->require_subcommand(1);

    setup_scan();
    setup_inspect();
    setup_convert();
    setup_install();
    setup_doctor();
}

int App::run(int argc, char** argv) {
    try {
        _app->parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return _app->exit(e);
    }
    return 0;
}

void App::setup_scan() {
    auto* scan_cmd = _app->add_subcommand("scan", "Scan a directory and show how cursors will be mapped");
    auto theme_dir = std::make_shared<std::string>();
    scan_cmd->add_option("dir", *theme_dir, "Windows cursor theme directory")
        ->required()->check(CLI::ExistingDirectory);

    scan_cmd->callback([theme_dir]() {
        auto scan_res = ThemeScanner::scan_directory(*theme_dir);
        if (!scan_res) {
            std::cerr << "ERROR: " << scan_res.error().to_string() << "\n";
            return;
        }

        std::cout << "Found:\n";
        for (const auto& f : scan_res->cursor_files) {
            std::cout << "  " << f.filename().string() << "\n";
        }
        if (scan_res->inf_file) {
            std::cout << "  " << scan_res->inf_file->filename().string() << " (install.inf)\n";
        }
        std::cout << "\n";

        auto assets = find_assets_dir();
        auto compat_aliases_content = read_file_content(assets / "compat_aliases.json");
        if (compat_aliases_content.empty()) {
            std::cerr << "ERROR: Cannot find assets/compat_aliases.json\n";
            return;
        }

        auto detector_res = CursorDetector::create(compat_aliases_content);
        if (!detector_res) {
            std::cerr << "ERROR: " << detector_res.error().to_string() << "\n";
            return;
        }

        auto detect_res = detector_res->detect(scan_res->cursor_files, scan_res->inf_file);

        std::cout << "Mapped:\n";
        for (const auto& [file, name] : detect_res.mapped) {
            std::cout << "  " << file.filename().string() << " -> " << name << "\n";
        }

        if (!detect_res.unmapped.empty()) {
            std::cout << "\nUnmapped (will be placed in extras/):\n";
            for (const auto& file : detect_res.unmapped) {
                std::cout << "  " << file.filename().string() << "\n";
            }
        }
    });
}

void App::setup_inspect() {
    auto* inspect_cmd = _app->add_subcommand("inspect", "Inspect a cursor file (.ani/.cur)");
    auto file_path = std::make_shared<std::string>();
    inspect_cmd->add_option("file", *file_path, "Cursor file path")
        ->required()->check(CLI::ExistingFile);

    inspect_cmd->callback([file_path]() {
        auto path = std::filesystem::path(*file_path);

        auto anim = read_cursor_file(path);
        if (!anim) {
            std::cerr << "ERROR: " << anim.error().to_string() << "\n";
            return;
        }

        // Determine file type from extension
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::string type_str = (ext == ".ani") ? "Animated Cursor (ANI)" : "Static Cursor (CUR)";

        std::cout << "File:     " << path.filename().string() << "\n";
        std::cout << "Type:     " << type_str << "\n";

        for (const auto& cs : anim->sizes) {
            std::cout << "Size:     " << cs.nominal_size << "x" << cs.nominal_size << "\n";
            std::cout << "Frames:   " << cs.frames.size() << "\n";

            if (!cs.frames.empty()) {
                std::cout << "Hotspot:  (" << cs.frames[0].hotspot_x
                          << ", " << cs.frames[0].hotspot_y << ")\n";

                if (cs.is_animated()) {
                    uint32_t total_ms = 0;
                    for (const auto& f : cs.frames) total_ms += f.delay_ms;
                    std::cout << "Duration: " << total_ms << "ms\n";
                }
            }
        }
    });
}

void App::setup_convert() {
    auto* convert_cmd = _app->add_subcommand("convert", "Convert a Windows theme to Linux XCursor format");
    auto theme_dir = std::make_shared<std::string>();
    auto theme_name = std::make_shared<std::string>();
    auto output_dir = std::make_shared<std::string>();
    auto comment = std::make_shared<std::string>();
    auto author = std::make_shared<std::string>();

    convert_cmd->add_option("dir", *theme_dir, "Windows cursor theme directory")
        ->required()->check(CLI::ExistingDirectory);
    convert_cmd->add_option("--theme,-t", *theme_name, "Override theme name");
    convert_cmd->add_option("--output,-o", *output_dir, "Override output directory");
    convert_cmd->add_option("--comment,-c", *comment, "Override theme comment");
    convert_cmd->add_option("--author,-a", *author, "Override theme author");

    convert_cmd->callback([theme_dir, theme_name, output_dir, comment, author]() {
        auto assets = find_assets_dir();

        BuildOptions opts;
        opts.input_dir = *theme_dir;
        opts.theme_name = *theme_name;
        opts.comment = *comment;
        opts.author = *author;
        if (!output_dir->empty()) opts.output_dir = *output_dir;
        opts.compat_aliases_json = read_file_content(assets / "compat_aliases.json");

        if (opts.compat_aliases_json.empty()) {
            std::cerr << "ERROR: Cannot find assets/compat_aliases.json\n";
            return;
        }

        std::cout << "Converting: " << *theme_dir << "\n";
        auto result = build_theme(opts);
        if (!result) {
            std::cerr << "ERROR: " << result.error().to_string() << "\n";
            return;
        }

        std::cout << "\nCreated: " << result->output_path.string() << "\n";
        std::cout << "  " << result->cursor_count << " cursors";
        if (result->extra_count > 0) {
            std::cout << ", " << result->extra_count << " extras";
        }
        std::cout << ", " << result->symlink_count << " symlinks\n";
        std::cout << "  index.theme generated\n";
        std::cout << "  metadata.json generated\n";
    });
}

void App::setup_install() {
    auto* install_cmd = _app->add_subcommand("install", "Convert and install a theme to ~/.local/share/icons/");
    auto theme_dir = std::make_shared<std::string>();
    auto theme_name = std::make_shared<std::string>();
    auto comment = std::make_shared<std::string>();
    auto author = std::make_shared<std::string>();

    install_cmd->add_option("dir", *theme_dir, "Windows cursor theme directory")
        ->required()->check(CLI::ExistingDirectory);
    install_cmd->add_option("--theme,-t", *theme_name, "Override theme name");
    install_cmd->add_option("--comment,-c", *comment, "Override theme comment");
    install_cmd->add_option("--author,-a", *author, "Override theme author");

    install_cmd->callback([theme_dir, theme_name, comment, author]() {
        auto assets = find_assets_dir();

        // 1. Convert
        BuildOptions build_opts;
        build_opts.input_dir = *theme_dir;
        build_opts.theme_name = *theme_name;
        build_opts.comment = *comment;
        build_opts.author = *author;
        build_opts.compat_aliases_json = read_file_content(assets / "compat_aliases.json");

        if (build_opts.compat_aliases_json.empty()) {
            std::cerr << "ERROR: Cannot find assets/compat_aliases.json\n";
            return;
        }

        std::cout << "Converting: " << *theme_dir << "\n";
        auto build_result = build_theme(build_opts);
        if (!build_result) {
            std::cerr << "ERROR: " << build_result.error().to_string() << "\n";
            return;
        }

        std::cout << "\nConverted: " << build_result->output_path.string() << "\n";
        std::cout << "  " << build_result->cursor_count << " cursors";
        if (build_result->extra_count > 0) {
            std::cout << ", " << build_result->extra_count << " extras";
        }
        std::cout << ", " << build_result->symlink_count << " symlinks\n";

        // 2. Install
        InstallOptions install_opts;
        install_opts.theme_dir = build_result->output_path;
        install_opts.theme_name = build_result->theme.theme_name;

        std::cout << "\nInstalling to ~/.local/share/icons/" << install_opts.theme_name << " ...\n";
        auto install_result = install_theme(install_opts);
        if (!install_result) {
            std::cerr << "ERROR: " << install_result.error().to_string() << "\n";
            return;
        }

        std::cout << "Installed: " << install_result->string() << "\n";
        std::cout << "\nDone! You may need to select the theme in your desktop settings.\n";
    });
}

void App::setup_doctor() {
    auto* doctor_cmd = _app->add_subcommand("doctor", "Check system dependencies and readiness");
    doctor_cmd->callback([]() {
        auto result = run_doctor();

        std::cout << "NekoCurX Doctor\n";
        std::cout << "===============\n\n";

        std::cout << "libXcursor:  " << (result.xcursor_available ? "✓ available" : "✗ not found") << "\n";
        std::cout << "Icons dir:   " << result.icons_dir.string() << "\n";
        std::cout << "Writable:    " << (result.icons_dir_writable ? "✓ yes" : "✗ no") << "\n";

        auto assets = find_assets_dir();
        bool compat_aliases_ok = std::filesystem::exists(assets / "compat_aliases.json");
        std::cout << "compat_aliases.json:  " << (compat_aliases_ok ? "✓ found" : "✗ not found") << "\n";

        if (result.xcursor_available && result.icons_dir_writable && compat_aliases_ok) {
            std::cout << "\nAll checks passed! ✓\n";
        } else {
            std::cout << "\nSome checks failed. Fix the issues above.\n";
        }
    });
}

} // namespace nkx
