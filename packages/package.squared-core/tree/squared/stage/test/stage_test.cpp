// The App and its Stage (7.1), on the host: the null graphics backend, the
// default skin from test/assets, layouts written in C++, and a model. A
// window comes and squared_main runs once; taps navigate and reach the model;
// frames allocate nothing; back at the root and a quit tap leave; quitting
// stops the model after it saves, and the next start begins afresh; an
// activity made again reattaches to the running model. C++ screens (7.2), and
// panes over a screen (7.3). The skin is the compiled-in default (7.4): the
// app's bundled files are a directory that does not exist, and the embedded
// bytes are checked against the skin's files. What screens keep (7.6): a
// rotation laid out once with nothing built again; scrolls and an unfinished
// edit kept beneath the top and through a lost context; the stack written
// in the background, shown again after the process ends, forgotten by a
// quit; the model's own save given back; the Console's `screens` lines; and
// a new activity that forgets the old one's keyboard.

#include "detail/default_skin.hpp"

#include <squared/app/event.hpp>
#include <squared/app/runtime.hpp>
#include <squared/assets/asset_manager.hpp>
#include <squared/dev/frame_allocations.hpp>
#include <squared/files/embedded_file_system.hpp>
#include <squared/files/posix_file_system.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics/src/context_config.hpp>
#include <squared/gui/button.hpp>
#include <squared/gui/check_box.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/layout_writer.hpp>
#include <squared/gui/list.hpp>
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/scroll_pane.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/src/detail/pane_frame.hpp>
#include <squared/gui/src/detail/ui.hpp>
#include <squared/gui/src/pointer_action.hpp>
#include <squared/gui/src/pointer_event.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/src/window_style.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/text_field.hpp>
#include <squared/gui/widget.hpp>
#include <squared/messaging/key.hpp>
#include <squared/messaging/model_runner.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/stage/app.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

/// What lets a test point the Stage at another skin.
struct sq::detail::StageProbe final
{
        static void read_skin_from( sq::Stage &stage, sq::files::FileSystem *files ) noexcept
        {
            stage.skin_files_ = files;
        }

        /// The Stage's widgets: internal since 7.7, a test's to look at.
        [[nodiscard]] static sq::gui::Ui *ui( sq::Stage &stage ) noexcept { return stage.ui(); }

        /// A size or safe area changed, and the layout waits for the next event or frame.
        [[nodiscard]] static bool layout_due( const sq::Stage &stage ) noexcept
        {
            return stage.layout_due_;
        }
};

namespace
{

/// The top screen's widgets, its panes' among them: through the probe.
[[nodiscard]] sq::gui::Ui *widgets_of( sq::App &app ) noexcept
{
    return sq::detail::StageProbe::ui( app.stage );
}

// A test fails loudly, with its line, in any build.
#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "stage_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );             \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using sq::messaging::Key;
using sq::messaging::Received;
using sq::messaging::Telegraph;

constexpr Key<bool, sq::messaging::state | sq::messaging::set> sound{ "settings.sound" };
constexpr Key<double, sq::messaging::state | sq::messaging::set> volume{ "settings.volume" };
constexpr Key<std::string_view, sq::messaging::state | sq::messaging::set> player{ "player.name" };
constexpr Key<std::int64_t, sq::messaging::state> ticks{ "counter.ticks" };
constexpr Key<std::int64_t, sq::messaging::state | sq::messaging::set> level{ "settings.level" };
// The same address as a double: a model whose key and the layout disagree.
constexpr Key<double, sq::messaging::state> level_as_number{ "settings.level" };

// A list of books: its rows, and each book's values by its uid.
using sq::messaging::with_uid;
constexpr Key<std::int64_t, sq::messaging::state, with_uid> books{ "books" };
constexpr Key<std::int64_t, sq::messaging::state> books_count{ "books.count" };
constexpr Key<std::string_view, sq::messaging::state, with_uid> book_title{ "book.title" };
constexpr Key<std::int64_t, sq::messaging::state, with_uid> book_taps{ "book.taps" };
constexpr Key<bool, sq::messaging::state | sq::messaging::set, with_uid> book_kept{ "book.kept" };
constexpr Key<void, sq::messaging::do_, with_uid> book_tap{ "book.tap" };
constexpr Key<void, sq::messaging::do_> books_reverse{ "books.reverse" };
constexpr Key<void, sq::messaging::do_> books_cut{ "books.cut" };
// What fills the cache, so what it does not keep goes.
constexpr Key<void, sq::messaging::do_> flood{ "test.flood" };
constexpr Key<std::int64_t, sq::messaging::state, with_uid> junk{ "test.junk" };
constexpr std::uint32_t book_count = 1000;

// --- the layouts, written in C++ as a screen's build() will --------------

struct Written final
{
        std::array<sq::gui::LayoutNode, 16> nodes{};
        std::array<char, 512> text{};
        std::array<float, 4> numbers{};
        sq::gui::Layout layout;
};

Written landing;
Written settings;
Written broken;
// Panes: one modal at the bottom, one on the right.
Written discard;
Written side;

/// Every layout, sorted by name, as sqgen gives them.
std::array<const sq::gui::Layout *, 4> sorted = {};

// --- C++ screens -------------------------------------------------------------

/// A custom node, with one registered maker and one not.
struct Gauges final
{
        static constexpr std::string_view name = "gauges";
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add_custom( "gauge" );
            root.row();
            root.add_custom( "dial" );
            root.row();
            root.add_custom( "opener" );
            root.row();
            root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
        }
};

struct About final
{
        static constexpr std::string_view name = "about";
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add<sq::gui::Label>( "About" );
            root.row();
            root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
            root.row();
            root.add<sq::gui::Button>( "Gauges" ).tap<Gauges>( sq::gui::Navigate::push );
        }
};

/// More than the scratch holds.
struct Big final
{
        static constexpr std::string_view name = "big";
        static void build( sq::gui::LayoutWriter &root )
        {
            for( int i = 0; i < 300; ++i ) {
                root.add<sq::gui::Label>( "line" );
                root.row();
            }
        }
};

/// Not a name; and the name of a layout file.
struct BadName final
{
        static constexpr std::string_view name = "Bad name";
        static void build( sq::gui::LayoutWriter & /*root*/ ) {}
};

struct Clash final
{
        static constexpr std::string_view name = "landing";
        static void build( sq::gui::LayoutWriter & /*root*/ ) {}
};

/// A pane with one bound label: its binding goes when it closes.
struct Meter final
{
        static constexpr std::string_view name = "meter";
        static constexpr sq::gui::PanePlace place = sq::gui::PanePlace::right;
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add<sq::gui::Label>( "-" ).bind( volume );
            root.row();
            root.add<sq::gui::Button>( "Shut" ).tap( sq::gui::Navigate::close );
        }
};

/// Every kind of bound widget: what the model says, shown; what the user edits, sent.
struct Bound final
{
        static constexpr std::string_view name = "bound";
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add<sq::gui::Label>( "no ticks yet" ).bind( ticks );
            root.row();
            root.add<sq::gui::CheckBox>( "Sound" ).bind( sound );
            root.row();
            root.add<sq::gui::Slider>().range( 0.0F, 10.0F ).fill_x().bind( volume );
            root.row();
            root.add<sq::gui::Label>( "-" ).bind( volume );
            root.row();
            root.add<sq::gui::ProgressBar>().range( 0.0F, 10.0F ).bind( volume );
            root.row();
            root.add<sq::gui::TextField>( "" ).fill_x().bind( player );
            root.row();
            root.add<sq::gui::Slider>().range( 0.0F, 10.0F ).fill_x().bind( level );
            root.row();
            root.add<sq::gui::Button>( "Meter" ).tap<Meter>( sq::gui::Navigate::open );
        }
};

/// A list of 1,000 books: each row its title, its taps, whether it is kept,
/// and a tap. Nothing else on it is bound, so nothing but the list itself
/// lays it out again when its count comes.
struct Shelf final
{
        static constexpr std::string_view name = "shelf";
        static void build( sq::gui::LayoutWriter &root )
        {
            sq::gui::NodeWriter row = root.add_list( books ).grow().add<sq::gui::Table>();
            row.add<sq::gui::Label>( "?" ).bind( book_title ).grow_x();
            row.add<sq::gui::Label>( "0" ).bind( book_taps );
            row.add<sq::gui::CheckBox>( "kept" ).bind( book_kept );
            row.add<sq::gui::Button>( "Tap" ).tap( book_tap );
            root.row();
            root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
        }
};

/// The same list again, in a pane: the model keeps one view of a list.
struct ShelfPane final
{
        static constexpr std::string_view name = "shelf_pane";
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add_list( books ).add<sq::gui::Label>().bind( book_title );
            root.row();
            root.add<sq::gui::Button>( "Shut" ).tap( sq::gui::Navigate::close );
        }
};

/// More bound widgets than a window holds.
struct Crowded final
{
        static constexpr std::string_view name = "crowded";
        static void build( sq::gui::LayoutWriter &root )
        {
            for( std::size_t i = 0; i < sq::Stage::max_bindings + 2; ++i ) {
                root.add<sq::gui::Label>().bind( ticks );
                root.row();
            }
        }
};

/// A C++ pane: on the left, not modal.
struct Ask final
{
        static constexpr std::string_view name = "ask";
        static constexpr sq::gui::PanePlace place = sq::gui::PanePlace::left;
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add<sq::gui::Label>( "Ask" );
            root.row();
            root.add<sq::gui::Button>( "Done" ).tap( sq::gui::Navigate::close );
        }
};

/// What a screen keeps: a text field's unfinished edit, a scroll pane's place.
struct Notes final
{
        static constexpr std::string_view name = "notes";
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add<sq::gui::TextField>( "" ).fill_x().bind( player );
            root.row();
            sq::gui::NodeWriter pane = root.add<sq::gui::ScrollPane>().grow();
            sq::gui::NodeWriter lines = pane.add<sq::gui::Table>();
            for( int i = 0; i < 150; ++i ) {
                lines.add<sq::gui::Label>( "a line of notes" );
                lines.row();
            }
            root.row();
            root.add<sq::gui::TextField>( "" ).fill_x();
            root.row();
            root.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
        }
};

/// A pane that keeps an edit of its own.
struct Draft final
{
        static constexpr std::string_view name = "draft";
        static constexpr sq::gui::PanePlace place = sq::gui::PanePlace::bottom;
        static void build( sq::gui::LayoutWriter &root )
        {
            root.add<sq::gui::TextField>( "" ).fill_x();
            root.row();
            root.add<sq::gui::Button>( "Drop" ).tap( sq::gui::Navigate::close );
        }
};

static_assert( sq::detail::screen_entry<Ask>.pane.place == sq::gui::PanePlace::left );
static_assert( sq::detail::screen_entry<About>.pane == sq::gui::PaneShape{} );

