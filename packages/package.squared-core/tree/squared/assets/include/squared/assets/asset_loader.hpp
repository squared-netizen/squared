#pragma once

#include <squared/assets/asset_load_context.hpp>
#include <squared/assets/asset_load_result.hpp>

#include <functional>
#include <string_view>

namespace sq::assets {

/**
 * @brief Callback that turns a path into one asset of type T.
 * @tparam T Asset value type this loader produces.
 *
 * @note std::function here is deliberate and bounded: one per asset type,
 * registered once at startup, never called per frame. The alternative is a
 * concept-constrained template, which would force the manager's cache to be a
 * template too and give every asset type its own copy of the whole class.
 */
template <typename T>
using AssetLoader = std::function<AssetLoadResult<T>(
    AssetLoadContext& context,
    std::string_view path
)>;

}  // namespace sq::assets
