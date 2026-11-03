export module sq.term:terminal_modes;

import std;

export namespace sq::term
{

/**
 * @brief The modes a program has set that change what the terminal's owner
 * does: how keys are sent, whether the cursor is drawn, how a paste is sent.
 *
 * ECMA-48 modes and the DEC and xterm ones sqterm takes (decided 2026-11-02:
 * the alternate screen, scroll regions, cursor keys and visibility,
 * bracketed paste).
 */
struct TerminalModes
{
        /** DECCKM (`CSI ? 1 h`): cursor keys send `ESC O A`, not `CSI A`. */
        bool application_cursor_keys : 1 = false;
        /** DECTCEM (`CSI ? 25 l` hides it). */
        bool cursor_visible : 1 = true;
        /** `CSI ? 2004 h`: a paste is sent between `CSI 200 ~` and `CSI 201 ~`. */
        bool bracketed_paste : 1 = false;
        /** `CSI ? 1049 h` (or 47, 1047): the second screen, which has no scrollback. */
        bool alternate_screen : 1 = false;
        /** DECAWM (`CSI ? 7 l` turns it off): a character past the last column goes to the next line. */
        bool autowrap : 1 = true;
        /** IRM (`CSI 4 h`, ECMA-48): a character pushes the rest of the line right. */
        bool insert : 1 = false;

        friend constexpr bool operator==( TerminalModes, TerminalModes ) noexcept = default;
};

} // namespace sq::term
