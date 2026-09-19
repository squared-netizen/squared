#pragma once

#include <squared/assets/asset_error_code.hpp>

#include <string>

namespace sq::assets {

/** @brief Structured asset loading failure information. */
struct AssetError {
    /** @brief Failure category; None means no error. */
    AssetErrorCode code{AssetErrorCode::None};

    /** @brief Human-readable diagnostic message. */
    std::string message;

    /** @brief Asset path the operation was attempting, when known. */
    std::string path;

    /** @brief Return whether this structure represents a failure. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != AssetErrorCode::None;
    }
};

}  // namespace sq::assets
