# term

A terminal emulator's core, for sqterm (Sprint 12) and any app that shows a
program's output: the bytes a program writes go in, a screen of cells comes
out (`Terminal`); and a program running in a pseudo-terminal, to read those
bytes from and write keys to (`Pty`). The two are apart: a `Terminal` owns
no pty, draws nothing and reads no keys, so it runs on the model's thread,
on the host, and in a test with no program at all.

It reads ECMA-48 over UTF-8: control functions, control sequences, colours
and attributes (SGR, with 256 colours and 24-bit colour), and operating
system commands for the title. Of the DEC and xterm functions it takes the
alternate screen, scroll regions, cursor keys and cursor visibility, and
bracketed paste. Compatibility with other terminals comes last: what it does
not implement it reads and ignores.

`import sq.term;` (it is not part of `import squared;`).

Developer counterpart: [../developer/term.md](../developer/term.md)

| Type | File | Purpose |
|---|---|---|
| `Terminal` | `squared/term/terminal.cppm` | The terminal: feed it bytes, read its rows, cursor, modes, title and replies |
| `TerminalConfig` | `squared/term/terminal_config.cppm` | Columns, rows, and the scrollback's size in bytes (1 MB) |
| `Cell` | `squared/term/cell.cppm` | One character cell: the character, 24-bit foreground and background, attributes, flags; 12 bytes |
| `Rgb` | `squared/term/rgb.cppm` | A colour, three bytes |
| `CellAttributes` | `squared/term/cell_attributes.cppm` | Bold, faint, italic, underline, blink, inverse, conceal, crossed out |
| `CellFlags` | `squared/term/cell_flags.cppm` | Default foreground or background (the theme's, not the `Rgb`), the halves of a wide character |
| `Cursor` | `squared/term/cursor.cppm` | Column and row, from 0 at the top left |
| `TerminalModes` | `squared/term/terminal_modes.cppm` | What a program has set that changes what the owner does: cursor keys, cursor visible, bracketed paste, alternate screen, autowrap, insert |
| `TerminalEvents` | `squared/term/terminal_events.cppm` | The bell rang; the title changed |
| `Pty` | `squared/term/pty.cppm` | A program in a pseudo-terminal: start it, read what it writes, write keys, resize, see it end, hang up |
| `PtyConfig` | `squared/term/pty_config.cppm` | The program (a path, or a name looked up on PATH), arguments, environment, directory, and the terminal's size |
| `PtyError`, `PtyErrorCode` | `squared/term/pty_error.cppm` | Why starting it failed: the step, and its `errno` |
| `ProcessExit` | `squared/term/process_exit.cppm` | How it ended: the exit code, or the signal |
| `ScreenPublisher` | `squared/term/screen_publisher.cppm` | A terminal's screen written as a snapshot, on the model's thread, with a version for each row |
| `ScreenView` | `squared/term/screen_view.cppm` | A screen snapshot as the UI reads it: rows, versions, cursor, modes, title |
| `KeyBytes`, `key_bytes`, `key_text_bytes` | `squared/term/key_bytes.cppm` | What a key sends to the program: a named key with its modifiers, or a typed character with Ctrl or Alt |
| `TextPoint` | `squared/term/text_point.cppm` | A place in the terminal's text: a line number (kept as more lines scroll off) and a column |
| `paste_text` | `squared/term/paste_text.cppm` | What a paste sends to the program: ESC and C1 taken out, newlines as CR, bracketed when the program asked |

## The shortest use

```cpp
sq::term::Terminal terminal{ { .columns = 80, .rows = 24 } };
sq::term::Pty shell;
if( const auto error = shell.spawn( { .program = "/system/bin/sh" } ) ) {
    report( error.what() );
}
std::array<std::byte, 4096> buffer;
terminal.feed( { buffer.data(), shell.read( buffer ) } );   // any piece of the stream
for( std::uint16_t r = 0; r < terminal.rows(); ++r ) {
    if( terminal.row_dirty( r ) ) {
        draw( r, terminal.row( r ) );                 // std::span<const sq::term::Cell>
    }
}
terminal.clear_dirty();
shell.write( terminal.replies() );                    // a cursor position report, say
terminal.clear_replies();
```

## Terminal

| Call | Does |
|---|---|
| `Terminal(config)` | a blank screen of `config.columns` x `config.rows` (each at least 1), the scrollback's ring allocated |
| `feed(bytes)`, `feed(text)` | reads what the program wrote. A character or sequence split between two calls is read whole. Never allocates |
| `resize(columns, rows)` | a new size. The main screen keeps its top left; when rows below the cursor are lost, the rows above go to the scrollback, so the cursor's line stays on screen. Lines are not rewrapped. Every row is marked dirty |
| `columns()`, `rows()` | the size |
| `row(r)` | row `r` of the screen in use (the alternate one while a program has it), as `std::span<const Cell>` |
| `row_dirty(r)`, `clear_dirty()` | changed since the last `clear_dirty`: draw only those |
| `cursor()` | where the next character goes |
| `modes()` | `TerminalModes`: how to send keys and a paste, whether to draw the cursor |
| `history_size()`, `history_line(back)` | lines scrolled off the top of the main screen; `back` 0 is the newest. A line is as long as its last non-blank cell |
| `title()` | the title a program set (OSC 0 or 2), UTF-8, at most 128 bytes |
| `take_events()` | the bell and a new title since the last call; then cleared |
| `replies()`, `clear_replies()` | bytes to write back to the program: replies to a device status report (`CSI 6 n`, the cursor position) and a device attributes request. At most 256 wait; more are dropped |

Preconditions: `r` below `rows()`, `back` below `history_size()`. One thread
uses a `Terminal`. Failure modes: none returned. Bytes that are not UTF-8
show as U+FFFD; a sequence it does not implement is ignored; an operating
system command over 256 bytes is dropped.

Moved, not copied.

## Cell

```cpp
const sq::term::Cell &cell = terminal.row( 0 )[3];
if( cell.flags.wide_tail ) { /* drawn by the cell before */ }
const auto fg = cell.flags.default_foreground ? theme.foreground : cell.foreground;
```

| Field | Meaning |
|---|---|
| `code` | the character, a Unicode scalar value; a blank is a space; 0 in a wide character's second cell |
| `foreground`, `background` | 24-bit colours. The 256 indexed colours (SGR 30-37, 90-97, `38;5;n`) arrive here already as xterm's RGB |
| `attributes` | `bold`, `faint`, `italic`, `underline`, `blink`, `inverse`, `conceal`, `crossed_out` |
| `flags.default_foreground`, `flags.default_background` | the colour is "the default": the theme's, and the `Rgb` is unused |
| `flags.wide`, `flags.wide_tail` | the two cells of a wide character (CJK, most emoji) |

Combining marks (an accent written as its own code point) are dropped for
now: the letter shows without it.

## What it reads

| Kind | Implemented |
|---|---|
| C0 | BEL, BS, HT, LF, VT, FF, CR; CAN and SUB end a sequence |
| C1 (`ESC` + letter, or the code point) | IND, NEL, HTS, RI; `ESC 7` and `ESC 8` (save and restore the cursor), `ESC c` (reset) |
| Cursor | CUU, CUD, CUF, CUB, CNL, CPL, CHA, HPA, HPR, CUP, HVP, VPA, VPR, CHT, CBT; `CSI s` and `CSI u` |
| Editing | ED (0, 1, 2; 3 clears the scrollback), EL, ECH, ICH, DCH, IL, DL, SU, SD, REP |
| Regions | DECSTBM (`CSI top ; bottom r`) |
| Tabs | HT, HTS, TBC (0, 3); stops every 8 columns |
| SGR | 0-9, 21-29, 30-39, 40-49, 90-97, 100-107; `38` and `48` with `5;n` or `2;r;g;b`, and the `:` forms of ITU T.416 |
| Modes | IRM (`CSI 4 h`); DECCKM (`?1`), DECAWM (`?7`), DECTCEM (`?25`), the alternate screen (`?47`, `?1047`, `?1049`), bracketed paste (`?2004`) |
| Reports | DSR 5 and 6, DA (`CSI c`, `CSI > c`) |
| OSC | 0 and 2, the title |

Erasing fills with the current background colour.

## Pty

A program in a pseudo-terminal, the same on Linux and Android (POSIX). Its
terminal starts with UTF-8 input and without XON/XOFF, so Ctrl-S and Ctrl-Q
reach the program.

```cpp
sq::term::Pty shell;
const std::array<std::string_view, 2> environment{ "TERM=sqterm", "HOME=/data/..." };
if( const auto error = shell.spawn( { .program = "/system/bin/sh", .environment = environment,
                                      .columns = 80, .rows = 24 } ) ) {
    report( error.what(), error.system_error );
}
shell.write( "ls\r" );                         // keys: Enter is \r, Ctrl-C is \x03
if( shell.wait_readable( 16 ) ) {
    terminal.feed( { buffer.data(), shell.read( buffer ) } );
}
shell.resize( terminal.columns(), terminal.rows() );   // after terminal.resize
if( const auto exit = shell.poll_exit() ) { /* exit->code, or exit->signal */ }
```

| Call | Does |
|---|---|
| `spawn(config)` | starts `config.program` (a path such as `/system/bin/sh` or `./run.sh`, or a name such as `zsh`, looked up on PATH as a shell does: the given environment's PATH, else this process's) with `config.arguments` after its name, in a new session with the pty as its terminal, sized `columns` x `rows`. `environment` empty: this process's own; `directory` empty: this process's. An error (true) when it could not |
| `read(into)` | what the program wrote, up to `into.size()` bytes; 0 when nothing waits. Never blocks |
| `write(bytes, timeout_ms)`, `write(text, timeout_ms)` | sends keys or a paste; waits for room up to `timeout_ms` (1000) in all and returns how many bytes went |
| `resize(columns, rows)` | the new size; the program gets SIGWINCH |
| `wait_readable(timeout_ms)` | waits up to `timeout_ms` for something to read, or for the end |
| `poll_exit()` | how the program ended, once it has (the process is reaped then); nothing while it runs |
| `closed()` | nothing more will come: every process holding the terminal has closed it |
| `hang_up()` | ends the program: SIGHUP to its process group, SIGKILL after 200 ms if it is still there; closes the pty. The destructor does it |
| `fd()`, `pid()` | the pty, to wait on with poll alongside other things; the program's process |

Failure modes, returned by `spawn` (`PtyError::code`): `already_running`
(this `Pty` has a program that has not ended), `no_program` (empty),
`open_master`, `open_slave`, `fork`, each with its `errno` in
`system_error`; `what()` says it in words. A program that cannot be run (or
a name not on PATH) is not one of them: the process was made, so it writes
`pty: cannot run <program>: <why>` to the terminal and exits with 127. On
Android 10 and later the "why" for a program in an app's own folder is
"permission denied" unless the app targets SDK 28 (sqterm does).

Preconditions: one thread uses a `Pty`. Moved, not copied.

A program that reads nothing fills the pty: a `write` then waits up to its
timeout and returns short. In line mode (the default, until a program turns
it off) the line discipline drops what does not fit instead, and a write
never waits.

## The screen as a snapshot

A terminal runs on the model's thread (the shell's output is read and fed
there); the UI draws it from the Telegraph's snapshot channel
([messaging.md](messaging.md#snapshots)).

```cpp
// The model's thread, after feeding what the shell wrote:
auto room = t.snapshot( screen, publisher.bytes_for( terminal ) );
t.publish( screen, publisher.write( terminal, room ) );

// The UI thread:
if( const auto shot = ui.snapshot( screen ) ) {
    if( const auto view = sq::term::ScreenView::read( shot->bytes ) ) {
        for( std::uint16_t r = 0; r < view->rows(); ++r ) {
            if( view->version( r ) != drawn[r] ) { draw_row( r, view->row( r ) ); drawn[r] = view->version( r ); }
        }
        if( view->cursor_shown() ) { draw_cursor( view->cursor() ); }
    }
}
```

| `ScreenPublisher` | Does |
|---|---|
| `bytes_for(terminal)` | the room a snapshot of it needs now: 160 bytes, 4 a row, 12 a cell |
| `write(terminal, out)` | the screen into `out`; each row changed since the last write (or every row, when the size or the view moved) gets a new version; the terminal's dirty marks cleared; the bytes written, 0 when `out` is too small |
| `scroll_back(lines)`, `scrolled_back()` | the view `lines` back into the scrollback (0: the screen); held to what the scrollback has |

| `ScreenView` | Gives |
|---|---|
| `read(bytes)` | the view, or nothing when the bytes are not a whole snapshot of this layout |
| `columns()`, `rows()`, `row(r)` | the size, and row `r`'s cells |
| `version(r)` | moves on each time row `r` changes: redraw the rows whose version is not the one drawn |
| `cursor()`, `cursor_shown()` | where the cursor is, and whether to draw it (the program shows it and the view is not scrolled back) |
| `modes()`, `title()`, `history_size()`, `scrolled_back()` | as the terminal had them |
| `top_line()` | the terminal's number for the line row 0 shows: row `r` is line `top_line() + r` (see [Copying and pasting](#copying-and-pasting)) |

Each snapshot is the whole screen (24 KB at 50 x 40): the UI may skip some,
and what it reads must stand alone. A scrolled-back view follows the bottom:
new output moves it.

## What keys send

A terminal widget turns the keys it is given into the bytes a program reads,
as xterm sends them, and writes them to the pty
([gui.md](gui.md): `key_down` for named keys, `key_text` for a character
typed with Ctrl or Alt, `text_input` for plain text, written as it is).

```cpp
bool key_down( sq::app::Key key, sq::app::KeyModifiers held ) override
{
    const sq::term::KeyBytes bytes = sq::term::key_bytes( key, held, view.modes() );
    return !bytes.empty() && send( bytes.view() );
}
bool key_text( std::string_view text, sq::app::KeyModifiers held ) override
{
    const sq::term::KeyBytes bytes = sq::term::key_text_bytes( text, held );
    return !bytes.empty() && send( bytes.view() );
}
```

| Function | Gives |
|---|---|
| `key_bytes( key, modifiers, modes )` | arrows `ESC [ A` (`ESC O A` when the program asked for application cursor keys), Home `ESC [ H`, End `ESC [ F`, Insert `ESC [ 2 ~`, Delete `ESC [ 3 ~`, Page Up and Down `ESC [ 5 ~` and `6 ~`, F1 to F4 `ESC O P` to `S`, F5 to F12 `ESC [ 15 ~` to `24 ~`; with modifiers xterm's parameter, 1 + Shift 1 + Alt 2 + Ctrl 4 + Meta 8 (`ESC [ 1 ; 5 A` is Ctrl-Up, `ESC [ 5 ; 2 ~` Shift-Page Up). Backspace DEL (Ctrl: BS), Enter CR, Tab HT (Shift: `ESC [ Z`), Escape ESC, Alt putting ESC before these four. Empty for Space (it is text) and `unknown` |
| `key_text_bytes( text, modifiers )` | Ctrl with `@` to `_` or a letter: its control character (Ctrl-C 0x03, Ctrl-[ ESC); with Space or `2` NUL, `3` to `7` ESC to US, `8` and `?` DEL, `/` US; any other text as it is. Alt puts ESC before it. Empty for empty text, or text over 14 bytes |
| `KeyBytes::view()`, `empty()` | the bytes; none. 16 bytes by value, no allocation |

## Copying and pasting

A selection's ends are `TextPoint`s: a line, as the terminal numbers them,
and a column. A line keeps its number as more lines scroll off the top, so
a selection made in a view, then scrolled, still names the same text. The
UI reads the numbers off a snapshot (`ScreenView::top_line`); the model,
which holds the scrollback, makes the text (`Terminal::copy_text`).

```cpp
// UI: row r of the view is line view.top_line() + r.
const sq::term::TextPoint from{ .line = view.top_line() + r0, .column = c0 };
const sq::term::TextPoint to{ .line = view.top_line() + r1, .column = c1 };

// Model: the text, sized once, then written.
std::string text( terminal.copy_text( from, to, {} ), '\0' );
terminal.copy_text( from, to, text );

// UI: a paste, as the program should get it.
std::string bytes( sq::term::paste_text( pasted, view.modes(), {} ), '\0' );
sq::term::paste_text( pasted, view.modes(), bytes );
```

| `Terminal` | Gives |
|---|---|
| `first_line()` | the number of the oldest line the scrollback keeps |
| `screen_line()` | the number of the screen's row 0; row `r` is `screen_line() + r` |
| `row_wrapped(row)` | the row went on into the next by autowrap |
| `copy_text(from, to, out)` | the text from `from` to `to` (both included, either first) as UTF-8 into `out`; the bytes the whole text takes, whatever `out` holds |

`copy_text` reads the scrollback's lines, then the screen in use's rows (on
the alternate screen, the program's rows after the main screen's
scrollback), held to the lines there are: a point before `first_line()`
starts at the oldest line kept, one past the last row ends there, and a
range with no line left gives 0. A line that wrapped runs on into the next
with no LF; any other ends with LF, its trailing blanks left out (a wrapped
line keeps its spaces: they are the text's). The last line gets no LF. A
wide character comes once. `out` takes whole characters in order until one
does not fit: call it with an empty span for the size, then with that much.
It allocates nothing; finding the first scrollback line walks back to it,
one step a line.

| Function | Gives |
|---|---|
| `paste_text( text, modes, out )` | `text` as the program is sent it, into `out`; the bytes it takes. ESC and the C1 controls (U+0080 to U+009F) taken out, so a paste starts no control sequence and cannot end a bracketed paste early; CR LF, LF and CR each become CR (Enter); between `ESC [ 200 ~` and `ESC [ 201 ~` when the program set bracketed paste (`modes.bracketed_paste`), so a shell takes it as text rather than running each line. Invalid UTF-8 passes as it is. No allocation |

Failure modes are returned sizes: 0 from `copy_text` for nothing to copy, a
size larger than `out` for a buffer too small (then call again with that
much). Neither fails otherwise.

sqterm's widget does both: a long press selects, its handles drag through
the scrollback, Copy asks the Shell for the text (a snapshot back), and
Paste sends `paste_text` of the clipboard in pieces. The clipboard is the
Stage's ([stage.md](stage.md#the-clipboard)).

