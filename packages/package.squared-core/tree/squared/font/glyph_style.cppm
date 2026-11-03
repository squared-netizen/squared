export module sq.font:glyph_style;

import std;

export namespace sq::font
{

/**
 * @brief How a glyph is drawn when the face has no style of its own for it:
 * bold made by emboldening the outline, italic by slanting it.
 */
struct GlyphStyle
{
        bool bold : 1 = false;
        bool italic : 1 = false;

        friend constexpr bool operator==( GlyphStyle, GlyphStyle ) noexcept = default;
};

} // namespace sq::font
