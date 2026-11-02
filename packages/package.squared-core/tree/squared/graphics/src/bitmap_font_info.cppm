export module sq.graphics:src.bitmap_font_info;

import std;

export namespace sq::graphics
{

/** @brief Portable BMFont face metadata. */
struct BitmapFontInfo final
{
        std::string face;
        int size{ 0 };
        bool bold{ false };
        bool italic{ false };
        bool unicode{ false };
};

} // namespace sq::graphics
