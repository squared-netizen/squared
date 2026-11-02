export module sq.assets:src.asset_error;

export import :src.asset_error_code;
import std;

export namespace sq::assets
{

/** @brief Structured asset loading failure information. */
struct AssetError
{
        /** @brief Failure category; None means no error. */
        AssetErrorCode code{ AssetErrorCode::None };

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

} // namespace sq::assets
