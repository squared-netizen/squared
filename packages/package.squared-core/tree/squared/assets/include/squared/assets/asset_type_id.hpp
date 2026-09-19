#pragma once

namespace sq::assets {

/** @brief Stable per-type identifier used to key the loader and cache tables. */
using AssetTypeId = const void*;

namespace detail {
/** @brief One byte per type, whose address is that type's identifier. */
template <typename T>
inline constexpr char asset_type_tag = 0;
}  // namespace detail

/**
 * @brief Return the identifier for one asset type.
 * @tparam T Any type; no registration or declaration is required.
 * @return An address unique to T and stable for the life of the program.
 *
 * @note This replaces std::type_index, which cannot be used: typeid is
 * rejected outright under -fno-rtti, and squared builds that way. An inline
 * variable has one address across every translation unit, so this is unique,
 * stable, and costs nothing at run time.
 */
template <typename T>
[[nodiscard]] constexpr AssetTypeId asset_type_id() noexcept
{
    return &detail::asset_type_tag<T>;
}

}  // namespace sq::assets
