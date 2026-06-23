#pragma once

#include <tl/expected.hpp>
#include <string>

namespace nkx {

enum class ErrorCode {
    InvalidRiffHeader,
    InvalidAniHeader,
    MissingAnimationChunk,
    UnsupportedFormat,
    InvalidIconData,
    IoError,
    AliasLoadError,
    InstallError,
    UnknownError
};

struct Error {
    ErrorCode code;
    std::string message;
    std::string context;

    std::string to_string() const {
        if (context.empty()) {
            return message;
        }
        return message + " (" + context + ")";
    }
};

template <typename T>
using Result = tl::expected<T, Error>;

} // namespace nkx
