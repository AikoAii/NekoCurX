#include <catch2/catch_test_macros.hpp>
#include "nkx/theme/scanner.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE("ThemeScanner works correctly", "[scanner]") {
    // Setup temp directory
    fs::path temp_dir = fs::temp_directory_path() / "nkx_test_scanner";
    fs::create_directories(temp_dir);

    // Create some files
    std::ofstream(temp_dir / "arrow.ani").put('a');
    std::ofstream(temp_dir / "busy.CUR").put('b'); // test case insensitivity
    std::ofstream(temp_dir / "install.inf").put('c');
    std::ofstream(temp_dir / "readme.txt").put('d');
    
    fs::create_directories(temp_dir / "subdir");
    std::ofstream(temp_dir / "subdir" / "link.ani").put('e');

    SECTION("Scans valid directory") {
        auto res = nkx::ThemeScanner::scan_directory(temp_dir);
        REQUIRE(res.has_value());
        
        auto& scan = res.value();
        REQUIRE(scan.cursor_files.size() == 3);
        REQUIRE(scan.inf_file.has_value());
        REQUIRE(scan.inf_file->filename() == "install.inf");
    }

    SECTION("Fails on non-existent directory") {
        auto res = nkx::ThemeScanner::scan_directory(temp_dir / "does_not_exist");
        REQUIRE_FALSE(res.has_value());
        REQUIRE(res.error().code == nkx::ErrorCode::IoError);
    }

    // Cleanup
    fs::remove_all(temp_dir);
}
