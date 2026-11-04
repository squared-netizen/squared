export module sq.term:text_point;

import std;

export namespace sq::term
{

/**
 * @brief A place in a terminal's text: a line, numbered as the terminal
 * numbers them (`Terminal::first_line`, `screen_line`: a line keeps its
 * number as more scroll off), and a column. Ordered by line, then column.
 */
struct TextPoint
{
        std::uint64_t line = 0;
        std::uint16_t column = 0;

        friend constexpr auto operator<=>( const TextPoint &,
                                           const TextPoint & ) noexcept = default;
};

static_assert( sizeof( TextPoint ) == 16 );

} // namespace sq::term
