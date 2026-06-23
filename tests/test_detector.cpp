#include <catch2/catch_test_macros.hpp>
#include "nkx/theme/detector.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE("CursorDetector works correctly", "[detector]") {
    std::string compat_aliases_json = R"({
        "left_ptr": {
            "windows_names": ["normal", "arrow"],
            "symlinks": ["default"]
        },
        "watch": {
            "windows_names": ["busy", "wait"],
            "symlinks": ["wait"]
        },
        "xterm": {
            "windows_names": ["text", "ibeam"],
            "symlinks": ["ibeam"]
        }
    })";

    auto detector_res = nkx::CursorDetector::create(compat_aliases_json);
    REQUIRE(detector_res.has_value());
    auto& detector = detector_res.value();

    SECTION("Maps basic filenames") {
        std::vector<fs::path> files = {
            "Arrow.ani", "busy.cur", "TEXT.ani", "Unknown.ani"
        };
        
        auto res = detector.detect(files, std::nullopt);
        
        REQUIRE(res.mapped.size() == 3);
        REQUIRE(res.mapped[fs::path("Arrow.ani")] == "left_ptr");
        REQUIRE(res.mapped[fs::path("busy.cur")] == "watch");
        REQUIRE(res.mapped[fs::path("TEXT.ani")] == "xterm");
        
        REQUIRE(res.unmapped.size() == 1);
        REQUIRE(res.unmapped[0] == fs::path("Unknown.ani"));
    }

    SECTION("Uses inf mapping properly") {
        // Setup temp directory and inf file
        fs::path temp_dir = fs::temp_directory_path() / "nkx_test_detector";
        fs::create_directories(temp_dir);
        
        fs::path inf_path = temp_dir / "install.inf";
        std::ofstream inf_file(inf_path);
        inf_file << "[Strings]\n";
        inf_file << "pointer = \"CustomCursor.ani\"\n";
        inf_file << "help = \"Unknown.ani\"\n"; // "help" is an inf role, but we didn't define "help" in our test JSON above
        inf_file.close();

        std::vector<fs::path> files = {
            "CustomCursor.ani", "Unknown.ani"
        };

        auto res = detector.detect(files, inf_path);
        
        // CustomCursor.ani -> inf role "pointer" -> alias "normal" -> left_ptr
        REQUIRE(res.mapped.size() == 1);
        REQUIRE(res.mapped[fs::path("CustomCursor.ani")] == "left_ptr");
        
        // Unknown.ani -> inf role "help", which is not in our test aliases_json, so it remains unmapped
        REQUIRE(res.unmapped.size() == 1);
        REQUIRE(res.unmapped[0] == fs::path("Unknown.ani"));

        fs::remove_all(temp_dir);
    }
}
