export module sq.term:src.screen_layout;

export import :cell;
export import :cursor;
export import :terminal_modes;
import std;

export namespace sq::term::detail
{

/**
 * A screen snapshot's bytes: this header, then a version for each row
 * (`std::uint32_t`), then the rows' cells, row by row. Written by
 * `ScreenPublisher` on the model's thread, read by `ScreenView` on the UI's;
 * both are this file, so the two never disagree.
 */
struct ScreenHeader
{
        // 2 (12.4): top_line.
        static constexpr std::uint32_t current = 2;

        std::uint32_t layout = current;
        std::uint16_t columns = 0;
        std::uint16_t rows = 0;
        Cursor cursor{};
        TerminalModes modes{};
        std::uint8_t title_size = 0;
        std::uint16_t reserved = 0;
        /** Lines scrolled off the top, and how many of them the view is back. */
        std::uint32_t history_size = 0;
        std::uint32_t scrolled_back = 0;
        /** The number of the line the view's row 0 shows (`Terminal::screen_line`, less the view's back). */
        std::uint64_t top_line = 0;
        std::array<char, 128> title{};
};

inline constexpr std::size_t cells_offset( std::uint16_t rows ) noexcept
{
    return sizeof( ScreenHeader ) + std::size_t{ rows } * sizeof( std::uint32_t );
}

inline constexpr std::size_t screen_bytes( std::uint16_t columns, std::uint16_t rows ) noexcept
{
    return cells_offset( rows ) + std::size_t{ columns } * rows * sizeof( Cell );
}

static_assert( sizeof( ScreenHeader ) % alignof( Cell ) == 0 );

} // namespace sq::term::detail
