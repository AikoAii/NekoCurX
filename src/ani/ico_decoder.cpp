#include "nkx/ani/ico_decoder.hpp"
#include <cstring>
#include <algorithm>

namespace nkx {

namespace {

#pragma pack(push, 1)
struct IconDir {
    uint16_t reserved;
    uint16_t type;   // 1=ICO, 2=CUR
    uint16_t count;
};

struct IconDirEntry {
    uint8_t  width;       // 0 means 256
    uint8_t  height;      // 0 means 256
    uint8_t  color_count;
    uint8_t  reserved;
    uint16_t planes;      // For CUR: hotspot_x
    uint16_t bit_count;   // For CUR: hotspot_y
    uint32_t bytes_in_res;
    uint32_t image_offset;
};

struct BitmapInfoHeader {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_ppm;
    int32_t  y_ppm;
    uint32_t colors_used;
    uint32_t colors_important;
};
#pragma pack(pop)

uint16_t read_u16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t read_u32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

bool is_png(const uint8_t* data, size_t size) {
    static const uint8_t png_magic[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    return size >= 8 && std::memcmp(data, png_magic, 8) == 0;
}

// Decode a 32-bit ARGB BMP DIB into RGBA pixels
auto decode_bmp_32(const uint8_t* pixel_data, uint32_t w, uint32_t h,
                   const uint8_t* /*and_mask*/, uint32_t /*and_mask_size*/) -> std::vector<uint8_t> {
    std::vector<uint8_t> rgba(w * h * 4);
    uint32_t row_bytes = w * 4;

    for (uint32_t y = 0; y < h; ++y) {
        // BMP rows are bottom-up; flip to top-down
        const uint8_t* src_row = pixel_data + (h - 1 - y) * row_bytes;
        uint8_t* dst_row = rgba.data() + y * w * 4;

        for (uint32_t x = 0; x < w; ++x) {
            // BMP stores BGRA
            dst_row[x * 4 + 0] = src_row[x * 4 + 2]; // R
            dst_row[x * 4 + 1] = src_row[x * 4 + 1]; // G
            dst_row[x * 4 + 2] = src_row[x * 4 + 0]; // B
            dst_row[x * 4 + 3] = src_row[x * 4 + 3]; // A
        }
    }

    return rgba;
}

// Decode a 24-bit RGB BMP DIB into RGBA pixels (with AND mask for transparency)
auto decode_bmp_24(const uint8_t* pixel_data, uint32_t w, uint32_t h,
                   const uint8_t* and_mask, uint32_t and_mask_size) -> std::vector<uint8_t> {
    std::vector<uint8_t> rgba(w * h * 4);
    // 24-bit rows are padded to 4-byte boundary
    uint32_t row_bytes = ((w * 3 + 3) / 4) * 4;
    uint32_t and_row_bytes = ((w + 31) / 32) * 4;

    for (uint32_t y = 0; y < h; ++y) {
        const uint8_t* src_row = pixel_data + (h - 1 - y) * row_bytes;
        uint8_t* dst_row = rgba.data() + y * w * 4;

        for (uint32_t x = 0; x < w; ++x) {
            dst_row[x * 4 + 0] = src_row[x * 3 + 2]; // R
            dst_row[x * 4 + 1] = src_row[x * 3 + 1]; // G
            dst_row[x * 4 + 2] = src_row[x * 3 + 0]; // B

            // AND mask: 1 = transparent, 0 = opaque
            if (and_mask && and_mask_size > 0) {
                uint32_t mask_y = h - 1 - y;
                uint32_t byte_idx = mask_y * and_row_bytes + (x / 8);
                uint8_t bit = (and_mask[byte_idx] >> (7 - (x % 8))) & 1;
                dst_row[x * 4 + 3] = bit ? 0 : 255;
            } else {
                dst_row[x * 4 + 3] = 255;
            }
        }
    }

    return rgba;
}

// Decode an 8-bit palettized BMP DIB into RGBA pixels
auto decode_bmp_8(const uint8_t* bih_start, const uint8_t* pixel_data, uint32_t w, uint32_t h,
                  uint32_t colors_used, const uint8_t* and_mask, uint32_t and_mask_size) -> std::vector<uint8_t> {
    std::vector<uint8_t> rgba(w * h * 4);
    
    uint32_t palette_count = colors_used > 0 ? colors_used : 256;
    const uint8_t* palette = bih_start + sizeof(BitmapInfoHeader);
    
    uint32_t row_bytes = ((w + 3) / 4) * 4;
    uint32_t and_row_bytes = ((w + 31) / 32) * 4;

    for (uint32_t y = 0; y < h; ++y) {
        const uint8_t* src_row = pixel_data + (h - 1 - y) * row_bytes;
        uint8_t* dst_row = rgba.data() + y * w * 4;

        for (uint32_t x = 0; x < w; ++x) {
            uint8_t idx = src_row[x];
            if (idx < palette_count) {
                // Palette entries are BGRA (4 bytes each)
                dst_row[x * 4 + 0] = palette[idx * 4 + 2]; // R
                dst_row[x * 4 + 1] = palette[idx * 4 + 1]; // G
                dst_row[x * 4 + 2] = palette[idx * 4 + 0]; // B
            }

            if (and_mask && and_mask_size > 0) {
                uint32_t mask_y = h - 1 - y;
                uint32_t byte_idx = mask_y * and_row_bytes + (x / 8);
                uint8_t bit = (and_mask[byte_idx] >> (7 - (x % 8))) & 1;
                dst_row[x * 4 + 3] = bit ? 0 : 255;
            } else {
                dst_row[x * 4 + 3] = 255;
            }
        }
    }

    return rgba;
}

auto decode_single_image(const uint8_t* data, size_t data_size,
                         uint32_t hotspot_x, uint32_t hotspot_y) -> Result<CursorFrame> {
    if (is_png(data, data_size)) {
        // TODO: PNG decoding with libpng
        // For now, skip PNG-encoded cursors
        return tl::unexpected(Error{ErrorCode::UnsupportedFormat,
            "PNG-encoded cursor frames not yet supported"});
    }

    // BMP/DIB path
    if (data_size < sizeof(BitmapInfoHeader)) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData, "BMP header too small"});
    }

