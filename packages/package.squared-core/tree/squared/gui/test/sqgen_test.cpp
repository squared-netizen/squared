// sqgen end to end (7.0b): the Makefile ran sqgen on test/sqgen/ and compiled
// this with what it wrote. The keys have their types; the layouts are found by
// name, check, build, and equal the tables the C++ writer makes. Then sqgen is
// run again: on unchanged input it writes nothing, and on the wrong files in
// test/sqgen/bad/ and keyword/ it prints what each expected.txt holds, writes
// nothing, and fails.

#include <squared/gui/button.hpp>
#include <squared/gui/check_box.hpp>
#include <squared/gui/image.hpp>
#include <squared/gui/instantiate.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/layout_writer.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/scroll_pane.hpp>
#include <squared/gui/separator.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/stack.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/text_field.hpp>
#include <squared/messaging/key.hpp>

#include <sys/wait.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <layouts.hpp>
#include <names.hpp>
#include <string>
#include <string_view>
#include <type_traits>

using namespace sq::gui;
namespace fs = std::filesystem;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "sqgen_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );             \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

template<typename K>
using value_of = typename std::remove_cvref_t<K>::value_type;

// The keys, as names.json declares them.
static_assert( std::is_same_v<value_of<decltype( names::counter_ticks )>, std::int64_t> );
static_assert( std::is_same_v<value_of<decltype( names::settings_sound )>, bool> );
static_assert( std::is_same_v<value_of<decltype( names::settings_volume )>, double> );
static_assert( std::is_same_v<value_of<decltype( names::settings_name )>, std::string_view> );
static_assert( std::is_same_v<value_of<decltype( names::player_pos )>, std::array<float, 2>> );
static_assert( std::is_same_v<value_of<decltype( names::game_start )>, void> );
constexpr auto state_set = sq::messaging::state | sq::messaging::set;
static_assert( decltype( names::settings_sound )::verbs == state_set );
static_assert( decltype( names::file_open )::verbs == sq::messaging::do_ );
static_assert( decltype( names::item_name )::takes_uid );
static_assert( !decltype( names::counter_ticks )::takes_uid );
// A list is its rows' key (an int taking a uid) and its count.
static_assert( decltype( names::inventory )::takes_uid );
static_assert( std::is_same_v<value_of<decltype( names::inventory_count )>, std::int64_t> );
static_assert( names::inventory_count.name() == sq::messaging::Name( "inventory.count" ) );

void layouts_found()
{
    const auto all = layouts::all();
    CHECK( all.size() == 3 );
    CHECK( all[0] == &layouts::discard );
    CHECK( all[1] == &layouts::landing );
    CHECK( all[2] == &layouts::settings );
    for( const Layout *layout : all ) {
        CHECK( layouts::find( layout->name() ) == layout );
        CHECK( check( *layout ).ok() );
    }
    CHECK( layouts::find( "nothing" ) == nullptr && layouts::find( "" ) == nullptr );
    CHECK( layouts::discard.kind() == LayoutKind::pane );
    CHECK( layouts::settings.kind() == LayoutKind::screen );
    CHECK( layouts::landing.text( layouts::landing.nodes()[0].text ) == "Hello \"world\"" );
    CHECK( layouts::discard.text( layouts::discard.nodes()[0].text ) == "Discard?" );
}

void settings_is_the_writers_table()
{
    std::array<LayoutNode, 32> nodes{};
    std::array<char, 1024> text{};
    std::array<float, 16> numbers{};
    LayoutWriter root( nodes, text, numbers, "settings" );
    root.add<CheckBox>( "Sound" ).bind( names::settings_sound );
    root.row();
    root.add<Slider>().fill_x().range( 0.0F, 11.0F ).step( 0.5F ).bind( names::settings_volume );
    root.row();
    root.add<TextField>().pad( 4.0F ).bind( names::settings_name );
    root.row();
    NodeWriter table = root.add<Table>();
    table.add<Button>( "Mute" ).tap( names::settings_sound, false );
    table.add<Button>( "Loud" ).tap( names::settings_volume, 11.0 );
    table.row();
    table.add<Separator>().vertical();
    table.add<ProgressBar>().range( 0.0F, 11.0F ).bind( names::settings_volume );
    root.row();
    root.add<ScrollPane>().add<Label>( "long" );
    NodeWriter stack = root.add<Stack>();
    stack.add<Image>( "white" );
    stack.add_custom( "minimap" );
    root.row();
    NodeWriter discard = root.add<Button>( "Discard" );
    discard.align( Alignment::end, Alignment::start ).tap( Navigate::open, "discard" );
    root.add<Button>( "Back" ).tap( Navigate::back );
    CHECK( root.complete() );

    const Layout written = root.layout();
    const Layout &compiled = layouts::settings;
    CHECK( written.nodes().size() == compiled.nodes().size() );
    for( std::size_t i = 0; i < written.nodes().size(); ++i ) {
        CHECK( written.nodes()[i] == compiled.nodes()[i] );
    }
    CHECK( written.text() == compiled.text() );
    CHECK( written.numbers().size() == compiled.numbers().size() );
    for( std::size_t i = 0; i < written.numbers().size(); ++i ) {
        CHECK( written.numbers()[i] == compiled.numbers()[i] );
    }

    // And it builds: the skin has no drawable "white", the first thing short.
    Table built;
    const LayoutCheck short_of{ LayoutProblem::unknown_drawable, 11 };
    CHECK( instantiate( compiled, built, Skin() ) == short_of );
    CHECK( built.child_count() == 8 );
}

