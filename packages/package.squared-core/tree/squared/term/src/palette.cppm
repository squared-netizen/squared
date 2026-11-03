export module sq.term:src.palette;

export import :rgb;
import std;

export namespace sq::term::detail
{

/**
 * The 256 indexed colours (SGR 30-37, 90-97, 38;5;n and their backgrounds)
 * as RGB: xterm's defaults, which most programs' colour choices assume. 16
 * named colours, a 6x6x6 cube, 24 greys. A cell stores the RGB, so a colour
 * is looked up once, when it is set.
 */
[[nodiscard]] constexpr Rgb indexed_colour( std::uint8_t index ) noexcept
{
    constexpr std::array<Rgb, 16> named{ {
        { 0, 0, 0 },
        { 205, 0, 0 },
        { 0, 205, 0 },
        { 205, 205, 0 },
        { 0, 0, 238 },
        { 205, 0, 205 },
        { 0, 205, 205 },
        { 229, 229, 229 },
        { 127, 127, 127 },
        { 255, 0, 0 },
        { 0, 255, 0 },
        { 255, 255, 0 },
        { 92, 92, 255 },
        { 255, 0, 255 },
        { 0, 255, 255 },
        { 255, 255, 255 },
    } };
    if( index < 16 ) {
        return named[index];
    }
    if( index < 232 ) {
        constexpr std::array<std::uint8_t, 6> levels{ 0, 95, 135, 175, 215, 255 };
        const unsigned cube = index - 16U;
        return { levels[cube / 36U], levels[( cube / 6U ) % 6U], levels[cube % 6U] };
    }
    const auto grey = static_cast<std::uint8_t>( 8U + ( index - 232U ) * 10U );
    return { grey, grey, grey };
}

static_assert( indexed_colour( 1 ) == Rgb{ 205, 0, 0 } );
static_assert( indexed_colour( 196 ) == Rgb{ 255, 0, 0 } );
static_assert( indexed_colour( 255 ) == Rgb{ 238, 238, 238 } );

} // namespace sq::term::detail
