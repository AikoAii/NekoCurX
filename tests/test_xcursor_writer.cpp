#include <catch2/catch_test_macros.hpp>
#include "nkx/xcursor/writer.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE("XCursor writer works correctly", "[xcursor_writer]") {
    SECTION("Writes a synthetic single-frame cursor") {
        // Create a 4x4 red cursor
        nkx::CursorFrame frame;
        frame.width = 4;
        frame.height = 4;
        frame.hotspot_x = 0;
        frame.hotspot_y = 0;
        frame.delay_ms = 0;
        frame.rgba_pixels.resize(4 * 4 * 4);
        for (uint32_t i = 0; i < 4 * 4; ++i) {
            frame.rgba_pixels[i * 4 + 0] = 255; // R
            frame.rgba_pixels[i * 4 + 1] = 0;   // G
            frame.rgba_pixels[i * 4 + 2] = 0;   // B
            frame.rgba_pixels[i * 4 + 3] = 255; // A
        }

        nkx::CursorSize cs;
        cs.nominal_size = 4;
        cs.frames.push_back(frame);

        nkx::CursorAnimation anim;
        anim.sizes.push_back(cs);

        auto out_path = fs::temp_directory_path() / "nkx_test_xcursor_output";
        auto result = nkx::write_xcursor(out_path, anim);
        REQUIRE(result.has_value());
        REQUIRE(fs::exists(out_path));
        REQUIRE(fs::file_size(out_path) > 0);

        // Check XCursor magic: "Xcur" (0x72756358)
        std::ifstream f(out_path, std::ios::binary);
        char magic[4];
        f.read(magic, 4);
        REQUIRE(magic[0] == 'X');
        REQUIRE(magic[1] == 'c');
        REQUIRE(magic[2] == 'u');
        REQUIRE(magic[3] == 'r');

        fs::remove(out_path);
    }

    SECTION("Rejects empty animation") {
        nkx::CursorAnimation empty_anim;
        auto out_path = fs::temp_directory_path() / "nkx_test_xcursor_empty";
        auto result = nkx::write_xcursor(out_path, empty_anim);
        REQUIRE_FALSE(result.has_value());
    }
}
