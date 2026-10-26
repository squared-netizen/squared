// An App with no layout files (7.1): nothing defines layouts::all(), so the
// framework's own, empty one links in its place. A C++ layout still shows, and
// a tap naming another layout says there is none.

#include <squared/app/event.hpp>
#include <squared/app/runtime.hpp>
#include <squared/assets/asset_manager.hpp>
#include <squared/files/posix_file_system.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics/src/context_config.hpp>
#include <squared/gui/button.hpp>
#include <squared/gui/layout.hpp>
#include <squared/gui/layout_writer.hpp>
#include <squared/gui/ui.hpp>
#include <squared/gui/widget.hpp>
#include <squared/stage/app.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "stage_alone_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );       \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

std::array<sq::gui::LayoutNode, 4> nodes{};
std::array<char, 64> text{};
std::array<float, 1> numbers{};
sq::gui::Layout only;

std::string said;

void collect( void * /*context*/, std::string_view line ) noexcept
{
    said += line;
    said += '\n';
}

sq::App the_app;

} // namespace

void squared_main( sq::App &app )
{
    app.stage.push( only );
}

int main()
{
    sq::gui::LayoutWriter writer( nodes, text, numbers, "only" );
    writer.add<sq::gui::Button>( "Settings" ).tap( "push settings" );
    CHECK( writer.complete() );
    only = writer.layout();

    CHECK( sq::Stage::find( "only" ) == nullptr ); // no layout files: none to find

    sq::graphics::Context graphics;
    sq::graphics::ContextConfig config;
    config.logical_width = 720;
    config.logical_height = 1280;
    CHECK( graphics.create( config ) );
    sq::files::PosixFileSystemRoots roots;
    roots.internal_root = STAGE_TEST_ASSETS;
    sq::files::PosixFileSystem files( roots );
    sq::assets::AssetManager cache( files );
    sq::app::Runtime runtime{ graphics, files, cache };

    sq::App &app = the_app;
    app.set_report( { .write = collect, .context = nullptr } );
    CHECK( app.create( runtime ) );
    app.resize( 720, 1280 );
    CHECK( app.stage.depth() == 1 && !app.stage.failed() );

    // Tap the button, which fills the screen's middle.
    sq::gui::Widget &root = *app.stage.ui()->content();
    const sq::gui::Actor &pressed = *root.child_at( 0 );
    sq::app::Event event;
    event.x = root.x() + pressed.x() + pressed.width() * 0.5F;
    event.y = root.y() + pressed.y() + pressed.height() * 0.5F;
    event.type = sq::app::Event::Type::PointerDown;
    app.handle_event( event );
    event.type = sq::app::Event::Type::PointerUp;
    app.handle_event( event );
    CHECK( said.find( "push settings: no layout or added screen is called that" ) !=
           std::string::npos );
    CHECK( app.stage.depth() == 1 );

    app.update( std::chrono::milliseconds( 16 ) );
    app.render( graphics );
    app.quit();
    app.dispose();
    std::printf( "stage_alone_test: ok\n" );
    return 0;
}
