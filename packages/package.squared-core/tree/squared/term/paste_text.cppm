export module sq.term:paste_text;

export import :terminal_modes;
import std;

export namespace sq::term
{

/**
 * @brief Pasted text as the program in the terminal is sent it, into `out`;
 * the bytes it takes, all of them, whatever `out` holds.
 *
 * As Termux sends a paste: ESC and the C1 controls (U+0080 to U+009F) are
 * taken out, so a paste cannot end a bracketed paste early or start a
 * control sequence of its own; CR LF and LF become CR, what Enter sends;
 * and, when the program asked for bracketed paste (`CSI ? 2004 h`), it is
 * sent between `CSI 200 ~` and `CSI 201 ~`, so a shell takes it as text
 * rather than running each line. Invalid UTF-8 passes as it is.
 *
 * `out` takes whole characters, in order, until one does not fit: call it
 * with an empty `out` for the size, then with that much. No allocation.
 *
 * @code
 * std::string bytes( sq::term::paste_text( text, modes, {} ), '\0' );
 * sq::term::paste_text( text, modes, bytes );
 * @endcode
 */
std::size_t paste_text( std::string_view text, TerminalModes modes, std::span<char> out ) noexcept;

} // namespace sq::term
