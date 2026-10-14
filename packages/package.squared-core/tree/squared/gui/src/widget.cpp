#include "detail/gui_detail.hpp"

#include <squared/gui/label.hpp>
#include <squared/gui/margin_container.hpp>
#include <squared/gui/panel.hpp>
#include <squared/gui/skin.hpp>
#include <squared/gui/src/input_event.hpp>
#include <squared/gui/src/input_key.hpp>
#include <squared/gui/src/input_type.hpp>
#include <squared/gui/src/insets.hpp>
#include <squared/gui/src/key.hpp>
#include <squared/gui/src/key_modifiers.hpp>
#include <squared/gui/src/painter.hpp>
#include <squared/gui/src/pointer_action.hpp>
#include <squared/gui/src/pointer_event.hpp>
#include <squared/gui/src/size.hpp>
#include <squared/gui/src/size_hints.hpp>
#include <squared/gui/stack.hpp>
#include <squared/gui/widget.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace sq::gui
{

// The helpers below were an anonymous namespace in the
// original single translation unit; they are shared now.
using namespace detail;

Size Widget::minimum_size( Painter &, const Skin & ) const
{
    return {};
}

Size Widget::preferred_size( Painter &, const Skin & ) const
{
    return { width(), height() };
}

Size Widget::maximum_size( Painter &, const Skin & ) const
{
    const float infinity = std::numeric_limits<float>::infinity();
    return { infinity, infinity };
}

SizeHints Widget::size_hints( Painter &painter, const Skin &skin ) const
{
    SizeHints hints{ minimum_size( painter, skin ), preferred_size( painter, skin ),
                     maximum_size( painter, skin ) };
    hints.preferred.width =
        clamp_dimension( hints.preferred.width, hints.minimum.width, hints.maximum.width );
    hints.preferred.height =
        clamp_dimension( hints.preferred.height, hints.minimum.height, hints.maximum.height );
    return hints;
}

void Widget::invalidate_layout() noexcept
{
    layout_valid_ = false;
    if( auto *widget = actor_cast<Widget>( parent() ) ) {
        widget->invalidate_layout();
    }
}

void Widget::validate_layout( Painter &painter, const Skin &skin )
{
    if( layout_valid_ ) {
        return;
    }
    layout( painter, skin );
    layout_valid_ = true;
}

void Widget::layout( Painter &painter, const Skin &skin )
{
    for( std::size_t index = 0; index < child_count(); ++index ) {
        if( auto *child = actor_cast<Widget>( child_at( index ) ) ) {
            child->validate_layout( painter, skin );
        }
    }
}

void Widget::paint( Painter &, const Skin &, float, float ) const
{}

bool Widget::pointer_event( const PointerEvent & )
{
    return false;
}

bool Widget::key_down( Key, KeyModifiers )
{
    return false;
}

bool Widget::text_input( std::string_view )
{
    return false;
}

bool Widget::text_editing( std::string_view, int, int )
{
    return false;
}

void Widget::focus_changed( bool )
{}

bool Widget::focusable() const noexcept
{
    return false;
}
bool Widget::wants_text_input() const noexcept
{
    return false;
}

void Widget::set_tooltip( std::string text )
{
    if( text.empty() ) {
        clear_tooltip();
        return;
    }
    set_tooltip_factory( [text = std::move( text )]() {
        auto stack = std::make_unique<Stack>();
        stack->add( std::make_unique<Panel>( "tooltip" ) );
        auto margin = std::make_unique<MarginContainer>( Insets{ 8.0F, 6.0F, 8.0F, 6.0F } );
        static_cast<void>( margin->set_content( std::make_unique<Label>( text ) ) );
        stack->add( std::move( margin ) );
        return stack;
    } );
}

void Widget::set_tooltip_factory( TooltipFactory factory )
{
    tooltip_factory_ = std::move( factory );
}

void Widget::clear_tooltip() noexcept
{
    tooltip_factory_ = {};
}

void Widget::input_event( InputEvent &event )
{
    bool handled = false;
    switch( event.type ) {
        case InputType::pointer_move:
        case InputType::pointer_down:
        case InputType::pointer_up:
        case InputType::pointer_cancel: {
            PointerAction action = PointerAction::move;
            if( event.type == InputType::pointer_down ) {
                action = PointerAction::down;
            } else if( event.type == InputType::pointer_up ) {
                action = PointerAction::up;
            } else if( event.type == InputType::pointer_cancel ) {
                action = PointerAction::cancel;
            }
            handled = pointer_event(
                { action, event.pointer_id, event.local_x(), event.local_y(), event.button } );
            break;
        }
        case InputType::key_down: {
            Key key = Key::escape;
            switch( event.key ) {
                case InputKey::left:
                    key = Key::left;
                    break;
                case InputKey::right:
                    key = Key::right;
                    break;
                case InputKey::up:
                    key = Key::up;
                    break;
                case InputKey::down:
                    key = Key::down;
                    break;
                case InputKey::home:
                    key = Key::home;
                    break;
                case InputKey::end:
                    key = Key::end;
                    break;
                case InputKey::backspace:
                    key = Key::backspace;
                    break;
                case InputKey::delete_key:
                    key = Key::delete_key;
                    break;
                case InputKey::enter:
                    key = Key::enter;
                    break;
                case InputKey::space:
                    key = Key::space;
                    break;
                case InputKey::tab:
                    key = Key::tab;
                    break;
                case InputKey::escape:
                    key = Key::escape;
                    break;
                case InputKey::unknown:
                    return;
            }
            handled = key_down( key, event.modifiers );
            break;
        }
        default:
            break;
    }
    if( handled ) {
        event.handle();
        event.stop();
    }
}

} // namespace sq::gui
