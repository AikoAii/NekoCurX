#include <catch2/catch_test_macros.hpp>
#include "nkx/ani/ico_decoder.hpp"
#include <cstring>

namespace {

// Build a minimal valid 32-bit CUR file in memory (2x2 pixels)
std::vector<uint8_t> make_mini_cur(uint16_t hotspot_x = 0, uint16_t hotspot_y = 0) {
    std::vector<uint8_t> data;
    auto write_u16 = [&](uint16_t v) {
        data.push_back(v & 0xFF);
        data.push_back((v >> 8) & 0xFF);
    };
    auto write_u32 = [&](uint32_t v) {
        data.push_back(v & 0xFF);
        data.push_back((v >> 8) & 0xFF);
        data.push_back((v >> 16) & 0xFF);
        data.push_back((v >> 24) & 0xFF);
    };
    auto write_i32 = [&](int32_t v) { write_u32(static_cast<uint32_t>(v)); };

    // ICONDIR
    write_u16(0);     // reserved
    write_u16(2);     // type = CUR
    write_u16(1);     // count = 1

    // Calculate sizes
    uint32_t w = 2, h = 2;
    uint32_t bmp_header_size = 40;
    uint32_t xor_size = w * h * 4; // 32-bit BGRA
    uint32_t and_row_bytes = 4;    // ((2+31)/32)*4 = 4
    uint32_t and_size = and_row_bytes * h;
    uint32_t image_data_size = bmp_header_size + xor_size + and_size;
    uint32_t image_offset = 6 + 16; // ICONDIR + 1 ICONDIRENTRY

    // ICONDIRENTRY
    data.push_back(static_cast<uint8_t>(w));  // width
    data.push_back(static_cast<uint8_t>(h));  // height
    data.push_back(0);                         // color count
    data.push_back(0);                         // reserved
    write_u16(hotspot_x);                      // planes/hotspot_x for CUR
    write_u16(hotspot_y);                      // bit_count/hotspot_y for CUR
    write_u32(image_data_size);                // bytes_in_res
    write_u32(image_offset);                   // image_offset

    // BITMAPINFOHEADER
    write_u32(bmp_header_size); // biSize
    write_i32(static_cast<int32_t>(w));  // biWidth
    write_i32(static_cast<int32_t>(h * 2)); // biHeight (doubled for XOR+AND)
    write_u16(1);   // biPlanes
    write_u16(32);  // biBitCount
    write_u32(0);   // biCompression
    write_u32(0);   // biSizeImage
    write_i32(0);   // biXPelsPerMeter
    write_i32(0);   // biYPelsPerMeter
    write_u32(0);   // biClrUsed
    write_u32(0);   // biClrImportant

    // XOR mask (32-bit BGRA, bottom-up)
    // Row 1 (bottom): red, green
    data.push_back(0); data.push_back(0); data.push_back(255); data.push_back(255); // BGRA: red
    data.push_back(0); data.push_back(255); data.push_back(0); data.push_back(255); // BGRA: green
    // Row 0 (top): blue, white
    data.push_back(255); data.push_back(0); data.push_back(0); data.push_back(255); // BGRA: blue
    data.push_back(255); data.push_back(255); data.push_back(255); data.push_back(255); // BGRA: white

    // AND mask (all opaque = 0)
    for (uint32_t i = 0; i < and_size; ++i) data.push_back(0);

    return data;
}

} // namespace

TEST_CASE("ICO decoder works correctly", "[ico_decoder]") {
    SECTION("Decodes a minimal 2x2 32-bit CUR") {
        auto cur_data = make_mini_cur(1, 1);
        auto result = nkx::decode_icon_data(std::span<const uint8_t>(cur_data));
        REQUIRE(result.has_value());
        REQUIRE(result->size() == 1);

        auto& frame = (*result)[0];
        REQUIRE(frame.width == 2);
        REQUIRE(frame.height == 2);
        REQUIRE(frame.hotspot_x == 1);
        REQUIRE(frame.hotspot_y == 1);
        REQUIRE(frame.rgba_pixels.size() == 2 * 2 * 4);

        // Top-left pixel should be blue (after bottom-up flip)
        // Row 0 in our output = top row of image = second BMP row (blue, white)
        REQUIRE(frame.rgba_pixels[0] == 0);   // R
        REQUIRE(frame.rgba_pixels[1] == 0);   // G
        REQUIRE(frame.rgba_pixels[2] == 255); // B
        REQUIRE(frame.rgba_pixels[3] == 255); // A
    }

    SECTION("Rejects empty data") {
        std::vector<uint8_t> empty;
        auto result = nkx::decode_icon_data(std::span<const uint8_t>(empty));
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Rejects garbage data") {
        std::vector<uint8_t> garbage = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        auto result = nkx::decode_icon_data(std::span<const uint8_t>(garbage));
        REQUIRE_FALSE(result.has_value());
    }
}
