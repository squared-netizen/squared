// The App and its Stage (7.1), on the host: the null graphics backend, the
// default skin from test/assets, layouts written in C++, and a model. A
// window comes and squared_main runs once; taps navigate and reach the model;
// frames allocate nothing; back at the root and a quit tap leave; quitting
// stops the model after it saves, and the next start begins afresh; an
// activity made again reattaches to the running model. C++ screens (7.2), and
// panes over a screen (7.3). The skin is the compiled-in default (7.4): the
// app's bundled files are a directory that does not exist, and the embedded
// bytes are checked against the skin's files.

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
#include <squared/gui/progress_bar.hpp>
#include <squared/gui/slider.hpp>
#include <squared/gui/src/pointer_action.hpp>
#include <squared/gui/src/pointer_event.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/table.hpp>
#include <squared/gui/text_field.hpp>
#include <squared/gui/ui.hpp>
#include <squared/gui/widget.hpp>
#include <squared/gui/window.hpp>
#include <squared/messaging/key.hpp>
#include <squared/messaging/model_runner.hpp>
#include <squared/messaging/telegraph.hpp>
#include <squared/stage/app.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
};

namespace
{

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
};

Counts counts;

struct Prefs final
{
        explicit Prefs( Counts *counted ) noexcept
            : counts_( counted )
        {
            ++counts_->made;
        }

