// Keys and navigation, from an app::Event to the focused widget.
//
// The platform layer and the GUI share one key, one modifier and one
// navigation type, so nothing converts them on the way. This checks that what
// a widget receives is exactly what the event carried, for every named key,
// and that focus moves and unknown input behave as before the merge.

#include <assert.h>

import sq.app;
import sq.gui;

import std;

using namespace sq;

namespace
{

// A focusable widget that records every key it is given and takes them all.
struct Recorder final : gui::Widget
{
        int downs = 0;
        app::Key last = app::Key::unknown;
        app::KeyModifiers modifiers;

        int texts = 0;
        int key_texts = 0;
        std::string text;

        bool key_down( app::Key key, app::KeyModifiers held ) override
        {
            ++downs;
            last = key;
            modifiers = held;
            return true;
        }

        bool text_input( std::string_view typed ) override
        {
            ++texts;
            text = typed;
            return true;
        }

        bool key_text( std::string_view typed, app::KeyModifiers held ) override
        {
            ++key_texts;
            text = typed;
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

app::Event text_event( std::string_view text, app::KeyModifiers modifiers = {} )
{
    app::Event event;
    event.type = app::Event::Type::TextInput;
    event.text = text;
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
                                  app::Key::space,     app::Key::tab,        app::Key::escape,
                                  app::Key::page_up,   app::Key::page_down,  app::Key::insert,
                                  app::Key::f1,        app::Key::f2,         app::Key::f3,
                                  app::Key::f4,        app::Key::f5,         app::Key::f6,
                                  app::Key::f7,        app::Key::f8,         app::Key::f9,
                                  app::Key::f10,       app::Key::f11,        app::Key::f12 };
    for( const app::Key key : keys ) {
        assert( ui.event( key_event( key ) ) && first.last == key );
    }
    assert( first.downs == 28 && second.downs == 0 );

    // Text: plain, and with Shift, to text_input; with Ctrl, Alt or Meta to key_text.
    assert( ui.event( text_event( "a" ) ) && first.texts == 1 && first.text == "a" );
    assert( ui.event( text_event( "A", { .shift = true } ) ) && first.texts == 2 );
    assert( ui.event( text_event( "c", { .control = true } ) ) && first.key_texts == 1 );
    assert( first.text == "c" && first.modifiers.control );
    assert( ui.event( text_event( "x", { .alt = true } ) ) && first.key_texts == 2 );
    assert( ui.event( text_event( "q", { .meta = true } ) ) && first.key_texts == 3 );
    assert( first.texts == 2 );
    // A widget that does not take it (the default) leaves it unhandled.
    {
        gui::Ui plain( 100.0F, 100.0F );
        auto &only = static_cast<gui::LinearLayout &>(
            plain.set_content( std::make_unique<gui::LinearLayout>() ) );
        struct Field final : gui::Widget
        {
                [[nodiscard]] bool focusable() const noexcept override { return true; }
                bool text_input( std::string_view ) override { return true; }
        };
        only.add( std::make_unique<Field>() );
        assert( plain.event( key_event( app::Key::tab ) ) );
        assert( plain.event( text_event( "c" ) ) );
        assert( !plain.event( text_event( "c", { .control = true } ) ) );
    }

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
