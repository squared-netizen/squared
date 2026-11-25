export module sq.preferences:preferences_error;

import std;

export namespace sq::preferences
{

/** @brief Why a preferences operation failed; `none` when it did not. */
enum class PreferencesErrorCode : std::uint8_t {
    none,
    /** @brief The files could not be read or written (the message says which). */
    io,
    /** @brief A file's name escapes the tree, or names SQ-INF. */
    invalid_name,
    /** @brief The folder holds a cartridge of another kind. */
    not_preferences,
    /** @brief settings.json is not one JSON object. */
    invalid_settings,
    /** @brief A setting's key is empty. */
    invalid_key,
    /** @brief An exported cartridge could not be packed, or read back (the message says why). */
    cartridge,
    /** @brief The cartridge imported is another app's preferences. */
    another_app,
};

/** @brief What went wrong, said: false (no error) when it did not. */
struct PreferencesError
{
        PreferencesErrorCode code{ PreferencesErrorCode::none };
        std::string message;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return code != PreferencesErrorCode::none;
        }
};

} // namespace sq::preferences
