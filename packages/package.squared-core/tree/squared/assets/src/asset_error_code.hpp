#pragma once

#include <cstdint>

namespace sq::assets
{

/** @brief Stable error categories produced by asset loading. */
enum class AssetErrorCode : std::uint8_t {
    None,
    NoLoader,
    LoaderAlreadyRegistered,
    InvalidArgument,
    SourceUnavailable,
    SourceTooLarge,
    CacheFull,
    DependencyTooDeep,
    DependencyCycle,
    LoaderFailed,
    NotLoaded
};

} // namespace sq::assets