/// Run sqgen with `arguments`; its exit status, and what it printed.
int run( const std::string &arguments, std::string &printed )
{
    const std::string command = std::string( SQGEN_PATH ) + " " + arguments + " 2>&1";
    FILE *pipe = popen( command.c_str(), "r" );
    CHECK( pipe != nullptr );
    std::array<char, 512> buffer{};
    printed.clear();
    while( std::fgets( buffer.data(), static_cast<int>( buffer.size() ), pipe ) != nullptr ) {
        printed += buffer.data();
    }
    const int status = pclose( pipe );
    return WIFEXITED( status ) ? WEXITSTATUS( status ) : -1;
}

/// A file's whole text.
std::string text_of( const fs::path &path )
{
    std::ifstream in( path, std::ios::binary );
    CHECK( in.is_open() );
    return { std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>() };
}

/// Run sqgen on the wrong files in test/sqgen/`name`/; it prints exactly what
/// their expected.txt holds, writes nothing, and fails.
void refuses( const fs::path &home, const std::string &name, bool layouts )
{
    const fs::path wrong = fs::path( SQGEN_FIXTURE ) / name;
    std::string arguments = "--names " + ( wrong / "names.json" ).string();
    if( layouts ) {
        arguments += " --layouts " + ( wrong / "layouts" ).string();
    }
    arguments += " --out " + ( home / "out" ).string();
    std::string printed;
    CHECK( run( arguments, printed ) == 1 );
    const std::string expected = text_of( wrong / "expected.txt" );
    if( printed != expected ) {
        std::fprintf( stderr, "sqgen on %s printed:\n%s", name.c_str(), printed.c_str() );
    }
    CHECK( printed == expected );
    CHECK( !fs::exists( home / "out" ) );
}

void runs_again()
{
    const char *tmp = std::getenv( "TMPDIR" );
    std::string place = std::string( tmp != nullptr ? tmp : "/tmp" ) + "/sqgen_test_XXXXXX";
    CHECK( mkdtemp( place.data() ) != nullptr );
    const fs::path home( place );
    const fs::path fixture( SQGEN_FIXTURE );
    std::string good = "--names " + ( fixture / "names.json" ).string();
    good += " --layouts " + ( fixture / "layouts" ).string();
    good += " --out " + ( home / "gen" ).string();

    // Unchanged input: the second run writes nothing, so nothing rebuilds.
    std::string printed;
    CHECK( run( good, printed ) == 0 );
    CHECK( printed.find( "11 keys, 3 layouts; 3 files changed" ) != std::string::npos );
    CHECK( run( good, printed ) == 0 );
    CHECK( printed.find( "0 files changed" ) != std::string::npos );

    // Wrong input: every problem, one line each; nothing written; exit 1.
    refuses( home, "bad", true );
    refuses( home, "keyword", false ); // a name that becomes a keyword

    // A file or directory that is not there.
    const std::string out = " --out " + ( home / "out" ).string();
    CHECK( run( "--names " + ( home / "none.json" ).string() + out, printed ) == 1 );
    CHECK( printed.ends_with( "none.json could not be read\n" ) );
    CHECK( run( "--layouts " + ( home / "none" ).string() + out, printed ) == 1 );
    CHECK( printed.ends_with( "none is not a directory\n" ) );
    CHECK( !fs::exists( home / "out" ) );

    CHECK( run( "--layouts x", printed ) == 2 ); // no --out
    fs::remove_all( home );
}

} // namespace

int main()
{
    layouts_found();
    settings_is_the_writers_table();
    runs_again();
    std::printf( "sqgen_test: ok\n" );
    return 0;
}
