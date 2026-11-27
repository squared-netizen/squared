module sq.tui;

import sq.term;
import std;

namespace sq::tui
{

namespace
{

constexpr std::string_view sync_begin = "\x1b[?2026h";
constexpr std::string_view sync_end = "\x1b[?2026l";

void append_number( std::string &out, unsigned value )
{
    std::array<char, 8> digits{};
    const auto [end, ec] = std::to_chars( digits.data(), digits.data() + digits.size(), value );
    out.append( digits.data(), end );
}

} // namespace

void Renderer::move_to( std::uint16_t x, std::uint16_t y )
{
    if( x_ == x && y_ == y ) {
        return;
    }
    out_ += "\x1b[";
    append_number( out_, y + 1U );
    out_ += ';';
    append_number( out_, x + 1U );
    out_ += 'H';
    x_ = x;
    y_ = y;
}

void Renderer::set_style( const sq::term::Cell &cell )
{
    const Style want = style_of( cell );
    if( style_ && *style_ == want ) {
        return;
    }
    // From nothing each time: one sequence, no state to get wrong.
    out_ += "\x1b[0";
    const sq::term::CellAttributes a = want.attributes;
    const std::array<std::pair<bool, std::string_view>, 8> attributes{ {
        { a.bold, ";1" },
        { a.faint, ";2" },
        { a.italic, ";3" },
        { a.underline, ";4" },
        { a.blink, ";5" },
        { a.inverse, ";7" },
        { a.conceal, ";8" },
        { a.crossed_out, ";9" },
    } };
    for( const auto &[on, code] : attributes ) {
        if( on ) {
            out_ += code;
        }
    }
    const auto colour = [this]( std::string_view lead, sq::term::Rgb c ) {
        out_ += lead;
        append_number( out_, c.r );
        out_ += ';';
        append_number( out_, c.g );
        out_ += ';';
        append_number( out_, c.b );
    };
    if( want.foreground ) {
        colour( ";38;2;", *want.foreground );
    }
    if( want.background ) {
        colour( ";48;2;", *want.background );
    }
    out_ += 'm';
    style_ = want;
}

void Renderer::put_code( char32_t code )
{
    sq::term::detail::append_utf8( out_, code );
}

std::string_view Renderer::render( const Screen &screen )
{
    out_.clear();
    const std::uint16_t columns = screen.columns();
    const std::uint16_t rows = screen.rows();
    const bool whole = whole_ || columns != columns_ || rows != rows_;
    x_ = -1;
    y_ = -1;
    style_.reset();

    out_ += sync_begin;
    const std::size_t empty = out_.size();
    if( whole ) {
        out_ += "\x1b[0m\x1b[2J";
        style_ = Style{};
    }
    for( std::uint16_t y = 0; y < rows; ++y ) {
        const std::span<const sq::term::Cell> row = screen.row( y );
        for( std::uint16_t x = 0; x < columns; ++x ) {
            const sq::term::Cell &cell = row[x];
            if( cell.flags.wide_tail ) {
                continue; // its head writes it
            }
            const bool changed = whole || cell != last_[std::size_t{ y } * columns + x] ||
                                 ( cell.flags.wide && x + 1U < columns &&
                                   row[x + 1U] != last_[std::size_t{ y } * columns + x + 1U] );
            if( !changed ) {
                continue;
            }
            if( whole && cell == sq::term::Cell{} ) {
                continue; // the screen was cleared to this
            }
            move_to( x, y );
            set_style( cell );
            put_code( cell.code == 0 ? U' ' : cell.code );
            x_ += cell.flags.wide ? 2 : 1;
            if( x_ >= columns ) {
                x_ = -1; // where the cursor is after the last column is the terminal's to say
            }
        }
    }
    const std::optional<Point> cursor = screen.cursor();
    const bool cells_written = out_.size() != empty;
    if( !cells_written && !whole && cursor == last_cursor_ ) {
        out_.clear();
        return {};
    }
    if( style_ && *style_ != Style{} ) {
        out_ += "\x1b[0m";
    }
    if( cursor ) {
        move_to( cursor->x, cursor->y );
        out_ += "\x1b[?25h";
    } else {
        out_ += "\x1b[?25l";
    }
    out_ += sync_end;

    last_.assign( screen.row( 0 ).data(), screen.row( 0 ).data() + std::size_t{ columns } * rows );
    columns_ = columns;
    rows_ = rows;
    last_cursor_ = cursor;
    whole_ = false;
    return out_;
}

} // namespace sq::tui