static_assert( sq::ScreenType<About> );
static_assert( !sq::ScreenType<Written> );

std::unique_ptr<sq::gui::Widget> make_gauge()
{
    return std::make_unique<sq::gui::Label>( "made gauge" );
}

/// The Stage an Opener opens panes on, and what it saw while its event was delivered.
sq::Stage *opening = nullptr;
bool opener_opened = false;
std::size_t opener_saw = 0;

/// A custom widget that opens two panes from inside its event: they wait.
class Opener final : public sq::gui::Widget
{
    public:
        [[nodiscard]] sq::gui::Size preferred_size( sq::gui::Painter & /*painter*/,
                                                    const sq::gui::Skin & /*skin*/ ) const override
        {
            return { 100.0F, 60.0F };
        }

        bool pointer_event( const sq::gui::PointerEvent &event ) override
        {
            if( event.action == sq::gui::PointerAction::up && opening != nullptr ) {
                opener_opened = opening->open( side.layout ) && opening->open( side.layout );
                opener_saw = opening->panes();
            }
            return true;
        }
};

std::unique_ptr<sq::gui::Widget> make_opener()
{
    return std::make_unique<Opener>();
}

void write_layouts()
{
    sq::gui::LayoutWriter one( landing.nodes, landing.text, landing.numbers, "landing" );
    one.add<sq::gui::Label>( "Hello" );
    one.row();
    one.add<sq::gui::Button>( "Settings" ).tap( "push settings" );
    one.row();
    one.add<sq::gui::Button>( "Quiet" ).tap( sound, false );
    one.row();
    one.add<sq::gui::Button>( "Quit" ).tap( sq::gui::Navigate::quit );
    one.row();
    one.add<sq::gui::Button>( "Nowhere" ).tap( "push nowhere" );
    one.row();
    one.add<sq::gui::Button>( "Pane" ).tap( "open discard" );
    one.row();
    one.add<sq::gui::Button>( "About" ).tap<About>( sq::gui::Navigate::push );
    one.row();
    one.add<sq::gui::Button>( "Hidden" ).tap( "push hidden" );
    CHECK( one.complete() );
    landing.layout = one.layout();

    sq::gui::LayoutWriter two( settings.nodes, settings.text, settings.numbers, "settings" );
    two.add<sq::gui::Button>( "Back" ).tap( sq::gui::Navigate::back );
    two.row();
    two.add<sq::gui::Button>( "Again" ).tap( "replace landing" );
    CHECK( two.complete() );
    settings.layout = two.layout();

    using sq::gui::LayoutKind;
    sq::gui::LayoutWriter three( discard.nodes, discard.text, discard.numbers, "discard",
                                 LayoutKind::pane );
    three.set_pane( { sq::gui::PanePlace::bottom, true } );
    three.add<sq::gui::Label>( "Discard?" );
    three.row();
    three.add<sq::gui::Button>( "No" ).tap( sq::gui::Navigate::close );
    three.row();
    three.add<sq::gui::Button>( "Side" ).tap( "open side" );
    three.row();
    three.add<sq::gui::Button>( "Away" ).tap( "push settings" );
    three.row();
    three.add<sq::gui::Button>( "Ask" ).tap<Ask>( sq::gui::Navigate::open );
    CHECK( three.complete() );
    discard.layout = three.layout();

    sq::gui::LayoutWriter four( side.nodes, side.text, side.numbers, "side", LayoutKind::pane );
    four.set_pane( { sq::gui::PanePlace::right, false } );
    four.add<sq::gui::Button>( "Shut" ).tap( sq::gui::Navigate::close );
    CHECK( four.complete() );
    side.layout = four.layout();

    sorted[0] = &discard.layout;
    sorted[1] = &landing.layout;
    sorted[2] = &settings.layout;
    sorted[3] = &side.layout;
}

/// names.json's screens, as sqgen writes them: one the app never adds.
constexpr std::array<std::string_view, 3> screen_names = { "about", "gauges", "missing" };

} // namespace

// What sqgen writes for an app with layout files: this one takes the place of
// the framework's empty default.
namespace layouts
{

std::span<const sq::gui::Layout *const> all() noexcept
{
    return sorted;
}

std::span<const std::string_view> screens() noexcept
{
    return screen_names;
}

} // namespace layouts

namespace
{

// --- the model ------------------------------------------------------------

struct Counts final
{
        std::atomic<int> made{ 0 };
        std::atomic<int> received{ 0 };
        std::atomic<int> saves{ 0 };
        std::atomic<int> foregrounds{ 0 };
        std::atomic<int> low_memory{ 0 };
        std::atomic<int> saved{ 0 };
};

Counts counts;

/// The model's books: the uid at each row, and each book's values by uid (1 to 1,000).
struct Books final
{
        std::vector<std::uint32_t> order;
        std::vector<std::int64_t> taps;
        std::vector<std::uint8_t> kept;

        Books()
            : order( book_count )
            , taps( book_count + 1 )
            , kept( book_count + 1 )
        {
            for( std::uint32_t row = 0; row < book_count; ++row ) {
                order[row] = row + 1;
            }
        }

        [[nodiscard]] std::uint32_t count() const noexcept
        {
            return static_cast<std::uint32_t>( order.size() );
        }
        [[nodiscard]] std::uint32_t uid_at( std::uint32_t row ) const noexcept
        {
            return order[row];
        }

        void publish( Telegraph &telegraph, std::uint32_t uid ) noexcept
        {
            // Its title, "book 17", made with no allocation.
            std::array<char, 16> title{};
            const int written = std::snprintf( title.data(), title.size(), "book %u", uid );
            const std::string_view text( title.data(), static_cast<std::size_t>( written ) );
            static_cast<void>( telegraph.state( book_title, uid, text ) );
            static_cast<void>( telegraph.state( book_taps, uid, taps[uid] ) );
            static_cast<void>( telegraph.state( book_kept, uid, kept[uid] != 0 ) );
        }
};

struct Prefs final
{
        explicit Prefs( Counts *counted ) noexcept
            : counts_( counted )
        {
            ++counts_->made;
        }

        void start( Telegraph &telegraph ) noexcept
        {
            CHECK( telegraph.list( books, books_ ) );
            static_cast<void>( telegraph.state( sound, sound_on_ ) );
            static_cast<void>( telegraph.state( volume, volume_ ) );
            static_cast<void>( telegraph.state( player, std::string_view( name_ ) ) );
            static_cast<void>( telegraph.state( ticks, received_ ) );
            static_cast<void>( telegraph.state( level, level_ ) );
        }

        // Each set is taken and stated back; each telegram counts a tick.
        void receive( Telegraph &telegraph, const Received &received ) noexcept
        {
            ++counts_->received;
            const sq::messaging::Name to = received.telegram.address().name;
            if( to == sound.name() ) {
                sound_on_ = received.telegram.as_boolean();
                static_cast<void>( telegraph.state( sound, sound_on_ ) );
            } else if( to == volume.name() ) {
                volume_ = received.telegram.as_number();
                static_cast<void>( telegraph.state( volume, volume_ ) );
            } else if( to == player.name() ) {
                // A received text is its bytes, all of them.
                const auto *text = reinterpret_cast<const char *>( received.data.data() );
                name_.assign( text, received.data.size() );
                static_cast<void>( telegraph.state( player, std::string_view( name_ ) ) );
            } else if( to == book_tap.name() ) {
                const std::uint32_t uid = received.telegram.address().uid;
                static_cast<void>( telegraph.state( book_taps, uid, ++books_.taps[uid] ) );
            } else if( to == book_kept.name() ) {
                const std::uint32_t uid = received.telegram.address().uid;
                books_.kept[uid] = received.telegram.as_boolean() ? 1 : 0;
                static_cast<void>( telegraph.state( book_kept, uid, books_.kept[uid] != 0 ) );
            } else if( to == books_reverse.name() ) {
                std::reverse( books_.order.begin(), books_.order.end() );
                telegraph.changed( books );
            } else if( to == books_cut.name() ) {
                books_.order.resize( 3 );
                telegraph.changed( books );
            } else if( to == flood.name() ) {
                for( std::uint32_t uid = 1; uid <= 600; ++uid ) {
                    static_cast<void>( telegraph.state( junk, uid, std::int64_t{ uid } ) );
                }
            } else if( to == level.name() ) {
                level_ = received.telegram.as_integer();
                if( level_ == 0 ) {
                    static_cast<void>( telegraph.state( level_as_number, 9.5 ) ); // the wrong type
                } else {
                    static_cast<void>( telegraph.state( level, level_ ) );
                }
            }
            static_cast<void>( telegraph.state( ticks, ++received_ ) );
        }

        void background( Telegraph & /*telegraph*/ ) noexcept { ++counts_->saves; }

        void foreground( Telegraph & /*telegraph*/, std::uint64_t /*elapsed*/ ) noexcept
        {
            ++counts_->foregrounds;
        }

        void low_memory( Telegraph & /*telegraph*/ ) noexcept { ++counts_->low_memory; }

        // What it keeps: given back as sets before start, the next time.
        void save( sq::Save &save ) noexcept
        {
            save.state( sound, sound_on_ );
            save.state( volume, volume_ );
            save.state( player, std::string_view( name_ ) );
            save.state( level, level_ );
            save.state( book_kept, 7, books_.kept[7] != 0 );
            ++counts_->saved;
        }

    private:
        Counts *counts_;
        Books books_;
        std::string name_ = "ann";
        double volume_ = 5.0;
        std::int64_t received_ = 0;
        std::int64_t level_ = 3;
        bool sound_on_ = true;
};

// --- squared_main, and how the test tells it what to do -------------------

enum class Start : std::uint8_t {
    inline_model,
    threaded_model,
    no_model,
    broken_layout,
};

Start start = Start::inline_model;
int mains = 0;
/// What squared_main tells the Stage: keep the screens in the background.
bool keep_screens = true;

} // namespace

void squared_main( sq::App &app )
{
    ++mains;
    switch( start ) {
        case Start::inline_model:
            CHECK( app.run<Prefs>( sq::messaging::on_ui_thread, &counts ) );
            CHECK( !app.run<Prefs>( sq::messaging::on_ui_thread, &counts ) ); // one to an app
            break;
        case Start::threaded_model:
            CHECK( app.run<Prefs>( &counts ) );
            break;
        case Start::no_model:
        case Start::broken_layout:
            break;
    }
    if( start == Start::broken_layout ) {
        app.stage.push( broken.layout );
        return;
    }
    CHECK( app.stage.add<About>() );
    CHECK( app.stage.add<About>() ); // again: the same screen
    CHECK( app.stage.add<Gauges>() );
    CHECK( app.stage.add<Ask>() );
    CHECK( app.stage.add<Meter>() );
    CHECK( app.stage.add<ShelfPane>() );
    CHECK( app.stage.add<Shelf>() );
    CHECK( app.stage.add<Notes>() );
    CHECK( app.stage.add<Draft>() );
    app.stage.keep_screens( keep_screens );
    CHECK( !app.stage.add<BadName>() );
    CHECK( !app.stage.add<Clash>() );
    CHECK( app.stage.custom( "gauge", make_gauge ) );
    CHECK( app.stage.custom( "opener", make_opener ) );
    app.stage.push( landing.layout );
}

