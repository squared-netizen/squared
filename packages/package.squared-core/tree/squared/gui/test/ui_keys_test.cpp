// Keys and navigation, from an app::Event to the focused widget.
//
// The platform layer and the GUI share one key, one modifier and one
// navigation type, so nothing converts them on the way. This checks that what
// a widget receives is exactly what the event carried, for every named key,
// and that focus moves and unknown input behave as before the merge.

#include <squared/app/event.hpp>
#include <squared/app/src/key.hpp>
#include <squared/app/src/key_modifiers.hpp>
#include <squared/app/src/navigation.hpp>
#include <squared/gui/linear_layout.hpp>
#include <squared/gui/src/detail/ui.hpp>
#include <squared/gui/widget.hpp>

#include <cassert>
#include <cstdio>
#include <memory>

using namespace sq;

namespace
{

// A focusable widget that records every key it is given and takes them all.
struct Recorder final : gui::Widget
{
        int downs = 0;
        app::Key last = app::Key::unknown;
        app::KeyModifiers modifiers;

        bool key_down( app::Key key, app::KeyModifiers held ) override
        {
            ++downs;
            last = key;
            modifiers = held;
            return true;
        }

        [[nodiscard]] bool focusable() const noexcept override { return true; }
};

app::Event key_event( app::Key key, app::KeyModifiers modifiers = {} )
{
    app::Event event;
    event.type = app::Event::Type::KeyDown;
    event.key = key;
    event.modifiers = modifiers;
    return event;
}

app::Event navigation_event( app::Navigation navigation )
{
    app::Event event;
    event.type = app::Event::Type::NavigationInput;
    event.navigation = navigation;
    return event;
}

} // namespace

int main()
{
    static_assert( sizeof( app::KeyModifiers ) == 1 );

    gui::Ui ui( 720.0F, 1280.0F );
    auto &layout =
        static_cast<gui::LinearLayout &>( ui.set_content( std::make_unique<gui::LinearLayout>() ) );
    auto &first = static_cast<Recorder &>( layout.add( std::make_unique<Recorder>() ) );
    auto &second = static_cast<Recorder &>( layout.add( std::make_unique<Recorder>() ) );

    // With nothing focused, Tab moves focus to the first focusable widget.
    assert( ui.event( key_event( app::Key::tab ) ) && ui.focused() == &first );

    // Modifiers arrive as they were sent.
    assert( ui.event( key_event( app::Key::left, { .shift = true, .control = true } ) ) );
    assert( first.downs == 1 && first.last == app::Key::left );
    assert( first.modifiers == ( app::KeyModifiers{ .shift = true, .control = true } ) );
    assert( first.modifiers.any() && !first.modifiers.alt && !first.modifiers.meta );
    assert( !app::KeyModifiers{}.any() );

    // An unknown key reaches nobody.
    assert( !ui.event( key_event( app::Key::unknown ) ) && first.downs == 1 );

    // Every named key arrives as itself.
    constexpr app::Key keys[] = { app::Key::left,      app::Key::right,      app::Key::up,
                                  app::Key::down,      app::Key::home,       app::Key::end,
                                  app::Key::backspace, app::Key::delete_key, app::Key::enter,
                                  app::Key::space,     app::Key::tab,        app::Key::escape };
    for( const app::Key key : keys ) {
        assert( ui.event( key_event( key ) ) && first.last == key );
    }
    assert( first.downs == 13 && second.downs == 0 );

    // Navigation: offered to the focused widget, which ignores it here, so
    // focus moves; activate and cancel arrive as Enter and Escape.
    assert( ui.event( navigation_event( app::Navigation::next ) ) && ui.focused() == &second );
    assert( ui.event( navigation_event( app::Navigation::activate ) ) &&
            second.last == app::Key::enter );
    assert( ui.event( navigation_event( app::Navigation::cancel ) ) &&
            second.last == app::Key::escape );
    assert( ui.event( navigation_event( app::Navigation::previous ) ) && ui.focused() == &first );
    assert( !ui.event( navigation_event( app::Navigation::unknown ) ) );

    std::printf( "ui keys: all assertions passed  sizeof(app::KeyModifiers)=%zu\n",
                 sizeof( app::KeyModifiers ) );
    return 0;
}
