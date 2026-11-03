export module sq.term:cell;

export import :cell_attributes;
export import :cell_flags;
export import :rgb;
import std;

export namespace sq::term
{

/**
 * @brief One character cell of a terminal: 12 bytes.
 *
 * The character, its two colours in 24-bit RGB, how it is drawn, and what
 * the colours and width mean (decided 2026-11-02: colours inline, over a
 * smaller cell with a table of colours beside the screen). A blank is a
 * space in the default colours: `Cell{}`.
 */
struct Cell
{
        /** A Unicode scalar value; 0 in a wide character's tail. */
        char32_t code = U' ';
        Rgb foreground{};
        Rgb background{};
        CellAttributes attributes{};
        CellFlags flags{};

        friend constexpr bool operator==( const Cell &, const Cell & ) noexcept = default;
};

static_assert( sizeof( Cell ) == 12,
               "a cell is 12 bytes: the grid and scrollback are sized by it" );
static_assert( std::is_trivially_copyable_v<Cell> );

} // namespace sq::term
