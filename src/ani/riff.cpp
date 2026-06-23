#include "nkx/ani/riff.hpp"
#include <cstring>

namespace nkx {

namespace {

// Read a little-endian uint32 from a stream
bool read_u32(std::istream& stream, uint32_t& out) {
    uint8_t buf[4];
    if (!stream.read(reinterpret_cast<char*>(buf), 4)) return false;
    out = static_cast<uint32_t>(buf[0])
        | (static_cast<uint32_t>(buf[1]) << 8)
        | (static_cast<uint32_t>(buf[2]) << 16)
        | (static_cast<uint32_t>(buf[3]) << 24);
    return true;
}

bool read_fourcc(std::istream& stream, std::array<char, 4>& out) {
    return !!stream.read(out.data(), 4);
}

auto parse_chunk(std::istream& stream) -> Result<RiffChunk> {
    RiffChunk chunk;

    if (!read_fourcc(stream, chunk.id)) {
        return tl::unexpected(Error{ErrorCode::InvalidRiffHeader, "Failed to read chunk ID"});
    }

    if (!read_u32(stream, chunk.size)) {
        return tl::unexpected(Error{ErrorCode::InvalidRiffHeader, "Failed to read chunk size"});
    }

    auto id = chunk.id_str();

    if (id == "RIFF" || id == "LIST") {
        // Container chunk: read form type then children
        if (!read_fourcc(stream, chunk.form_type)) {
            return tl::unexpected(Error{ErrorCode::InvalidRiffHeader, "Failed to read form type"});
        }

        // size includes the 4 bytes of form_type
        uint32_t bytes_remaining = chunk.size - 4;

        while (bytes_remaining > 0) {
            auto start_pos = stream.tellg();

            auto child_result = parse_chunk(stream);
            if (!child_result) {
                // If we can't parse a child, stop (could be padding or EOF)
                break;
            }

            auto end_pos = stream.tellg();
            uint32_t bytes_consumed = static_cast<uint32_t>(end_pos - start_pos);

            if (bytes_consumed > bytes_remaining) {
                break;
            }

            bytes_remaining -= bytes_consumed;
            chunk.children.push_back(std::move(*child_result));
        }
    } else {
        // Data chunk: read raw bytes
        chunk.data.resize(chunk.size);
        if (!stream.read(reinterpret_cast<char*>(chunk.data.data()), chunk.size)) {
            return tl::unexpected(Error{ErrorCode::InvalidRiffHeader,
                "Failed to read chunk data", id});
        }

        // RIFF chunks are word-aligned (padded to even size)
        if (chunk.size % 2 != 0) {
            stream.seekg(1, std::ios::cur);
        }
    }

    return chunk;
}

} // namespace

const RiffChunk* RiffChunk::find_child(const std::string& child_id) const {
    for (const auto& child : children) {
        if (child.id_str() == child_id) return &child;
    }
    return nullptr;
}

std::vector<const RiffChunk*> RiffChunk::find_children(const std::string& child_id) const {
    std::vector<const RiffChunk*> result;
    for (const auto& child : children) {
        if (child.id_str() == child_id) result.push_back(&child);
    }
    return result;
}

auto parse_riff(std::istream& stream) -> Result<RiffChunk> {
    auto result = parse_chunk(stream);
    if (!result) return result;

    if (result->id_str() != "RIFF") {
        return tl::unexpected(Error{ErrorCode::InvalidRiffHeader,
            "Expected RIFF header, got " + result->id_str()});
    }

    return result;
}

} // namespace nkx
