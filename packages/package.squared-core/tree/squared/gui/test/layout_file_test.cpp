// Layout files and names.json (7.0b): what the reader accepts, what it
// refuses and how it says so (the file, then a line and column for a syntax
// error or the path to the value for the rest), and that a file gives the
// same table the C++ writer does.
//
// The cases are in layout_file_cases.txt, one per line: a kind, the JSON, and
// the problem expected. Every case runs; each one that fails is printed.

#include <squared/gui/button.hpp>
#include <squared/gui/check_box.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/layout_file.hpp>
#include <squared/gui/layout_writer.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/table.hpp>
#include <squared/messaging/key.hpp>

#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

using namespace sq::gui;
namespace data = sq::data;
using sq::messaging::do_;
using sq::messaging::Key;
using sq::messaging::set;
using sq::messaging::state;
using sq::messaging::with_uid;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "layout_file_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );       \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

constexpr std::string_view names_json = R"({
  "names": {
    "counter.ticks":   { "type": "int",    "verbs": ["state"], "note": "Ticks" },
    "settings.sound":  { "type": "bool",   "verbs": ["state", "set"] },
    "settings.volume": { "type": "number", "verbs": ["state", "set"] },
    "settings.name":   { "type": "text",   "verbs": ["state", "set"] },
    "file.open":       { "type": "text",   "verbs": ["do"] },
    "game.start":      {                   "verbs": ["do"] },
    "player.pos":      { "type": "vec2",   "verbs": ["state"] },
    "item.name":       { "type": "text",   "verbs": ["state"], "uid": true },
    "item.count":      { "type": "int",    "verbs": ["state", "set"], "uid": true },
    "item.use":        {                   "verbs": ["do"], "uid": true },
    "other.name":      { "type": "text",   "verbs": ["state"], "uid": true },
    "inventory":       { "list": "item" }
  },
  "screens": ["workspace"]
})";

using Lines = std::vector<std::string>;

/// Problems, each as one line: "where: what".
Lines lines_of( const FileProblems &problems )
{
    Lines lines;
    for( const FileProblem &problem : problems ) {
        lines.push_back( problem.where + ": " + problem.what );
    }
    return lines;
}

/// Read names.json from `json`; the problems, as lines.
Lines names_problems( std::string_view json, NameSchema &schema )
{
    FileProblems problems;
    data::JsonValue root;
    if( read_json( "names.json", json, root, problems ) ) {
        const bool read = read_names( "names.json", root, schema, problems );
        CHECK( read == problems.empty() );
    }
    return lines_of( problems );
}

NameSchema schema_of_test()
{
    NameSchema schema;
    CHECK( names_problems( names_json, schema ).empty() );
    return schema;
}

/// Storage for one layout.
struct Scratch
{
        std::array<LayoutNode, 64> nodes{};
        std::array<char, 1024> text{};
        std::array<float, 32> numbers{};
};

const std::vector<LayoutTarget> targets = {
    { "landing", LayoutKind::screen },
    { "settings", LayoutKind::screen },
    { "discard", LayoutKind::pane },
};

/// Read `json` as the layout t.json; its problems, as lines.
Lines layout_problems( std::string_view json, Scratch &scratch )
{
    static const NameSchema schema = schema_of_test();
    FileProblems problems;
    data::JsonValue root;
    if( !read_json( "t.json", json, root, problems ) ) {
        return lines_of( problems );
    }
    // The writer is made with the file's kind; read_layout reads it again,
    // and says what is wrong with it.
    FileProblems ignored;
    const LayoutKind kind = read_layout_kind( "t.json", root, ignored );
    LayoutWriter out( scratch.nodes, scratch.text, scratch.numbers, "t", kind );
    const bool read = read_layout( "t.json", root, schema, targets, out, problems );
    CHECK( read == problems.empty() );
    if( read ) {
        CHECK( check( out.layout() ).ok() );
    }
    return lines_of( problems );
}

/// One line of the cases file, split at its tabs.
struct Case
{
        int line = 0;
        std::string_view kind;
        std::string_view json;
        std::string_view expected;
};

/// The problems a case's JSON has.
Lines problems_of( const Case &one )
{
    if( one.kind == "names" ) {
        NameSchema schema;
        return names_problems( one.json, schema );
    }
    Scratch scratch;
    if( one.kind == "layout" ) {
        return layout_problems( one.json, scratch );
    }
    const std::string json = "{ \"rows\": [ [ " + std::string( one.json ) + " ] ] }";
    return layout_problems( json, scratch );
}