namespace
{

// --- the platform, as the template's is ----------------------------------

std::vector<std::string> reports;

void collect( void * /*context*/, std::string_view line ) noexcept
{
    reports.emplace_back( line );
}

[[nodiscard]] bool reported( std::string_view part )
{
    for( const std::string &line : reports ) {
        if( line.find( part ) != std::string::npos ) {
            return true;
        }
    }
    return false;
}

constexpr int pixel_width = 1080;
constexpr int pixel_height = 2340;
// The Stage's logical size for it: the short side is 720.
constexpr float logical_width = 720.0F;
constexpr float logical_height = 1560.0F;

/// The app's bundled files at `assets`; its private files at `local`, or none.
sq::files::PosixFileSystemRoots roots_at( const std::string &assets, const std::string &local )
{
    sq::files::PosixFileSystemRoots roots;
    roots.internal_root = assets;
    roots.local_root = local;
    return roots;
}

struct Platform final
{
        explicit Platform( const std::string &assets, const std::string &local = {} )
            : files( roots_at( assets, local ) )
            , cache( files )
            , runtime{ graphics, files, cache }
        {
            sq::graphics::ContextConfig config;
            config.logical_width = pixel_width;
            config.logical_height = pixel_height;
            CHECK( graphics.create( config ) );
        }

        sq::graphics::Context graphics;
        sq::files::PosixFileSystem files;
        sq::assets::AssetManager cache;
        sq::app::Runtime runtime;

        /// What the template does when a window comes: what Android kept
        /// for the activity (nothing: a new launch), create, the surface, the size.
        void open( sq::App &app, std::span<const std::byte> saved = {} )
        {
            app.set_report( { .write = collect, .context = nullptr } );
            app.set_saved_state( saved );
            CHECK( app.create( runtime ) );
            app.surface_created( graphics );
            app.resize( pixel_width, pixel_height );
        }

        void frame( sq::App &app )
        {
            app.update( std::chrono::milliseconds( 16 ) );
            app.render( graphics );
        }
};

/// What Android keeps for the activity, as its save instance state asks the App.
std::vector<std::byte> kept_for_android( sq::App &app )
{
    std::vector<std::byte> out( sq::App::saved_state_bytes );
    out.resize( app.save_state( out ) );
    return out;
}

/// `text` as bytes Android kept.
std::vector<std::byte> bytes_of( std::string_view text )
{
    const auto *first = reinterpret_cast<const std::byte *>( text.data() );
    return { first, first + text.size() };
}

/// Bytes Android kept, as text.
std::string text_of( std::span<const std::byte> bytes )
{
    return { reinterpret_cast<const char *>( bytes.data() ), bytes.size() };
}

/// The button in `table` that says `text`; nullptr when there is none.
sq::gui::Button *button_in( sq::gui::Widget &table, std::string_view text )
{
    for( std::size_t i = 0; i < table.child_count(); ++i ) {
        // In the screens searched so, the buttons are the widgets that take
        // focus, text fields aside. Bound's slider takes it too: tap that by place.
        auto *widget = static_cast<sq::gui::Widget *>( table.child_at( i ) );
        const bool field = sq::gui::actor_cast<sq::gui::TextField>( widget ) != nullptr;
        if( field || !widget->focusable() ) {
            continue;
        }
        auto *found = static_cast<sq::gui::Button *>( widget );
        if( found->text() == text ) {
            return found;
        }
    }
    return nullptr;
}

/// The `index`th pane's window, the lowest first: after the screen's content.
sq::gui::PaneFrame &window( sq::App &app, std::size_t index )
{
    sq::gui::Group &root = *widgets_of( app )->content()->parent();
    CHECK( index + 1 < root.child_count() );
    return *sq::gui::actor_cast<sq::gui::PaneFrame>( root.child_at( index + 1 ) );
}

/// The button that says `text`, in a pane (the top one first) or the screen.
sq::gui::Button *button( sq::App &app, std::string_view text )
{
    sq::gui::Ui *ui = widgets_of( app );
    CHECK( ui != nullptr && ui->content() != nullptr );
    for( std::size_t i = ui->pane_count(); i > 0; --i ) {
        if( sq::gui::Button *found = button_in( window( app, i - 1 ).content_table(), text ) ) {
            return found;
        }
    }
    return button_in( *ui->content(), text );
}

/// Where `actor` is on the stage, in logical units.
void stage_position( const sq::gui::Actor &actor, float &x, float &y )
{
    x = 0.0F;
    y = 0.0F;
    for( const sq::gui::Actor *at = &actor; at != nullptr; at = at->parent() ) {
        x += at->x();
        y += at->y();
    }
}

/// Tap the middle of `pressed`, as a finger would: in pixels.
void tap_on( sq::App &app, const sq::gui::Actor &pressed )
{
    float x = 0.0F;
    float y = 0.0F;
    stage_position( pressed, x, y );
    x += pressed.width() * 0.5F;
    y += pressed.height() * 0.5F;
    sq::app::Event event;
    event.x = x * static_cast<float>( pixel_width ) / logical_width;
    event.y = y * static_cast<float>( pixel_height ) / logical_height;
    event.type = sq::app::Event::Type::PointerDown;
    app.handle_event( event );
    event.type = sq::app::Event::Type::PointerUp;
    app.handle_event( event );
}

/// Press or release on `pressed`, at `across` (0 to 1) of its width, as a finger would.
void touch( sq::App &app, const sq::gui::Actor &pressed, float across, sq::app::Event::Type type )
{
    float x = 0.0F;
    float y = 0.0F;
    stage_position( pressed, x, y );
    x += pressed.width() * across;
    y += pressed.height() * 0.5F;
    sq::app::Event event;
    event.x = x * static_cast<float>( pixel_width ) / logical_width;
    event.y = y * static_cast<float>( pixel_height ) / logical_height;
    event.type = type;
    app.handle_event( event );
}

/// Tap the button that says `text`.
void tap( sq::App &app, std::string_view text )
{
    sq::gui::Button *pressed = button( app, text );
    CHECK( pressed != nullptr );
    tap_on( app, *pressed );
}

void press_back( sq::App &app )
{
    sq::app::Event event;
    event.type = sq::app::Event::Type::BackRequested;
    app.handle_event( event );
}

/// The app's bundled files: none at all. The Stage's skin is compiled in.
std::string assets_directory()
{
    return "no-such-directory";
}

sq::App the_app;

// --- the tests ------------------------------------------------------------

void starts_and_navigates()
{
    start = Start::inline_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    CHECK( mains == 1 );
    CHECK( counts.made == 1 );
    CHECK( reported( "a model runs already" ) );
    CHECK( !app.stage.failed() );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );
    CHECK( widgets_of( app )->content()->child_count() == 8 );

    // The system bars are hidden unless the app says otherwise (7.8).
    CHECK( app.hides_bars() );
    app.stage.hide_bars( false );
    CHECK( !app.hides_bars() && !app.stage.hides_bars() );
    app.stage.hide_bars( true );

    // The model's first state arrives, and a tap's telegram reaches it.
    platform.frame( app );
    CHECK( app.telegraph().value( sound ) == true );
    tap( app, "Quiet" );
    platform.frame( app );
    platform.frame( app );
    CHECK( counts.received == 1 );
    CHECK( app.telegraph().value( sound ) == false );

    // Navigation waits for the tap's delivery, then builds the next screen.
    tap( app, "Settings" );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &settings.layout );
    CHECK( button( app, "Back" ) != nullptr );
    tap( app, "Again" );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &landing.layout );
    tap( app, "Settings" );
    CHECK( app.stage.depth() == 3 );
    tap( app, "Back" );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &landing.layout );
    press_back( app );
    CHECK( app.stage.depth() == 1 );
    CHECK( !app.quit_requested() );

    // Taps that go nowhere say so, and change nothing.
    tap( app, "Nowhere" );
    CHECK( reported( "push nowhere: no layout or added screen is called that" ) );
    CHECK( app.stage.depth() == 1 );

    // The stack holds max_depth.
    for( std::size_t i = 1; i < sq::Stage::max_depth; ++i ) {
        CHECK( app.stage.push( settings.layout ) );
    }
    CHECK( !app.stage.push( settings.layout ) );
    CHECK( reported( "the stack holds 16 screens" ) );
    while( app.stage.back() ) {
    }
    CHECK( app.stage.depth() == 1 );

    // A frame allocates nothing.
    platform.frame( app );
    for( int i = 0; i < 20; ++i ) {
        sq::dev::frame_begin();
        platform.frame( app );
        const sq::dev::FrameAllocations made = sq::dev::frame_end();
        CHECK( made.count == 0 );
    }

    // Leaving and coming back reach the model; low memory too.
    app.background();
    app.foreground();
    app.low_memory();
    platform.frame( app );
    CHECK( counts.saves == 1 && counts.foregrounds == 1 && counts.low_memory == 1 );

    // An activity made again (no quit): the model runs on, squared_main
    // does not run again, and the stack comes back with new widgets.
    // Android keeps what the App gives it for the new one.
    app.stage.push( settings.layout );
    const std::vector<std::byte> android = kept_for_android( app );
    app.dispose();
    CHECK( app.running() );
    CHECK( widgets_of( app ) == nullptr );
    Platform again( assets_directory() );
    again.open( app, android );
    CHECK( mains == 1 && counts.made == 1 );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &settings.layout );
    CHECK( button( app, "Back" ) != nullptr );
    tap( app, "Back" );

    // Back at the root sends the app to the background, as Home does: it
    // does not quit (it did until 7.8). Asked once, taken once.
    CHECK( !app.take_background_request() );
    press_back( app );
    CHECK( !app.quit_requested() && app.stage.depth() == 1 );
    CHECK( app.take_background_request() && !app.take_background_request() );

    // A quit: the model saves and stops, and Android is given nothing to keep.
    app.quit();
    CHECK( app.quit_requested() );
    std::array<std::byte, 64> none{};
    CHECK( app.save_state( none ) == 0 );
    const int saves = counts.saves;
    app.dispose();
    CHECK( !app.running() );
    CHECK( counts.saves == saves + 1 );
    CHECK( app.stage.depth() == 0 );
    CHECK( !app.quit_requested() );

    // The next launch in the same process starts afresh: a new model, and a
    // Telegraph that holds nothing of the last one's.
    Platform third( assets_directory() );
    third.open( app );
    CHECK( mains == 2 && counts.made == 2 );
    CHECK( !app.telegraph().value( sound ).has_value() );
    third.frame( app );
    CHECK( app.telegraph().value( sound ) == true );
    CHECK( app.stage.depth() == 1 );
    tap( app, "Quit" );
    CHECK( app.quit_requested() );
    app.dispose();
    CHECK( !app.running() );
}

