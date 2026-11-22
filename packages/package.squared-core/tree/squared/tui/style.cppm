export module sq.tui:style;

import sq.term;
import std;

export namespace sq::tui
{

/**
 * @brief How text is drawn: its colours and attributes.
 *
 * A colour left empty is the terminal's own foreground or background,
 * whatever its theme makes it. `Style{}` is plain text.
 */
struct Style
{
        std::optional<sq::term::Rgb> foreground;
        std::optional<sq::term::Rgb> background;
        sq::term::CellAttributes attributes{};

        friend constexpr bool operator==( const Style &, const Style & ) noexcept = default;
};

/** @brief A colour from its hex value: `rgb( 0xFF8800 )`. */
[[nodiscard]] constexpr sq::term::Rgb rgb( std::uint32_t hex ) noexcept
{
    return { static_cast<std::uint8_t>( ( hex >> 16U ) & 0xFFU ),
             static_cast<std::uint8_t>( ( hex >> 8U ) & 0xFFU ),
             static_cast<std::uint8_t>( hex & 0xFFU ) };
}

/** @brief A cell's character in a style. */
[[nodiscard]] constexpr sq::term::Cell cell_of( char32_t code, const Style &style ) noexcept
{
    sq::term::Cell cell;
    cell.code = code;
    cell.attributes = style.attributes;
    cell.flags.default_foreground = !style.foreground.has_value();
    cell.flags.default_background = !style.background.has_value();
    if( style.foreground ) {
        cell.foreground = *style.foreground;
    }
    if( style.background ) {
        cell.background = *style.background;
    }
    return cell;
}

/** @brief The style a cell is drawn in. */
[[nodiscard]] constexpr Style style_of( const sq::term::Cell &cell ) noexcept
{
    Style style;
    style.attributes = cell.attributes;
    if( !cell.flags.default_foreground ) {
        style.foreground = cell.foreground;
    }
    if( !cell.flags.default_background ) {
        style.background = cell.background;
    }
    return style;
}

} // namespace sq::tui
