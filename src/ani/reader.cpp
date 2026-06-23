#include "nkx/ani/reader.hpp"
#include "nkx/ani/riff.hpp"
#include "nkx/ani/ico_decoder.hpp"
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

namespace nkx {

namespace {

#pragma pack(push, 1)
struct AniHeader {
    uint32_t cb_size;
    uint32_t num_frames;
    uint32_t num_steps;
    uint32_t cx_width;
    uint32_t cy_height;
    uint32_t c_bit_count;
    uint32_t c_planes;
    uint32_t jif_rate;    // default delay in jiffies (1/60s)
    uint32_t fl;          // flags: AF_ICON=0x1, AF_SEQUENCE=0x2
};
#pragma pack(pop)

uint32_t read_u32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

// Convert jiffies (1/60s) to milliseconds
uint32_t jiffies_to_ms(uint32_t jiffies) {
    return (jiffies * 1000 + 30) / 60; // rounded
}

auto read_ani(std::istream& stream) -> Result<CursorAnimation> {
    auto riff_result = parse_riff(stream);
    if (!riff_result) return tl::unexpected(riff_result.error());

    auto& root = *riff_result;
    if (root.form_type_str() != "ACON") {
        return tl::unexpected(Error{ErrorCode::InvalidAniHeader,
            "Expected ACON form type, got " + root.form_type_str()});
    }

    // 1. Parse anih header
    auto* anih_chunk = root.find_child("anih");
    if (!anih_chunk || anih_chunk->data.size() < sizeof(AniHeader)) {
        return tl::unexpected(Error{ErrorCode::InvalidAniHeader, "Missing or invalid anih chunk"});
    }

    AniHeader anih;
    std::memcpy(&anih, anih_chunk->data.data(), sizeof(AniHeader));

    // 2. Parse rate chunk (optional)
    std::vector<uint32_t> rates;
    auto* rate_chunk = root.find_child("rate");
    if (rate_chunk && !rate_chunk->data.empty()) {
        uint32_t n = static_cast<uint32_t>(rate_chunk->data.size() / 4);
        rates.resize(n);
        for (uint32_t i = 0; i < n; ++i) {
            rates[i] = read_u32_le(rate_chunk->data.data() + i * 4);
        }
    }

    // 3. Parse seq chunk (optional)
    std::vector<uint32_t> sequence;
    auto* seq_chunk = root.find_child("seq ");
    if (seq_chunk && !seq_chunk->data.empty()) {
        uint32_t n = static_cast<uint32_t>(seq_chunk->data.size() / 4);
        sequence.resize(n);
        for (uint32_t i = 0; i < n; ++i) {
            sequence[i] = read_u32_le(seq_chunk->data.data() + i * 4);
        }
    }

    // 4. Find LIST/fram chunk containing icon data
    std::vector<const RiffChunk*> icon_chunks;
    for (const auto& child : root.children) {
        if (child.id_str() == "LIST" && child.form_type_str() == "fram") {
            icon_chunks = child.find_children("icon");
            break;
        }
    }

    if (icon_chunks.empty()) {
        return tl::unexpected(Error{ErrorCode::MissingAnimationChunk,
            "No icon chunks found in LIST/fram"});
    }

    // 5. Decode all icon frames, grouping by size
    // Each icon chunk might contain multiple sizes
    // We need: map<nominal_size, vector<CursorFrame>> 
    // where each vector entry is one animation step at that size
    
    // First, decode all raw frames from each icon chunk
    struct DecodedIcon {
        std::vector<CursorFrame> frames_by_size; // one per size in this icon
    };
    std::vector<DecodedIcon> decoded_icons;

    for (const auto* icon : icon_chunks) {
        auto frames_result = decode_icon_data(
            std::span<const uint8_t>(icon->data.data(), icon->data.size()));
        if (!frames_result) {
            continue; // skip bad frames
        }
        DecodedIcon di;
        di.frames_by_size = std::move(*frames_result);
        decoded_icons.push_back(std::move(di));
    }

    if (decoded_icons.empty()) {
        return tl::unexpected(Error{ErrorCode::InvalidIconData,
            "Failed to decode any icon frames"});
    }

    // 6. Build animation steps with proper ordering and timing
    uint32_t num_steps = anih.num_steps > 0 ? anih.num_steps : static_cast<uint32_t>(decoded_icons.size());

    // Collect all unique sizes across all decoded icons
    std::map<uint32_t, std::vector<CursorFrame>> size_to_frames;

    for (uint32_t step = 0; step < num_steps; ++step) {
        uint32_t frame_idx = step;
        if (!sequence.empty() && step < sequence.size()) {
            frame_idx = sequence[step];
        }
        if (frame_idx >= decoded_icons.size()) {
            frame_idx = frame_idx % decoded_icons.size();
        }

        uint32_t delay_jiffies = anih.jif_rate;
        if (!rates.empty() && step < rates.size()) {
            delay_jiffies = rates[step];
        }
        uint32_t delay_ms = jiffies_to_ms(delay_jiffies);

        for (auto& frame : decoded_icons[frame_idx].frames_by_size) {
            CursorFrame f = frame; // copy
            f.delay_ms = delay_ms;
            size_to_frames[f.width].push_back(std::move(f));
        }
    }

    // 7. Build CursorAnimation with CursorSize entries
    CursorAnimation animation;
    for (auto& [nominal, frames] : size_to_frames) {
        CursorSize cs;
        cs.nominal_size = nominal;
        cs.frames = std::move(frames);
        animation.sizes.push_back(std::move(cs));
    }

    return animation;
}

auto read_cur(std::istream& stream) -> Result<CursorAnimation> {
    // Read entire file
    stream.seekg(0, std::ios::end);
    auto file_size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(static_cast<size_t>(file_size));
    if (!stream.read(reinterpret_cast<char*>(data.data()), file_size)) {
        return tl::unexpected(Error{ErrorCode::IoError, "Failed to read .cur file"});
    }

    auto frames_result = decode_icon_data(std::span<const uint8_t>(data));
    if (!frames_result) return tl::unexpected(frames_result.error());

    // Group by size
    CursorAnimation animation;
    std::map<uint32_t, std::vector<CursorFrame>> size_to_frames;
    for (auto& frame : *frames_result) {
        size_to_frames[frame.width].push_back(std::move(frame));
    }
    for (auto& [nominal, frames] : size_to_frames) {
        CursorSize cs;
        cs.nominal_size = nominal;
        cs.frames = std::move(frames);
        animation.sizes.push_back(std::move(cs));
    }

    return animation;
}

} // namespace

auto read_cursor_file(const std::filesystem::path& path) -> Result<CursorAnimation> {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return tl::unexpected(Error{ErrorCode::IoError, "Cannot open file", path.string()});
    }

    // Peek first 4 bytes to determine file type
    char magic[4] = {};
    file.read(magic, 4);
    file.seekg(0);

    if (std::memcmp(magic, "RIFF", 4) == 0) {
        return read_ani(file);
    } else {
        // Assume ICO/CUR format
        return read_cur(file);
    }
}

} // namespace nkx
