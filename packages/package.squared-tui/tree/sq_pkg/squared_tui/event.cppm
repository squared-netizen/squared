export module sq.tui:event;

export import :key;
import std;

export namespace sq::tui
{

/** @brief Which mouse button, or the wheel. */
enum class MouseButton : std::uint8_t {
    none,
    left,
    middle,
    right,
    wheel_up,
    wheel_down,
};

/** @brief What the mouse did. */
enum class MouseAction : std::uint8_t {
    press,
    release,
    /** Moved, a button held or not (`button` says). */
    move,
};

/** @brief A mouse report (SGR, mode 1006): cells from the top left, from 0. */
struct Mouse
{
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        MouseButton button = MouseButton::none;
        MouseAction action = MouseAction::press;
        Modifiers modifiers{};

        friend constexpr bool operator==( const Mouse &, const Mouse & ) noexcept = default;
};

/** @brief Text pasted (bracketed paste, mode 2004): as it was, newlines included. */
struct Paste
{
        std::string text;

        friend bool operator==( const Paste &, const Paste & ) = default;
};

/** @brief The terminal's new size, in cells. */
struct Resize
{
        std::uint16_t columns = 0;
        std::uint16_t rows = 0;

        friend constexpr bool operator==( Resize, Resize ) noexcept = default;
};

/** @brief The terminal's window gained or lost the focus (mode 1004). */
struct Focus
{
        bool in = true;

        friend constexpr bool operator==( Focus, Focus ) noexcept = default;
};

/** @brief Everything a terminal program is told. */
using Event = std::variant<Key, Mouse, Paste, Resize, Focus>;

} // namespace sq::tui
