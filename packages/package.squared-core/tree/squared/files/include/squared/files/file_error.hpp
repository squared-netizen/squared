#pragma once

#include <squared/files/file_error_code.hpp>

#include <string>

namespace sq::files {

/** @brief Structured file system failure information. */
struct FileError {
    /** @brief Failure category; None means no error. */
    FileErrorCode code{FileErrorCode::None};

    /** @brief Human-readable diagnostic message. */
    std::string message;

    /** @brief Resolved path the operation was attempting, when known. */
    std::string path;

    /** @brief Return whether this structure represents a failure. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != FileErrorCode::None;
    }
};

}  // namespace sq::files
