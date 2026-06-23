#pragma once

#include "nkx/core/error.hpp"
#include <array>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

namespace nkx {

struct RiffChunk {
    std::array<char, 4> id{};
    uint32_t size = 0;
    std::array<char, 4> form_type{}; // Only for RIFF/LIST chunks
    std::vector<uint8_t> data;       // For data chunks
    std::vector<RiffChunk> children; // For RIFF/LIST chunks

    std::string id_str() const {
        return std::string(id.data(), 4);
    }

    std::string form_type_str() const {
        return std::string(form_type.data(), 4);
    }

    bool is_container() const {
        auto s = id_str();
        return s == "RIFF" || s == "LIST";
    }

    // Find first child chunk with given id
    const RiffChunk* find_child(const std::string& child_id) const;

    // Find all children with given id
    std::vector<const RiffChunk*> find_children(const std::string& child_id) const;
};

auto parse_riff(std::istream& stream) -> Result<RiffChunk>;

} // namespace nkx
