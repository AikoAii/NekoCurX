#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace nkx {

struct CursorFrame {
    uint32_t width;
    uint32_t height;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t delay_ms;                // 0 for static cursors
    std::vector<uint8_t> rgba_pixels; // RGBA, row-major, top-to-bottom
};

struct CursorSize {
    uint32_t nominal_size;
    std::vector<CursorFrame> frames;

    bool is_animated() const { return frames.size() > 1; }
};

struct CursorAnimation {
    std::vector<CursorSize> sizes; // Multi-size support

    bool is_animated() const {
        return !sizes.empty() && sizes.front().is_animated();
    }
};

struct CursorTheme {
    std::string theme_name;
    std::map<std::string, CursorAnimation> cursors; // linux_name → animation
    std::map<std::string, CursorAnimation> extras;  // unmapped cursors
};

} // namespace nkx