void model_on_its_thread()
{
    start = Start::threaded_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    CHECK( app.running() );
    CHECK( !app.telegraph().value( sound ).has_value() );
    tap( app, "Quiet" );
    // The model answers on its thread; the frame drains it.
    bool answered = false;
    for( int i = 0; i < 400 && !answered; ++i ) {
        platform.frame( app );
        answered = app.telegraph().value( sound ) == false;
        if( !answered ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        }
    }
    CHECK( answered );
    app.quit();
    const int saves = counts.saves;
    app.dispose(); // joins the model's thread
    CHECK( counts.saves == saves + 1 );
}

void cpp_screens()
{
    start = Start::no_model;
    reports.clear();
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    // names.json's screens, checked at start; the refused adds said why.
    CHECK( reported( "lists the screen missing, and it is not added" ) );
    CHECK( !reported( "lists the screen about" ) && !reported( "lists the screen gauges" ) );
    CHECK( reported( "add Bad name: not a name" ) );
    CHECK( reported( "add landing: a layout file has that name" ) );

    // A typed tap reaches a C++ screen, by its name.
    tap( app, "About" );
    CHECK( app.stage.showing<About>() && app.stage.depth() == 2 );
    CHECK( app.stage.top_name() == "about" && app.stage.top()->name() == "about" );
    CHECK( widgets_of( app )->content()->child_count() == 3 );

    // A custom node made by its maker; one with none stands in, and is said.
    tap( app, "Gauges" );
    CHECK( app.stage.showing<Gauges>() && app.stage.depth() == 3 );
    sq::gui::Widget &root = *widgets_of( app )->content();
    CHECK( static_cast<sq::gui::Label *>( root.child_at( 0 ) )->text() == "made gauge" );
    CHECK( reported( "stage: gauges: " ) );
    platform.frame( app );
    for( int i = 0; i < 10; ++i ) {
        sq::dev::frame_begin();
        platform.frame( app );
        CHECK( sq::dev::frame_end().count == 0 );
    }

    // Panes opened from inside an event wait for it, and count as open.
    opening = &app.stage;
    tap_on( app, *root.child_at( 2 ) );
    CHECK( opener_opened && opener_saw == 2 );
    CHECK( app.stage.panes() == 2 && widgets_of( app )->pane_count() == 2 );
    CHECK( app.stage.close() && app.stage.close() && !app.stage.close() );
    opening = nullptr;

    // Back builds the C++ screen again, then the layout beneath it.
    press_back( app );
    CHECK( app.stage.showing<About>() && button( app, "Gauges" ) != nullptr );
    tap( app, "Back" );
    CHECK( app.stage.top() == &landing.layout && app.stage.depth() == 1 );

    // A name no layout or added screen has.
    tap( app, "Hidden" );
    CHECK( reported( "push hidden: no layout or added screen is called that" ) );

    // A screen bigger than the scratch shows what fit, and says so.
    CHECK( app.stage.push<Big>() );
    CHECK( app.stage.showing<Big>() );
    CHECK( reported( "big: not all of it was written" ) );
    CHECK( app.stage.back() );

    // A C++ screen as the root; its Back leaves.
    app.stage.replace<About>();
    CHECK( app.stage.depth() == 1 && app.stage.showing<About>() );
    tap( app, "Back" );
    CHECK( !app.quit_requested() && app.take_background_request() );
    app.quit();
    app.dispose();

    // After the quit, squared_main adds them again; nothing is said twice.
    reports.clear();
    Platform again( assets_directory() );
    again.open( app );
    CHECK( reported( "lists the screen missing" ) && !reported( "another screen" ) );
    tap( app, "About" );
    CHECK( app.stage.showing<About>() );
    app.quit();
    app.dispose();
}

void panes()
{
    start = Start::no_model;
    reports.clear();
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    sq::gui::Ui &ui = *widgets_of( app );

    // A tap opens a pane over the screen, which stays: modal, at the bottom,
    // the width of the screen, no title bar.
    tap( app, "Pane" );
    CHECK( app.stage.panes() == 1 && app.stage.pane_name() == "discard" );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );
    CHECK( ui.pane_count() == 1 );
    const sq::gui::PaneFrame &bottom = window( app, 0 );
    CHECK( bottom.modal() && bottom.style() == "pane" );
    // The skin's pane style (default-pane), not its window's, which has a
    // title bar painted in.
    const sq::gui::Skin &skin = widgets_of( app )->skin();
    const sq::gui::WindowStyle &framed = skin.window_style( "pane" );
    CHECK( framed.background && framed.background != skin.window_style( "default" ).background );
    // No title bar (36 tall by default): the content starts at the insets.
    CHECK( bottom.content_table().y() < 36.0F );
    CHECK( bottom.x() == 0.0F && bottom.width() == logical_width );
    CHECK( bottom.y() + bottom.height() == logical_height );
    CHECK( bottom.height() < logical_height );

    // Modal: the screen beneath takes no taps.
    tap( app, "Settings" );
    CHECK( app.stage.depth() == 1 && app.stage.panes() == 1 );

    // A pane opens another; over a modal pane it is modal too. On the
    // right: the height of the screen.
    tap( app, "Side" );
    CHECK( app.stage.panes() == 2 && app.stage.pane_name() == "side" );
    const sq::gui::PaneFrame &right = window( app, 1 );
    CHECK( right.modal() );
    CHECK( right.x() + right.width() == logical_width && right.x() > 0.0F );
    CHECK( right.y() == 0.0F && right.height() == logical_height );

    // Back closes the top pane first; a close closes the pane it is in.
    press_back( app );
    CHECK( app.stage.panes() == 1 && app.stage.pane_name() == "discard" );
    CHECK( ui.pane_count() == 1 );
    tap( app, "No" );
    CHECK( app.stage.panes() == 0 && ui.pane_count() == 0 && app.stage.depth() == 1 );
    CHECK( app.stage.pane_name().empty() );

    // Not modal: a tap outside reaches the screen. Leaving the screen closes
    // its panes, and they do not come back with it.
    CHECK( app.stage.open( side.layout ) );
    CHECK( !window( app, 0 ).modal() );
    tap( app, "Settings" );
    CHECK( app.stage.depth() == 2 && app.stage.panes() == 0 && ui.pane_count() == 0 );
    CHECK( app.stage.back() );
    CHECK( app.stage.depth() == 1 && app.stage.panes() == 0 && ui.pane_count() == 0 );

    // A C++ pane, by its type: on the left. A close in a pane that is not
    // the top one closes that one.
    CHECK( app.stage.open( side.layout ) );
    CHECK( app.stage.open<Ask>() );
    CHECK( app.stage.pane_name() == "ask" && app.stage.panes() == 2 );
    const sq::gui::PaneFrame &left = window( app, 1 );
    CHECK( left.x() == 0.0F && left.height() == logical_height && !left.modal() );
    tap( app, "Shut" );
    CHECK( app.stage.panes() == 1 && app.stage.pane_name() == "ask" );
    tap( app, "Done" );
    CHECK( app.stage.panes() == 0 && ui.pane_count() == 0 );

    // A pane's tap navigates the screen: away, and the panes go with it.
    tap( app, "Pane" );
    tap( app, "Away" );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &settings.layout );
    CHECK( app.stage.panes() == 0 && ui.pane_count() == 0 );
    CHECK( app.stage.back() );

    // By name, an added C++ screen opens as a pane.
    tap( app, "Pane" );
    tap( app, "Ask" );
    CHECK( app.stage.pane_name() == "ask" && window( app, 1 ).modal() );
    while( app.stage.close() ) {
    }
    CHECK( app.stage.panes() == 0 && ui.pane_count() == 0 );

    // What is refused, and said.
    CHECK( !app.stage.open( landing.layout ) );
    CHECK( reported( "open landing: a screen: push it" ) );
    CHECK( !app.stage.push( discard.layout ) );
    CHECK( reported( "push discard: a pane: open it" ) );
    CHECK( !app.stage.replace( discard.layout ) );
    CHECK( reported( "replace discard: a pane: open it" ) );
    CHECK( !app.stage.close() );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );

    // Four at most; a frame with them allocates nothing.
    for( std::size_t i = 0; i < sq::Stage::max_panes; ++i ) {
        CHECK( app.stage.open( side.layout ) );
    }
    CHECK( !app.stage.open( side.layout ) && !app.stage.open<Ask>() );
    CHECK( reported( "open side: 4 panes are open already" ) );
    CHECK( ui.pane_count() == 4 );
    platform.frame( app );
    for( int i = 0; i < 10; ++i ) {
        sq::dev::frame_begin();
        platform.frame( app );
        CHECK( sq::dev::frame_end().count == 0 );
    }

    // An activity made again: the screen and its panes come back, built anew.
    const std::vector<std::byte> android = kept_for_android( app );
    app.dispose();
    CHECK( widgets_of( app ) == nullptr && app.stage.panes() == 4 );
    Platform again( assets_directory() );
    again.open( app, android );
    CHECK( app.stage.panes() == 4 && widgets_of( app )->pane_count() == 4 );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );

    // Back closes them one by one, then leaves at the root.
    for( std::size_t i = 0; i < sq::Stage::max_panes; ++i ) {
        press_back( app );
    }
    CHECK( app.stage.panes() == 0 && widgets_of( app )->pane_count() == 0 );
    CHECK( !app.quit_requested() );
    press_back( app );
    CHECK( !app.quit_requested() && app.take_background_request() );
    app.quit();
    app.dispose();
}

[[nodiscard]] bool near( float a, float b )
{
    return std::abs( a - b ) < 0.05F;
}

