export module sq.term:src.csi_sequence;

import std;

export namespace sq::term::detail
{

/**
 * One control sequence (ECMA-48's CSI ... final), as the parser hands it
 * over: its parameters, a private marker (`?`, `>`, `<`, `=`, or 0) and up to
 * two intermediates.
 *
 * Parameters are numbers up to 65535 (larger ones are clamped); an empty one
 * is 0, which ECMA-48 makes "the default" for most functions. A parameter
 * written after `:` is a sub-parameter of the one before it (ITU T.416's
 * colours, `38:2::255:0:0`): bit i of `sub` is set when parameter i is one.
 */
struct CsiSequence
{
        static constexpr std::size_t max_parameters = 32;

        std::array<std::uint16_t, max_parameters> parameters{};
        std::uint32_t sub = 0;
        std::uint8_t count = 0;
        char private_marker = 0;
        std::array<char, 2> intermediates{};
        std::uint8_t intermediate_count = 0;
        char final = 0;

        /** Parameter `index`, or `fallback` when it is missing or 0. */
        [[nodiscard]] constexpr std::uint16_t at( std::size_t index,
                                                  std::uint16_t fallback ) const noexcept
        {
            if( index >= count || parameters[index] == 0 ) {
                return fallback;
            }
            return parameters[index];
        }

        /** Parameter `index` as written, 0 when missing. */
        [[nodiscard]] constexpr std::uint16_t raw( std::size_t index ) const noexcept
        {
            return index < count ? parameters[index] : std::uint16_t{ 0 };
        }

        [[nodiscard]] constexpr bool is_sub( std::size_t index ) const noexcept
        {
            return index < count && ( ( sub >> index ) & 1U ) != 0;
        }
};

} // namespace sq::term::detail
