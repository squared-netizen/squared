export module sq.assets:src.asset_error_code;

import std;

export namespace sq::assets
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