void bound_values()
{
    start = Start::inline_model;
    reports.clear();
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    CHECK( app.stage.push<Bound>() );
    sq::gui::Widget &root = *widgets_of( app )->content();
    auto &count = *static_cast<sq::gui::Label *>( root.child_at( 0 ) );
    auto &box = *static_cast<sq::gui::CheckBox *>( root.child_at( 1 ) );
    auto &slider = *static_cast<sq::gui::Slider *>( root.child_at( 2 ) );
    auto &shown = *static_cast<sq::gui::Label *>( root.child_at( 3 ) );
    auto &bar = *static_cast<sq::gui::ProgressBar *>( root.child_at( 4 ) );
    auto &field = *static_cast<sq::gui::TextField *>( root.child_at( 5 ) );
    auto &steps = *static_cast<sq::gui::Slider *>( root.child_at( 6 ) );
    const sq::gui::Actor &meter_button = *root.child_at( 7 );
    // Seven widgets, five addresses held in the cache (volume is shown three times).
    CHECK( app.stage.bindings() == 7 );
    CHECK( app.telegraph().cache_stats().bound == 5 );

    // Until the model says, each shows its own; then what it says.
    CHECK( count.text() == "no ticks yet" && shown.text() == "-" );
    platform.frame( app );
    CHECK( count.text() == "0" && box.checked() && near( slider.value(), 5.0F ) );
    CHECK( shown.text() == "5.0" && near( bar.value(), 5.0F ) && field.text() == "ann" );

    // A check box sends set; the model's answer shows, and is not sent back.
    tap_on( app, box );
    CHECK( !box.checked() );
    platform.frame( app );
    CHECK( app.telegraph().value( sound ) == false && !box.checked() && count.text() == "1" );
    platform.frame( app );
    CHECK( count.text() == "1" ); // nothing more was sent

    // A slider held: one set a frame, the last; what the model says
    // meanwhile waits until it is let go, and the other widgets show it.
    touch( app, slider, 0.8F, sq::app::Event::Type::PointerDown );
    touch( app, slider, 0.7F, sq::app::Event::Type::PointerMove );
    touch( app, slider, 0.8F, sq::app::Event::Type::PointerMove );
    CHECK( slider.dragging() && near( slider.value(), 8.0F ) );
    platform.frame( app ); // sends set 8, once
    CHECK( app.telegraph().set( volume, 2.0 ) == sq::messaging::SendResult::sent );
    platform.frame( app );
    CHECK( count.text() == "3" ); // the drag's one set, and the other
    CHECK( shown.text() == "2.0" && near( bar.value(), 2.0F ) && near( slider.value(), 8.0F ) );
    touch( app, slider, 0.8F, sq::app::Event::Type::PointerUp );
    CHECK( !slider.dragging() );
    platform.frame( app );
    CHECK( near( slider.value(), 2.0F ) && count.text() == "3" );

    // A text field sends when the edit is done; the model's value meanwhile
    // waits until the field is left.
    tap_on( app, field );
    CHECK( field.focused() );
    CHECK( field.text_input( "!" ) && field.text() == "ann!" );
    platform.frame( app );
    CHECK( app.telegraph().value( player ) == "ann" ); // not yet: no Done
    CHECK( field.key_down( sq::app::Key::enter ) );
    platform.frame( app );
    CHECK( app.telegraph().value( player ) == "ann!" );
    const sq::messaging::SendResult zed = app.telegraph().set( player, std::string_view( "zed" ) );
    CHECK( zed == sq::messaging::SendResult::sent );
    platform.frame( app );
    CHECK( field.text() == "ann!" );
    widgets_of( app )->clear_focus();
    platform.frame( app );
    CHECK( field.text() == "zed" && app.telegraph().value( player ) == "zed" );

    // Left with changes, without Done: the edit is finished, and sent.
    tap_on( app, field );
    CHECK( field.text_input( "?" ) );
    widgets_of( app )->clear_focus();
    platform.frame( app );
    CHECK( app.telegraph().value( player ) == "zed?" );

    // A slider bound to an integer sends a whole number, the nearest.
    CHECK( near( steps.value(), 3.0F ) );
    touch( app, steps, 0.66F, sq::app::Event::Type::PointerDown );
    touch( app, steps, 0.66F, sq::app::Event::Type::PointerUp );
    platform.frame( app );
    platform.frame( app );
    const std::int64_t sent = app.telegraph().value( level ).value_or( 0 );
    CHECK( sent > 3 && sent < 10 && near( steps.value(), static_cast<float>( sent ) ) );

    // A value of another type than the one bound is not shown: the model's
    // key and the layout disagree, and the widget keeps what it had.
    const float before = steps.value();
    CHECK( app.telegraph().set( level, std::int64_t{ 0 } ) == sq::messaging::SendResult::sent );
    platform.frame( app );
    CHECK( near( steps.value(), before ) );

    // A frame with nothing new allocates nothing.
    platform.frame( app );
    for( int i = 0; i < 10; ++i ) {
        sq::dev::frame_begin();
        platform.frame( app );
        CHECK( sq::dev::frame_end().count == 0 );
    }

    // A value shown again allocates nothing when it is no longer than the
    // last: the widget's text is reused (and these are past the short-string
    // size, which would allocate nothing anyway).
    const std::string_view longer = "a name longer than sixteen";
    const std::string_view again = "another name, as long!";
    CHECK( app.telegraph().set( player, longer ) == sq::messaging::SendResult::sent );
    platform.frame( app );
    CHECK( field.text() == longer );
    CHECK( app.telegraph().set( player, again ) == sq::messaging::SendResult::sent );
    sq::dev::frame_begin();
    platform.frame( app );
    CHECK( sq::dev::frame_end().count == 0 && field.text() == again );

    // A pane's bound widget shows the value at once; closing it unbinds it.
    tap_on( app, meter_button );
    CHECK( app.stage.bindings() == 8 );
    auto *meter = button( app, "Shut" )->parent();
    CHECK( static_cast<sq::gui::Label *>( meter->child_at( 0 ) )->text() == "2.0" );
    tap( app, "Shut" );
    CHECK( app.stage.bindings() == 7 && app.telegraph().cache_stats().bound == 5 );

    // Leaving the screen unbinds them all.
    CHECK( app.stage.back() );
    CHECK( app.stage.bindings() == 0 && app.telegraph().cache_stats().bound == 0 );

    // More than a window holds: the rest show their own text, and it is said.
    CHECK( app.stage.push<Crowded>() );
    CHECK( app.stage.bindings() == sq::Stage::max_bindings );
    CHECK( reported( "crowded: more than 128 bound values" ) );
    CHECK( app.stage.back() );
    CHECK( app.stage.bindings() == 0 );

    app.quit();
    app.dispose();
    CHECK( app.telegraph().cache_stats().bound == 0 );
}

/// The widget `which` of the row showing book row `row`: nullptr when no row table shows it.
sq::gui::Widget *in_row( sq::gui::List &list, std::uint32_t row, std::size_t which )
{
    const std::size_t slot = list.slot_showing( row );
    if( slot >= list.rows_built() ) {
        return nullptr;
    }
    auto &cells = *static_cast<sq::gui::Widget *>( list.table( slot ).child_at( 0 ) );
    return static_cast<sq::gui::Widget *>( cells.child_at( which ) );
}

[[nodiscard]] std::string title_at( sq::gui::List &list, std::uint32_t row )
{
    const sq::gui::Widget *title = in_row( list, row, 0 );
    return title != nullptr ? static_cast<const sq::gui::Label *>( title )->text() : "(none)";
}

/// Every row on screen shows the book the model has there: `book <uid>`.
[[nodiscard]] bool rows_shown( sq::gui::List &list, bool reversed )
{
    const std::uint32_t first = list.first_row();
    const std::uint32_t last = std::min( first + list.rows_on_screen(), list.count() );
    for( std::uint32_t row = first; row < last; ++row ) {
        const std::uint32_t uid = reversed ? book_count - row : row + 1;
        if( title_at( list, row ) != "book " + std::to_string( uid ) ) {
            std::fprintf( stderr, "row %u shows %s\n", row, title_at( list, row ).c_str() );
            return false;
        }
    }
    return true;
}

