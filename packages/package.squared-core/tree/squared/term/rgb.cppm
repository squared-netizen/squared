export module sq.term:rgb;

import std;

export namespace sq::term
{

/** @brief A 24-bit colour, as a terminal cell holds it: three bytes. */
struct Rgb
{
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;

        friend constexpr bool operator==( Rgb, Rgb ) noexcept = default;
};

static_assert( sizeof( Rgb ) == 3 );

} // namespace sq::term
