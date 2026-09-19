#pragma once

#include <squared/data/json_error.hpp>

#include <string>

namespace sq::data {

/** @brief Result of serializing one owned JSON value. */
struct JsonWriteResult {
    /** @brief Serialized UTF-8 text; valid only when error is empty. */
    std::string text;

    /** @brief Serialization failure, or a no-error JsonError on success. */
    JsonError error;

    /** @brief Return whether serialization succeeded. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error;
    }
};

}  // namespace sq::data
