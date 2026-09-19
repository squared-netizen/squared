#pragma once

#include <cstddef>
#include <cstdint>

namespace sq::assets {

/**
 * @brief Resource limits fixed for one AssetManager.
 *
 * Every limit is a precondition: passing zero is a programmer error and is
 * asserted, not reported. These are the numbers that make the manager's
 * footprint knowable before it runs.
 */
struct AssetManagerOptions final {
    /** @brief Maximum cached assets across all types; must be positive. */
    std::size_t maximum_cached_assets{1024};

    /** @brief Maximum bytes read for one source file; must be positive. */
    std::uint64_t maximum_asset_bytes{64U * 1024U * 1024U};

    /** @brief Maximum nested dependency depth; must be positive. */
    std::size_t maximum_dependency_depth{64};
};

}  // namespace sq::assets
