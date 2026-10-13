#pragma once

namespace sq::assets {

/** @brief Stable error categories produced by asset loading. */
enum class AssetErrorCode {
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

}  // namespace sq::assets
