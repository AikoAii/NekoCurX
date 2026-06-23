#pragma once

#include "nkx/core/error.hpp"
#include "nkx/core/types.hpp"
#include <filesystem>

namespace nkx {

auto write_xcursor(const std::filesystem::path& output_path,
                   const CursorAnimation& animation) -> Result<void>;

} // namespace nkx
