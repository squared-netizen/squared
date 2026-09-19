#pragma once

#include <squared/data/json_error_code.hpp>

#include <cstddef>
#include <string>

namespace sq::data {

/** @brief Structured JSON failure information. */
struct JsonError {
    /** @brief Failure category; None means no error. */
    JsonErrorCode code{JsonErrorCode::None};

    /** @brief Human-readable diagnostic message. */
    std::string message;

    /** @brief Zero-based byte offset into the failing document. */
    std::size_t byte_offset{0};

    /** @brief One-based source line of the failure. */
    std::size_t line{0};

    /** @brief One-based source column of the failure. */
    std::size_t column{0};

    /** @brief Return whether this structure represents a failure. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != JsonErrorCode::None;
    }
};

}  // namespace sq::data
