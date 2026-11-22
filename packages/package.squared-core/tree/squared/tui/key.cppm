export module sq.tui:key;

import std;

export namespace sq::tui
{

/** @brief Which key: a character, or one of the keys that has a name. */
enum class KeyCode : std::uint8_t {
    /** `Key::character` says which. */
    character,
    enter,
    tab,
    backspace,
    escape,
    up,
    down,
    left,
    right,
    home,
    end,
    page_up,
    page_down,
    insert,
    delete_,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
};

/** @brief The modifiers held with a key or a mouse button. */
struct Modifiers
{
        bool shift : 1 = false;
        bool alt : 1 = false;
        bool control : 1 = false;

        friend constexpr bool operator==( Modifiers, Modifiers ) noexcept = default;
};

/**
 * @brief A key as the terminal reported it.
 *
 * A character key keeps its character as typed (`'a'`, `'A'`, `'é'`); Ctrl
 * with a letter is the lower-case letter and `modifiers.control`, since a
 * terminal sends Ctrl+A and Ctrl+Shift+A alike. Compares equal to a
 * character when it is that character with no Ctrl or Alt, so `key == 'q'`
 * reads as it should.
 */
struct Key
{
        KeyCode code = KeyCode::character;
        char32_t character = 0;
        Modifiers modifiers{};

        friend constexpr bool operator==( const Key &, const Key & ) noexcept = default;

        friend constexpr bool operator==( const Key &key, char32_t c ) noexcept
        {
            return key.code == KeyCode::character && key.character == c && !key.modifiers.alt &&
                   !key.modifiers.control;
        }
};

} // namespace sq::tui