void bound_lists()
{
    start = Start::inline_model;
    reports.clear();
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    platform.frame( app );
    const std::size_t bound_before = app.telegraph().cache_stats().bound;
    CHECK( app.stage.push<Shelf>() );
    sq::gui::Widget &root = *widgets_of( app )->content();
    sq::gui::List &list = *sq::gui::actor_cast<sq::gui::List>( root.child_at( 0 ) );
    CHECK( app.stage.lists() == 1 );
    // Each row table's three values are bound; the list itself is not a value.
    CHECK( app.stage.bindings() == list.rows_built() * 3 );
    // Built as it was laid out: the rows on screen and one more, empty until
    // the model says. The count was in the cache already: the model's start.
    CHECK( list.rows_built() == list.rows_on_screen() + 1 && list.rows_built() > 10 );
    CHECK( list.row_height() > 0.0F && list.count() == book_count );
    CHECK( title_at( list, 0 ).empty() );

    // The reset brings the count; the view, the rows and their books.
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    CHECK( list.count() == book_count && app.telegraph().value( books_count ) == 1000 );
    CHECK( rows_shown( list, false ) );
    CHECK( static_cast<sq::gui::Label *>( in_row( list, 0, 1 ) )->text() == "0" );

    // A row's tap goes to its book, though the finger moved a little; a
    // row's check box sets its book's value.
    sq::gui::Widget &tapped = *in_row( list, 2, 3 );
    touch( app, tapped, 0.5F, sq::app::Event::Type::PointerDown );
    touch( app, tapped, 0.55F, sq::app::Event::Type::PointerMove );
    touch( app, tapped, 0.55F, sq::app::Event::Type::PointerUp );
    platform.frame( app );
    platform.frame( app );
    CHECK( static_cast<sq::gui::Label *>( in_row( list, 2, 1 ) )->text() == "1" );
    CHECK( app.telegraph().value( book_taps, 3 ) == 1 );
    CHECK( app.telegraph().value( book_taps, 1 ) == 0 );
    tap_on( app, *in_row( list, 4, 2 ) );
    platform.frame( app );
    platform.frame( app );
    CHECK( static_cast<sq::gui::CheckBox *>( in_row( list, 4, 2 ) )->checked() );
    CHECK( app.telegraph().value( book_kept, 5 ) == true );

    // Kept inside its rows: past the last is the last.
    list.set_scroll_y( 1.0e9F );
    const float last = static_cast<float>( book_count ) * list.row_height() - list.height();
    CHECK( list.scroll_y() == last );

    // Scrolled far: rows asked for as they come; the same tables, other books.
    const std::size_t built = list.rows_built();
    list.set_scroll_y( list.row_height() * 500.0F );
    platform.frame( app );
    // Before the model answers, a row shows nothing, and its tap has no book to go to.
    CHECK( title_at( list, 500 ).empty() );
    tap_on( app, *in_row( list, 500, 3 ) );
    CHECK( reported( "stage: tap do book.tap: its row's item has not come yet; not sent" ) );
    platform.frame( app );
    platform.frame( app );
    CHECK( list.first_row() == 500 && rows_shown( list, false ) && list.rows_built() == built );

    // A full cache lets go of what it does not keep, and keeps the rows
    // asked for: scrolled inside them, nothing is asked, and all shows.
    CHECK( app.telegraph().do_( flood ).result == sq::messaging::SendResult::sent );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    // A row stays in its table while it is on screen: one row moves a row.
    const std::size_t table_of_515 = list.slot_showing( 515 );
    list.set_scroll_y( list.row_height() * 510.0F );
    platform.frame( app );
    CHECK( rows_shown( list, false ) && list.slot_showing( 515 ) == table_of_515 );

    // The cache keeps the rows asked for and their books, and no more.
    const std::size_t kept = app.telegraph().cache_stats().bound;
    const std::size_t half = sq::messaging::telegraph_size::cache / 2;
    CHECK( kept > bound_before && kept - bound_before <= half + 64 );

    // A drag on a row's button scrolls the list, and taps nothing.
    const float before = list.scroll_y();
    const std::uint32_t dragged = list.first_row() + 5;
    sq::gui::Widget &button = *in_row( list, dragged, 3 );
    touch( app, button, 0.5F, sq::app::Event::Type::PointerDown );
    float x = 0.0F;
    float y = 0.0F;
    stage_position( button, x, y );
    sq::app::Event drag;
    drag.type = sq::app::Event::Type::PointerMove;
    drag.x = ( x + button.width() * 0.5F ) * static_cast<float>( pixel_width ) / logical_width;
    for( int step = 1; step <= 5; ++step ) {
        const float up = y + button.height() * 0.5F - static_cast<float>( step ) * 20.0F;
        drag.y = up * static_cast<float>( pixel_height ) / logical_height;
        app.handle_event( drag );
    }
    drag.type = sq::app::Event::Type::PointerUp;
    app.handle_event( drag );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    // As far as the finger went, once; the button let go, and was not tapped.
    CHECK( std::abs( list.scroll_y() - before - 100.0F ) < 1.0F );
    CHECK( !static_cast<sq::gui::Button &>( button ).pressed() );
    CHECK( app.telegraph().value( book_taps, dragged + 1 ) == 0 && rows_shown( list, false ) );

    // A drag where no widget takes the finger (a title) scrolls it as far.
    const float from = list.scroll_y();
    sq::gui::Widget &title = *in_row( list, list.first_row() + 3, 0 );
    touch( app, title, 0.5F, sq::app::Event::Type::PointerDown );
    x = 0.0F;
    y = 0.0F;
    stage_position( title, x, y );
    drag.type = sq::app::Event::Type::PointerMove;
    drag.x = ( x + title.width() * 0.5F ) * static_cast<float>( pixel_width ) / logical_width;
    const float raised = y + title.height() * 0.5F - 60.0F;
    drag.y = raised * static_cast<float>( pixel_height ) / logical_height;
    app.handle_event( drag );
    drag.type = sq::app::Event::Type::PointerUp;
    app.handle_event( drag );
    platform.frame( app );
    CHECK( std::abs( list.scroll_y() - from - 60.0F ) < 1.0F );

    // A sort: the rows on screen show the books now there.
    CHECK( app.telegraph().do_( books_reverse ).result == sq::messaging::SendResult::sent );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    CHECK( rows_shown( list, true ) );

    // Scrolling, once warm, allocates nothing: the rows, their texts, the asks.
    list.set_scroll_y( 0.0F );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    for( int step = 0; step < 40; ++step ) {
        sq::dev::frame_begin();
        list.set_scroll_y( list.row_height() * static_cast<float>( step * 7 ) );
        platform.frame( app );
        CHECK( sq::dev::frame_end().count == 0 );
    }
    platform.frame( app );
    CHECK( rows_shown( list, true ) );

    // The model keeps one view of a list: a second widget of it stays empty.
    CHECK( app.stage.open<ShelfPane>() );
    CHECK( reported( "stage: shelf_pane: books is shown already; this one stays empty" ) );
    CHECK( app.stage.lists() == 1 );
    tap( app, "Shut" );

    // Leaving the screen lets the list go, and all the cache kept for it.
    CHECK( app.stage.back() );
    CHECK( app.stage.lists() == 0 && app.stage.bindings() == 0 );
    CHECK( app.telegraph().cache_stats().bound == bound_before );

    // Again, once the cache has let it all go: a new widget, which asks for
    // all of it, from its count; the model sends it again.
    CHECK( app.telegraph().do_( flood ).result == sq::messaging::SendResult::sent );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    CHECK( !app.telegraph().value( books_count ).has_value() ); // its count too
    CHECK( app.stage.push<Shelf>() );
    for( int i = 0; i < 4; ++i ) {
        platform.frame( app );
    }
    sq::gui::Actor *second = widgets_of( app )->content()->child_at( 0 );
    sq::gui::List &again = *sq::gui::actor_cast<sq::gui::List>( second );
    CHECK( again.count() == book_count && rows_shown( again, true ) );
    again.set_scroll_y( again.row_height() * static_cast<float>( book_count - 10 ) );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    CHECK( rows_shown( again, true ) ); // book 5, kept, is on row 995 now
    CHECK( static_cast<sq::gui::CheckBox *>( in_row( again, book_count - 5, 2 ) )->checked() );

    // Cut to three: the count, the scroll back inside it, the rows past it empty.
    CHECK( app.telegraph().do_( books_cut ).result == sq::messaging::SendResult::sent );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    CHECK( again.count() == 3 && again.scroll_y() == 0.0F && rows_shown( again, true ) );
    CHECK( in_row( again, 3, 0 ) == nullptr && title_at( again, 2 ) == "book 998" );

    app.quit();
    app.dispose();
    CHECK( app.telegraph().cache_stats().bound == 0 );
}

// --- what screens keep (7.6) ------------------------------------------------

/// A directory of private files of its own, empty: `<TMPDIR>/sq_stage_<name>`.
std::string private_files( std::string_view name )
{
    const char *temporary = std::getenv( "TMPDIR" );
    std::string path = std::string( temporary != nullptr ? temporary : "/tmp" ) + "/sq_stage_";
    path += name;
    std::filesystem::remove_all( path );
    std::filesystem::create_directories( path );
    return path;
}

/// A file's bytes as text; empty when there is none.
std::string read_file( const std::string &path )
{
    std::ifstream in( path, std::ios::binary );
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

void write_file( const std::string &path, std::string_view text )
{
    std::ofstream out( path, std::ios::binary | std::ios::trunc );
    out << text;
}

/// The process ends while the app is in the background, and starts again:
/// a new App in the same place, as Android's next process makes one.
void new_process()
{
    std::destroy_at( &the_app );
    std::construct_at( &the_app );
}

template<typename K>
concept saves = requires( sq::Save &save, const K &key ) { save.state( key, std::int64_t{} ); };

// A save keeps what the model takes set on: a state-only key does not compile.
static_assert( saves<decltype( level )> && !saves<decltype( ticks )> );

/// The text field a pane opens with: the `index`th pane's first widget.
sq::gui::TextField *pane_field( sq::App &app, std::size_t index );
static_assert( sq::SavingModel<Prefs> && !sq::SavingModel<Books> );

sq::gui::TextField *pane_field( sq::App &app, std::size_t index )
{
    sq::gui::Actor *first = window( app, index ).content_table().child_at( 0 );
    return sq::gui::actor_cast<sq::gui::TextField>( first );
}

void rotation_lays_out_once()
{
    start = Start::inline_model;
    reports.clear();
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    CHECK( app.stage.push<Shelf>() );
    platform.frame( app );
    platform.frame( app );
    sq::gui::Widget *const content = widgets_of( app )->content();
    sq::gui::List &list = *sq::gui::actor_cast<sq::gui::List>( content->child_at( 0 ) );
    const std::size_t built = list.rows_built();
    sq::gui::Table *const first = &list.table( 0 );
    CHECK( list.width() < logical_width && list.height() > logical_width );

    // The size, then the insets, as a rotation brings them: nothing is laid
    // out until the next frame, and then once, with both; nothing is built.
    app.resize( pixel_height, pixel_width );
    CHECK( sq::detail::StageProbe::layout_due( app.stage ) );
    sq::app::SafeArea notch{};
    notch.left = 90;
    app.safe_area_changed( notch );
    CHECK( sq::detail::StageProbe::layout_due( app.stage ) );
    CHECK( list.width() < logical_width ); // not yet
    platform.frame( app );
    CHECK( !sq::detail::StageProbe::layout_due( app.stage ) );
    CHECK( widgets_of( app )->content() == content && &list.table( 0 ) == first );
    CHECK( list.rows_built() == built ); // shorter: no more tables needed
    CHECK( list.width() > logical_width && list.height() < logical_width );
    float x = 0.0F;
    float y = 0.0F;
    stage_position( list, x, y );
    CHECK( x >= 59.0F ); // 90 pixels of 2,340 across 1,560 units

    // An event lays out what waits first, so a finger lands where it shows.
    app.resize( pixel_width, pixel_height );
    app.safe_area_changed( {} );
    sq::app::Event finger;
    finger.type = sq::app::Event::Type::PointerDown;
    finger.x = 1.0F;
    finger.y = 1.0F;
    app.handle_event( finger );
    CHECK( !sq::detail::StageProbe::layout_due( app.stage ) );
    finger.type = sq::app::Event::Type::PointerUp;
    app.handle_event( finger );
    press_back( app );
    CHECK( app.stage.depth() == 1 );
    app.quit();
    app.dispose();
}

void kept_beneath_the_top()
{
    start = Start::inline_model;
    reports.clear();
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    platform.frame( app );
    CHECK( app.stage.push<Notes>() );
    platform.frame( app );
    sq::gui::Widget *root = widgets_of( app )->content();
    auto *scroll = sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) );
    auto *note = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) );
    CHECK( scroll != nullptr && note != nullptr );
    CHECK( sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 0 ) )->text() == "ann" );
    scroll->set_scroll_y( 300.0F );
    CHECK( scroll->scroll_y() == 300.0F );
    tap_on( app, *note );
    CHECK( note->focused() && note->text_input( "half a thought" ) && note->edited() );

    // Pushed over, then back: built again, as it was, the edit unfinished.
    CHECK( app.stage.push<About>() );
    CHECK( app.stage.back() );
    root = widgets_of( app )->content();
    scroll = sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) );
    note = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) );
    CHECK( scroll->scroll_y() == 300.0F );
    CHECK( note->text() == "half a thought" && note->edited() && !note->focused() );

    // A bound field's edit is the user's while it waits: the model's value
    // does not take its place until it is finished.
    auto *name = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 0 ) );
    tap_on( app, *name );
    CHECK( name->text_input( "e" ) );
    CHECK( app.stage.push<About>() && app.stage.back() );
    root = widgets_of( app )->content();
    name = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 0 ) );
    note = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) );
    CHECK( name->text() == "anne" && name->edited() );
    CHECK( !note->edited() && note->text().empty() ); // one edit is kept: the first
    const sq::messaging::SendResult bob = app.telegraph().set( player, std::string_view( "bob" ) );
    CHECK( bob == sq::messaging::SendResult::sent );
    platform.frame( app );
    platform.frame( app );
    CHECK( name->text() == "anne" );
    tap_on( app, *name );
    CHECK( name->key_down( sq::app::Key::enter ) ); // finished: sent
    platform.frame( app );
    platform.frame( app );
    CHECK( app.telegraph().value( player ) == "anne" && !name->edited() );

    // Over 128 bytes, the first 128 kept, cut where a character starts.
    tap_on( app, *note );
    std::string long_note( 127, 'a' );
    long_note += "\xC3\xA9 and more"; // é across byte 128
    CHECK( note->text_input( long_note ) );
    CHECK( app.stage.push<About>() && app.stage.back() );
    note = sq::gui::actor_cast<sq::gui::TextField>( widgets_of( app )->content()->child_at( 2 ) );
    CHECK( note->text() == std::string( 127, 'a' ) && note->edited() );

    // A lost context builds the screen again (its new widgets may well be
    // where the old ones were in memory): it keeps where it was.
    root = widgets_of( app )->content();
    scroll = sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) );
    scroll->set_scroll_y( 120.0F );
    platform.graphics.destroy();
    sq::graphics::ContextConfig config;
    config.logical_width = pixel_width;
    config.logical_height = pixel_height;
    CHECK( platform.graphics.create( config ) && !platform.graphics.resources_preserved() );
    app.surface_created( platform.graphics );
    root = widgets_of( app )->content();
    scroll = sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) );
    note = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) );
    CHECK( scroll->scroll_y() == 120.0F && note->edited() );

    // The activity made again (no quit): the screen and its pane are built
    // anew, as they were.
    CHECK( app.stage.open<Draft>() );
    auto *draft = pane_field( app, 0 );
    tap_on( app, *draft );
    CHECK( draft->text_input( "pane" ) );
    const std::vector<std::byte> android = kept_for_android( app );
    app.dispose();
    Platform again( assets_directory() );
    again.open( app, android );
    root = widgets_of( app )->content();
    CHECK( sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) )->scroll_y() == 120.0F );
    CHECK( sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) )->edited() );
    draft = pane_field( app, 0 );
    CHECK( draft->text() == "pane" && draft->edited() );
    tap( app, "Drop" );

    // A list's place: put back once its rows reach it.
    CHECK( app.stage.push<Shelf>() );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    auto *list = sq::gui::actor_cast<sq::gui::List>( widgets_of( app )->content()->child_at( 0 ) );
    list->set_scroll_y( list->row_height() * 300.0F );
    const float far = list->scroll_y();
    CHECK( app.stage.push<About>() && app.stage.back() );
    list = sq::gui::actor_cast<sq::gui::List>( widgets_of( app )->content()->child_at( 0 ) );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    CHECK( list->scroll_y() == far && list->first_row() == 300 );
    CHECK( title_at( *list, 300 ) == "book 301" );
    // Back, and the screen beneath forgets what the one left kept.
    CHECK( app.stage.back() );
    CHECK( app.stage.push<Shelf>() );
    list = sq::gui::actor_cast<sq::gui::List>( widgets_of( app )->content()->child_at( 0 ) );
    platform.frame( app );
    CHECK( list->scroll_y() == 0.0F );

    // The Console's `screens`: the stack, and what each keeps.
    reports.clear();
    app.stage.describe( { .write = collect, .context = nullptr } );
    CHECK( reported( "screens: 3, panes: 0; kept in the background" ) );
    CHECK( reported( "  1 landing" ) && reported( "  3 shelf  scroll 0" ) );
    CHECK( reported( "  2 notes  scroll 120  edit in text field 1, 127 bytes" ) );
    CHECK( reported( "bound: " ) );

    // Low memory: the tables' working sizes go, and come back when needed.
    CHECK( sq::gui::Table::kept_count() > 0 );
    const int heard = counts.low_memory;
    app.low_memory();
    CHECK( sq::gui::Table::kept_count() == 0 );
    platform.frame( app );
    CHECK( counts.low_memory == heard + 1 );
    list->set_scroll_y( 400.0F );
    platform.frame( app );
    CHECK( sq::gui::Table::kept_count() > 0 );
    app.quit();
    app.dispose();
}

