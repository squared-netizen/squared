// sq::Settings without a window: the preferences squared's sections keep,
// applied to the Stage (the zoom's range, pinch to zoom); an app's sections;
// the view made from them; the one whose view the screen shows.

#include <stdio.h>

import sq.app;
import sq.data;
import sq.files;
import sq.gui;
import sq.preferences;
import sq.settings;
import sq.stage;

import std;

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "settings_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );          \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

sq::App the_app;

int acted = 0;
void act( void *context ) noexcept
{
    acted += *static_cast<int *>( context );
}

} // namespace

void squared_main( sq::App & /*app*/ )
{}

int main()
{
    const char *tmp = std::getenv( "TMPDIR" );
    const std::string base = std::string( tmp != nullptr ? tmp : "/tmp" ) + "/sq-settings-test-" +
                             std::to_string( std::random_device{}() );
    std::filesystem::create_directories( base + "/local" );
    sq::files::PosixFileSystem fs{ sq::files::PosixFileSystemRoots{
        .internal_root = base + "/internal", .local_root = base + "/local", .external_root = {} } };
    auto opened = sq::preferences::Preferences::open( fs, "dev.squared.test" );
    CHECK( opened.has_value() );
    sq::preferences::Preferences &preferences = *opened;
    sq::Stage &stage = the_app.stage;

    {
        sq::Settings settings( stage, preferences );
        CHECK( sq::Settings::current() == &settings );

        // Nothing kept: squared's limits, pinch on.
        settings.apply();
        CHECK( stage.zoom_least() == sq::Stage::least_zoom &&
               stage.zoom_most() == sq::Stage::most_zoom );
        CHECK( stage.pinch_zooms() );

        // Kept: applied, as at a start.
        CHECK( !preferences.set( sq::Settings::zoom_least,
                                 sq::data::JsonValue( std::int64_t{ 75 } ) ) );
        CHECK( !preferences.set( sq::Settings::zoom_most,
                                 sq::data::JsonValue( std::int64_t{ 200 } ) ) );
        CHECK( !preferences.set( sq::Settings::zoom_pinch, sq::data::JsonValue( false ) ) );
        settings.apply();
        CHECK( stage.zoom_least() == 75 && stage.zoom_most() == 200 && !stage.pinch_zooms() );

        // An app's sections, to the most.
        static constexpr std::array<std::string_view, 3> bells{ "off", "flash", "vibrate" };
        static int one = 1;
        static const std::array entries{
            sq::SettingEntry::choice( "terminal.bell", "Bell", bells, "flash" ),
            sq::SettingEntry::number( "terminal.scrollback", "Scrollback", 100, 100000, 100, 5000 ),
            sq::SettingEntry::text( "terminal.shell", "Shell", "zsh" ),
            sq::SettingEntry::toggle( "terminal.blink", "Blink" ),
            sq::SettingEntry::action( "Clear history", &act, &one ),
        };
        CHECK( entries[1].type == sq::SettingType::number && entries[1].most == 100000.0 );
        CHECK( entries[4].act == &act && entries[4].key.empty() );
        entries[4].act( entries[4].context );
        CHECK( acted == 1 );
        for( std::size_t i = 0; i < sq::Settings::max_sections; ++i ) {
            CHECK( settings.add( { "Terminal", entries } ) );
        }
        CHECK( !settings.add( { "One too many", entries } ) );
        CHECK( settings.sections().size() == sq::Settings::max_sections );

        // The view: made from all of it, a scroll of the sections.
        const std::unique_ptr<sq::gui::Widget> view = settings.make_view();
        CHECK( view != nullptr );
        CHECK( view->actor_interface( sq::gui::ScrollPane::actor_interface_id ) != nullptr );

        // Only what the app keeps: none of squared's, none of its own.
        sq::Settings bare( stage, preferences, sq::StandardSettings::none );
        CHECK( sq::Settings::current() == &bare );
        CHECK( bare.make_view() != nullptr );
        bare.apply(); // no Zoom section: the Stage is left as it is
        CHECK( stage.zoom_least() == 75 && !stage.pinch_zooms() );

        // Export and import: the preferences' own until the app sets its
        // own (null hides them); Export's name is the day's, the first not
        // in the folder.
        const auto never = []( void *, sq::preferences::Preferences &, std::string_view ) noexcept {
            return false;
        };
        bare.set_transfer( nullptr, nullptr );
        CHECK( bare.make_view() != nullptr );
        bare.set_transfer( never, never );
        CHECK( bare.make_view() != nullptr );
        const std::string name = bare.export_name( base );
        CHECK( name.starts_with( "dev.squared.test-preferences-2" ) && name.ends_with( ".sq" ) &&
               name.size() ==
                   std::string_view( "dev.squared.test-preferences-YYYY-MM-DD.sq" ).size() );
        std::ofstream( base + "/" + name ) << "x";
        CHECK( bare.export_name( base ) == name.substr( 0, name.size() - 3 ) + "-2.sq" );
        CHECK( bare.export_name( "" ) == name );
    }
    // Gone with the last one made.
    CHECK( sq::Settings::current() == nullptr );

    // Reset: squared's limits again.
    CHECK( !preferences.reset_all() );
    {
        sq::Settings settings( stage, preferences,
                               sq::StandardSettings::zoom | sq::StandardSettings::input );
        settings.apply();
        CHECK( stage.zoom_least() == sq::Stage::least_zoom &&
               stage.zoom_most() == sq::Stage::most_zoom );
        CHECK( stage.pinch_zooms() );
    }

    std::filesystem::remove_all( base );
    std::puts( "settings_test: ok" );
    return 0;
}
