#include "detail/gui_detail.hpp"

#include <squared/gui/skin.hpp>
#include <squared/gui/src/detail/pane_frame.hpp>
#include <squared/gui/src/painter.hpp>
#include <squared/gui/src/pointer_event.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/src/window_style.hpp>
#include <squared/gui/table.hpp>

#include <algorithm>
#include <memory>
#include <string_view>
#include <utility>

namespace sq::gui
{

using namespace detail;

PaneFrame::PaneFrame( std::string_view style )
    : style_( style )
{
    auto content = std::make_unique<Table>();
    content_ = content.get();
    static_cast<void>( add_actor( std::move( content ) ) );
}

Size PaneFrame::minimum_size( Painter &painter, const Skin &skin ) const
{
    const WindowStyle &style = skin.window_style( style_ );
    const Size table = content_->minimum_size( painter, skin );
    const Size background = style.background ? style.background->minimum_size() : Size{};
    const float across = table.width + style.content_insets.left + style.content_insets.right;
    const float down = table.height + style.content_insets.top + style.content_insets.bottom;
    return { std::max( across, background.width ), std::max( down, background.height ) };
}

Size PaneFrame::preferred_size( Painter &painter, const Skin &skin ) const
{
    const WindowStyle &style = skin.window_style( style_ );
    const Size table = content_->preferred_size( painter, skin );
    const Size background = style.background ? style.background->minimum_size() : Size{};
    const float across = table.width + style.content_insets.left + style.content_insets.right;
    const float down = table.height + style.content_insets.top + style.content_insets.bottom;
    return { std::max( across, background.width ), std::max( down, background.height ) };
}

void PaneFrame::layout( Painter &painter, const Skin &skin )
{
    const WindowStyle &style = skin.window_style( style_ );
    const float across = width() - style.content_insets.left - style.content_insets.right;
    const float down = height() - style.content_insets.top - style.content_insets.bottom;
    content_->set_bounds( style.content_insets.left, style.content_insets.top,
                          std::max( 0.0F, across ), std::max( 0.0F, down ) );
    content_->invalidate_layout();
    content_->validate_layout( painter, skin );
}

void PaneFrame::paint( Painter &painter, const Skin &skin, float x, float y ) const
{
    const WindowStyle &style = skin.window_style( style_ );
    if( style.background ) {
        style.background->draw( painter, bounds_of( *this, x, y ) );
    }
}

bool PaneFrame::pointer_event( const PointerEvent &event )
{
    if( !enabled() ) {
        return false;
    }
    // A tap between its widgets does not fall through to the screen.
    return contains( event.x, event.y ) || modal_;
}

} // namespace sq::gui
