// sq::Settings and the Settings screen's view (settings.cppm says what is here).

module sq.settings;

import std;
import sq.data;
import sq.dialogs;
import sq.files;
import sq.gui;
import sq.preferences;
import sq.stage;

namespace sq
{

namespace
{

/// The Settings whose view the screen shows: one an app.
Settings *the_settings = nullptr;

constexpr float padding = 16.0F;
constexpr float spacing = 10.0F;

/// What the screen's custom node makes: the current Settings' view.
std::unique_ptr<gui::Widget> make_settings_view( Stage & /*stage*/ )
{
    Settings *settings = Settings::current();
    return settings != nullptr ? settings->make_view() : nullptr;
}

/// A number as its entry shows it: whole when its step is.
std::string number_text( std::string_view label, double value, double step,
                         std::string_view unit = {} )
{
    if( step >= 1.0 ) {
        return std::format( "{}: {}{}", label, std::llround( value ), unit );
    }
    return std::format( "{}: {:.2f}{}", label, value, unit );
}

/// A text entry's field, finished: its entry's key written.
void text_committed( void *context, gui::TextField &field )
{
    Settings *settings = Settings::current();
    const auto *entry = static_cast<const SettingEntry *>( context );
    if( settings != nullptr && entry != nullptr ) {
        static_cast<void>(
            settings->preferences().set( entry->key, data::JsonValue( field.text() ) ) );
    }
}

/// The view's table, a row at a time: a label and its control, or one across both.
class Rows final
{
    public:
        explicit Rows( gui::Table &table ) noexcept
            : table_( table )
        {}

        void heading( std::string_view title )
        {
            table_.row();
            table_.add( std::make_unique<gui::Label>( std::string( title ) ) )
                .column_span( 2 )
                .fill_x()
                .pad( 6.0F );
        }

        template<typename W>
        W &across( std::unique_ptr<W> widget )
        {
            W &made = *widget;
            table_.row();
            table_.add( std::move( widget ) ).column_span( 2 ).fill_x();
            return made;
        }

        template<typename W>
        W &pair( std::unique_ptr<gui::Label> label, std::unique_ptr<W> control )
        {
            W &made = *control;
            table_.row();
            table_.add( std::move( label ) ).fill_x();
            table_.add( std::move( control ) ).grow_x().fill_x();
            return made;
        }

