module sq.term;

import std;

namespace sq::term
{

namespace
{

constexpr std::uint8_t clamp_byte( std::uint16_t value ) noexcept
{
    return static_cast<std::uint8_t>( std::min<std::uint16_t>( value, 255 ) );
}

std::size_t tab_words( std::uint16_t columns ) noexcept
{
    return ( std::size_t{ columns } + 63U ) / 64U;
}

} // namespace

// What the parser calls: forwarded to the terminal's private functions, so
// they are not part of its public face.
struct Terminal::Sink
{
        Terminal &terminal;

        void print( char32_t code ) noexcept { terminal.print( code ); }
        void execute( std::uint8_t control ) noexcept { terminal.execute( control ); }
        void escape( const detail::CsiSequence &sequence ) noexcept { terminal.escape( sequence ); }
        void csi( const detail::CsiSequence &sequence ) noexcept { terminal.csi( sequence ); }
        void osc( std::string_view text ) noexcept { terminal.osc( text ); }
};

Terminal::Terminal( const TerminalConfig &config )
{
    primary_.resize( config.columns, config.rows, false, Cell{} );
    alternate_.resize( config.columns, config.rows, false, Cell{} );
    scrollback_.reset( config.scrollback_bytes );
    tabs_ = std::make_unique<std::uint64_t[]>( tab_words( primary_.columns() ) );
    reset_tabs();
    bottom_ = static_cast<std::uint16_t>( primary_.rows() - 1U );
}

void Terminal::feed( std::span<const std::byte> bytes ) noexcept
{
    Sink sink{ *this };
    parser_.feed( bytes, sink );
}

void Terminal::resize( std::uint16_t columns, std::uint16_t rows )
{
    columns = std::max<std::uint16_t>( columns, 1 );
    rows = std::max<std::uint16_t>( rows, 1 );
    // Fewer rows on the main screen, the cursor below them: the rows above
    // it go to the scrollback, so its line stays on screen.
    if( !modes_.alternate_screen && cursor_.row >= rows ) {
        const auto shift = static_cast<std::uint16_t>( cursor_.row - rows + 1U );
        for( std::uint16_t r = 0; r < shift; ++r ) {
            scrollback_.push( primary_.row( r ), primary_.wrapped( r ) );
        }
        primary_.scroll_up( 0, static_cast<std::uint16_t>( primary_.rows() - 1U ), shift, Cell{} );
        cursor_.row = static_cast<std::uint16_t>( cursor_.row - shift );
    }
    primary_.resize( columns, rows, true, Cell{} );
    alternate_.resize( columns, rows, true, Cell{} );
    tabs_ = std::make_unique<std::uint64_t[]>( tab_words( columns ) );
    reset_tabs();
    top_ = 0;
    bottom_ = static_cast<std::uint16_t>( rows - 1U );
    cursor_.column = std::min<std::uint16_t>( cursor_.column, columns - 1U );
    cursor_.row = std::min<std::uint16_t>( cursor_.row, rows - 1U );
    pending_wrap_ = false;
}

// --- text ---------------------------------------------------------------------

Cell Terminal::blank() const noexcept
{
    // Erasing fills with the current background (xterm's "bce"; sqterm's
    // terminfo says so).
    Cell cell{};
    cell.background = pen_.background;
    cell.flags.default_background = pen_.flags.default_background;
    return cell;
}

void Terminal::put( std::uint16_t column, const Cell &cell ) noexcept
{
    const std::span<Cell> cells = active().row( cursor_.row );
    // Half of a wide character overwritten: the other half goes too.
    if( cells[column].flags.wide_tail && column > 0 ) {
        cells[column - 1U] = blank();
    }
    if( cells[column].flags.wide && column + 1U < cells.size() ) {
        cells[column + 1U] = blank();
    }
    cells[column] = cell;
}

void Terminal::print( char32_t code ) noexcept
{
    const int width = detail::char_width( code );
    if( width == 0 ) {
        return;
    }
    detail::Grid &grid = active();
    const std::uint16_t columns = grid.columns();
    if( pending_wrap_ && modes_.autowrap ) {
        grid.set_wrapped( cursor_.row, true );
        line_feed();
        cursor_.column = 0;
    }
    pending_wrap_ = false;
    if( width == 2 && cursor_.column + 1U >= columns ) {
        if( columns < 2 ) {
            return;
        }
        if( modes_.autowrap ) {
            put( cursor_.column, blank() );
            grid.set_wrapped( cursor_.row, true );
            grid.mark( cursor_.row );
            line_feed();
            cursor_.column = 0;
        } else {
            cursor_.column = static_cast<std::uint16_t>( columns - 2U );
        }
    }
    if( modes_.insert ) {
        insert_cells( static_cast<std::uint16_t>( width ) );
    }
    Cell cell = pen_;
    cell.code = code;
    cell.flags.wide = width == 2;
    cell.flags.wide_tail = false;
    put( cursor_.column, cell );
    if( width == 2 ) {
        Cell tail = pen_;
        tail.code = 0;
        tail.flags.wide = false;
        tail.flags.wide_tail = true;
        put( static_cast<std::uint16_t>( cursor_.column + 1U ), tail );
    }
    grid.mark( cursor_.row );
    last_printed_ = code;
    const unsigned next = cursor_.column + static_cast<unsigned>( width );
    if( next >= columns ) {
        cursor_.column = static_cast<std::uint16_t>( columns - 1U );
        pending_wrap_ = modes_.autowrap;
    } else {
        cursor_.column = static_cast<std::uint16_t>( next );
    }
}

// --- control functions ------------------------------------------------------------

void Terminal::execute( std::uint8_t control ) noexcept
{
    switch( control ) {
        case 0x07: // BEL
            events_.bell = true;
            return;
        case 0x08: // BS
            if( cursor_.column > 0 ) {
                --cursor_.column;
            }
            pending_wrap_ = false;
            return;
        case 0x09: // HT
            tab_forward( 1 );
            return;
        case 0x0A: // LF
        case 0x0B: // VT
        case 0x0C: // FF
        case 0x84: // IND
            line_feed();
            return;
        case 0x0D: // CR
            cursor_.column = 0;
            pending_wrap_ = false;
            return;
        case 0x85: // NEL
            cursor_.column = 0;
            line_feed();
            return;
        case 0x88: // HTS
            tabs_[cursor_.column / 64U] |= std::uint64_t{ 1 } << ( cursor_.column % 64U );
            return;
        case 0x8D: // RI
            reverse_line_feed();
            return;
        default:
            return;
    }
}

void Terminal::escape( const detail::CsiSequence &sequence ) noexcept
{
    if( sequence.intermediate_count > 0 ) {
        // Character sets (ESC ( B) and the like: not taken.
        return;
    }
    switch( sequence.final ) {
        case '7': // DECSC
            save_cursor();
            return;
        case '8': // DECRC
            restore_cursor();
            return;
        case 'D':
            execute( 0x84 );
            return;
        case 'E':
            execute( 0x85 );
            return;
        case 'H':
            execute( 0x88 );
            return;
        case 'M':
            execute( 0x8D );
            return;
        case 'c': // RIS
            full_reset();
            return;
        default: // ST (ESC \), the keypad modes: nothing to do
            return;
    }
}

void Terminal::line_feed() noexcept
{
    pending_wrap_ = false;
    if( cursor_.row == bottom_ ) {
        scroll_up( 1 );
    } else if( cursor_.row + 1U < active().rows() ) {
        ++cursor_.row;
    }
}

void Terminal::reverse_line_feed() noexcept
{
    pending_wrap_ = false;
    if( cursor_.row == top_ ) {
        scroll_down( 1 );
    } else if( cursor_.row > 0 ) {
        --cursor_.row;
    }
}

void Terminal::scroll_up( std::uint16_t count ) noexcept
{
    detail::Grid &grid = active();
    // Lines leave the top of the main screen's whole width into the
    // scrollback, when the region starts at the top (as xterm).
    if( !modes_.alternate_screen && top_ == 0 ) {
        const auto height = static_cast<std::uint16_t>( bottom_ - top_ + 1U );
        const std::uint16_t lines = std::min( count, height );
        for( std::uint16_t r = 0; r < lines; ++r ) {
            scrollback_.push( grid.row( r ), grid.wrapped( r ) );
        }
    }
    grid.scroll_up( top_, bottom_, count, blank() );
}

void Terminal::scroll_down( std::uint16_t count ) noexcept
{
    active().scroll_down( top_, bottom_, count, blank() );
}

void Terminal::move_to( int column, int row ) noexcept
{
    const detail::Grid &grid = active();
    cursor_.column = static_cast<std::uint16_t>( std::clamp( column, 0, grid.columns() - 1 ) );
    cursor_.row = static_cast<std::uint16_t>( std::clamp( row, 0, grid.rows() - 1 ) );
    pending_wrap_ = false;
}

// --- control sequences ------------------------------------------------------------

void Terminal::csi( const detail::CsiSequence &s ) noexcept
{
    if( s.intermediate_count > 0 ) {
        return; // the cursor style (CSI SP q) and the like: not taken
    }
    if( s.private_marker == '?' ) {
        if( s.final == 'h' || s.final == 'l' ) {
            set_mode( s, s.final == 'h' );
        }
        return;
    }
    if( s.private_marker == '>' ) {
        if( s.final == 'c' && s.raw( 0 ) == 0 ) {
            reply( "\x1b[>0;0;0c" );
        }
        return;
    }
    if( s.private_marker != 0 ) {
        return;
    }
    const int column = cursor_.column;
    const int row = cursor_.row;
    const int n = s.at( 0, 1 );
    switch( s.final ) {
        case '@': // ICH
            insert_cells( s.at( 0, 1 ) );
            return;
        case 'A': // CUU: not past the top margin from below it
            move_to( column, std::max( row - n, row >= top_ ? int{ top_ } : 0 ) );
            return;
        case 'B': // CUD
            move_to( column, std::min( row + n, row <= bottom_ ? int{ bottom_ } : rows() - 1 ) );
            return;
        case 'C': // CUF
        case 'a': // HPR
            move_to( column + n, row );
            return;
        case 'D': // CUB
            move_to( column - n, row );
            return;
        case 'E': // CNL
            move_to( 0, std::min( row + n, row <= bottom_ ? int{ bottom_ } : rows() - 1 ) );
            return;
        case 'F': // CPL
            move_to( 0, std::max( row - n, row >= top_ ? int{ top_ } : 0 ) );
            return;
        case 'G': // CHA
        case '`': // HPA
            move_to( n - 1, row );
            return;
        case 'H': // CUP
        case 'f': // HVP
            move_to( s.at( 1, 1 ) - 1, n - 1 );
            return;
        case 'I': // CHT
            tab_forward( s.at( 0, 1 ) );
            return;
        case 'J': // ED
            erase_display( s.raw( 0 ) );
            return;
        case 'K': // EL
            erase_line( s.raw( 0 ) );
            return;
        case 'L': // IL
            insert_lines( s.at( 0, 1 ) );
            return;
        case 'M': // DL
            delete_lines( s.at( 0, 1 ) );
            return;
        case 'P': // DCH
            delete_cells( s.at( 0, 1 ) );
            return;
        case 'S': // SU
            scroll_up( s.at( 0, 1 ) );
            return;
        case 'T': // SD (with more than one parameter, xterm's mouse tracking: not taken)
            if( s.count <= 1 ) {
                scroll_down( s.at( 0, 1 ) );
            }
            return;
        case 'X': { // ECH
            const int last = std::min( column + n, int{ columns() } ) - 1;
            active().fill( cursor_.row, cursor_.column, static_cast<std::uint16_t>( last ),
                           blank() );
            pending_wrap_ = false;
            return;
        }
        case 'Z': // CBT
            tab_backward( s.at( 0, 1 ) );
            return;
        case 'b': { // REP
            const int times = std::min( n, int{ columns() } * rows() );
            for( int i = 0; i < times; ++i ) {
                print( last_printed_ );
            }
            return;
        }
        case 'c': // DA
            if( s.raw( 0 ) == 0 ) {
                reply( "\x1b[?62;22c" );
            }
            return;
        case 'd': // VPA
            move_to( column, n - 1 );
            return;
        case 'e': // VPR
            move_to( column, row + n );
            return;
        case 'g': // TBC
            if( s.raw( 0 ) == 0 ) {
                tabs_[cursor_.column / 64U] &= ~( std::uint64_t{ 1 } << ( cursor_.column % 64U ) );
            } else if( s.raw( 0 ) == 3 ) {
                std::fill_n( tabs_.get(), tab_words( columns() ), std::uint64_t{ 0 } );
            }
            return;
        case 'h': // SM
        case 'l': // RM
            set_mode( s, s.final == 'h' );
            return;
        case 'm':
            select_graphic_rendition( s );
            return;
        case 'n': // DSR
            if( s.raw( 0 ) == 5 ) {
                reply( "\x1b[0n" );
            } else if( s.raw( 0 ) == 6 ) {
                // CPR: ESC [ row ; column R
                std::array<char, 16> text{ '\x1b', '[' };
                char *end =
                    std::to_chars( text.data() + 2, text.data() + text.size(), row + 1 ).ptr;
                *end++ = ';';
                end = std::to_chars( end, text.data() + text.size(), column + 1 ).ptr;
                *end++ = 'R';
                reply( { text.data(), static_cast<std::size_t>( end - text.data() ) } );
            }
            return;
        case 'r': { // DECSTBM
            const int top = s.at( 0, 1 ) - 1;
            const int bottom = s.at( 1, rows() ) - 1;
            if( top < bottom && bottom < rows() ) {
                top_ = static_cast<std::uint16_t>( top );
                bottom_ = static_cast<std::uint16_t>( bottom );
                move_to( 0, 0 );
            }
            return;
        }
        case 's': // SCOSC
            if( s.count == 0 ) {
                save_cursor();
            }
            return;
        case 'u': // SCORC
            if( s.count == 0 ) {
                restore_cursor();
            }
            return;
        default:
            return;
    }
}

void Terminal::set_mode( const detail::CsiSequence &s, bool on ) noexcept
{
    for( std::size_t i = 0; i < s.count; ++i ) {
        const std::uint16_t mode = s.raw( i );
        if( s.private_marker == 0 ) {
            if( mode == 4 ) { // IRM
                modes_.insert = on;
            }
            continue;
        }
        switch( mode ) {
            case 1:
                modes_.application_cursor_keys = on;
                break;
            case 7:
                modes_.autowrap = on;
                pending_wrap_ = false;
                break;
            case 25:
                modes_.cursor_visible = on;
                break;
            case 47:
            case 1047:
                use_alternate( on, false );
                break;
            case 1049:
                use_alternate( on, true );
                break;
            case 2004:
                modes_.bracketed_paste = on;
                break;
            default:
                break;
        }
    }
}

void Terminal::use_alternate( bool on, bool save_and_clear ) noexcept
{
    if( on == modes_.alternate_screen ) {
        return;
    }
    if( on ) {
        if( save_and_clear ) {
            saved_primary_ = { cursor_, pen_, pending_wrap_ };
        }
        modes_.alternate_screen = true;
        alternate_.clear( blank() );
    } else {
        modes_.alternate_screen = false;
        if( save_and_clear ) {
            cursor_ = saved_primary_.cursor;
            pen_ = saved_primary_.pen;
            pending_wrap_ = saved_primary_.pending_wrap;
            move_to( cursor_.column, cursor_.row );
        }
        primary_.mark_all();
    }
}

void Terminal::select_graphic_rendition( const detail::CsiSequence &s ) noexcept
{
    if( s.count == 0 ) {
        pen_ = Cell{};
        return;
    }
    // A colour of SGR 38 or 48, from parameter `i` on; `i` moves past it.
    const auto extended = [&s]( std::size_t &i, Rgb &out ) noexcept -> bool {
        bool found = false;
        if( s.is_sub( i + 1 ) ) {
            // ITU T.416: 38:5:n, 38:2:[colour space]:r:g:b, or 38:2:r:g:b.
            std::size_t end = i + 1;
            while( s.is_sub( end ) ) {
                ++end;
            }
            const std::size_t subs = end - ( i + 1 );
            if( s.raw( i + 1 ) == 5 && subs >= 2 ) {
                out = detail::indexed_colour( clamp_byte( s.raw( i + 2 ) ) );
                found = true;
            } else if( s.raw( i + 1 ) == 2 && subs >= 4 ) {
                const std::size_t first = subs >= 5 ? i + 3 : i + 2;
                out = { clamp_byte( s.raw( first ) ), clamp_byte( s.raw( first + 1 ) ),
                        clamp_byte( s.raw( first + 2 ) ) };
                found = true;
            }
            i = end;
            return found;
        }
        // The semicolon form: 38;5;n and 38;2;r;g;b.
        if( s.raw( i + 1 ) == 5 ) {
            out = detail::indexed_colour( clamp_byte( s.raw( i + 2 ) ) );
            i += 3;
            return true;
        }
        if( s.raw( i + 1 ) == 2 ) {
            out = { clamp_byte( s.raw( i + 2 ) ), clamp_byte( s.raw( i + 3 ) ),
                    clamp_byte( s.raw( i + 4 ) ) };
            i += 5;
            return true;
        }
        i += 2;
        return false;
    };
    std::size_t i = 0;
    while( i < s.count ) {
        if( s.is_sub( i ) ) {
            ++i;
            continue;
        }
        const std::uint16_t p = s.raw( i );
        CellAttributes &a = pen_.attributes;
        if( p == 38 || p == 48 ) {
            Rgb colour{};
            if( extended( i, colour ) ) {
                if( p == 38 ) {
                    pen_.foreground = colour;
                    pen_.flags.default_foreground = false;
                } else {
                    pen_.background = colour;
                    pen_.flags.default_background = false;
                }
            }
            continue;
        }
        if( p >= 30 && p <= 37 ) {
            pen_.foreground = detail::indexed_colour( static_cast<std::uint8_t>( p - 30U ) );
            pen_.flags.default_foreground = false;
        } else if( p >= 90 && p <= 97 ) {
            pen_.foreground = detail::indexed_colour( static_cast<std::uint8_t>( p - 90U + 8U ) );
            pen_.flags.default_foreground = false;
        } else if( p >= 40 && p <= 47 ) {
            pen_.background = detail::indexed_colour( static_cast<std::uint8_t>( p - 40U ) );
            pen_.flags.default_background = false;
        } else if( p >= 100 && p <= 107 ) {
            pen_.background = detail::indexed_colour( static_cast<std::uint8_t>( p - 100U + 8U ) );
            pen_.flags.default_background = false;
        } else {
            switch( p ) {
                case 0:
                    pen_ = Cell{};
                    break;
                case 1:
                    a.bold = true;
                    break;
                case 2:
                    a.faint = true;
                    break;
                case 3:
                    a.italic = true;
                    break;
                case 4: // 4:0 is "not underlined"; 4:1 to 4:5 are kinds of underline
                    a.underline = !( s.is_sub( i + 1 ) && s.raw( i + 1 ) == 0 );
                    break;
                case 5:
                case 6:
                    a.blink = true;
                    break;
                case 7:
                    a.inverse = true;
                    break;
                case 8:
                    a.conceal = true;
                    break;
                case 9:
                    a.crossed_out = true;
                    break;
                case 21:
                    a.underline = true;
                    break;
                case 22:
                    a.bold = false;
                    a.faint = false;
                    break;
                case 23:
                    a.italic = false;
                    break;
                case 24:
                    a.underline = false;
                    break;
                case 25:
                    a.blink = false;
                    break;
                case 27:
                    a.inverse = false;
                    break;
                case 28:
                    a.conceal = false;
                    break;
                case 29:
                    a.crossed_out = false;
                    break;
                case 39:
                    pen_.foreground = {};
                    pen_.flags.default_foreground = true;
                    break;
                case 49:
                    pen_.background = {};
                    pen_.flags.default_background = true;
                    break;
                default:
                    break;
            }
        }
        ++i;
    }
}

// --- erasing, inserting, deleting ---------------------------------------------------

void Terminal::erase_display( std::uint16_t mode ) noexcept
{
    detail::Grid &grid = active();
    const auto last_column = static_cast<std::uint16_t>( grid.columns() - 1U );
    pending_wrap_ = false;
    switch( mode ) {
        case 0:
            erase_line( 0 );
            for( auto r = static_cast<std::uint16_t>( cursor_.row + 1U ); r < grid.rows(); ++r ) {
                grid.fill( r, 0, last_column, blank() );
                grid.set_wrapped( r, false );
            }
            return;
        case 1:
            for( std::uint16_t r = 0; r < cursor_.row; ++r ) {
                grid.fill( r, 0, last_column, blank() );
                grid.set_wrapped( r, false );
            }
            erase_line( 1 );
            return;
        case 2:
            grid.clear( blank() );
            return;
        case 3: // xterm's: the scrollback
            scrollback_.clear();
            return;
        default:
            return;
    }
}

void Terminal::erase_line( std::uint16_t mode ) noexcept
{
    detail::Grid &grid = active();
    const auto last_column = static_cast<std::uint16_t>( grid.columns() - 1U );
    std::uint16_t first = 0;
    std::uint16_t last = last_column;
    if( mode == 0 ) {
        first = cursor_.column;
    } else if( mode == 1 ) {
        last = cursor_.column;
    } else if( mode != 2 ) {
        return;
    }
    // A wide character cut in two by the erase goes whole.
    const std::span<Cell> cells = grid.row( cursor_.row );
    if( first > 0 && cells[first].flags.wide_tail ) {
        --first;
    }
    if( last < last_column && cells[last].flags.wide ) {
        ++last;
    }
    grid.fill( cursor_.row, first, last, blank() );
    if( mode != 1 ) {
        grid.set_wrapped( cursor_.row, false );
    }
    pending_wrap_ = false;
}

void Terminal::insert_cells( std::uint16_t count ) noexcept
{
    detail::Grid &grid = active();
    const std::span<Cell> cells = grid.row( cursor_.row );
    const std::size_t column = cursor_.column;
    const std::size_t n = std::min<std::size_t>( count, cells.size() - column );
    std::move_backward( cells.begin() + static_cast<std::ptrdiff_t>( column ),
                        cells.end() - static_cast<std::ptrdiff_t>( n ), cells.end() );
    std::fill_n( cells.begin() + static_cast<std::ptrdiff_t>( column ), n, blank() );
    if( cells.back().flags.wide ) {
        cells.back() = blank();
    }
    grid.mark( cursor_.row );
    pending_wrap_ = false;
}

void Terminal::delete_cells( std::uint16_t count ) noexcept
{
    detail::Grid &grid = active();
    const std::span<Cell> cells = grid.row( cursor_.row );
    const std::size_t column = cursor_.column;
    const std::size_t n = std::min<std::size_t>( count, cells.size() - column );
    std::move( cells.begin() + static_cast<std::ptrdiff_t>( column + n ), cells.end(),
               cells.begin() + static_cast<std::ptrdiff_t>( column ) );
    std::fill( cells.end() - static_cast<std::ptrdiff_t>( n ), cells.end(), blank() );
    if( cells[column].flags.wide_tail ) {
        cells[column] = blank();
    }
    grid.mark( cursor_.row );
    pending_wrap_ = false;
}

void Terminal::insert_lines( std::uint16_t count ) noexcept
{
    if( cursor_.row < top_ || cursor_.row > bottom_ ) {
        return;
    }
    active().scroll_down( cursor_.row, bottom_, count, blank() );
    cursor_.column = 0;
    pending_wrap_ = false;
}

void Terminal::delete_lines( std::uint16_t count ) noexcept
{
    if( cursor_.row < top_ || cursor_.row > bottom_ ) {
        return;
    }
    active().scroll_up( cursor_.row, bottom_, count, blank() );
    cursor_.column = 0;
    pending_wrap_ = false;
}

// --- tabs ---------------------------------------------------------------------

void Terminal::reset_tabs() noexcept
{
    const std::uint16_t columns = primary_.columns();
    std::fill_n( tabs_.get(), tab_words( columns ), std::uint64_t{ 0 } );
    for( std::uint16_t c = 8; c < columns; c = static_cast<std::uint16_t>( c + 8U ) ) {
        tabs_[c / 64U] |= std::uint64_t{ 1 } << ( c % 64U );
    }
}

bool Terminal::tab_at( int column ) const noexcept
{
    const auto at = static_cast<unsigned>( column );
    return ( ( tabs_[at / 64U] >> ( at % 64U ) ) & 1U ) != 0;
}

void Terminal::tab_forward( std::uint16_t count ) noexcept
{
    const int last = columns() - 1;
    int column = cursor_.column;
    for( std::uint16_t i = 0; i < count && column < last; ++i ) {
        do {
            ++column;
        } while( column < last && !tab_at( column ) );
    }
    cursor_.column = static_cast<std::uint16_t>( column );
    pending_wrap_ = false;
}

void Terminal::tab_backward( std::uint16_t count ) noexcept
{
    int column = cursor_.column;
    for( std::uint16_t i = 0; i < count && column > 0; ++i ) {
        do {
            --column;
        } while( column > 0 && !tab_at( column ) );
    }
    cursor_.column = static_cast<std::uint16_t>( column );
    pending_wrap_ = false;
}

// --- the rest -------------------------------------------------------------------

void Terminal::save_cursor() noexcept
{
    saved_ = { cursor_, pen_, pending_wrap_ };
}

void Terminal::restore_cursor() noexcept
{
    pen_ = saved_.pen;
    move_to( saved_.cursor.column, saved_.cursor.row );
    pending_wrap_ = saved_.pending_wrap;
}

void Terminal::full_reset() noexcept
{
    pen_ = Cell{};
    saved_ = {};
    saved_primary_ = {};
    modes_ = {};
    cursor_ = {};
    pending_wrap_ = false;
    top_ = 0;
    bottom_ = static_cast<std::uint16_t>( primary_.rows() - 1U );
    primary_.clear( Cell{} );
    alternate_.clear( Cell{} );
    reset_tabs();
    title_size_ = 0;
    last_printed_ = U' ';
}

void Terminal::reply( std::string_view text ) noexcept
{
    if( replies_size_ + text.size() > replies_.size() ) {
        return;
    }
    std::copy( text.begin(), text.end(), replies_.begin() + replies_size_ );
    replies_size_ = static_cast<std::uint16_t>( replies_size_ + text.size() );
}

void Terminal::osc( std::string_view text ) noexcept
{
    // OSC 0 (icon name and title) and OSC 2 (title); the rest is not taken.
    const std::size_t semicolon = text.find( ';' );
    if( semicolon == std::string_view::npos ) {
        return;
    }
    const std::string_view number = text.substr( 0, semicolon );
    if( number != "0" && number != "2" ) {
        return;
    }
    std::string_view title = text.substr( semicolon + 1 );
    if( title.size() > title_.size() ) {
        // Cut at a character's start, not inside one.
        std::size_t size = title_.size();
        while( size > 0 && ( static_cast<unsigned char>( title[size] ) & 0xC0U ) == 0x80U ) {
            --size;
        }
        title = title.substr( 0, size );
    }
    std::copy( title.begin(), title.end(), title_.begin() );
    title_size_ = static_cast<std::uint16_t>( title.size() );
    events_.title = true;
}

} // namespace sq::term
