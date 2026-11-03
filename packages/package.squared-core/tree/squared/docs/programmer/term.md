# term

A terminal emulator's core, for sqterm (Sprint 12) and any app that shows a
program's output: the bytes a program writes go in, a screen of cells comes
out. It owns no pty, draws nothing and reads no keys, so it runs on the
model's thread, on the host, and in a test with no program at all.

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

## The shortest use

```cpp
sq::term::Terminal terminal{ { .columns = 80, .rows = 24 } };
terminal.feed( bytes_read_from_the_program );        // any piece of the stream
for( std::uint16_t r = 0; r < terminal.rows(); ++r ) {
    if( terminal.row_dirty( r ) ) {
        draw( r, terminal.row( r ) );                 // std::span<const sq::term::Cell>
    }
}
terminal.clear_dirty();
write_to_the_program( terminal.replies() );           // a cursor position report, say
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
