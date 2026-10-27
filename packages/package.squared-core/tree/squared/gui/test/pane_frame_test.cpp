// The Ui's panes (7.7): a PaneFrame shown over the content, drawn with its
// window style's background inside which its table sits at the style's
// insets; a modal one dims what is beneath and takes every tap; a tap on the
// frame between its widgets stays there; closing one gives the focus back.

#include <squared/gui/button.hpp>
#include <squared/gui/label.hpp>
#include <squared/gui/linear_layout.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/src/color_drawable.hpp>
#include <squared/gui/src/detail/pane_frame.hpp>
#include <squared/gui/src/detail/ui.hpp>
#include <squared/gui/src/insets.hpp>
#include <squared/gui/src/painter.hpp>
#include <squared/gui/src/pointer_action.hpp>
#include <squared/gui/src/rectangle.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/src/window_style.hpp>
#include <squared/gui/table.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <vector>

namespace
{

#define CHECK( ... )                                                                               \
    do {                                                                                           \
        if( !( __VA_ARGS__ ) ) {                                                                   \
            std::fprintf( stderr, "pane_frame_test.cpp:%d: %s\n", __LINE__, #__VA_ARGS__ );        \
            std::abort();                                                                          \
        }                                                                                          \
    } while( false )

using namespace sq::gui;

/// Text is 8 by 16 a character; every fill is kept.
class Recording final : public Painter
{
    public:
        struct Fill
        {
                Rectangle where;
                sq::graphics::Color color;
        };
        std::vector<Fill> fills;

        [[nodiscard]] Size measure_text( std::string_view text ) override
        {
            return { 8.0F * static_cast<float>( text.size() ), 16.0F };
        }
        void fill_rectangle( const Rectangle &rectangle, sq::graphics::Color color ) override
        {
            fills.push_back( { rectangle, color } );
        }
        void stroke_rectangle( const Rectangle &, sq::graphics::Color, float ) override {}
        void draw_region( const sq::graphics::TextureRegion &, const Rectangle &,
                          sq::graphics::Color ) override
        {}
        void draw_text( std::string_view, float, float, sq::graphics::Color ) override {}
        void push_clip( const Rectangle & ) override {}
        void pop_clip() override {}
};

[[nodiscard]] bool near( float a, float b )
{
    return std::fabs( a - b ) < 0.001F;
}

[[nodiscard]] bool same( sq::graphics::Color a, sq::graphics::Color b )
{
    return near( a.red, b.red ) && near( a.green, b.green ) && near( a.blue, b.blue ) &&
           near( a.alpha, b.alpha );
}

constexpr sq::graphics::Color pane_colour{ .red = 0.2F, .green = 0.4F, .blue = 0.6F };
constexpr sq::graphics::Color dim{ .red = 0.0F, .green = 0.0F, .blue = 0.0F, .alpha = 0.5F };

/// A skin with a `pane` window style: a flat background, 10 units of insets.
[[nodiscard]] Skin pane_skin()
{
    Skin skin;
    WindowStyle style = skin.window_style( "default" );
    style.background = std::make_shared<ColorDrawable>( pane_colour );
    style.content_insets = Insets{ 10.0F, 10.0F, 10.0F, 10.0F };
    style.modal_overlay = dim;
    skin.add_window_style( "pane", style );
    return skin;
}

[[nodiscard]] bool filled( const Recording &painter, sq::graphics::Color color, Rectangle where )
{
    for( const Recording::Fill &fill : painter.fills ) {
        const bool at = near( fill.where.x, where.x ) && near( fill.where.y, where.y ) &&
                        near( fill.where.width, where.width ) &&
                        near( fill.where.height, where.height );
        if( at && same( fill.color, color ) ) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    Ui ui( 720.0F, 1280.0F, pane_skin() );
    Recording painter;
    int screen_taps = 0;
    auto content = std::make_unique<LinearLayout>();
    auto &behind = static_cast<Button &>(
        content->add( std::make_unique<Button>( "Behind", [&screen_taps] { ++screen_taps; } ) ) );
    ui.set_content( std::move( content ) );

    // A pane: the style's background, the table inside its insets.
    auto made = std::make_unique<PaneFrame>( "pane" );
    made->content_table().add( std::make_unique<Label>( "Sure?" ) );
    made->content_table().row();
    int pane_taps = 0;
    made->content_table().add( std::make_unique<Button>( "Yes", [&pane_taps] { ++pane_taps; } ) );
    PaneFrame &pane = ui.show_pane( std::move( made ) );
    CHECK( ui.pane_count() == 1 && pane.style() == "pane" && !pane.modal() );
    ui.layout( painter );
    // No size given: its preferred one, the table's and the insets.
    const Size table = pane.content_table().preferred_size( painter, ui.skin() );
    CHECK( near( pane.width(), table.width + 20.0F ) );
    CHECK( near( pane.height(), table.height + 20.0F ) );
    const Size wanted = pane.preferred_size( painter, ui.skin() );
    CHECK( near( wanted.width, table.width + 20.0F ) );
    CHECK( near( wanted.height, table.height + 20.0F ) );
    pane.set_bounds( 100.0F, 600.0F, 400.0F, 200.0F );
    pane.invalidate_layout();
    ui.layout( painter );
    const Table &inside = pane.content_table();
    CHECK( near( inside.x(), 10.0F ) && near( inside.y(), 10.0F ) );
    CHECK( near( inside.width(), 380.0F ) && near( inside.height(), 180.0F ) );
    painter.fills.clear();
    ui.paint( painter );
    CHECK( filled( painter, pane_colour, { 100.0F, 600.0F, 400.0F, 200.0F } ) );
    CHECK( !filled( painter, dim, { 0.0F, 0.0F, 720.0F, 1280.0F } ) ); // not modal: no dim

    // A tap on the frame, between its widgets, stays in the pane.
    CHECK( ui.pointer( PointerAction::down, 105.0F, 795.0F ) );
    CHECK( ui.pointer( PointerAction::up, 105.0F, 795.0F ) );
    CHECK( screen_taps == 0 );
    // Not modal: a tap outside it reaches the screen.
    const float bx = behind.x() + behind.width() * 0.5F;
    const float by = behind.y() + behind.height() * 0.5F;
    CHECK( by < 600.0F ); // the button is above the pane
    static_cast<void>( ui.pointer( PointerAction::down, bx, by ) );
    static_cast<void>( ui.pointer( PointerAction::up, bx, by ) );
    CHECK( screen_taps == 1 );

    // Modal: the screen dims, and a tap outside goes nowhere.
    pane.set_modal( true );
    painter.fills.clear();
    ui.paint( painter );
    CHECK( filled( painter, dim, { 0.0F, 0.0F, 720.0F, 1280.0F } ) );
    CHECK( ui.pointer( PointerAction::down, bx, by ) );
    CHECK( ui.pointer( PointerAction::up, bx, by ) );
    CHECK( screen_taps == 1 );

    // A style the skin does not have is its default.
    PaneFrame plain( "nowhere" );
    CHECK( plain.minimum_size( painter, ui.skin() ).width >= 0.0F );
    CHECK( ui.skin().window_style( plain.style() ).background ==
           ui.skin().window_style( "default" ).background );

    // Closed: freed, and the screen takes taps again.
    ui.close_pane( pane );
    CHECK( ui.pane_count() == 0 );
    static_cast<void>( ui.pointer( PointerAction::down, bx, by ) );
    static_cast<void>( ui.pointer( PointerAction::up, bx, by ) );
    CHECK( screen_taps == 2 && pane_taps == 0 );

    std::printf( "pane_frame_test: ok  sizeof(PaneFrame)=%zu\n", sizeof( PaneFrame ) );
    return 0;
}
