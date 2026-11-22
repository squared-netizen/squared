module sq.tui;

import sq.term;
import std;

namespace sq::tui
{

namespace
{

sq::term::TerminalConfig config_of( std::uint16_t columns, std::uint16_t rows )
{
    sq::term::TerminalConfig config;
    config.columns = columns;
    config.rows = rows;
    return config;
}

const sq::term::Cell blank{};

} // namespace

Test::Test( std::uint16_t columns, std::uint16_t rows )
    : screen_( columns, rows )
    , emulator_( config_of( columns, rows ) )
{}

void Test::show()
{
    output_ = renderer_.render( screen_ );
    emulator_.feed( output_ );
}

std::string Test::text( std::uint16_t x, std::uint16_t y ) const
{
    std::string out;
    if( y >= emulator_.rows() ) {
        return out;
    }
    const std::span<const sq::term::Cell> row = emulator_.row( y );
    for( std::size_t i = x; i < row.size(); ++i ) {
        if( row[i].flags.wide_tail ) {
            continue;
        }
        sq::term::detail::append_utf8( out, row[i].code == 0 ? U' ' : row[i].code );
    }
    while( !out.empty() && out.back() == ' ' ) {
        out.pop_back();
    }
    return out;
}

const sq::term::Cell &Test::cell( std::uint16_t x, std::uint16_t y ) const noexcept
{
    if( y >= emulator_.rows() || x >= emulator_.columns() ) {
        return blank;
    }
    return emulator_.row( y )[x];
}

std::optional<Point> Test::cursor() const noexcept
{
    const sq::term::Cursor c = emulator_.cursor();
    if( !emulator_.modes().cursor_visible ) {
        return std::nullopt;
    }
    return Point{ c.column, c.row };
}

void Test::resize( std::uint16_t columns, std::uint16_t rows )
{
    screen_.resize( columns, rows );
    emulator_.resize( columns, rows );
    renderer_.invalidate();
}

std::vector<Event> Test::type( std::string_view bytes )
{
    std::vector<Event> events;
    decoder_.feed( bytes, events );
    decoder_.flush( events );
    return events;
}

} // namespace sq::tui
