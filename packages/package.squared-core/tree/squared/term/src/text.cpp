// Text out of a terminal and into it: Terminal::copy_text (a selection's
// text, from the scrollback and the screen) and paste_text (what a paste
// sends). Both write whole characters into a caller's buffer and say how many
// bytes the whole text takes, so a caller sizes once and allocates once.

module sq.term;

import std;

namespace sq::term
{

namespace
{

/// Bytes into a caller's span: whole pieces, in order, until one does not
/// fit; every piece counted. No allocation.
class Out
{
    public:
        explicit Out( std::span<char> out ) noexcept
            : out_( out )
        {}

        void put( std::string_view piece ) noexcept
        {
            if( !full_ && out_.size() - written_ >= piece.size() ) {
                std::copy( piece.begin(), piece.end(), out_.data() + written_ );
                written_ += piece.size();
            } else {
                full_ = true; // nothing after a piece that did not fit
            }
            total_ += piece.size();
        }

        void put( char32_t code ) noexcept
        {
            std::array<char, 4> bytes{};
            put( { bytes.data(), encode( code, bytes ) } );
        }

        [[nodiscard]] std::size_t total() const noexcept { return total_; }

    private:
        /// UTF-8 for `code`; U+FFFD for a surrogate or past U+10FFFF.
        static std::size_t encode( char32_t code, std::array<char, 4> &out ) noexcept
        {
            if( ( code >= 0xD800 && code <= 0xDFFF ) || code > 0x10FFFF ) {
                code = 0xFFFD;
            }
            const auto byte = []( char32_t value ) { return static_cast<char>( value ); };
            if( code < 0x80 ) {
                out[0] = byte( code );
                return 1;
            }
            if( code < 0x800 ) {
                out[0] = byte( 0xC0 | ( code >> 6 ) );
                out[1] = byte( 0x80 | ( code & 0x3F ) );
                return 2;
            }
            if( code < 0x10000 ) {
                out[0] = byte( 0xE0 | ( code >> 12 ) );
                out[1] = byte( 0x80 | ( ( code >> 6 ) & 0x3F ) );
                out[2] = byte( 0x80 | ( code & 0x3F ) );
                return 3;
            }
            out[0] = byte( 0xF0 | ( code >> 18 ) );
            out[1] = byte( 0x80 | ( ( code >> 12 ) & 0x3F ) );
            out[2] = byte( 0x80 | ( ( code >> 6 ) & 0x3F ) );
            out[3] = byte( 0x80 | ( code & 0x3F ) );
            return 4;
        }

        std::span<char> out_;
        std::size_t written_ = 0;
        std::size_t total_ = 0;
        bool full_ = false;
};

/// One line's part, `first` to `last` (both included; past the line's cells
/// is blank), and the LF after it unless the line wrapped.
void put_line( Out &out, std::span<const Cell> cells, bool wrapped, std::uint16_t first,
               std::uint16_t last, bool ends_line ) noexcept
{
    std::size_t end = std::min<std::size_t>( std::size_t{ last } + 1, cells.size() );
    std::size_t begin = std::min<std::size_t>( first, end );
    // A line that ends here (no autowrap on into the next) loses its
    // trailing blanks; a wrapped one keeps them up to its last column: they
    // are spaces between words.
    if( !( wrapped && ends_line ) ) {
        while( end > begin && cells[end - 1].code == U' ' ) {
            --end;
        }
    }
    for( std::size_t c = begin; c < end; ++c ) {
        const Cell &cell = cells[c];
        if( cell.flags.wide_tail || cell.code == 0 ) {
            continue; // drawn by the cell before it
        }
        out.put( cell.code );
    }
    if( ends_line && !wrapped ) {
        out.put( std::string_view( "\n" ) );
    }
}

} // namespace

std::size_t Terminal::copy_text( TextPoint from, TextPoint to, std::span<char> out ) const noexcept
{
    if( to < from ) {
        std::swap( from, to );
    }
    const std::uint64_t first = first_line();
    const std::uint64_t screen = screen_line();
    const std::uint64_t last = screen + rows() - 1;
    if( from.line < first ) {
        from = { first, 0 };
    }
    if( to.line > last ) {
        to = { last, static_cast<std::uint16_t>( columns() - 1 ) };
    }
    Out text( out );
    if( from > to ) {
        return 0;
    }
    constexpr std::uint16_t whole = std::numeric_limits<std::uint16_t>::max();
    // Each line: from the start column on the first, to the end column on
    // the last; the last one's LF only when the selection runs past its end.
    const auto put = [&]( std::uint64_t line, std::span<const Cell> cells, bool wrapped ) {
        const std::uint16_t begin = line == from.line ? from.column : 0;
        const std::uint16_t end = line == to.line ? to.column : whole;
        const bool ends_line = line != to.line;
        put_line( text, cells, wrapped, begin, end, ends_line );
    };
    if( from.line < screen ) {
        const auto oldest = static_cast<std::size_t>( screen - 1 - from.line );
        const auto newest =
            static_cast<std::size_t>( screen - 1 - std::min( to.line, screen - 1 ) );
        std::uint64_t line = from.line;
        scrollback_.visit( oldest, newest, [&]( const detail::Scrollback::Line &kept ) {
            put( line, kept.cells, kept.wrapped );
            ++line;
        } );
    }
    for( std::uint64_t line = std::max( from.line, screen ); line <= to.line; ++line ) {
        const auto r = static_cast<std::uint16_t>( line - screen );
        put( line, row( r ), row_wrapped( r ) );
    }
    return text.total();
}

std::size_t paste_text( std::string_view text, TerminalModes modes, std::span<char> out ) noexcept
{
    Out sent( out );
    if( modes.bracketed_paste ) {
        sent.put( std::string_view( "\x1b[200~" ) );
    }
    for( std::size_t i = 0; i < text.size(); ) {
        const auto byte = static_cast<unsigned char>( text[i] );
        if( byte == 0x1B ) {
            ++i; // ESC: a paste starts no control sequence
            continue;
        }
        if( byte == 0xC2 && i + 1 < text.size() ) {
            const auto next = static_cast<unsigned char>( text[i + 1] );
            if( next >= 0x80 && next <= 0x9F ) {
                i += 2; // a C1 control, as UTF-8
                continue;
            }
        }
        if( byte == '\r' || byte == '\n' ) {
            // CR LF, LF and CR are each one Enter.
            i += byte == '\r' && i + 1 < text.size() && text[i + 1] == '\n' ? 2 : 1;
            sent.put( std::string_view( "\r" ) );
            continue;
        }
        // One character whole: a lead byte and what follows it.
        std::size_t length = 1;
        if( byte >= 0xC0 ) {
            length = byte >= 0xF0 ? 4 : byte >= 0xE0 ? 3 : 2;
        }
        std::size_t end = i + 1;
        while( end < text.size() && end < i + length &&
               ( static_cast<unsigned char>( text[end] ) & 0xC0 ) == 0x80 ) {
            ++end;
        }
        sent.put( text.substr( i, end - i ) );
        i = end;
    }
    if( modes.bracketed_paste ) {
        sent.put( std::string_view( "\x1b[201~" ) );
    }
    return sent.total();
}

} // namespace sq::term
