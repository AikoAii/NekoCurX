#include "nkx/core/types.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace nkx {

namespace {

// Helper to get pixel with bounds clamping
uint32_t get_pixel(const std::vector<uint8_t>& img, uint32_t w, uint32_t h, int x, int y) {
    x = std::clamp(x, 0, static_cast<int>(w) - 1);
    y = std::clamp(y, 0, static_cast<int>(h) - 1);
    uint32_t idx = (y * w + x) * 4;
    return (static_cast<uint32_t>(img[idx]) << 24) |
           (static_cast<uint32_t>(img[idx + 1]) << 16) |
           (static_cast<uint32_t>(img[idx + 2]) << 8) |
           (static_cast<uint32_t>(img[idx + 3]));
}

CursorFrame scale_frame(const CursorFrame& src, uint32_t target_size) {
    CursorFrame dst;
    dst.width = target_size;
    dst.height = target_size;
    dst.delay_ms = src.delay_ms;
    
    // Scale hotspot proportionally
    float scale_x = static_cast<float>(target_size) / src.width;
    float scale_y = static_cast<float>(target_size) / src.height;
    dst.hotspot_x = static_cast<uint32_t>(std::round(src.hotspot_x * scale_x));
    dst.hotspot_y = static_cast<uint32_t>(std::round(src.hotspot_y * scale_y));

    dst.rgba_pixels.resize(target_size * target_size * 4);

    for (uint32_t y = 0; y < target_size; ++y) {
        for (uint32_t x = 0; x < target_size; ++x) {
            float gx = x / scale_x;
            float gy = y / scale_y;
            int gxi = static_cast<int>(gx);
            int gyi = static_cast<int>(gy);
            
            uint32_t c00 = get_pixel(src.rgba_pixels, src.width, src.height, gxi, gyi);
            uint32_t c10 = get_pixel(src.rgba_pixels, src.width, src.height, gxi + 1, gyi);
            uint32_t c01 = get_pixel(src.rgba_pixels, src.width, src.height, gxi, gyi + 1);
            uint32_t c11 = get_pixel(src.rgba_pixels, src.width, src.height, gxi + 1, gyi + 1);

            float tx = gx - gxi;
            float ty = gy - gyi;

            auto blend = [tx, ty](uint8_t v00, uint8_t v10, uint8_t v01, uint8_t v11) -> uint8_t {
                float a = v00 * (1.0f - tx) + v10 * tx;
                float b = v01 * (1.0f - tx) + v11 * tx;
                return static_cast<uint8_t>(std::round(a * (1.0f - ty) + b * ty));
            };

            uint32_t idx = (y * target_size + x) * 4;
            dst.rgba_pixels[idx]     = blend(c00 >> 24, c10 >> 24, c01 >> 24, c11 >> 24);
            dst.rgba_pixels[idx + 1] = blend((c00 >> 16) & 0xFF, (c10 >> 16) & 0xFF, (c01 >> 16) & 0xFF, (c11 >> 16) & 0xFF);
            dst.rgba_pixels[idx + 2] = blend((c00 >> 8) & 0xFF, (c10 >> 8) & 0xFF, (c01 >> 8) & 0xFF, (c11 >> 8) & 0xFF);
            dst.rgba_pixels[idx + 3] = blend(c00 & 0xFF, c10 & 0xFF, c01 & 0xFF, c11 & 0xFF);
        }
    }
    return dst;
}

CursorSize scale_cursor_size(const CursorSize& src, uint32_t target_size) {
    CursorSize dst;
    dst.nominal_size = target_size;
    dst.frames.reserve(src.frames.size());
    for (const auto& f : src.frames) {
        dst.frames.push_back(scale_frame(f, target_size));
    }
    return dst;
}

} // namespace

void CursorAnimation::normalize(bool linux_friendly) {
    // Determine current sizes
    std::vector<uint32_t> current_sizes;
    uint32_t max_size = 0;
    const CursorSize* best_source = nullptr;

    for (auto& cs : sizes) {
        // Enforce nominal size == image size
        if (!cs.frames.empty()) {
            cs.nominal_size = cs.frames[0].width;
        }
        current_sizes.push_back(cs.nominal_size);
        if (cs.nominal_size > max_size) {
            max_size = cs.nominal_size;
            best_source = &cs;
        }
    }

    if (!linux_friendly || !best_source) return;

    // We want 32, 48, 64 if max_size >= 128
    // If max_size is 64, we want 32, 48
    std::vector<uint32_t> desired_sizes;
    if (max_size >= 128) {
        desired_sizes = {32, 48, 64};
    } else if (max_size >= 64) {
        desired_sizes = {32, 48};
    } else if (max_size >= 48) {
        desired_sizes = {32};
    }

    // Copy best source to avoid dangling pointer when sizes reallocates!
    CursorSize source_copy = *best_source;

    for (uint32_t ds : desired_sizes) {
        if (std::find(current_sizes.begin(), current_sizes.end(), ds) == current_sizes.end()) {
            std::cout << "[DEBUG] Scaling from " << source_copy.nominal_size << " to " << ds << "\n";
            std::cout << "[DEBUG] source width=" << (source_copy.frames.empty() ? 0 : source_copy.frames[0].width)
                      << " height=" << (source_copy.frames.empty() ? 0 : source_copy.frames[0].height) << "\n";
            std::cout << "[DEBUG] frame count=" << source_copy.frames.size() << "\n";
            std::cout << "[DEBUG] Calling reserve(" << source_copy.frames.size() << ")\n";

            sizes.push_back(scale_cursor_size(source_copy, ds));
        }
    }

    // Sort sizes ascending
    std::sort(sizes.begin(), sizes.end(), [](const CursorSize& a, const CursorSize& b) {
        return a.nominal_size < b.nominal_size;
    });
}

} // namespace nkx
