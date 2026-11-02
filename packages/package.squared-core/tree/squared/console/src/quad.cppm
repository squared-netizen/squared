export module sq.console:src.quad;

export import :src.colour;
import std;

export namespace sq::console
{

/** @brief One rectangle the layout asks to be drawn: a glyph, stretched to the rectangle. */
struct Quad final
{
        int x{ 0 };
        int y{ 0 };
        int width{ 0 };
        int height{ 0 };
        std::uint8_t glyph{ 0 };
        Colour colour{ Colour::text };
};

} // namespace sq::console
