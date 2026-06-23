#pragma once

#include "nkx/core/error.hpp"
#include "nkx/core/types.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace nkx {

// Decodes an ICO/CUR data blob (as found in ANI icon chunks or standalone .cur files)
// into one or more CursorFrames (one per size found inside the ICO).
auto decode_icon_data(std::span<const uint8_t> data) -> Result<std::vector<CursorFrame>>;

} // namespace nkx
