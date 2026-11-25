// sq.preferences against a real directory under $TMPDIR: made when missing,
// kept across opens, typed settings with their fallbacks, the app's own
// files, names that would leave the tree, and a cartridge of another kind.

#include <cassert>
#include <cstdio>
#include <cstdlib>

import std;
import sq.data;
import sq.files;
import sq.preferences;

using sq::files::FileType;
using sq::preferences::Preferences;
using sq::preferences::PreferencesErrorCode;

namespace
{

std::string text( const std::vector<std::byte> &bytes )
{
    return { reinterpret_cast<const char *>( bytes.data() ), bytes.size() };
}

std::span<const std::byte> bytes( std::string_view s )
{
    return std::as_bytes( std::span( s.data(), s.size() ) );
}

} // namespace

int main()
{
    const char *tmp = std::getenv( "TMPDIR" );
    const std::string base = std::string( tmp != nullptr ? tmp : "/tmp" ) +
                             "/sq-preferences-test-" + std::to_string( std::random_device{}() );
    std::filesystem::create_directories( base + "/local" );
    sq::files::PosixFileSystem fs{ sq::files::PosixFileSystemRoots{
        .internal_root = base + "/internal", .local_root = base + "/local", .external_root = {} } };

    // Made, empty, when it is not there: the manifest sqcart 3.0 reads.
    {
        auto p = Preferences::open( fs, "dev.squared.test" );
        assert( p );
        assert( p->id() == "preferences.dev.squared.test" );
        assert( p->keys().empty() );
        const auto manifest = fs.read( FileType::Local, "preferences/SQ-INF/manifest.json" );
        assert( manifest );
        const auto m = sq::data::parse_json( text( manifest.bytes ) );
        assert( m && *m.value.find( "kind" )->string_if() == "preferences" );
        assert( *m.value.find( "id" )->string_if() == "preferences.dev.squared.test" );
        assert( *m.value.find( "tree" )->string_if() == "tree/" );
        assert( fs.is_directory( FileType::Local, "preferences/tree" ) );

        // Typed settings, each written at once; the fallbacks when missing
        // or of another type.
        assert( p->get_integer( "ui.font.size", 14 ) == 14 );
        const std::uint64_t before = p->generation();
        assert( !p->set( "ui.font.size", sq::data::JsonValue( std::int64_t{ 18 } ) ) );
        assert( !p->set( "ui.zoom.pinch", sq::data::JsonValue( false ) ) );
        assert( !p->set( "ui.skin", sq::data::JsonValue( "cyberpunk" ) ) );
        assert( !p->set( "ui.zoom", sq::data::JsonValue( 1.25 ) ) );
        assert( p->generation() == before + 4 );
        assert( p->get_integer( "ui.font.size", 14 ) == 18 );
        assert( !p->get_bool( "ui.zoom.pinch", true ) );
        assert( p->get_text( "ui.skin", "default" ) == "cyberpunk" );
        assert( p->get_number( "ui.zoom", 1.0 ) == 1.25 );
        assert( p->get_number( "ui.font.size", 0.0 ) == 18.0 );    // a whole number as a number
        assert( p->get_integer( "ui.zoom", 0 ) == 1 );             // a real one rounded toward zero
        assert( p->get_text( "ui.font.size", "none" ) == "none" ); // another type: the fallback
        assert( p->set( "", sq::data::JsonValue( true ) ).code ==
                PreferencesErrorCode::invalid_key );

        // The app's own files, any type, folders made; names kept in the tree.
        const std::array<std::byte, 4> font{ std::byte{ 0 }, std::byte{ 1 }, std::byte{ 0 },
                                             std::byte{ 0 } };
        assert( !p->write( "fonts/Mine.ttf", font ) );
        assert( !p->write( "theme.json", bytes( R"({"background": "#0b0719"})" ) ) );
        const auto read = p->read( "fonts/Mine.ttf" );
        assert( read && read.bytes.size() == 4 && read.bytes[1] == std::byte{ 1 } );
        const auto top = p->list();
        assert( ( top == std::vector<std::string>{ "fonts", "settings.json", "theme.json" } ) );
        assert( ( p->list( "fonts" ) == std::vector<std::string>{ "Mine.ttf" } ) );
        for( const std::string_view bad :
             { "../escape", "/abs", "a/../b", "SQ-INF/manifest.json", "fonts//x", "", "a\\b" } ) {
            assert( p->path( bad ).empty() );
            assert( p->write( bad, font ).code == PreferencesErrorCode::invalid_name );
        }
        assert( p->path( "skins/mine/skin.json" ) == "preferences/tree/skins/mine/skin.json" );
        assert( !p->remove( "theme.json" ) );
        assert( !p->read( "theme.json" ) );
    }

    // Kept: opened again, the settings and the files are there.
    {
        auto p = Preferences::open( fs, "dev.squared.test" );
        assert( p );
        assert( p->get_integer( "ui.font.size", 14 ) == 18 );
        assert( ( p->keys() == std::vector<std::string>{ "ui.font.size", "ui.skin", "ui.zoom",
                                                         "ui.zoom.pinch" } ) );
        assert( !p->reset( "ui.skin" ) );
        assert( p->get_text( "ui.skin", "default" ) == "default" );
        assert( !p->reset( "not.there" ) ); // nothing to forget is no error
        assert( !p->reset_all() );
        assert( p->keys().empty() );
        assert( p->read( "fonts/Mine.ttf" ) ); // reset_all keeps the files
    }
    {
        auto p = Preferences::open( fs, "dev.squared.test" );
        assert( p && p->keys().empty() );
    }

    // Another app's id, as an import keeps it: opened as it is.
    {
        assert( !fs.make_directories( FileType::Local, "other/SQ-INF" ) );
        assert( !fs.write_atomically(
            FileType::Local, "other/SQ-INF/manifest.json",
            bytes(
                R"({"format":"squared-cartridge","format_version":3,"kind":"preferences",)"
                R"("id":"preferences.dev.squared.sqterm","version":"1.0.0","tree":"tree/"})" ) ) );
        const auto other = Preferences::open( fs, "dev.squared.test", "other" );
        assert( other && other->id() == "preferences.dev.squared.sqterm" && other->keys().empty() );
    }

    // A cartridge of another kind, or settings that are not an object: refused.
    {
        assert( !fs.make_directories( FileType::Local, "kit/SQ-INF" ) );
        assert( !fs.write_atomically(
            FileType::Local, "kit/SQ-INF/manifest.json",
            bytes(
                R"({"format":"squared-cartridge","format_version":3,"kind":"kit","id":"kit.x","version":"1.0.0"})" ) ) );
        const auto kit = Preferences::open( fs, "dev.squared.test", "kit" );
        assert( !kit && kit.error().code == PreferencesErrorCode::not_preferences );

        assert( !fs.make_directories( FileType::Local, "broken/tree" ) );
        auto made = Preferences::open( fs, "dev.squared.test", "broken" );
        assert( made );
        assert( !fs.write_atomically( FileType::Local, "broken/tree/settings.json",
                                      bytes( "[1, 2]" ) ) );
        const auto broken = Preferences::open( fs, "dev.squared.test", "broken" );
        assert( !broken && broken.error().code == PreferencesErrorCode::invalid_settings );
    }

    // Exported and imported back: one cartridge, settings and files.
    {
        const std::filesystem::path cartridge = base + "/shared/backup.sq";
        std::filesystem::create_directories( base + "/shared" );
        auto p = Preferences::open( fs, "dev.squared.test" );
        assert( p );
        assert( !p->set( "ui.skin", sq::data::JsonValue( "cyberpunk" ) ) );
        assert( !p->set( "ui.zoom", sq::data::JsonValue( 1.5 ) ) );
        assert( !p->write( "skins/mine.json", bytes( "{}" ) ) );
        assert( !p->export_to( cartridge ) );
        assert( std::filesystem::is_regular_file( cartridge ) );
        assert( !std::filesystem::exists( base + "/shared/backup.sq.part" ) );
        // One there already: replaced, whole.
        assert( !p->export_to( cartridge ) );

        // Changed after: replaced, the cartridge's settings and files come back.
        assert( !p->set( "ui.skin", sq::data::JsonValue( "plain" ) ) );
        assert( !p->set( "ui.font.size", sq::data::JsonValue( std::int64_t{ 20 } ) ) );
        assert( !p->write( "later.txt", bytes( "later" ) ) );
        const std::uint64_t before = p->generation();
        assert( !p->import_from( cartridge ) );
        assert( p->generation() > before );
        assert( p->get_text( "ui.skin", "" ) == "cyberpunk" );
        assert( p->get_number( "ui.zoom", 0.0 ) == 1.5 );
        assert( p->find( "ui.font.size" ) == nullptr );
        assert( text( p->read( "skins/mine.json" ).bytes ) == "{}" );
        assert( !p->read( "later.txt" ) );
        assert( !std::filesystem::exists( base + "/local/preferences.import" ) );
        assert( !std::filesystem::exists( base + "/local/preferences.old" ) );

        // Merged: its settings and files over these, the others kept.
        assert( !p->set( "ui.skin", sq::data::JsonValue( "plain" ) ) );
        assert( !p->set( "ui.font.size", sq::data::JsonValue( std::int64_t{ 20 } ) ) );
        assert( !p->write( "later.txt", bytes( "later" ) ) );
        assert( !p->import_from( cartridge, sq::preferences::ImportMode::merge ) );
        assert( p->get_text( "ui.skin", "" ) == "cyberpunk" );
        assert( p->get_integer( "ui.font.size", 0 ) == 20 );
        assert( p->read( "later.txt" ) && p->read( "skins/mine.json" ) );

        // As it reads when opened again.
        auto again = Preferences::open( fs, "dev.squared.test" );
        assert( again && again->get_integer( "ui.font.size", 0 ) == 20 &&
                again->get_text( "ui.skin", "" ) == "cyberpunk" );

        // Another app's preferences, and what is no cartridge: refused, nothing changed.
        auto other = Preferences::open( fs, "dev.squared.other", "theirs" );
        assert( other && !other->export_to( base + "/shared/theirs.sq" ) );
        assert( p->import_from( base + "/shared/theirs.sq" ).code ==
                PreferencesErrorCode::another_app );
        std::filesystem::path junk = base + "/shared/junk.sq";
        std::ofstream( junk ) << "not a cartridge";
        assert( p->import_from( junk ).code == PreferencesErrorCode::cartridge );
        assert( p->get_integer( "ui.font.size", 0 ) == 20 && p->read( "later.txt" ) );
        assert( !std::filesystem::exists( base + "/local/preferences.import" ) );
    }

    std::filesystem::remove_all( base );
    std::printf( "preferences: all assertions passed\n" );
    return 0;
}
