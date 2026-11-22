export module sq.tui:rect;

import std;

export namespace sq::tui
{

/** @brief A rectangle of cells: its top left corner and its size. */
struct Rect
{
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t width = 0;
        std::uint16_t height = 0;

        [[nodiscard]] constexpr std::uint32_t right() const noexcept
        {
            return std::uint32_t{ x } + width;
        }
        [[nodiscard]] constexpr std::uint32_t bottom() const noexcept
        {
            return std::uint32_t{ y } + height;
        }

        friend constexpr bool operator==( Rect, Rect ) noexcept = default;
};

/** @brief A cell's place: column and row, from 0. */
struct Point
{
        std::uint16_t x = 0;
        std::uint16_t y = 0;

        friend constexpr bool operator==( Point, Point ) noexcept = default;
};

} // namespace sq::tui
