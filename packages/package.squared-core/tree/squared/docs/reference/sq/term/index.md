# namespace term

## Namespaces

* [detail](detail/index.md)


## Records

* [CellAttributes](CellAttributes.md)
* [CellFlags](CellFlags.md)
* [Cell](Cell.md)
* [Cursor](Cursor.md)
* [KeyBytes](KeyBytes.md)
* [ProcessExit](ProcessExit.md)
* [PtyConfig](PtyConfig.md)
* [PtyError](PtyError.md)
* [Pty](Pty.md)
* [Rgb](Rgb.md)
* [ScreenPublisher](ScreenPublisher.md)
* [ScreenView](ScreenView.md)
* [TerminalConfig](TerminalConfig.md)
* [TerminalEvents](TerminalEvents.md)
* [TerminalModes](TerminalModes.md)
* [Terminal](Terminal.md)
* [TextPoint](TextPoint.md)


## Functions

### key_bytes

*KeyBytes key_bytes(app::Key key, app::KeyModifiers modifiers, TerminalModes modes)*



**brief** The bytes a named key sends, as xterm sends them: arrows `CSI A` (`SS3 A` in application cursor mode, DECCKM), Home and End, Insert, Delete, Page Up and Down (`CSI 5 ~`), F1 to F12, with Shift, Alt and Ctrl as xterm's modifier parameter (`CSI 1 ; 5 A` for Ctrl-Up); Backspace DEL (Ctrl: BS), Enter CR, Tab HT (Shift: `CSI Z`), Escape ESC, Alt putting ESC before the last four. Empty for `unknown` and `space` (a space is text).

### key_text_bytes

*KeyBytes key_text_bytes(std::string_view text, app::KeyModifiers modifiers)*



**brief** The bytes a key that types `text` sends with Ctrl or Alt held (`Widget::key_text`): Ctrl makes a control character of `@` to `_`, the letters, `?` (DEL), space and `2` (NUL), `3` to `7` (ESC to US), `8` (DEL) and `/` (US); Alt puts ESC before it. Text with no control character is sent as it is (with ESC for Alt); text over 14 bytes sends nothing.

### operator<=>

*std::strong_ordering operator<=>(const TextPoint & , const TextPoint & )*

*Defined at [term/text_point.cppm:18](../../../../term/text_point.cppm#L18)*

### operator==

*bool operator==(CellAttributes , CellAttributes )*

*Defined at [term/cell_attributes.cppm:28](../../../../term/cell_attributes.cppm#L28)*

### operator==

*bool operator==(CellFlags , CellFlags )*

*Defined at [term/cell_flags.cppm:24](../../../../term/cell_flags.cppm#L24)*

### operator==

*bool operator==(Cursor , Cursor )*

*Defined at [term/cursor.cppm:14](../../../../term/cursor.cppm#L14)*

### operator==

*bool operator==(ProcessExit , ProcessExit )*

*Defined at [term/process_exit.cppm:16](../../../../term/process_exit.cppm#L16)*

### operator==

*bool operator==(Rgb , Rgb )*

*Defined at [term/rgb.cppm:15](../../../../term/rgb.cppm#L15)*

### operator==

*bool operator==(TerminalModes , TerminalModes )*

*Defined at [term/terminal_modes.cppm:31](../../../../term/terminal_modes.cppm#L31)*

### operator==

*bool operator==(const Cell & , const Cell & )*

*Defined at [term/cell.cppm:28](../../../../term/cell.cppm#L28)*

### paste_text

*std::size_t paste_text(std::string_view text, TerminalModes modes, std::span<char> out)*



**brief** Pasted text as the program in the terminal is sent it, into `out`; the bytes it takes, all of them, whatever `out` holds.

 As Termux sends a paste: ESC and the C1 controls (U+0080 to U+009F) are taken out, so a paste cannot end a bracketed paste early or start a control sequence of its own; CR LF and LF become CR, what Enter sends; and, when the program asked for bracketed paste (`CSI ? 2004 h`), it is sent between `CSI 200 ~` and `CSI 201 ~`, so a shell takes it as text rather than running each line. Invalid UTF-8 passes as it is.

 `out` takes whole characters, in order, until one does not fit: call it with an empty `out` for the size, then with that much. No allocation.



 std::string bytes( sq::term::paste_text( text, modes, {} ), '\0' );

 sq::term::paste_text( text, modes, bytes );

## Enums

| enum class PtyErrorCode |

--

| none |
| already_running |
| no_program |
| open_master |
| open_slave |
| fork |


*Defined at [term/pty_error.cppm:9](../../../../term/pty_error.cppm#L9)*



**brief** Which step of starting a program failed. 