    private:
        gui::Table &table_;
};

} // namespace

Settings::Settings( Stage &stage, preferences::Preferences &preferences,
                    StandardSettings standard ) noexcept
    : stage_( stage )
    , preferences_( preferences )
    , standard_( standard )
{
    the_settings = this;
    static_cast<void>( stage_.custom( "settings_view", &make_settings_view ) );
}

Settings::~Settings()
{
    if( the_settings == this ) {
        the_settings = nullptr;
    }
}

Settings *Settings::current() noexcept
{
    return the_settings;
}

bool Settings::add( SettingSection section ) noexcept
{
    if( count_ == max_sections ) {
        return false;
    }
    sections_[count_++] = section;
    return true;
}

void Settings::apply() noexcept
{
    if( !has( standard_, StandardSettings::zoom ) ) {
        return;
    }
    const auto least = preferences_.get_integer( zoom_least, Stage::least_zoom );
    const auto most = preferences_.get_integer( zoom_most, Stage::most_zoom );
    stage_.set_zoom_range( static_cast<int>( std::clamp<std::int64_t>( least, 0, 1000 ) ),
                           static_cast<int>( std::clamp<std::int64_t>( most, 0, 1000 ) ) );
    stage_.pinch_zoom( preferences_.get_bool( zoom_pinch, true ) );
}

std::unique_ptr<gui::Widget> Settings::make_view()
{
    auto table = std::make_unique<gui::Table>();
    table->set_padding( padding );
    table->set_spacing( spacing );
    Rows rows( *table );

    if( has( standard_, StandardSettings::zoom ) ) {
        rows.heading( "Zoom" );
        // The zoom is the Stage's (kept as a pinch keeps it); the rest are
        // the preferences', applied as they change.
        auto zoom_text =
            std::make_unique<gui::Label>( number_text( "Zoom", stage_.zoom(), 1.0, "%" ) );
        gui::Label *zoom_label = zoom_text.get();
        auto &zoom =
            rows.pair( std::move( zoom_text ),
                       std::make_unique<gui::Slider>( static_cast<float>( stage_.zoom_least() ),
                                                      static_cast<float>( stage_.zoom_most() ),
                                                      static_cast<float>( stage_.zoom() ) ) );
        zoom.set_step( static_cast<float>( Stage::zoom_step ) );
        zoom.set_on_change( [this, zoom_label]( float value ) {
            stage_.set_zoom( static_cast<int>( std::lround( value ) ) );
            zoom_label->set_text( number_text( "Zoom", stage_.zoom(), 1.0, "%" ) );
        } );

        auto &pinch =
            rows.across( std::make_unique<gui::CheckBox>( "Pinch to zoom", stage_.pinch_zooms() ) );
        pinch.set_on_change( [this]( bool on ) {
            static_cast<void>( preferences_.set( zoom_pinch, data::JsonValue( on ) ) );
            apply();
        } );

        // Least and Most: the range; the zoom's slider keeps to it.
        const auto range_row = [this, &rows, &zoom, zoom_label](
                                   std::string_view label, std::string_view key, bool least ) {
            const int now = least ? stage_.zoom_least() : stage_.zoom_most();
            auto shows = std::make_unique<gui::Label>( number_text( label, now, 1.0, "%" ) );
            gui::Label *text = shows.get();
            auto &slider =
                rows.pair( std::move( shows ),
                           std::make_unique<gui::Slider>( static_cast<float>( Stage::least_zoom ),
                                                          static_cast<float>( Stage::most_zoom ),
                                                          static_cast<float>( now ) ) );
            slider.set_step( static_cast<float>( Stage::zoom_step ) );
            gui::Slider *shown = &zoom;
            slider.set_on_change(
                [this, label, key, least, text, shown, zoom_label]( float value ) {
                    // Never past the other end: least at most the most, and the other way.
                    int percent = static_cast<int>( std::lround( value ) );
                    percent = least ? std::min( percent, stage_.zoom_most() )
                                    : std::max( percent, stage_.zoom_least() );
                    static_cast<void>( preferences_.set(
                        key, data::JsonValue( static_cast<std::int64_t>( percent ) ) ) );
                    apply();
                    text->set_text( number_text( label, percent, 1.0, "%" ) );
                    shown->set_range( static_cast<float>( stage_.zoom_least() ),
                                      static_cast<float>( stage_.zoom_most() ) );
                    shown->set_value( static_cast<float>( stage_.zoom() ) );
                    zoom_label->set_text( number_text( "Zoom", stage_.zoom(), 1.0, "%" ) );
                } );
        };
        range_row( "Least", zoom_least, true );
        range_row( "Most", zoom_most, false );
    }

    if( has( standard_, StandardSettings::input ) ) {
        rows.heading( "Input" );
        auto &keyboard = rows.across( std::make_unique<gui::CheckBox>(
            "Keyboard on start", preferences_.get_bool( keyboard_on_start, false ) ) );
        keyboard.set_on_change( [this]( bool on ) {
            static_cast<void>( preferences_.set( keyboard_on_start, data::JsonValue( on ) ) );
        } );
    }

    for( const SettingSection &section : sections() ) {
        rows.heading( section.title );
        for( const SettingEntry &entry : section.entries ) {
            switch( entry.type ) {
                case SettingType::toggle: {
                    auto &box = rows.across( std::make_unique<gui::CheckBox>(
                        std::string( entry.label ),
                        preferences_.get_bool( entry.key, entry.fallback != 0.0 ) ) );
                    box.set_on_change( [this, key = entry.key]( bool on ) {
                        static_cast<void>( preferences_.set( key, data::JsonValue( on ) ) );
                    } );
                    break;
                }
                case SettingType::number: {
                    const double value =
                        std::clamp( preferences_.get_number( entry.key, entry.fallback ),
                                    entry.least, entry.most );
                    auto shows = std::make_unique<gui::Label>(
                        number_text( entry.label, value, entry.step ) );
                    gui::Label *text = shows.get();
                    auto &slider =
                        rows.pair( std::move( shows ),
                                   std::make_unique<gui::Slider>( static_cast<float>( entry.least ),
                                                                  static_cast<float>( entry.most ),
                                                                  static_cast<float>( value ) ) );
                    slider.set_step( static_cast<float>( entry.step ) );
                    slider.set_on_change( [this, &entry, text]( float now ) {
                        const double kept =
                            entry.step >= 1.0 ? std::round( now ) : static_cast<double>( now );
                        if( entry.step >= 1.0 ) {
                            static_cast<void>( preferences_.set(
                                entry.key, data::JsonValue( static_cast<std::int64_t>( kept ) ) ) );
                        } else {
                            static_cast<void>(
                                preferences_.set( entry.key, data::JsonValue( kept ) ) );
                        }
                        text->set_text( number_text( entry.label, kept, entry.step ) );
                    } );
                    break;
                }
                case SettingType::choice: {
                    const std::string now = preferences_.get_text( entry.key, entry.text_fallback );
                    auto &button =
                        rows.pair( std::make_unique<gui::Label>( std::string( entry.label ) ),
                                   std::make_unique<gui::Button>( now ) );
                    gui::Button *shown = &button;
                    button.set_on_click( [this, &entry, shown] {
                        if( entry.choices.empty() ) {
                            return;
                        }
                        // The next choice after the one kept, round to the first.
                        const std::string kept =
                            preferences_.get_text( entry.key, entry.text_fallback );
                        const auto at =
                            std::ranges::find( entry.choices, std::string_view( kept ) );
                        const auto next =
                            ( at == entry.choices.end() || at + 1 == entry.choices.end() )
                                ? entry.choices.begin()
                                : at + 1;
                        static_cast<void>(
                            preferences_.set( entry.key, data::JsonValue( *next ) ) );
                        shown->set_text( std::string( *next ) );
                    } );
                    break;
                }
                case SettingType::text: {
                    auto &field =
                        rows.pair( std::make_unique<gui::Label>( std::string( entry.label ) ),
                                   std::make_unique<gui::TextField>(
                                       preferences_.get_text( entry.key, entry.text_fallback ) ) );
                    // The callback's context is void *; text_committed only reads the entry.
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                    field.set_on_commit( &text_committed, const_cast<SettingEntry *>( &entry ) );
                    break;
                }
                case SettingType::action: {
                    auto &button =
                        rows.across( std::make_unique<gui::Button>( std::string( entry.label ) ) );
                    button.set_on_click( [act = entry.act, context = entry.context] {
                        if( act != nullptr ) {
                            act( context );
                        }
                    } );
                    break;
                }
            }
        }
    }

    if( has( standard_, StandardSettings::preferences ) ) {
        rows.heading( "Preferences" );
        if( export_to_ != nullptr ) {
            auto &out = rows.across( std::make_unique<gui::Button>( "Export..." ) );
            out.set_on_click( [this] { transfer_dialog( true ); } );
        }
        if( import_from_ != nullptr ) {
            auto &in = rows.across( std::make_unique<gui::Button>( "Import..." ) );
            in.set_on_click( [this] { transfer_dialog( false ); } );
        }
        auto &reset = rows.across( std::make_unique<gui::Button>( "Reset all" ) );
        reset.set_on_click( [this] {
            static_cast<void>( preferences_.reset_all() );
            apply();
            // Shown again from the defaults.
            static_cast<void>( stage_.replace<SettingsScreen>() );
        } );
    }

    auto pane = std::make_unique<gui::ScrollPane>();
    pane->set_content( std::move( table ) );
    return pane;
}

void Settings::set_transfer( Transfer export_to, Transfer import_from, void *context ) noexcept
{
    export_to_ = export_to;
    import_from_ = import_from;
    transfer_context_ = context;
}

std::string Settings::export_name( std::string_view folder ) const
{
    // The app's id: the cartridge's, without "preferences.".
    std::string_view app = preferences_.id();
    if( app.starts_with( "preferences." ) ) {
        app.remove_prefix( std::string_view( "preferences." ).size() );
    }
    const std::chrono::year_month_day day{ std::chrono::floor<std::chrono::days>(
        std::chrono::system_clock::now() ) };
    const std::string stem = std::format( "{}-preferences-{:%Y-%m-%d}", app, day );
    for( int n = 1;; ++n ) {
        std::string name = n == 1 ? stem + ".sq" : std::format( "{}-{}.sq", stem, n );
        std::error_code ec;
        if( folder.empty() ||
            !std::filesystem::exists( std::filesystem::path( folder ) / name, ec ) ) {
            return name;
        }
    }
}

void Settings::transfer_dialog( bool exporting )
{
    FileDialogOptions options;
    options.mode = exporting ? FileDialogMode::save : FileDialogMode::open;
    options.title = exporting ? "Export preferences" : "Import preferences";
    options.extensions = { ".sq" };
    // Shared storage, where they travel from (as sqterm's packages do), when there is one.
    if( files::FileSystem *fs = stage_.files(); fs != nullptr ) {
        options.folder = fs->resolve_platform_path( files::FileType::External, "" );
    }
    if( exporting ) {
        options.name = export_name( options.folder );
    }
    options.chosen = exporting ? &Settings::exported : &Settings::imported;
    options.context = this;
    static_cast<void>( FileDialog::show( stage_, std::move( options ) ) );
}

void Settings::exported( void *context, std::string_view path ) noexcept
{
    auto *settings = static_cast<Settings *>( context );
    if( settings == nullptr || path.empty() || settings->export_to_ == nullptr ) {
        return; // cancelled
    }
    static_cast<void>(
        settings->export_to_( settings->transfer_context_, settings->preferences_, path ) );
}

void Settings::imported( void *context, std::string_view path ) noexcept
{
    auto *settings = static_cast<Settings *>( context );
    if( settings == nullptr || path.empty() || settings->import_from_ == nullptr ) {
        return; // cancelled
    }
    if( settings->import_from_( settings->transfer_context_, settings->preferences_, path ) ) {
        settings->apply(); // every setting again
        static_cast<void>( settings->stage_.replace<SettingsScreen>() );
    }
}

void SettingsScreen::build( gui::LayoutWriter &root )
{
    root.add_custom( "settings_view" ).grow();
}

} // namespace sq
