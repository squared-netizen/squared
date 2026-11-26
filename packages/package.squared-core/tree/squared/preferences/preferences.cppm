export module sq.preferences:preferences;

export import :preferences_error;
import std;
import sq.data;
import sq.files;

export namespace sq::preferences
{

/** @brief What an import does with the preferences there already. */
enum class ImportMode : std::uint8_t {
    /** @brief The cartridge's, whole: every setting and file there now is gone. */
    replace,
    /** @brief Laid over them: its settings and files win, the others stay. */
    merge,
};

/**
 * @brief An app's preferences: one squared cartridge (sqcart 3.0) of kind
 * `preferences`, exploded in the app's private storage, as every workspace
 * is while it is used.
 *
 *     <Local>/preferences/
 *       SQ-INF/manifest.json   kind "preferences", id "preferences.<app id>"
 *       tree/settings.json     the typed settings: one JSON object
 *       tree/...               anything else the app keeps, any file type
 *
 * Settings are keyed with dots by section (`ui.font.size`); only what
 * differs from a default is kept, and each change is written at once
 * (settings.json whole, atomically). `generation()` counts changes, so what
 * draws from a setting reads it again when it moves.
 *
 * On the UI thread, as the files it holds are: the model is sent what it
 * needs.
 */
class Preferences final
{
    public:
        /** @brief The cartridge's kind. */
        static constexpr std::string_view kind = "preferences";
        /** @brief The settings' schema, the cartridge's version. */
        static constexpr std::string_view schema = "1.0.0";
        /** @brief Where the settings are, under the tree. */
        static constexpr std::string_view settings_file = "settings.json";

        /**
         * @brief The app's preferences in `folder` (Local): made, empty,
         * when they are not there.
         * @param files The app's file system.
         * @param app_id The app's id (`dev.squared.sqide`): the cartridge's
         * is `preferences.` and it.
         * @param folder Where, in the app's private storage.
         */
        [[nodiscard]] static std::expected<Preferences, PreferencesError>
        open( files::FileSystem &files, std::string_view app_id,
              std::string_view folder = "preferences" );

        /** @brief The cartridge's id: `preferences.<app id>`. */
        [[nodiscard]] const std::string &id() const noexcept { return id_; }
        /** @brief Where it is, in Local storage. */
        [[nodiscard]] const std::string &folder() const noexcept { return folder_; }
        /** @brief Counts the changes since it was opened. */
        [[nodiscard]] std::uint64_t generation() const noexcept { return generation_; }

        /** @brief A setting as it is kept, or null when it is not. */
        [[nodiscard]] const data::JsonValue *find( std::string_view key ) const noexcept;
        /** @brief A setting, or `fallback` when it is missing or of another type. */
        [[nodiscard]] bool get_bool( std::string_view key, bool fallback ) const noexcept;
        /** @brief A whole number (a real one rounded toward zero), or `fallback`. */
        [[nodiscard]] std::int64_t get_integer( std::string_view key,
                                                std::int64_t fallback ) const noexcept;
        /** @brief A number, or `fallback`. */
        [[nodiscard]] double get_number( std::string_view key, double fallback ) const noexcept;
        /** @brief A string, or `fallback`. */
        [[nodiscard]] std::string get_text( std::string_view key, std::string_view fallback ) const;
        /** @brief Every key kept, in order. */
        [[nodiscard]] std::vector<std::string> keys() const;

        /** @brief Keep a setting, and write the settings. */
        PreferencesError set( std::string_view key, data::JsonValue value );
        /** @brief Forget a setting (back to its default), and write the settings. */
        PreferencesError reset( std::string_view key );
        /** @brief Forget every setting; the app's other files are kept. */
        PreferencesError reset_all();

        /**
         * @brief A file of the app's, as a Local path the file system takes;
         * empty when `name` would leave the tree or names SQ-INF.
         */
        [[nodiscard]] std::string path( std::string_view name ) const;
        /** @brief A file of the app's, read whole. */
        [[nodiscard]] files::FileReadResult read( std::string_view name ) const;
        /** @brief A file of the app's, written whole (atomically), its folders made. */
        PreferencesError write( std::string_view name, std::span<const std::byte> bytes );
        /** @brief A file of the app's, gone. */
        PreferencesError remove( std::string_view name );
        /** @brief The names in one of the app's folders (`""`: the tree's top). */
        [[nodiscard]] std::vector<std::string> list( std::string_view folder = {} ) const;

        /**
         * @brief The preferences, settings and files, packed into one
         * cartridge at `cartridge` (a platform path: what the file dialog
         * chose), through sq.drive: written beside it and renamed over it,
         * so one there already is replaced only by a whole one.
         */
        PreferencesError export_to( const std::filesystem::path &cartridge ) const;

        /**
         * @brief A cartridge `export_to` made (this app's: another app's, or
         * another kind, is refused) taken in. It is exploded beside the
         * folder and checked before anything here changes; then `replace`
         * swaps it in whole, `merge` lays its settings and files over these.
         * The settings are read again and `generation()` moves.
         */
        PreferencesError import_from( const std::filesystem::path &cartridge,
                                      ImportMode mode = ImportMode::replace );

    private:
        Preferences( files::FileSystem &files, std::string id, std::string folder,
                     data::JsonValue settings );
        PreferencesError save();

        files::FileSystem *files_;
        std::string id_;
        std::string folder_;
        data::JsonValue settings_;
        std::uint64_t generation_{ 0 };
};

} // namespace sq::preferences
