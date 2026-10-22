// Layouts (7.0): reading taps, checking layouts, the C++ writer giving the
// same table a compiled layout file is, typed taps in the telegram text form,
// the writer refusing what cannot go, and the instantiator building widgets
// from nodes, with its hooks. A fuzz writes random layouts: each one checks,
// and builds.

#include <squared/dev/frame_allocations.hpp>
#include <squared/gui/button.hpp>
#include <squared/gui/check_box.hpp>
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
#include <squared/messaging/name.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace sq::gui;
using sq::messaging::do_;
using sq::messaging::Key;
using sq::messaging::Name;
using sq::messaging::set;
using sq::messaging::state;
using sq::messaging::with_uid;

namespace
{

// A test fails loudly, with its line, in any build.
#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "screen_layout_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );     \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

inline constexpr Key<bool, state | set> settings_sound{ "settings.sound" };
inline constexpr Key<std::int64_t, state | set> settings_volume{ "settings.volume" };
inline constexpr Key<std::string_view, do_> file_open{ "file.open" };
inline constexpr Key<void, do_> game_start{ "game.start" };
inline constexpr Key<std::string_view, state, with_uid> item_name{ "item.name" };

template<typename K>
concept bindable = requires( NodeWriter writer, const K &key ) { writer.bind( key ); };

// Binding takes a key the node can show: one that carries `state`, without a uid.
static_assert( bindable<decltype( settings_sound )> );
static_assert( !bindable<decltype( file_open )> );
static_assert( !bindable<decltype( item_name )> );

// What sqgen writes for settings.json: a check box bound to settings.sound,
// then, on a new row, a button whose tap is `back`.
inline constexpr Name sound = settings_sound.name();
inline constexpr char settings_text[] = "\0Sound\0settings.sound\0Back\0back\0";
inline constexpr std::string_view settings_words( settings_text, sizeof( settings_text ) - 1 );
inline constexpr LayoutNode settings_nodes[] = {
    { .bind = sound, .text = 1, .bind_text = 7, .type = NodeType::check_box },
    { .text = 22, .tap = 27, .type = NodeType::button, .new_row = true },
};
inline constexpr Layout settings{ "settings", LayoutKind::screen, settings_nodes, settings_words };
static_assert( settings.text( 7 ) == "settings.sound" && settings.text( 0 ).empty() );

/// Storage for one written layout.
struct Scratch
{
        std::array<LayoutNode, 64> nodes{};
        std::array<char, 1024> text{};
        std::array<float, 32> numbers{};
};

/// A node of `type`, with `inside` nodes inside it.
LayoutNode node( NodeType type = NodeType::label, std::uint16_t inside = 0 )
{
    LayoutNode made{};
    made.type = type;
    made.inside = inside;
    return made;
}

/// A layout of `nodes` and `text`, for the checker.
LayoutCheck check_of( std::span<const LayoutNode> nodes, std::string_view text = {},
                      std::span<const float> numbers = {} )
{
    return check( Layout( "t", LayoutKind::screen, nodes, text, numbers ) );
}

constexpr std::array<std::string_view, 7> bad_taps = {
    "push", "push Settings", "back now", "jump x", "do", "Do x", "push two words",
};

void taps_read()
{
    CHECK( read_tap( "" ).kind == TapKind::none );
    CHECK( read_tap( "back" ).kind == TapKind::back );
    CHECK( read_tap( "close" ).kind == TapKind::close );
    CHECK( read_tap( "quit" ).kind == TapKind::quit );
    const Tap push = read_tap( "push settings" );
    CHECK( push.kind == TapKind::push && push.target == "settings" );
    CHECK( read_tap( "replace game" ).target == "game" );
    CHECK( read_tap( "open discard" ).kind == TapKind::open );
    const Tap sent = read_tap( "do file.open \"a.txt\"" );
    CHECK( sent.kind == TapKind::telegram && sent.target == "do file.open \"a.txt\"" );
    CHECK( read_tap( "set settings.sound true" ).kind == TapKind::telegram );
    for( const std::string_view bad : bad_taps ) {
        CHECK( read_tap( bad ).kind == TapKind::invalid );
    }
    for( std::size_t i = 0; i < node_type_count; ++i ) {
        const auto type = static_cast<NodeType>( i );
        CHECK( node_type_named( node_type_name( type ) ) == type );
    }
    CHECK( !node_type_named( "list" ) && !node_type_named( "" ) );
    CHECK( node_type_name( static_cast<NodeType>( 99 ) ).empty() );
}

void check_trees()
{
    using P = LayoutProblem;
    CHECK( check( settings ).ok() );
    CHECK( check( Layout() ).ok() ); // nothing: a blank screen

    const std::array<LayoutNode, 1> past = { node( NodeType::table, 1 ) };
    CHECK( check_of( past ).problem == P::bad_tree );
    std::array<LayoutNode, 3> spill = { node( NodeType::table, 1 ), node(), node() };
    spill[1] = node( NodeType::stack, 1 );
    CHECK( check_of( spill ) == LayoutCheck{ P::bad_tree, 1 } );

    const std::array<LayoutNode, 2> leaf = { node( NodeType::label, 1 ), node() };
    CHECK( check_of( leaf ).problem == P::not_a_container );

    const std::array<LayoutNode, 1> empty_scroll = { node( NodeType::scroll ) };
    CHECK( check_of( empty_scroll ).ok() ); // shows nothing, harmlessly
    const std::array<LayoutNode, 2> one = { node( NodeType::scroll, 1 ), node() };
    CHECK( check_of( one ).ok() );
    const std::array<LayoutNode, 3> two = { node( NodeType::scroll, 2 ), node(), node() };
    CHECK( check_of( two ).problem == P::scroll_holds_one );

    std::array<LayoutNode, 3> stacked = { node( NodeType::stack, 2 ), node(), node() };
    stacked[2].new_row = true;
    CHECK( check_of( stacked ) == LayoutCheck{ P::misplaced_row, 2 } );
    std::array<LayoutNode, 2> rows = { node(), node() };
    rows[1].new_row = true;
    CHECK( check_of( rows ).ok() ); // the layout's own table

    std::array<LayoutNode, layout_depth + 1> deep{};
    for( std::size_t i = 0; i < deep.size(); ++i ) {
        deep[i] = node( NodeType::table, static_cast<std::uint16_t>( deep.size() - 1 - i ) );
    }
    CHECK( check_of( deep ) == LayoutCheck{ P::too_deep, layout_depth } );
    std::array<LayoutNode, layout_depth> deepest{};
    for( std::size_t i = 0; i < deepest.size(); ++i ) {
        deepest[i] = node( NodeType::table, static_cast<std::uint16_t>( deepest.size() - 1 - i ) );
    }
    CHECK( check_of( deepest ).ok() );

    const std::array<LayoutNode, 1> odd = { node( static_cast<NodeType>( 99 ) ) };
    CHECK( check_of( odd ).problem == P::bad_type );
}

void check_words()
{
    using P = LayoutProblem;
    // "Hi" at 1, "x.y" at 4, "push a" at 8.
    const std::string_view words( "\0Hi\0x.y\0push a\0", 15 );
    CHECK( check_of( settings_nodes, "Sound" ).problem == P::bad_text ); // no leading '\0'
    std::array<LayoutNode, 1> one = { node() };
    one[0].text = 40;
    CHECK( check_of( one, words ).problem == P::bad_text );

    one = { node( NodeType::button ) };
    one[0].bind = Name( "x.y" );
    one[0].bind_text = 4;
    CHECK( check_of( one, words ).problem == P::bad_bind ); // a button shows no value
    one[0].type = NodeType::label;
    CHECK( check_of( one, words ).ok() );
    one[0].bind = Name( "x.z" );
    CHECK( check_of( one, words ).problem == P::bad_bind ); // not what its text hashes to
    one[0].bind = Name( "x.y" );
    one[0].bind_text = 0;
    CHECK( check_of( one, words ).problem == P::bad_bind ); // no text

    one = { node() };
    one[0].tap = 8;
    CHECK( check_of( one, words ).problem == P::bad_tap ); // a label is not tapped
    one[0].type = NodeType::button;
    CHECK( check_of( one, words ).ok() );
    one[0].tap = 1;
    CHECK( check_of( one, words ).problem == P::bad_tap ); // "Hi" is not a tap

    one = { node( NodeType::custom ) };
    CHECK( check_of( one ).problem == P::no_custom_name );
    one[0].text = 1;
    CHECK( check_of( one, words ).ok() );

    for( std::size_t i = 0; i <= static_cast<std::size_t>( P::unknown_drawable ); ++i ) {
        CHECK( !problem_text( static_cast<P>( i ) ).empty() );
    }
}

void check_numbers()
{
    using P = LayoutProblem;
    const std::array<float, 3> range = { 0.0F, 10.0F, 1.0F };
    std::array<LayoutNode, 1> one = { node( NodeType::slider ) };
    one[0].has_numbers = true;
    CHECK( check_of( one, {}, range ).ok() );
    CHECK( check_of( one, {}, std::span( range ).first( 2 ) ).problem == P::bad_numbers );
    const std::array<float, 3> upside = { 10.0F, 0.0F, 1.0F };
    CHECK( check_of( one, {}, upside ).problem == P::bad_numbers );
    const std::array<float, 3> back_step = { 0.0F, 10.0F, -1.0F };
    CHECK( check_of( one, {}, back_step ).problem == P::bad_numbers );

    one[0].type = NodeType::progress_bar;
    CHECK( check_of( one, {}, range ).ok() );
    one[0].numbers = 1;
    CHECK( check_of( one, {}, range ).problem == P::bad_numbers ); // from 10 to 1
    one[0].type = NodeType::label;
    CHECK( check_of( one, {}, range ).problem == P::bad_numbers ); // takes none
}

void writer_writes_the_same_table()
{
    Scratch scratch;
    LayoutWriter root( scratch.nodes, scratch.text, scratch.numbers, "settings" );
    root.add<CheckBox>( "Sound" ).bind( settings_sound );
    root.row();
    root.add<Button>( "Back" ).tap( Navigate::back );
    CHECK( root.complete() );
    const Layout written = root.layout();
    CHECK( written.name() == "settings" && written.kind() == LayoutKind::screen );
    CHECK( written.nodes().size() == 2 );
    CHECK( written.nodes()[0] == settings_nodes[0] && written.nodes()[1] == settings_nodes[1] );
    CHECK( written.text() == settings.text() );

    // A row asked for before the first node starts nothing: there is no row
    // before it to end.
    root.clear();
    root.row();
    root.add<Label>( "first" );
    CHECK( !root.layout().nodes()[0].new_row );

    // Nothing allocates while writing.
    sq::dev::frame_begin();
    for( int round = 0; round < 100; ++round ) {
        root.clear();
        root.add<Label>( "Volume" );
        NodeWriter volume = root.add<Slider>().range( 0.0F, 11.0F ).step( 1.0F );
        volume.bind( settings_volume ).grow_x();
        root.row();
        NodeWriter inner = root.add<Table>().span( 2 );
        inner.add<Button>( "Play" ).tap( game_start );
        inner.row();
        inner.add<Button>( "Open" ).tap( file_open, "notes.txt" );
    }
    const sq::dev::FrameAllocations used = sq::dev::frame_end();
    CHECK( used.count == 0 );
    CHECK( root.complete() && check( root.layout() ).ok() );
}

void typed_taps()
{
    Scratch scratch;
    LayoutWriter root( scratch.nodes, scratch.text, scratch.numbers );
    root.add<Button>( "a" ).tap( settings_sound, true );
    root.add<Button>( "b" ).tap( settings_volume, 7 );
    root.add<Button>( "c" ).tap( file_open, "notes \"2\".txt" );
    root.add<Button>( "d" ).tap( game_start );
    root.add<Button>( "e" ).tap( Navigate::push, "settings" );
    root.add<Button>( "f" ).tap( Navigate::close );
    CHECK( root.complete() );
    const Layout layout = root.layout();
    std::vector<std::string_view> taps;
    for( const LayoutNode &each : layout.nodes() ) {
        taps.push_back( layout.text( each.tap ) );
    }
    CHECK( taps[0] == "set settings.sound true" );
    CHECK( taps[1] == "set settings.volume 7" );
    CHECK( taps[2] == "do file.open \"notes \\\"2\\\".txt\"" );
    CHECK( taps[3] == "do game.start" );
    CHECK( taps[4] == "push settings" );
    CHECK( taps[5] == "close" );
    CHECK( check( layout ).ok() );
}

/// Whether `write` leaves the writer refusing something, with what it did
/// write still a layout that checks.
template<typename Write>
bool refused( LayoutWriter &root, Write write )
{
    root.clear();
    write();
    const bool was = !root.complete() && check( root.layout() ).ok();
    root.clear();
    return was;
}

void writer_refuses()
{
    Scratch scratch;
    LayoutWriter root( scratch.nodes, scratch.text, scratch.numbers );

    // A container takes nodes until one goes outside it.
    NodeWriter inner = root.add<Table>();
    CHECK( inner.add<Label>( "in" ).written() );
    root.add<Label>( "after" );
    CHECK( !inner.add<Label>( "late" ).written() && !root.complete() );
    root.clear();
    CHECK( root.complete() );

    CHECK( refused( root, [&root] {
        NodeWriter scroll = root.add<ScrollPane>();
        scroll.add<Label>( "one" );
        scroll.add<Label>( "two" );
    } ) );
    CHECK( refused( root, [&root] { root.add<Label>( "x" ).add<Label>( "y" ); } ) );
    CHECK( refused( root, [&root] { root.add<Table>().bind( settings_sound ); } ) );
    CHECK( refused( root, [&root] { root.add<Label>( "x" ).tap( Navigate::back ); } ) );
    CHECK( refused( root, [&root] { root.add<Button>( "x" ).tap( "jump" ); } ) );
    CHECK( refused( root, [&root] { root.add<Button>( "x" ).tap( Navigate::back, "where" ); } ) );
    CHECK( refused( root, [&root] { root.add<Slider>().range( 2.0F, 1.0F ); } ) );
    CHECK( refused( root, [&root] { root.add<ProgressBar>().step( 1.0F ); } ) );
    CHECK( refused( root, [&root] { root.add<Label>( "x" ).range( 0.0F, 1.0F ); } ) );
    CHECK( refused( root, [&root] { root.add<Label>( "x" ).vertical(); } ) );
    CHECK( refused( root, [&root] { root.add<Stack>().row(); } ) );
    CHECK( refused( root, [&root] {
        NodeWriter at = root.add<Table>();
        for( std::size_t i = 0; i < layout_depth; ++i ) {
            at = at.add<Table>();
        }
    } ) );

    // Out of room: what fitted is still a layout.
    std::array<LayoutNode, 3> few{};
    std::array<char, 12> little{};
    LayoutWriter small( few, little, {} );
    for( int i = 0; i < 5; ++i ) {
        small.add<Label>( "four" );
    }
    CHECK( !small.complete() && small.layout().nodes().size() == 3 );
    CHECK( small.layout().nodes()[2].text == 0 ); // its words did not fit
    CHECK( check( small.layout() ).ok() );
    small.clear();
    small.add<Slider>().range( 0.0F, 2.0F ); // no room for numbers
    CHECK( !small.complete() && check( small.layout() ).ok() );

    // Words fill the text to its last byte, and no further.
    std::array<char, 7> tight{};
    LayoutWriter exact( few, tight, {} );
    exact.add<Label>( "four" ); // '\0', "four", '\0': 6 bytes
    CHECK( exact.complete() );
    exact.add<Label>( "x" ); // 2 more: no room
    CHECK( !exact.complete() && exact.layout().text().size() == 6 );
}

/// What the hooks saw.
struct Seen
{
        std::vector<std::string> taps;
        std::vector<std::string> bound;
        std::vector<const Widget *> bound_widgets;
        int customs = 0;
};

struct Dial final : Widget
{};

void seen_tap( void *context, std::string_view line ) noexcept
{
    static_cast<Seen *>( context )->taps.emplace_back( line );
}

void seen_bound( void *context, const Layout &layout, const LayoutNode &bound,
                 Widget &widget ) noexcept
{
    auto &seen = *static_cast<Seen *>( context );
    seen.bound.emplace_back( layout.text( bound.bind_text ) );
    seen.bound_widgets.push_back( &widget );
}

std::unique_ptr<Widget> make_dial( void *context, std::string_view name )
{
    if( name != "dial" ) {
        return nullptr;
    }
    ++static_cast<Seen *>( context )->customs;
    return std::make_unique<Dial>();
}

void instantiates_a_compiled_layout()
{
    const Skin skin;
    Seen seen;
    const LayoutHooks hooks{ seen_tap, seen_bound, make_dial, &seen };
    Table root;
    CHECK( instantiate( settings, root, skin, hooks ).ok() );
    CHECK( root.child_count() == 2 );
    const auto *box = static_cast<const CheckBox *>( root.child_at( 0 ) );
    auto *back = static_cast<Button *>( root.child_at( 1 ) );
    CHECK( box->text() == "Sound" && back->text() == "Back" );
    CHECK( seen.bound.size() == 1 && seen.bound[0] == "settings.sound" );
    CHECK( seen.bound_widgets[0] == box );
    CHECK( back->key_down( sq::app::Key::enter ) );
    CHECK( seen.taps.size() == 1 && seen.taps[0] == "back" );

    // A layout that fails its check builds nothing.
    Table untouched;
    const std::array<LayoutNode, 3> bad = { node( NodeType::scroll, 2 ), node(), node() };
    const Layout bad_layout( "bad", LayoutKind::pane, bad, {} );
    const LayoutCheck refused = instantiate( bad_layout, untouched, skin );
    CHECK( refused == LayoutCheck{ LayoutProblem::scroll_holds_one, 0 } );
    CHECK( untouched.child_count() == 0 );
}

void instantiates_every_kind()
{
    const Skin skin;
    Seen seen;
    const LayoutHooks hooks{ seen_tap, seen_bound, make_dial, &seen };
    Table root;
    {
        Scratch scratch;
        LayoutWriter writer( scratch.nodes, scratch.text, scratch.numbers );
        writer.add<Label>( "Title" ).span( 2 ).pad( 4.0F ).align( Alignment::start );
        writer.row();
        NodeWriter volume = writer.add<Slider>().range( 5.0F, 10.0F ).step( 0.5F );
        volume.bind( settings_volume ).grow_x();
        writer.add<ProgressBar>().range( 1.0F, 3.0F );
        writer.row();
        NodeWriter inner = writer.add<Table>().fill();
        inner.add<TextField>( "typed" );
        inner.row();
        inner.add<Separator>().vertical();
        writer.add<ScrollPane>().add<Label>( "long" );
        writer.row();
        NodeWriter stack = writer.add<Stack>();
        stack.add<Image>( "no such drawable" ); // node 9
        stack.add_custom( "dial" );
        writer.add_custom( "gauge" );
        writer.add<Button>( "Open" ).tap( file_open, "notes.txt" );
        CHECK( writer.complete() );
        const LayoutCheck built = instantiate( writer.layout(), root, skin, hooks );
        CHECK( built == LayoutCheck{ LayoutProblem::unknown_drawable, 9 } ); // the first short
        // A tap's line was copied: the layout may go.
        scratch = {};
    }
    CHECK( root.child_count() == 8 );
    CHECK( static_cast<const Label *>( root.child_at( 0 ) )->text() == "Title" );
    CHECK( static_cast<const Slider *>( root.child_at( 1 ) )->value() == 5.0F );
    CHECK( static_cast<const ProgressBar *>( root.child_at( 2 ) )->value() == 1.0F );
    CHECK( static_cast<const Table *>( root.child_at( 3 ) )->child_count() == 2 );
    CHECK( static_cast<const ScrollPane *>( root.child_at( 4 ) )->child_count() == 1 );
    CHECK( static_cast<const Stack *>( root.child_at( 5 ) )->child_count() == 2 );
    CHECK( static_cast<const Label *>( root.child_at( 6 ) )->text() == "no custom type gauge" );
    CHECK( seen.customs == 1 && seen.bound.size() == 1 && seen.bound[0] == "settings.volume" );
    CHECK( static_cast<Button *>( root.child_at( 7 ) )->key_down( sq::app::Key::enter ) );
    CHECK( seen.taps.size() == 1 && seen.taps[0] == "do file.open \"notes.txt\"" );

    // A custom type no hook makes, alone: that is what is said.
    Scratch scratch;
    LayoutWriter only( scratch.nodes, scratch.text, scratch.numbers );
    only.add_custom( "gauge" );
    Table alone;
    const LayoutCheck missing = instantiate( only.layout(), alone, skin, hooks );
    CHECK( missing == LayoutCheck{ LayoutProblem::unknown_custom, 0 } );
}

/// A number from 0 to below `n`.
int pick( std::mt19937 &random, int n )
{
    return static_cast<int>( random() % static_cast<unsigned>( n ) );
}

/// Add a `W` inside `at`, or to the layout's own table when `at` is null.
template<typename W>
NodeWriter add( LayoutWriter &root, NodeWriter *at, std::vector<NodeWriter> &written )
{
    if( at == nullptr ) {
        written.push_back( root.add<W>( "w" ) );
    } else {
        written.push_back( at->add<W>( "w" ) );
    }
    NodeWriter made = written.back();
    return made;
}

/// One random step of writing.
void write_one( std::mt19937 &random, LayoutWriter &root, std::vector<NodeWriter> &written )
{
    NodeWriter *at = nullptr;
    if( !written.empty() && pick( random, 2 ) == 0 ) {
        const int index = pick( random, static_cast<int>( written.size() ) );
        at = &written[static_cast<std::size_t>( index )];
    }
    switch( pick( random, 14 ) ) {
        case 0:
            add<Label>( root, at, written ).bind( settings_volume );
            break;
        case 1:
            add<Button>( root, at, written ).tap( Navigate::push, "next" );
            break;
        case 2:
            add<CheckBox>( root, at, written ).bind( settings_sound ).grow();
            break;
        case 3: {
            NodeWriter slider = add<Slider>( root, at, written );
            slider.range( static_cast<float>( pick( random, 3 ) ), 2.0F ).step( 0.5F );
            break;
        }
        case 4:
            add<Table>( root, at, written ).fill();
            break;
        case 5:
            add<ScrollPane>( root, at, written );
            break;
        case 6:
            add<Stack>( root, at, written );
            break;
        case 7:
            add<Separator>( root, at, written ).vertical();
            break;
        case 8:
            root.row();
            break;
        case 9:
            if( at != nullptr ) {
                at->row();
            }
            break;
        case 10:
            add<Button>( root, at, written ).tap( file_open, "x" ).style( "s" );
            break;
        case 11:
            add<ProgressBar>( root, at, written ).range( 0.0F, 9.0F );
            break;
        case 12:
            add<TextField>( root, at, written ).bind( settings_volume ).pad( 2.0F );
            break;
        default:
            add<Label>( root, at, written ).tap( "back" ); // refused: a label
            break;
    }
}

/// Random writing: whatever is asked, what is written checks, and builds.
void fuzz()
{
    std::mt19937 random( 20261021 );
    const Skin skin;
    int refusing = 0;
    for( int round = 0; round < 2000; ++round ) {
        Scratch scratch;
        LayoutWriter root( scratch.nodes, scratch.text, scratch.numbers );
        std::vector<NodeWriter> written;
        const int steps = pick( random, 80 );
        for( int step = 0; step < steps; ++step ) {
            write_one( random, root, written );
        }
        const Layout layout = root.layout();
        const LayoutCheck checked = check( layout );
        CHECK( checked.ok() );
        refusing += root.complete() ? 0 : 1;
        Table built;
        CHECK( instantiate( layout, built, skin ) == checked );
    }
    std::printf( "screen_layout_test: 2000 random layouts, %d with a refusal\n", refusing );
}

} // namespace

int main()
{
    taps_read();
    check_trees();
    check_words();
    check_numbers();
    writer_writes_the_same_table();
    typed_taps();
    writer_refuses();
    instantiates_a_compiled_layout();
    instantiates_every_kind();
    fuzz();
    std::printf( "screen_layout_test: ok (LayoutNode %zu, Layout %zu, LayoutWriter %zu bytes)\n",
                 sizeof( LayoutNode ), sizeof( Layout ), sizeof( LayoutWriter ) );
    return 0;
}
