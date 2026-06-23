#include "nkx/xcursor/writer.hpp"
#include <X11/Xcursor/Xcursor.h>
#include <cstdio>

namespace nkx {

auto write_xcursor(const std::filesystem::path& output_path,
                   const CursorAnimation& animation) -> Result<void> {

    // Count total images across all sizes and frames
    int total_images = 0;
    for (const auto& cs : animation.sizes) {
        total_images += static_cast<int>(cs.frames.size());
    }

    if (total_images == 0) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData, "No frames to write"});
    }

    XcursorImages* images = XcursorImagesCreate(total_images);
    if (!images) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to allocate XcursorImages"});
    }

    int idx = 0;
    for (const auto& cs : animation.sizes) {
        for (const auto& frame : cs.frames) {
            XcursorImage* img = XcursorImageCreate(
                static_cast<int>(frame.width),
                static_cast<int>(frame.height));
            if (!img) {
                XcursorImagesDestroy(images);
                return tl::unexpected(Error{ErrorCode::IoError, "Failed to allocate XcursorImage"});
            }

            img->xhot = frame.hotspot_x;
            img->yhot = frame.hotspot_y;
            img->delay = frame.delay_ms;
            img->size = cs.nominal_size;

            // Convert RGBA (our format) to ARGB (XCursor format)
            // XcursorPixel is uint32_t, stored as native-endian ARGB
            const uint8_t* src = frame.rgba_pixels.data();
            XcursorPixel* dst = img->pixels;
            uint32_t pixel_count = frame.width * frame.height;

            for (uint32_t p = 0; p < pixel_count; ++p) {
                uint8_t r = src[p * 4 + 0];
                uint8_t g = src[p * 4 + 1];
                uint8_t b = src[p * 4 + 2];
                uint8_t a = src[p * 4 + 3];
                dst[p] = (static_cast<uint32_t>(a) << 24)
                       | (static_cast<uint32_t>(r) << 16)
                       | (static_cast<uint32_t>(g) << 8)
                       | (static_cast<uint32_t>(b));
            }

            images->images[idx] = img;
            ++idx;
        }
    }
    images->nimage = idx;

    // Write to file
    FILE* fp = std::fopen(output_path.c_str(), "wb");
    if (!fp) {
        XcursorImagesDestroy(images);
        return tl::unexpected(Error{ErrorCode::IoError, "Cannot open output file", output_path.string()});
    }

    XcursorBool ok = XcursorFileSaveImages(fp, images);
    std::fclose(fp);
    XcursorImagesDestroy(images);

    if (!ok) {
        return tl::unexpected(Error{ErrorCode::IoError, "XcursorFileSaveImages failed", output_path.string()});
    }

    return {};
}

} // namespace nkx
