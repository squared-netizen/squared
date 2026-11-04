export module sq.term:key_bytes;

export import :terminal_modes;

import sq.app;
import std;

export namespace sq::term
{

/**
 * @brief What a key sends to the program in the terminal: a few bytes, held
 * by value (16 bytes, no allocation). Empty for a key that sends nothing.
 */
struct KeyBytes
{
        std::array<char, 15> bytes{};
        std::uint8_t size = 0;

        [[nodiscard]] std::string_view view() const noexcept { return { bytes.data(), size }; }
        [[nodiscard]] bool empty() const noexcept { return size == 0; }
};

/**
 * @brief The bytes a named key sends, as xterm sends them: arrows `CSI A`
 * (`SS3 A` in application cursor mode, DECCKM), Home and End, Insert,
 * Delete, Page Up and Down (`CSI 5 ~`), F1 to F12, with Shift, Alt and Ctrl
 * as xterm's modifier parameter (`CSI 1 ; 5 A` for Ctrl-Up); Backspace DEL
 * (Ctrl: BS), Enter CR, Tab HT (Shift: `CSI Z`), Escape ESC, Alt putting ESC
 * before the last four. Empty for `unknown` and `space` (a space is text).
 */
[[nodiscard]] KeyBytes key_bytes( app::Key key, app::KeyModifiers modifiers,
                                  TerminalModes modes ) noexcept;

/**
 * @brief The bytes a key that types `text` sends with Ctrl or Alt held
 * (`Widget::key_text`): Ctrl makes a control character of `@` to `_`, the
 * letters, `?` (DEL), space and `2` (NUL), `3` to `7` (ESC to US), `8` (DEL)
 * and `/` (US); Alt puts ESC before it. Text with no control character is
 * sent as it is (with ESC for Alt); text over 14 bytes sends nothing.
 */
[[nodiscard]] KeyBytes key_text_bytes( std::string_view text,
                                       app::KeyModifiers modifiers ) noexcept;

} // namespace sq::term