/// Whether `found` is what `expected` says: "ok", "N problems", one problem
/// exactly, or one problem starting so (`expected` ending " ...").
bool as_expected( const Lines &found, std::string_view expected )
{
    if( expected == "ok" ) {
        return found.empty();
    }
    constexpr std::string_view many = " problems";
    if( expected.ends_with( many ) ) {
        const std::string_view digits = expected.substr( 0, expected.size() - many.size() );
        std::size_t count = 0;
        const char *end = digits.data() + digits.size();
        const auto [ptr, error] = std::from_chars( digits.data(), end, count );
        CHECK( error == std::errc{} && ptr == end );
        return found.size() == count;
    }
    if( found.size() != 1 ) {
        return false;
    }
    constexpr std::string_view more = " ...";
    if( expected.ends_with( more ) ) {
        expected.remove_suffix( more.size() );
        return found[0].starts_with( expected );
    }
    return found[0] == expected;
}

/// Split `text` at its first tab; what is before it, and the rest after it.
bool split_at_tab( std::string_view &text, std::string_view &before )
{
    const std::size_t tab = text.find( '\t' );
    if( tab == std::string_view::npos ) {
        return false;
    }
    before = text.substr( 0, tab );
    text.remove_prefix( tab + 1 );
    return true;
}

bool case_of( std::string_view text, Case &one )
{
    if( !split_at_tab( text, one.kind ) || !split_at_tab( text, one.json ) ) {
        return false;
    }
    one.expected = text;
    const bool known = one.kind == "names" || one.kind == "layout" || one.kind == "node";
    return known && !one.expected.empty() && text.find( '\t' ) == std::string_view::npos;
}

void cases()
{
    std::ifstream file( LAYOUT_FILE_CASES );
    CHECK( file.is_open() );
    int line = 0;
    int run = 0;
    int failed = 0;
    for( std::string text; std::getline( file, text ); ) {
        ++line;
        if( text.empty() || text.starts_with( '#' ) ) {
            continue;
        }
        Case one;
        one.line = line;
        if( !case_of( text, one ) ) {
            std::fprintf( stderr, "layout_file_cases.txt:%d: not a case\n", line );
            ++failed;
            continue;
        }
        ++run;
        const Lines found = problems_of( one );
        if( as_expected( found, one.expected ) ) {
            continue;
        }
        ++failed;
        std::fprintf( stderr, "layout_file_cases.txt:%d: expected %.*s\n", line,
                      static_cast<int>( one.expected.size() ), one.expected.data() );
        for( const std::string &problem : found ) {
            std::fprintf( stderr, "  found %s\n", problem.c_str() );
        }
        if( found.empty() ) {
            std::fprintf( stderr, "  found no problem\n" );
        }
    }
    std::printf( "layout_file_test: %d cases\n", run );
    CHECK( run > 60 );
    CHECK( failed == 0 );
}

void names_read()
{
    const NameSchema schema = schema_of_test();
    CHECK( schema.names.size() == 13 ); // the list is two: its rows and its count
    for( std::size_t i = 1; i < schema.names.size(); ++i ) {
        CHECK( schema.names[i - 1].name < schema.names[i].name );
    }
    const NameEntry *rows = schema.find( "inventory" );
    CHECK( rows != nullptr && rows->uid && rows->list == "item" );
    CHECK( rows->type == sq::messaging::ValueType::integer );
    const NameEntry *count = schema.find( "inventory.count" );
    CHECK( count != nullptr && !count->uid );
    CHECK( count->verbs.has( sq::messaging::Verb::state ) );
    const NameEntry *sound = schema.find( "settings.sound" );
    CHECK( schema.find_id( sq::messaging::Name( "settings.sound" ) ) == sound );
    CHECK( schema.find( "counter.ticks" )->note == "Ticks" );
    CHECK( schema.has_screen( "workspace" ) && !schema.has_screen( "landing" ) );
    CHECK( schema.find( "nothing" ) == nullptr );
}

