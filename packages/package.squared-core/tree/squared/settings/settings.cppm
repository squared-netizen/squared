export module sq.settings:settings;

import std;
import sq.gui;
import sq.preferences;
import sq.stage;

export namespace sq
{

/** @brief What a setting's entry shows, and what it writes to its key. */
enum class SettingType : std::uint8_t {
    toggle, ///< on or off: a check box; true or false
    number, ///< a value in a range: a slider; a number
    choice, ///< one of `choices`: a button that steps through them; the choice's text
    text,   ///< a line of text: a field, written when the edit is finished; text
    action, ///< a button that calls the app (`act`); no key
};

/**
 * @brief One setting on the screen: its preferences key, its label, what it
 * shows. Constant data, as an app declares its sections, made by the
 * factories below:
 *
 * ```cpp
 * constexpr std::array<std::string_view, 3> bells{ "off", "flash", "vibrate" };
 * constexpr std::array terminal{
 *     sq::SettingEntry::number( "terminal.scrollback", "Scrollback", 100, 100000, 100, 5000 ),
 *     sq::SettingEntry::choice( "terminal.bell", "Bell", bells, "flash" ),
 * };
 * ```
 */
struct SettingEntry final
{
        [[nodiscard]] static constexpr SettingEntry
        toggle( std::string_view key, std::string_view label, bool fallback = false ) noexcept
        {
            SettingEntry e;
            e.key = key;
            e.label = label;
            e.type = SettingType::toggle;
            e.fallback = fallback ? 1.0 : 0.0;
            return e;
        }
        [[nodiscard]] static constexpr SettingEntry number( std::string_view key,
                                                            std::string_view label, double least,
                                                            double most, double step,
                                                            double fallback ) noexcept
        {
            SettingEntry e;
            e.key = key;
            e.label = label;
            e.type = SettingType::number;
            e.least = least;
            e.most = most;
            e.step = step;
            e.fallback = fallback;
            return e;
        }
        [[nodiscard]] static constexpr SettingEntry
        choice( std::string_view key, std::string_view label,
                std::span<const std::string_view> choices, std::string_view fallback ) noexcept
        {
            SettingEntry e;
            e.key = key;
            e.label = label;
            e.type = SettingType::choice;
            e.choices = choices;
            e.text_fallback = fallback;
            return e;
        }
        [[nodiscard]] static constexpr SettingEntry text( std::string_view key,
                                                          std::string_view label,
                                                          std::string_view fallback = {} ) noexcept
        {
            SettingEntry e;
            e.key = key;
            e.label = label;
            e.type = SettingType::text;
            e.text_fallback = fallback;
            return e;
        }
        [[nodiscard]] static constexpr SettingEntry action( std::string_view label,
                                                            void ( *act )( void *context ) noexcept,
                                                            void *context = nullptr ) noexcept
        {
            SettingEntry e;
            e.label = label;
            e.type = SettingType::action;
            e.act = act;
            e.context = context;
            return e;
        }

        std::string_view key;
        std::string_view label;
        SettingType type = SettingType::toggle;
        /// A number's range and step.
        double least = 0.0;
        double most = 1.0;
        double step = 1.0;
        /// A toggle's (0 or 1) or a number's default.
        double fallback = 0.0;
        /// A choice's: the choices, and the default (one of them).
        std::span<const std::string_view> choices;
        /// A choice's or a text's default.
        std::string_view text_fallback;
        /// An action's: called on the UI thread with `context`.
        void ( *act )( void *context ) noexcept = nullptr;
        void *context = nullptr;
};

/** @brief A section of the screen: its title and its entries, in order. */
struct SettingSection final
{
        std::string_view title;
        std::span<const SettingEntry> entries;
};

/**
 * @brief The sections squared gives every app; an app keeps the ones it
 * wants (a game's canvas, which never zooms, leaves Zoom out).
 *
 * - **Zoom**: the UI's zoom (the Stage's, kept as a pinch keeps it), Pinch
 *   to zoom (`ui.zoom.pinch`), and the range a pinch and the slider keep to,
 *   Least and Most (`ui.zoom.least`, `ui.zoom.most`).
 * - **Input**: Keyboard on start (`input.keyboard_on_start`, for the app to
 *   read at its start).
 * - **Preferences**: Reset all.
 */
enum class StandardSettings : std::uint8_t {
    none = 0,
    zoom = 1U << 0U,
    input = 1U << 1U,
    preferences = 1U << 2U,
    all = 0x07,
};

[[nodiscard]] constexpr StandardSettings operator|( StandardSettings a,
                                                    StandardSettings b ) noexcept
{
    return static_cast<StandardSettings>( static_cast<std::uint8_t>( a ) |
                                          static_cast<std::uint8_t>( b ) );
}

[[nodiscard]] constexpr bool has( StandardSettings set, StandardSettings one ) noexcept
{
    return ( static_cast<std::uint8_t>( set ) & static_cast<std::uint8_t>( one ) ) != 0;
}

/**
 * @brief An app's settings: its preferences, shown as sections on the
 * Settings screen, each change written at once (no Save) and applied live.
 * One an app, on the UI thread, made after its preferences are open and
 * living as long as the Stage shows the screen:
 *
 * ```cpp
 * static sq::Settings settings( app.stage, preferences );
 * settings.add( { "Terminal", terminal } );
 * settings.apply();                       // at each start
 * // a drawer's item, a button: push settings
 * ```
 *
 * The screen is `SettingsScreen` (`push settings`), a custom node the
 * Settings registers (`settings_view`).
 */
class Settings final
{
    public:
        /** @brief The most sections an app adds, besides squared's. */
        static constexpr std::size_t max_sections = 8;

