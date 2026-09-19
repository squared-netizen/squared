#pragma once

#include <squared/files/file_error.hpp>

#include <string>

namespace sq::files {

/**
 * @brief Text read from one file, or the failure that prevented it.
 *
 * The bytes are not validated or transcoded. A file that is not UTF-8 is
 * returned as-is rather than rejected, because the callers that care about
 * encoding know more about the file than this layer does.
 */
struct FileTextResult {
    /** @brief File contents; valid only when error is empty. */
    std::string text;

    /** @brief Read failure, or a no-error FileError on success. */
    FileError error;

    /** @brief Return whether the read succeeded. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error;
    }
};

}  // namespace sq::files