void kept_after_the_process()
{
    start = Start::inline_model;
    reports.clear();
    const std::string local = private_files( "keep" );
    const std::string screens = local + "/squared/screens"; // 7.6's; no longer written
    const std::string model = local + "/squared/model.state";
    std::filesystem::create_directories( local + "/squared" );
    write_file( screens, "squared screens 1\nscreen notes 0 - -\n" ); // left by 7.7
    Platform platform( assets_directory(), local );
    sq::App &app = the_app;
    platform.open( app );
    CHECK( std::filesystem::is_directory( local + "/squared" ) );
    CHECK( !std::filesystem::exists( screens ) && app.stage.depth() == 1 ); // removed, not shown
    platform.frame( app );
    tap( app, "Quiet" ); // the model's sound off
    const sq::messaging::SendResult zed = app.telegraph().set( player, std::string_view( "zed" ) );
    CHECK( zed == sq::messaging::SendResult::sent );
    CHECK( app.stage.push<Notes>() );
    platform.frame( app );
    sq::gui::Widget *root = widgets_of( app )->content();
    sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) )->set_scroll_y( 300.0F );
    auto *note = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) );
    tap_on( app, *note );
    CHECK( note->text_input( "hi \"you\"" ) );
    CHECK( app.stage.push<Shelf>() );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    auto *list = sq::gui::actor_cast<sq::gui::List>( widgets_of( app )->content()->child_at( 0 ) );
    list->set_scroll_y( list->row_height() * 200.0F );
    for( int i = 0; i < 3; ++i ) {
        platform.frame( app );
    }
    tap_on( app, *in_row( *list, 200, 2 ) ); // book 201 kept: not saved, 7 is
    CHECK( app.stage.open( side.layout ) );
    CHECK( app.stage.open<Draft>() );
    tap_on( app, *pane_field( app, 1 ) );
    CHECK( pane_field( app, 1 )->text_input( "\xC3\xA9" ) );
    platform.frame( app );

    // To the background: the model saves; Android is given the stack, whole.
    const int saved = counts.saved;
    app.background();
    CHECK( counts.saved == saved + 1 ); // on this thread: at once
    const std::vector<std::byte> android = kept_for_android( app );
    const std::string written = text_of( android );
    const long offset = std::lround( list->scroll_y() );
    const std::string expected = "squared app 1\n"
                                 "squared screens 1\n"
                                 "screen landing 0 - -\n"
                                 "screen notes 1 300 1 x68692022796f7522\n"
                                 "screen shelf 1 " +
                                 std::to_string( offset ) +
                                 " - -\n"
                                 "pane side 0 - -\n"
                                 "pane draft 0 0 xc3a9\n";
    std::printf( "%s", written.c_str() );
    CHECK( written == expected );
    CHECK( !std::filesystem::exists( screens ) );
    const std::string kept = read_file( model );
    CHECK( kept.find( "set settings.sound false\n" ) != std::string::npos );
    CHECK( kept.find( "set player.name \"zed\"\n" ) != std::string::npos );
    CHECK( kept.find( "set book[7].kept false\n" ) != std::string::npos );
    app.foreground();

    // Android ends the process in the background; the next start shows the
    // stack again, and the model has its values back before it starts: the
    // sets, and nothing else a hand put in the file.
    app.background();
    new_process();
    write_file( model, read_file( model ) + "state settings.volume 9.5\nnot a line\n" );
    reports.clear();
    const int made = counts.made;
    Platform after( assets_directory(), local );
    after.open( app, android );
    CHECK( counts.made == made + 1 );
    CHECK( app.stage.depth() == 3 && app.stage.top_name() == "shelf" );
    CHECK( app.stage.panes() == 2 && app.stage.pane_name() == "draft" );
    auto *draft = pane_field( app, 1 );
    CHECK( draft->text() == "\xC3\xA9" && draft->edited() );
    tap( app, "Drop" );
    after.frame( app );
    CHECK( app.telegraph().value( sound ) == false );
    CHECK( app.telegraph().value( player ) == "zed" && app.telegraph().value( volume ) == 5.0 );
    for( int i = 0; i < 3; ++i ) {
        after.frame( app );
    }
    list = sq::gui::actor_cast<sq::gui::List>( widgets_of( app )->content()->child_at( 0 ) );
    CHECK( std::lround( list->scroll_y() ) == offset && list->first_row() == 200 );
    tap( app, "Shut" );
    press_back( app );
    root = widgets_of( app )->content();
    CHECK( app.stage.top_name() == "notes" );
    CHECK( sq::gui::actor_cast<sq::gui::ScrollPane>( root->child_at( 1 ) )->scroll_y() == 300.0F );
    note = sq::gui::actor_cast<sq::gui::TextField>( root->child_at( 2 ) );
    CHECK( note->text() == "hi \"you\"" && note->edited() );

    // A save the model's thread cannot write is said when the app comes back.
    std::filesystem::remove( model );
    std::filesystem::create_directory( model ); // a directory in the way
    reports.clear();
    app.background();
    app.foreground();
    CHECK( reported( "the model's save could not be written" ) );
    std::filesystem::remove( model );

    // A quit: Android is given nothing, so the next launch is fresh; the
    // model's save stays.
    app.quit();
    CHECK( kept_for_android( app ).empty() );
    app.dispose();
    CHECK( std::filesystem::exists( model ) );

    // What Android kept that cannot be shown is said; the stack is squared_main's.
    reports.clear();
    Platform bad( assets_directory(), local );
    bad.open( app, bytes_of( "squared app 1\nsquared screens 1\nscreen landing 0 - -\n"
                             "screen nowhere 0 - -\n" ) );
    CHECK( reported( "the screens kept were not shown again: line 3: no layout or added screen" ) );
    CHECK( app.stage.depth() == 1 );
    const auto refused = [&app, &local]( std::string_view wrong, std::string_view said ) {
        new_process();
        reports.clear();
        Platform again( assets_directory(), local );
        again.open( app, bytes_of( wrong ) );
        CHECK( reported( said ) && app.stage.depth() == 1 );
    };
    constexpr std::string_view not_shown = "the screens kept were not shown again";
    refused( "squared app 1\nsquared screens 2\nscreen landing 0 - -\n", not_shown );
    refused( "squared app 1\nsquared screens 1\npane side 0 - -\n", not_shown );
    refused( "squared app 1\nsquared screens 1\nscreen side 0 - -\n", not_shown );
    refused( "squared app 1\nsquared screens 1\nscreen notes 5 1 2 3 4 5 - -\n", not_shown );
    refused( "squared app 1\nsquared screens 1\nscreen notes 0 1 xa\n", not_shown );
    refused( "squared app 1\nsquared screens 1\nscreen notes 0 - - more\n", not_shown );
    refused( "squared app 1\nsquared screens 1\nscreen landing 0 - -\npane side 0 - -\n"
             "screen notes 0 - -\n",
             not_shown );
    refused( "squared app 1\nsquared screens 1\nscreen notes 1 -5 - -\n", not_shown );
    refused( "squared screens 1\nscreen landing 0 - -\n", "what Android kept is not this app's" );

    // An app that keeps none: what Android kept is not shown, and it is given
    // the first line alone (which still tells a kept activity from a new launch).
    keep_screens = false;
    new_process();
    Platform off( assets_directory(), local );
    off.open( app, bytes_of( "squared app 1\nsquared screens 1\nscreen landing 0 - -\n"
                             "screen notes 0 - -\n" ) );
    CHECK( app.stage.depth() == 1 );
    CHECK( app.stage.push<Notes>() );
    app.background();
    CHECK( text_of( kept_for_android( app ) ) == "squared app 1\n" );
    keep_screens = true;

    // A screen pushed by its layout alone, not found by its name: nothing kept.
    app.stage.keep_screens( true );
    app.foreground();
    Written loose;
    sq::gui::LayoutWriter writer( loose.nodes, loose.text, loose.numbers, "loose" );
    writer.add<sq::gui::Label>( "not in layouts::all()" );
    loose.layout = writer.layout();
    CHECK( app.stage.push( loose.layout ) );
    reports.clear();
    app.background();
    CHECK( text_of( kept_for_android( app ) ) == "squared app 1\n" );
    CHECK( reported( "the screens are not kept: loose is not found by its name" ) );
    app.quit();
    app.dispose();
    std::filesystem::remove_all( local );
}