        /** @brief The keys squared's sections keep. */
        static constexpr std::string_view zoom_pinch = "ui.zoom.pinch";
        static constexpr std::string_view zoom_least = "ui.zoom.least";
        static constexpr std::string_view zoom_most = "ui.zoom.most";
        static constexpr std::string_view keyboard_on_start = "input.keyboard_on_start";

        /**
         * @brief The settings of `stage`'s app, kept in `preferences`, with
         * squared's `standard` sections; registers the screen's view with
         * the Stage. One at a time: a second replaces the first's view.
         */
        Settings( Stage &stage, preferences::Preferences &preferences,
                  StandardSettings standard = StandardSettings::all ) noexcept;
        ~Settings();
        Settings( const Settings & ) = delete;
        Settings &operator=( const Settings & ) = delete;
        Settings( Settings && ) = delete;
        Settings &operator=( Settings && ) = delete;

        /** @brief Add the app's section, after the ones before it; false when there are `max_sections`. */
        bool add( SettingSection section ) noexcept;

        /**
         * @brief Apply the preferences squared's sections keep to the Stage:
         * the zoom's range, pinch to zoom. At each start, and after an
         * import (the Settings does it after its own changes).
         */
        void apply() noexcept;

        [[nodiscard]] StandardSettings standard() const noexcept { return standard_; }
        [[nodiscard]] std::span<const SettingSection> sections() const noexcept
        {
            return std::span( sections_ ).first( count_ );
        }
        [[nodiscard]] Stage &stage() noexcept { return stage_; }
        [[nodiscard]] preferences::Preferences &preferences() noexcept { return preferences_; }

        /** @brief The view of the screen: the sections, laid out to scroll. What the custom node makes. */
        [[nodiscard]] std::unique_ptr<gui::Widget> make_view();

        /** @brief The Settings whose view the screen shows; nullptr when none is made. */
        [[nodiscard]] static Settings *current() noexcept;

        /**
         * @brief Writes the preferences to a file, or reads them back from
         * one (preferences.md, "Export and import"); false when it failed.
         */
        using Transfer = bool ( * )( void *context, preferences::Preferences &preferences,
                                     std::string_view path ) noexcept;

        /**
         * @brief Export... and Import... in the Preferences section: each
         * opens the file dialog (sq.dialogs) where the person chooses the
         * file, then calls `export_to` or `import_from` with its path. Export
         * starts in shared storage with a name of the day
         * (`<app id>-preferences-YYYY-MM-DD.sq`, `-2` and on for another the
         * same day: none is overwritten unless the person says so); after an
         * import every setting is applied again and the screen shown again.
         * Until set, the preferences' own: `Preferences::export_to`, and
         * `import_from` replacing them whole. Null hides the button.
         */
        void set_transfer( Transfer export_to, Transfer import_from,
                           void *context = nullptr ) noexcept;

        /** @brief The name Export... starts with in `folder`: the day's, the first not there. */
        [[nodiscard]] std::string export_name( std::string_view folder ) const;

    private:
        static void exported( void *context, std::string_view path ) noexcept;
        static void imported( void *context, std::string_view path ) noexcept;
        void transfer_dialog( bool exporting );

        Stage &stage_;
        preferences::Preferences &preferences_;
        StandardSettings standard_;
        std::array<SettingSection, max_sections> sections_{};
        std::size_t count_ = 0;
        Transfer export_to_ = nullptr;
        Transfer import_from_ = nullptr;
        void *transfer_context_ = nullptr;
};

/**
 * @brief The Settings screen (`push settings`, a drawer's Settings): the
 * view the app's `Settings` makes, filling the screen.
 */
struct SettingsScreen final
{
        static constexpr std::string_view name = "settings";
        static void build( gui::LayoutWriter &root );
};

} // namespace sq