void a_file_gives_the_writers_table()
{
    static constexpr Key<bool, state | set> sound{ "settings.sound" };
    static constexpr Key<double, state | set> volume{ "settings.volume" };
    constexpr std::string_view json = R"({ "kind": "screen", "rows": [
      [ { "type": "check_box", "text": "Sound", "bind": "settings.sound", "grow_x": true } ],
      [ { "type": "slider", "min": 0, "max": 11, "step": 0.5, "bind": "settings.volume" },
        { "type": "progress_bar", "max": 11, "bind": "settings.volume", "align": ["end", "start"] } ],
      [ { "type": "table", "span": 2, "rows": [
          [ { "type": "button", "text": "Mute", "tap": "set settings.sound false" } ],
          [ { "type": "button", "text": "Back", "tap": "back", "style": "plain", "pad": 2.5 } ] ] } ]
    ] })";
    Scratch from_file;
    CHECK( layout_problems( json, from_file ).empty() );

    Scratch from_cpp;
    LayoutWriter root( from_cpp.nodes, from_cpp.text, from_cpp.numbers, "t" );
    root.add<CheckBox>( "Sound" ).grow_x().bind( sound );
    root.row();
    root.add<Slider>().range( 0.0F, 11.0F ).step( 0.5F ).bind( volume );
    NodeWriter bar = root.add<ProgressBar>();
    bar.align( Alignment::end, Alignment::start ).range( 0.0F, 11.0F ).bind( volume );
    root.row();
    NodeWriter inner = root.add<Table>().span( 2 );
    inner.add<Button>( "Mute" ).tap( sound, false );
    inner.row();
    inner.add<Button>( "Back" ).pad( 2.5F ).style( "plain" ).tap( Navigate::back );
    CHECK( root.complete() );

    const Layout cpp = root.layout();
    CHECK( cpp.nodes().size() == 6 );
    for( std::size_t i = 0; i < cpp.nodes().size(); ++i ) {
        CHECK( from_file.nodes[i] == cpp.nodes()[i] );
    }
    const std::string_view file_text( from_file.text.data(), cpp.text().size() );
    CHECK( file_text == cpp.text() );
    for( std::size_t i = 0; i < cpp.numbers().size(); ++i ) {
        CHECK( from_file.numbers[i] == cpp.numbers()[i] );
    }
}

/// A list, from a file and from C++: the same table.
void a_list_file_gives_the_writers_table()
{
    static constexpr Key<std::int64_t, state, with_uid> inventory{ "inventory" };
    static constexpr Key<std::string_view, state, with_uid> name{ "item.name" };
    static constexpr Key<std::int64_t, state | set, with_uid> count{ "item.count" };
    static constexpr Key<void, do_, with_uid> use{ "item.use" };
    constexpr std::string_view json = R"({ "rows": [
      [ { "type": "list", "bind": "inventory", "grow": true, "row": { "type": "table", "rows": [
          [ { "type": "label", "bind": "item.name", "grow_x": true },
            { "type": "slider", "max": 9, "bind": "item.count" },
            { "type": "button", "text": "Use", "tap": "do item.use" } ] ] } } ],
      [ { "type": "button", "text": "Back", "tap": "back" } ]
    ] })";
    Scratch from_file;
    CHECK( layout_problems( json, from_file ).empty() );

    Scratch from_cpp;
    LayoutWriter root( from_cpp.nodes, from_cpp.text, from_cpp.numbers, "t" );
    NodeWriter list = root.add_list( inventory ).grow();
    NodeWriter row = list.add<Table>();
    row.add<Label>().grow_x().bind( name );
    row.add<Slider>().range( 0.0F, 9.0F ).bind( count );
    row.add<Button>( "Use" ).tap( use );
    root.row();
    root.add<Button>( "Back" ).tap( Navigate::back );
    CHECK( root.complete() );

    const Layout cpp = root.layout();
    CHECK( cpp.nodes().size() == 6 && cpp.nodes()[3].sends_set && cpp.nodes()[4].row_uid );
    for( std::size_t i = 0; i < cpp.nodes().size(); ++i ) {
        CHECK( from_file.nodes[i] == cpp.nodes()[i] );
    }
    const std::string_view file_text( from_file.text.data(), cpp.text().size() );
    CHECK( file_text == cpp.text() );
}

} // namespace

int main()
{
    names_read();
    cases();
    a_file_gives_the_writers_table();
    a_list_file_gives_the_writers_table();
    std::printf( "layout_file_test: ok\n" );
    return 0;
}