    BitmapInfoHeader bih;
    std::memcpy(&bih, data, sizeof(BitmapInfoHeader));

    uint32_t w = static_cast<uint32_t>(bih.width);
    // ICO/CUR BMP height is doubled (XOR mask + AND mask)
    uint32_t h = static_cast<uint32_t>(std::abs(bih.height)) / 2;

    if (w == 0 || h == 0 || w > 1024 || h > 1024) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData, "Invalid BMP dimensions"});
    }

    // Calculate palette size
    uint32_t palette_size = 0;
    if (bih.bit_count <= 8) {
        uint32_t palette_count = bih.colors_used > 0 ? bih.colors_used : (1u << bih.bit_count);
        palette_size = palette_count * 4;
    }

    const uint8_t* pixel_data = data + sizeof(BitmapInfoHeader) + palette_size;

    std::vector<uint8_t> rgba;

    if (bih.bit_count == 32) {
        uint32_t xor_size = w * h * 4;
        uint32_t and_row_bytes = ((w + 31) / 32) * 4;
        uint32_t and_size = and_row_bytes * h;
        const uint8_t* and_mask = pixel_data + xor_size;
        rgba = decode_bmp_32(pixel_data, w, h, and_mask, and_size);
    } else if (bih.bit_count == 24) {
        uint32_t xor_row_bytes = ((w * 3 + 3) / 4) * 4;
        uint32_t xor_size = xor_row_bytes * h;
        uint32_t and_row_bytes = ((w + 31) / 32) * 4;
        uint32_t and_size = and_row_bytes * h;
        const uint8_t* and_mask = pixel_data + xor_size;
        rgba = decode_bmp_24(pixel_data, w, h, and_mask, and_size);
    } else if (bih.bit_count == 8) {
        uint32_t xor_row_bytes = ((w + 3) / 4) * 4;
        uint32_t xor_size = xor_row_bytes * h;
        uint32_t and_row_bytes = ((w + 31) / 32) * 4;
        uint32_t and_size = and_row_bytes * h;
        const uint8_t* and_mask = pixel_data + xor_size;
        rgba = decode_bmp_8(data, pixel_data, w, h, bih.colors_used, and_mask, and_size);
    } else {
        return tl::unexpected(Error{ErrorCode::UnsupportedFormat,
            "Unsupported BMP bit depth: " + std::to_string(bih.bit_count)});
    }

    CursorFrame frame;
    frame.width = w;
    frame.height = h;
    frame.hotspot_x = hotspot_x;
    frame.hotspot_y = hotspot_y;
    frame.delay_ms = 0;
    frame.rgba_pixels = std::move(rgba);

    return frame;
}

} // namespace

auto decode_icon_data(std::span<const uint8_t> data) -> Result<std::vector<CursorFrame>> {
    if (data.size() < sizeof(IconDir)) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData, "Data too small for ICO header"});
    }

    IconDir dir;
    std::memcpy(&dir, data.data(), sizeof(IconDir));

    if (dir.reserved != 0 || (dir.type != 1 && dir.type != 2)) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData,
            "Invalid ICO/CUR header (reserved=" + std::to_string(dir.reserved) +
            ", type=" + std::to_string(dir.type) + ")"});
    }

    bool is_cur = (dir.type == 2);
    std::vector<CursorFrame> frames;

    for (uint16_t i = 0; i < dir.count; ++i) {
        size_t entry_offset = sizeof(IconDir) + i * sizeof(IconDirEntry);
        if (entry_offset + sizeof(IconDirEntry) > data.size()) {
            break;
        }

        IconDirEntry entry;
        std::memcpy(&entry, data.data() + entry_offset, sizeof(IconDirEntry));

        uint32_t hotspot_x = 0;
        uint32_t hotspot_y = 0;
        if (is_cur) {
            hotspot_x = entry.planes;
            hotspot_y = entry.bit_count;
        }

        if (entry.image_offset + entry.bytes_in_res > data.size()) {
            continue; // skip malformed entry
        }

        const uint8_t* image_data = data.data() + entry.image_offset;
        size_t image_size = entry.bytes_in_res;

        auto frame_result = decode_single_image(image_data, image_size, hotspot_x, hotspot_y);
        if (frame_result) {
            frames.push_back(std::move(*frame_result));
        }
        // Skip frames that fail to decode (e.g. PNG not yet supported)
    }

    if (frames.empty()) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData, "No decodable frames found in ICO/CUR"});
    }

    return frames;
}

} // namespace nkx
