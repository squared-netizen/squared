#pragma once

namespace sq::console
{

/**
 * @brief Where the console is drawn, in pixels, origin at the top left.
 *
 * `cell` is the size of one character square: a multiple of 8 keeps the font
 * crisp (8 times a whole scale). `scroll` is how many wrapped rows the view is
 * moved back from the newest; 0 shows the newest at the bottom.
 */
struct Frame final
{
        int x{ 0 };
        int y{ 0 };
        int width{ 0 };
        int height{ 0 };
        int cell{ 16 };
        int scroll{ 0 };
};

} // namespace sq::console
