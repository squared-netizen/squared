module sq.tui;

import std;

namespace sq::tui
{

namespace
{

/** A coordinate a learner may well get wrong (negative, too large): off the screen. */
std::optional<std::uint16_t> place( int value ) noexcept
{
    if( value < 0 || value > 0xFFFF ) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>( value );
}

/** The terminal, or a message and exit(1): a first program has nowhere to put an error. */
Terminal open_or_exit( const TerminalOptions &options )
{
    auto opened = Terminal::open( options );
    if( !opened ) {
        std::cerr << "sq::tui::Simple: " << describe( opened.error() ) << '\n';
        std::exit( 1 );
    }
    return std::move( *opened );
}

} // namespace

Simple::Simple( const TerminalOptions &options )
    : terminal_( open_or_exit( options ) )
    , screen_( terminal_.columns(), terminal_.rows() )
{}

void Simple::put( int x, int y, std::string_view text, const Style &style ) noexcept
{
    const auto px = place( x );
    const auto py = place( y );
    if( px && py ) {
        screen_.put( *px, *py, text, style );
    }
}

void Simple::put( int x, int y, char32_t code, const Style &style ) noexcept
{
    const auto px = place( x );
    const auto py = place( y );
    if( px && py ) {
        screen_.put( *px, *py, code, style );
    }
}

void Simple::show()
{
    terminal_.show( screen_ );
}

std::optional<Key> Simple::key( std::chrono::milliseconds wait )
{
    using clock = std::chrono::steady_clock;
    const bool forever = wait.count() < 0;
    const clock::time_point deadline =
        clock::now() + ( forever ? std::chrono::milliseconds{ 0 } : wait );
    for( ;; ) {
        std::chrono::milliseconds left{ -1 };
        if( !forever ) {
            left = std::chrono::duration_cast<std::chrono::milliseconds>( deadline - clock::now() );
            if( left.count() < 0 ) {
                left = std::chrono::milliseconds{ 0 };
            }
        }
        const std::optional<Event> event = terminal_.next( left );
        if( !event ) {
            if( forever ) {
                continue;
            }
            return std::nullopt;
        }
        if( const Key *k = std::get_if<Key>( &*event ) ) {
            return *k;
        }
        if( const Resize *r = std::get_if<Resize>( &*event ) ) {
            screen_.resize( r->columns, r->rows );
            show();
        }
        if( !forever && clock::now() >= deadline ) {
            return std::nullopt;
        }
    }
}

Key Simple::key()
{
    for( ;; ) {
        if( const auto k = key( std::chrono::milliseconds{ -1 } ) ) {
            return *k;
        }
    }
}

} // namespace sq::tui
