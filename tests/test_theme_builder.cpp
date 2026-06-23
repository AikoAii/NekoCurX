#include <catch2/catch_test_macros.hpp>
#include "nkx/theme/builder.hpp"
#include <filesystem>
#include <fstream>

// This test just verifies the builder doesn't crash on an empty directory.
// Real end-to-end tests require actual .ani/.cur fixture files.

TEST_CASE("ThemeBuilder handles edge cases", "[theme_builder]") {
    SECTION("Returns error on empty directory") {
        auto temp = std::filesystem::temp_directory_path() / "nkx_test_builder_empty";
        std::filesystem::create_directories(temp);

        nkx::BuildOptions opts;
        opts.input_dir = temp;
        opts.compat_aliases_json = R"({
            "left_ptr": {
                "windows_names": ["normal"],
                "symlinks": ["default"]
            }
        })";

        auto result = nkx::build_theme(opts);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().code == nkx::ErrorCode::IoError);

        std::filesystem::remove_all(temp);
    }
}