/// A keyboard that lives as long as one activity, as the template's does.
struct Keyboard final : sq::app::TextInputService
{
        bool alive = true;
        int asked = 0;
        void start( const sq::app::TextInputRequest & /*request*/ ) override
        {
            CHECK( alive );
            ++asked;
        }
        void update_area( const sq::app::TextInputArea & /*area*/ ) override { CHECK( alive ); }
        void stop() override { CHECK( alive ); }
        [[nodiscard]] bool active() const noexcept override
        {
            CHECK( alive );
            return asked > 0;
        }
};

void a_new_activity_forgets_the_old_one()
{
    // Quit, then opened again from the recent apps: the process lives on, the
    // activity that handed over its keyboard is gone. (The phone's SIGSEGV at
    // 0x28, pkg57: the Ui asked the old keyboard whether it was active.)
    start = Start::inline_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    Keyboard keyboard;
    app.set_text_input_service( &keyboard );
    CHECK( app.stage.push<Notes>() );
    tap_on( app, *widgets_of( app )->content()->child_at( 2 ) );
    CHECK( keyboard.asked == 1 );
    platform.frame( app );
    app.quit();
    app.dispose();
    keyboard.alive = false;
    reports.clear();
    Platform again( assets_directory() );
    again.open( app );
    again.frame( app );
    CHECK( app.stage.push<Notes>() );
    tap_on( app, *widgets_of( app )->content()->child_at( 2 ) );
    again.frame( app );
    // And the old activity's report is not called either: nothing was set.
    app.quit();
    app.dispose();
}

void back_tap_at_the_root_goes_to_the_background()
{
    start = Start::no_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    app.stage.replace( settings.layout ); // a root with a Back button
    CHECK( app.stage.depth() == 1 && app.stage.top() == &settings.layout );
    tap( app, "Back" );
    CHECK( app.stage.depth() == 1 );
    CHECK( !app.quit_requested() && app.take_background_request() );
    CHECK( !app.take_background_request() );
    app.quit();
    CHECK( app.quit_requested() );
    app.dispose();
    CHECK( !app.quit_requested() && app.stage.depth() == 0 );
}

/// Swiped away in the recent apps while the process lives on: the next
/// activity comes with nothing Android kept, a new launch, and starts fresh,
/// as after a quit. One Android made again keeps the run.
void a_swipe_starts_fresh()
{
    start = Start::inline_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    const int mains_before = mains;
    const int made = counts.made;
    platform.frame( app );
    CHECK( app.stage.push( settings.layout ) );
    const int saves = counts.saves;
    app.background(); // the model saves as the app leaves the screen
    CHECK( counts.saves == saves + 1 );
    const std::vector<std::byte> kept = kept_for_android( app );
    CHECK( text_of( kept ).starts_with( "squared app 1\nsquared screens 1\nscreen landing" ) );
    app.dispose(); // not a quit: the activity goes, the process and the model stay
    CHECK( app.running() );

    // A new launch: nothing kept. squared_main again, a new model, the first screen.
    Platform again( assets_directory() );
    again.open( app );
    CHECK( mains == mains_before + 1 && counts.made == made + 1 );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );
    CHECK( counts.saves == saves + 1 ); // the old model stopped with nothing more to save

    // An activity Android made again: the same run, the same stack.
    CHECK( app.stage.push( settings.layout ) );
    const std::vector<std::byte> again_kept = kept_for_android( app );
    app.dispose();
    Platform third( assets_directory() );
    third.open( app, again_kept );
    CHECK( mains == mains_before + 1 && counts.made == made + 1 );
    CHECK( app.stage.depth() == 2 );

    // With no screens kept, the first line alone still keeps the run.
    app.stage.keep_screens( false );
    const std::vector<std::byte> header = kept_for_android( app );
    CHECK( text_of( header ) == "squared app 1\n" );
    app.dispose();
    Platform fourth( assets_directory() );
    fourth.open( app, header );
    CHECK( counts.made == made + 1 && app.stage.depth() == 2 );
    app.stage.keep_screens( true );
    app.quit();
    app.dispose();
}

/// Showing a list, and values that grow as they come, allocate when a
/// screen is built, never in a frame (7.8: hello's two sites, pkg58's run).
void frames_after_a_build_allocate_nothing()
{
    start = Start::inline_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    const auto frames = [&]( int count ) {
        for( int i = 0; i < count; ++i ) {
            sq::dev::frame_begin();
            platform.frame( app );
            const sq::dev::FrameAllocations made = sq::dev::frame_end();
            CHECK( made.count == 0 );
        }
    };
    // A list in a pane, then on a screen: sized when built, not in a frame.
    CHECK( app.stage.open<ShelfPane>() );
    frames( 30 );
    sq::gui::Actor *in_pane = window( app, 0 ).content_table().child_at( 0 );
    sq::gui::List &pane_list = *sq::gui::actor_cast<sq::gui::List>( in_pane );
    const std::size_t first = pane_list.slot_showing( 0 ); // its row is the label itself
    CHECK( first < pane_list.rows_built() );
    const sq::gui::Actor *cell = pane_list.table( first ).child_at( 0 );
    const auto *title = static_cast<const sq::gui::Label *>( cell );
    CHECK( title != nullptr && title->text() == "book 1" );
    CHECK( app.stage.close() );
    CHECK( app.stage.push<Shelf>() ); // a list of 1,000 books
    frames( 30 );
    // A value that comes longer than any text this window has laid out (14
    // characters, within a string's own room): the painter has room for it.
    CHECK( app.stage.back() );
    CHECK( app.stage.push<Bound>() );
    frames( 3 );
    const std::string_view longer = "abcdefghijklmn";
    CHECK( app.telegraph().set( player, longer ) == sq::messaging::SendResult::sent );
    frames( 5 );
    CHECK( app.telegraph().value( player ) == longer );
    app.quit();
    app.dispose();
}

void no_skin_draws_red()
{
    start = Start::no_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    reports.clear();
    // A skin that is not there: an embedded file system with nothing in it.
    sq::files::EmbeddedFileSystem nothing( {} );
    sq::detail::StageProbe::read_skin_from( app.stage, &nothing );
    platform.open( app );
    CHECK( app.stage.failed() );
    CHECK( reported( "stage: the skin did not load: skins/default/skin/uiskin.atlas" ) );
    CHECK( widgets_of( app ) == nullptr );
    CHECK( app.stage.depth() == 1 ); // the stack is kept: a window with the skin builds it
    platform.frame( app );
    press_back( app );
    CHECK( !app.quit_requested() );
    app.quit();
    app.dispose();
    sq::detail::StageProbe::read_skin_from( app.stage, nullptr );
}

/// The compiled-in skin is its files, byte for byte, and all of them.
void the_skin_is_its_files()
{
    const std::span<const sq::files::EmbeddedFile> table = sq::detail::default_skin();
    CHECK( table.size() == 4 );
    constexpr std::string_view under = "skins/default/skin/";
    for( const sq::files::EmbeddedFile &file : table ) {
        CHECK( file.path.starts_with( under ) );
        const std::string name( file.path.substr( under.size() ) );
        std::ifstream in( std::string( STAGE_SKIN_DIR ) + "/" + name, std::ios::binary );
        CHECK( in.good() );
        std::ostringstream read;
        read << in.rdbuf();
        const std::string bytes = read.str();
        CHECK( bytes.size() == file.bytes.size() );
        for( std::size_t i = 0; i < bytes.size(); ++i ) {
            CHECK( static_cast<unsigned char>( bytes[i] ) == file.bytes[i] );
        }
    }
    for( const char *name : { "default.fnt", "uiskin.atlas", "uiskin.json", "uiskin.png" } ) {
        CHECK( std::ifstream( std::string( STAGE_SKIN_DIR ) + "/" + name ).good() );
    }
}

void a_bad_layout_is_said()
{
    // A tap that is not one, hand-made: check() refuses it, nothing builds.
    broken.nodes[0] = sq::gui::LayoutNode{ .tap = 1, .type = sq::gui::NodeType::button };
    broken.text = { '\0', 'j', 'u', 'm', 'p', '\0' };
    broken.layout = sq::gui::Layout( "broken", sq::gui::LayoutKind::screen,
                                     std::span( broken.nodes ).first( 1 ),
                                     std::string_view( broken.text.data(), 6 ) );
    start = Start::broken_layout;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    reports.clear();
    platform.open( app );
    CHECK( reported( "stage: broken: " ) );
    CHECK( widgets_of( app )->content()->child_count() == 0 );
    platform.frame( app );
    app.quit();
    app.dispose();
}

} // namespace

int main()
{
    the_skin_is_its_files();
    write_layouts();
    starts_and_navigates();
    model_on_its_thread();
    cpp_screens();
    panes();
    bound_values();
    bound_lists();
    rotation_lays_out_once();
    kept_beneath_the_top();
    kept_after_the_process();
    a_new_activity_forgets_the_old_one();
    back_tap_at_the_root_goes_to_the_background();
    a_swipe_starts_fresh();
    frames_after_a_build_allocate_nothing();
    no_skin_draws_red();
    a_bad_layout_is_said();
    std::printf( "stage_test: ok (%zu lines reported)\n", reports.size() );
    return 0;
}
