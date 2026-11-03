export module sq.term:cursor;

import std;

export namespace sq::term
{

/** @brief Where the next character goes: a column and a row, from 0 at the top left. */
struct Cursor
{
        std::uint16_t column = 0;
        std::uint16_t row = 0;

        friend constexpr bool operator==( Cursor, Cursor ) noexcept = default;
};

} // namespace sq::term
