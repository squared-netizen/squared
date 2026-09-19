#pragma once

#include <squared/assets/asset_error.hpp>
#include <squared/assets/asset_handle.hpp>

namespace sq::assets {

/** @brief One loaded asset, or the failure that prevented loading it. */
template <typename T>
struct AssetLoadResult {
    /** @brief The asset; null unless error is empty. */
    AssetHandle<T> asset;

    /** @brief Load failure, or a no-error AssetError on success. */
    AssetError error;

    /** @brief Return whether the load succeeded. */
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return !error && asset != nullptr;
    }
};

}  // namespace sq::assets
