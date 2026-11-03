export module sq.term:cell_attributes;

import std;

export namespace sq::term
{

/**
 * @brief How a cell's character is drawn (ECMA-48's SGR), one bit each.
 *
 * `bold` and `faint` are the two intensities; a terminal that shows bold as
 * a brighter colour does so when it draws, the cell keeps the colour it was
 * given.
 */
struct CellAttributes
{
        bool bold : 1 = false;
        bool faint : 1 = false;
        bool italic : 1 = false;
        bool underline : 1 = false;
        bool blink : 1 = false;
        /** Foreground and background swapped when drawn. */
        bool inverse : 1 = false;
        /** Drawn as a blank. */
        bool conceal : 1 = false;
        bool crossed_out : 1 = false;

        friend constexpr bool operator==( CellAttributes, CellAttributes ) noexcept = default;
};

static_assert( sizeof( CellAttributes ) == 1 );

} // namespace sq::term