        void start( Telegraph &telegraph ) noexcept
        {
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

    private:
        Counts *counts_;
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

/// The app's bundled files at `assets`, nothing writable.
sq::files::PosixFileSystemRoots roots_at( const std::string &assets )
{
    sq::files::PosixFileSystemRoots roots;
    roots.internal_root = assets;
    return roots;
}

struct Platform final
{
        explicit Platform( const std::string &assets )
            : files( roots_at( assets ) )
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

        /// What the template does when a window comes: create, the surface, the size.
        void open( sq::App &app )
        {
            app.set_report( { .write = collect, .context = nullptr } );
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

/// The button in `table` that says `text`; nullptr when there is none.
sq::gui::Button *button_in( sq::gui::Widget &table, std::string_view text )
{
    for( std::size_t i = 0; i < table.child_count(); ++i ) {
        // In the screens searched so, the buttons are the widgets that take
        // focus. Bound's slider and text field take it too: tap those by place.
        auto *widget = static_cast<sq::gui::Widget *>( table.child_at( i ) );
        if( !widget->focusable() ) {
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
sq::gui::Window &window( sq::App &app, std::size_t index )
{
    sq::gui::Group &root = *app.stage.ui()->content()->parent();
    CHECK( index + 1 < root.child_count() );
    return *static_cast<sq::gui::Window *>( root.child_at( index + 1 ) );
}

/// The button that says `text`, in a pane (the top one first) or the screen.
sq::gui::Button *button( sq::App &app, std::string_view text )
{
    sq::gui::Ui *ui = app.stage.ui();
    CHECK( ui != nullptr && ui->content() != nullptr );
    for( std::size_t i = ui->window_count(); i > 0; --i ) {
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
    CHECK( app.stage.ui()->content()->child_count() == 8 );

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
    app.stage.push( settings.layout );
    app.dispose();
    CHECK( app.running() );
    CHECK( app.stage.ui() == nullptr );
    Platform again( assets_directory() );
    again.open( app );
    CHECK( mains == 1 && counts.made == 1 );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &settings.layout );
    CHECK( button( app, "Back" ) != nullptr );
    tap( app, "Back" );

    // Back at the root leaves; the model saves and stops.
    press_back( app );
    CHECK( app.quit_requested() );
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
    CHECK( app.stage.ui()->content()->child_count() == 3 );

    // A custom node made by its maker; one with none stands in, and is said.
    tap( app, "Gauges" );
    CHECK( app.stage.showing<Gauges>() && app.stage.depth() == 3 );
    sq::gui::Widget &root = *app.stage.ui()->content();
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
    CHECK( app.stage.panes() == 2 && app.stage.ui()->window_count() == 2 );
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
    CHECK( app.quit_requested() );
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
    sq::gui::Ui &ui = *app.stage.ui();

    // A tap opens a pane over the screen, which stays: modal, at the bottom,
    // the width of the screen, no title bar.
    tap( app, "Pane" );
    CHECK( app.stage.panes() == 1 && app.stage.pane_name() == "discard" );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );
    CHECK( ui.window_count() == 1 );
    const sq::gui::Window &bottom = window( app, 0 );
    CHECK( bottom.modal() && !bottom.titled() && !bottom.movable() );
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
    const sq::gui::Window &right = window( app, 1 );
    CHECK( right.modal() );
    CHECK( right.x() + right.width() == logical_width && right.x() > 0.0F );
    CHECK( right.y() == 0.0F && right.height() == logical_height );

    // Back closes the top pane first; a close closes the pane it is in.
    press_back( app );
    CHECK( app.stage.panes() == 1 && app.stage.pane_name() == "discard" );
    CHECK( ui.window_count() == 1 );
    tap( app, "No" );
    CHECK( app.stage.panes() == 0 && ui.window_count() == 0 && app.stage.depth() == 1 );
    CHECK( app.stage.pane_name().empty() );

    // Not modal: a tap outside reaches the screen. Leaving the screen closes
    // its panes, and they do not come back with it.
    CHECK( app.stage.open( side.layout ) );
    CHECK( !window( app, 0 ).modal() );
    tap( app, "Settings" );
    CHECK( app.stage.depth() == 2 && app.stage.panes() == 0 && ui.window_count() == 0 );
    CHECK( app.stage.back() );
    CHECK( app.stage.depth() == 1 && app.stage.panes() == 0 && ui.window_count() == 0 );

    // A C++ pane, by its type: on the left. A close in a pane that is not
    // the top one closes that one.
    CHECK( app.stage.open( side.layout ) );
    CHECK( app.stage.open<Ask>() );
    CHECK( app.stage.pane_name() == "ask" && app.stage.panes() == 2 );
    const sq::gui::Window &left = window( app, 1 );
    CHECK( left.x() == 0.0F && left.height() == logical_height && !left.modal() );
    tap( app, "Shut" );
    CHECK( app.stage.panes() == 1 && app.stage.pane_name() == "ask" );
    tap( app, "Done" );
    CHECK( app.stage.panes() == 0 && ui.window_count() == 0 );

    // A pane's tap navigates the screen: away, and the panes go with it.
    tap( app, "Pane" );
    tap( app, "Away" );
    CHECK( app.stage.depth() == 2 && app.stage.top() == &settings.layout );
    CHECK( app.stage.panes() == 0 && ui.window_count() == 0 );
    CHECK( app.stage.back() );

    // By name, an added C++ screen opens as a pane.
    tap( app, "Pane" );
    tap( app, "Ask" );
    CHECK( app.stage.pane_name() == "ask" && window( app, 1 ).modal() );
    while( app.stage.close() ) {
    }
    CHECK( app.stage.panes() == 0 && ui.window_count() == 0 );

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
    CHECK( ui.window_count() == 4 );
    platform.frame( app );
    for( int i = 0; i < 10; ++i ) {
        sq::dev::frame_begin();
        platform.frame( app );
        CHECK( sq::dev::frame_end().count == 0 );
    }

    // An activity made again: the screen and its panes come back, built anew.
    app.dispose();
    CHECK( app.stage.ui() == nullptr && app.stage.panes() == 4 );
    Platform again( assets_directory() );
    again.open( app );
    CHECK( app.stage.panes() == 4 && app.stage.ui()->window_count() == 4 );
    CHECK( app.stage.depth() == 1 && app.stage.top() == &landing.layout );

    // Back closes them one by one, then leaves at the root.
    for( std::size_t i = 0; i < sq::Stage::max_panes; ++i ) {
        press_back( app );
    }
    CHECK( app.stage.panes() == 0 && app.stage.ui()->window_count() == 0 );
    CHECK( !app.quit_requested() );
    press_back( app );
    CHECK( app.quit_requested() );
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
    sq::gui::Widget &root = *app.stage.ui()->content();
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
    app.stage.ui()->clear_focus();
    platform.frame( app );
    CHECK( field.text() == "zed" && app.telegraph().value( player ) == "zed" );

    // Left with changes, without Done: the edit is finished, and sent.
    tap_on( app, field );
    CHECK( field.text_input( "?" ) );
    app.stage.ui()->clear_focus();
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

void back_tap_at_the_root_leaves()
{
    start = Start::no_model;
    Platform platform( assets_directory() );
    sq::App &app = the_app;
    platform.open( app );
    app.stage.replace( settings.layout ); // a root with a Back button
    CHECK( app.stage.depth() == 1 && app.stage.top() == &settings.layout );
    tap( app, "Back" );
    CHECK( app.stage.depth() == 1 );
    CHECK( app.quit_requested() );
    app.dispose();
    CHECK( !app.quit_requested() && app.stage.depth() == 0 );
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
    CHECK( app.stage.ui() == nullptr );
    CHECK( app.stage.depth() == 1 ); // the stack is kept: a window with the skin builds it
    platform.frame( app );
    press_back( app );
    CHECK( app.quit_requested() );
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
    CHECK( app.stage.ui()->content()->child_count() == 0 );
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
    back_tap_at_the_root_leaves();
    no_skin_draws_red();
    a_bad_layout_is_said();
    std::printf( "stage_test: ok (%zu lines reported)\n", reports.size() );
    return 0;
}
