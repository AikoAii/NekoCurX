#include <catch2/catch_test_macros.hpp>
#include "nkx/ani/riff.hpp"
#include <sstream>
#include <cstring>

namespace {

// Helper: build a minimal valid RIFF buffer in memory
std::string make_riff(const std::string& form_type,
                      const std::vector<std::pair<std::string, std::string>>& chunks) {
    // Build inner chunks
    std::string inner;
    for (const auto& [id, data] : chunks) {
        inner += id; // 4-byte chunk ID
        uint32_t size = static_cast<uint32_t>(data.size());
        inner.append(reinterpret_cast<const char*>(&size), 4);
        inner += data;
        if (data.size() % 2 != 0) inner += '\0'; // pad
    }

    // Build RIFF header
    std::string result;
    result += "RIFF";
    uint32_t total = static_cast<uint32_t>(4 + inner.size()); // form_type + inner
    result.append(reinterpret_cast<const char*>(&total), 4);
    result += form_type;
    result += inner;
    return result;
}

} // namespace

TEST_CASE("RIFF parser works correctly", "[riff]") {
    SECTION("Parses valid RIFF with data chunks") {
        auto buf = make_riff("TEST", {
            {"abcd", "hello"},
            {"efgh", "world!"}
        });

        std::istringstream stream(buf);
        auto result = nkx::parse_riff(stream);
        REQUIRE(result.has_value());
        REQUIRE(result->id_str() == "RIFF");
        REQUIRE(result->form_type_str() == "TEST");
        REQUIRE(result->children.size() == 2);
        REQUIRE(result->children[0].id_str() == "abcd");
        REQUIRE(result->children[0].data.size() == 5);
        REQUIRE(result->children[1].id_str() == "efgh");
        REQUIRE(result->children[1].data.size() == 6);
    }

    SECTION("find_child works") {
        auto buf = make_riff("TEST", {
            {"aaaa", "1234"},
            {"bbbb", "5678"}
        });
        std::istringstream stream(buf);
        auto result = nkx::parse_riff(stream);
        REQUIRE(result.has_value());

        auto* found = result->find_child("bbbb");
        REQUIRE(found != nullptr);
        REQUIRE(found->data.size() == 4);

        auto* not_found = result->find_child("cccc");
        REQUIRE(not_found == nullptr);
    }

    SECTION("Rejects non-RIFF data") {
        std::string garbage = "NOT_RIFF_DATA_HERE__";
        std::istringstream stream(garbage);
        auto result = nkx::parse_riff(stream);
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("Handles truncated stream") {
        std::string truncated = "RIFF";
        std::istringstream stream(truncated);
        auto result = nkx::parse_riff(stream);
        REQUIRE_FALSE(result.has_value());
    }
}
