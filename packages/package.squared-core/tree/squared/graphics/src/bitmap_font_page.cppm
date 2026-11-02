export module sq.graphics:src.bitmap_font_page;

import std;

export namespace sq::graphics
{

/** @brief One texture-page dependency declared by a BMFont. */
struct BitmapFontPage final
{
        std::size_t id{ 0 };
        /** @brief Safe relative asset path resolved by the application. */
        std::string file;
};

} // namespace sq::graphics
