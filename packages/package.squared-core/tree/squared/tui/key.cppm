export module sq.tui:key;

import sq.app;
import std;

export namespace sq::tui
{

/**
 * @brief The named keys and the modifiers are squared's own, sq.app's: the
 * same an app's key events carry, and the same sq.term's `key_bytes` turns
 * into the bytes a terminal sends. The input decoder is its inverse.
 */
using KeyName = sq::app::Key;
using Modifiers = sq::app::KeyModifiers;

/**
 * @brief A key as the terminal reported it: a named key (`name`), or a
 * character (`name` is `KeyName::unknown`).
 *
 * A character keeps its case as typed (`'a'`, `'A'`, `'é'`); Ctrl with a
 * letter is the lower-case letter and `modifiers.control`, since a terminal
 * sends Ctrl+A and Ctrl+Shift+A alike. Compares equal to a character when
 * it is that character with no Ctrl or Alt, so `key == 'q'` reads as it
 * should.
 */
struct Key
{
        KeyName name = KeyName::unknown;
        char32_t character = 0;
        Modifiers modifiers{};

        [[nodiscard]] constexpr bool is_character() const noexcept
        {
            return name == KeyName::unknown;
        }

        friend constexpr bool operator==( const Key &, const Key & ) noexcept = default;

        friend constexpr bool operator==( const Key &key, char32_t c ) noexcept
        {
            return key.is_character() && key.character == c && !key.modifiers.alt &&
                   !key.modifiers.control;
        }
};

} // namespace sq::tui
