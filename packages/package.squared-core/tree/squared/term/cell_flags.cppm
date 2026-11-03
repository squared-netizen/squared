export module sq.term:cell_flags;

import std;

export namespace sq::term
{

/**
 * @brief What a cell's colours and width mean.
 *
 * A colour given as "the default" (SGR 39, 49, or nothing yet) is the
 * terminal's own foreground or background, whatever the theme makes it, not
 * an RGB value: the flags say so, and the cell's `Rgb` is then unused.
 */
struct CellFlags
{
        bool default_foreground : 1 = true;
        bool default_background : 1 = true;
        /** The first of two cells a wide character (CJK, most emoji) takes. */
        bool wide : 1 = false;
        /** The second: nothing of its own, drawn by the cell before it. */
        bool wide_tail : 1 = false;

        friend constexpr bool operator==( CellFlags, CellFlags ) noexcept = default;
};

static_assert( sizeof( CellFlags ) == 1 );

} // namespace sq::term
