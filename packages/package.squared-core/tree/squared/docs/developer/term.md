# term &mdash; internals

Sprint 12.1 (2026-11-02): the core of sqterm's terminal, a module of its own
so the Neovim front end (9d) and any app with a terminal pane use it too.
Bytes in, cells out; nothing of a pty, a renderer or keys.

Programmer counterpart: [../programmer/term.md](../programmer/term.md)

## Decided (2026-11-02, the user)

- Its own emulator, with [libvtty](https://github.com/TragicWarrior/libvtty)
  as the inspiration, its stream buffer above all: the emulator runs on bytes
  pushed into it, no pty and no curses (libvtty's `NOPTY` and `NOCURSES`
  mode). libvtty is LGPL 2.1: read for design only, none of its code here.
- ECMA-48 where it can be followed; compatibility with other terminals last,
  as fish treats POSIX sh. `TERM=sqterm`, with a terminfo entry of its own
  that says exactly what is implemented (written with the app, 12.3).
- Beyond ECMA-48: the alternate screen, scroll regions, cursor keys and
  visibility, bracketed paste.
- 12-byte cells, colours inline (over 8-byte cells and a table of 24-bit
  colours beside the screen: simpler, 50% more memory for the grid and
  scrollback; the user's pick, priority 2 over 1).
- Wide characters take two cells; combining marks are dropped for now.
- Scrollback 1 MB by default, settable per session.

## Layout

```
term/
  cell.cppm cell_attributes.cppm cell_flags.cppm rgb.cppm cursor.cppm
  terminal.cppm terminal_config.cppm terminal_events.cppm terminal_modes.cppm module.cppm
  src/   utf8_decoder.cppm parser.cppm csi_sequence.cppm char_width.cppm palette.cppm
         grid.cppm grid.cpp scrollback.cppm scrollback.cpp terminal.cpp
  test/  term_test.cpp
  Makefile
```

It imports only `std`, so the top `Makefile` builds it with nothing before
it. Not in the umbrella (`import squared;`): an app that wants a terminal
imports `sq.term`.

## The flow

```
feed(bytes) ─► Parser ──print / execute / escape / csi / osc──► Terminal::Sink ─► Terminal
                 │                                                                 │
             Utf8Decoder                                              Grid (main, alternate)
                                                                      Scrollback (main only)
```

`Parser::feed` is a template on a `ParserHandler` (a concept: five calls).
The terminal's handler is `Terminal::Sink`, a private struct in
`src/terminal.cpp` that forwards to the terminal's private functions, so the
calls are direct and inlined, and the test's handler records them. No
virtual function anywhere (priority 3; a vtable for one implementation would
be priority 4 spending 3).

## Parser

Paul Williams' state machine ("A parser for DEC's ANSI-compatible video
terminals", vt100.net), the reference for ECMA-48's syntax, on UTF-8:

- Ground: printable ASCII goes straight to `print` in a tight loop (most of a
  terminal's input), as libvtty's fast path. Other bytes go through the
  `Utf8Decoder`; a decoded U+0080..U+009F is a C1 control (U+009B is CSI).
  Raw bytes 0x80..0xFF are never 8-bit controls: in UTF-8 they are text.
- Escape, CSI (entry, parameters, intermediates, ignore), OSC string, and an
  ignored string for DCS, SOS, PM and APC.
- A C0 control inside a sequence runs where it stands; ESC, CAN and SUB end
  the sequence. DEL is ignored everywhere. A byte of 0x80 or more inside a
  sequence ends it and is read again as text.
- Parameters: up to 32, each clamped to 65535; `:` marks a sub-parameter
  (a bit in `CsiSequence::sub`), for ITU T.416 colours. A private marker
  (`? > < =`) is only allowed first; after a parameter the sequence is
  ignored. Up to two intermediates; more, and it is ignored.
- OSC: up to 256 bytes, kept raw (UTF-8 passes through); longer is dropped
  whole ("garbage in, garbage out", as the Telegraph's ring); ended by BEL or
  ESC (the `\` of ST then arrives as an escape sequence and does nothing).

`Utf8Decoder`: Unicode's table 3-7, one byte at a time, so a character split
between two reads is put together; each ill-formed part is one U+FFFD (the
"maximal subpart" rule). `code_then_again` says the byte that broke a
sequence starts again.

## Grid

One block of `rows x columns` cells allocated by `resize`, and a table from
the row on screen to the row in the block (`std::uint16_t` each). Scrolling
a region is `std::rotate` over that table, two bytes a row, and the rows
coming in are filled: no cells move. A dirty bit per row (`uint64_t` words)
says what to draw; a wrapped byte per row (indexed by the block row, so it
moves with it) says autowrap carried the text on, kept for copying and a
later rewrap.

## Scrollback

The lines leaving the top of the main screen (when the scroll region starts
at the top, as xterm), in one ring of `Cell`s:

```
[ header | cells ... | footer ] [ header | ... | footer ] ... [ left over ]
```

A header and a footer cell hold the line's length (`code`), the header also
the wrapped flag. The footer lets `line(back)` walk from the newest line
backwards; the header lets the oldest be dropped from the front. Trailing
blank cells (`Cell{}`) are trimmed, so a short line costs 24 bytes plus 12 a
character. A line never splits across the ring's end: the remainder there is
left, and the lines are then `[head, wrap_end)` followed by `[0, tail)`.
Pushing drops lines from the front until the new one fits; a line longer
than the ring is cut to fit. `line(back)` costs `back` steps: a view
scrolled back 1,000 lines reads 1,000 footers per frame, a few microseconds.

Cells as the ring's unit (over bytes with lengths written in): no casts and
no alignment to think about; costs 24 bytes a line instead of 8.

## Terminal

Members: the two grids, the scrollback, tab stops (a bit per column), the
parser, the pen (a `Cell` whose colours and attributes the next character
takes), the saved cursors (`ESC 7` and the one `?1049` keeps), the title
(128 bytes) and replies (256 bytes) inline, the cursor, the region, the
modes. 960 bytes (`static_assert`ed at most 1024).

- **The pending wrap.** A character in the last column leaves the cursor
  there with `pending_wrap_` set; the next printable character wraps first.
  Any cursor movement, CR, BS, an erase clears it. So `0123456789\r` on a
  10-column screen does not leave an empty line, as ECMA-48 terminals behave.
- **Wide characters.** Two cells, `wide` and `wide_tail`; at the last column
  the line wraps first (or, autowrap off, it goes one column left).
  Overwriting either half blanks the other; an erase that cuts one in two
  takes it whole; ICH and DCH clean a half pushed to the edge.
- **Erasing** fills with the pen's background (`bce`).
- **Colours**: indexed colours become RGB when set (`detail::indexed_colour`,
  xterm's palette), so a cell never needs a palette to be drawn; OSC 4
  palette changes are therefore not taken.
- **The alternate screen** is allocated with the main one (the user accepted
  the second grid); it has no scrollback. `?1049` saves the cursor and pen
  and clears the alternate screen on the way in, restores them on the way
  out.

## Character width

`detail::char_width`: 0 (combining marks: Mn, Me, Cf; Hangul medial vowels
and final consonants), 2 (East Asian Width W and F), else 1, with everything
below U+0300 one cell. Two sorted range tables (357 and 121 ranges, 3.8 KB),
a binary search each. Generated 2026-11-02 from Unicode 15.1.0, Python 3's
`unicodedata`, by:

```python
import unicodedata as u
def width(cp):
    c = chr(cp); cat = u.category(c)
    if cat in ('Mn', 'Me') or (cat == 'Cf' and cp != 0xAD) or 0x1160 <= cp <= 0x11FF: return 0
    return 2 if u.east_asian_width(c) in ('W', 'F') else 1
# ranges of width 0 and of width 2 over U+0300..U+10FFFF, surrogates skipped
```

Run once, by hand; no generator is kept in the tree (squared's tools are
bash and C++; this is a table that changes with Unicode, a few times a
decade).

## Memory

| What | When | Size |
|---|---|---|
| `Terminal` itself | where its owner puts it | 960 bytes |
| Each grid | constructor, `resize` | rows x columns x 12, + 2 a row (table), + 1 a row (wrapped), + 8 per 64 rows (dirty) |
| Scrollback ring | constructor | `scrollback_bytes`, rounded down to 12 |
| Tab stops | constructor, `resize` | 8 bytes per 64 columns |
| `feed` | never | 0 |

80 x 24 with the default scrollback: 2 x 23,112 + 1,048,572 + 16 + 960, about
1.05 MB. A phone's portrait 50 x 40: about 1.1 MB. The test counts every
allocation over 20 feeds of a mixed stream: none.

## Standard library

`std::unique_ptr<T[]>` with `make_unique_for_overwrite` for the cells, the
row table and the ring (filled right after, so zeroing would be wasted);
`std::span` for every row handed out; `std::rotate` for scrolling;
`std::to_chars` for the cursor report (`std::format` passed over: its code
for one integer pair is not worth it). No `std::vector`: every size is set
by `resize` alone. No `std::deque` for the scrollback: a ring over one
block (standard-library-deviations.md's rule).

## Patterns

- **State machine** (the parser): an `enum class` and switches, not a State
  class per state: no allocation, no indirection.
- **Static polymorphism** for the parser's handler: a concept, not a virtual
  interface.

## Tests

`term/test/term_test.cpp`: the decoder (well-formed, overlong, surrogates,
past U+10FFFF, cut sequences); the parser through a recording handler
(parameters, defaults, sub-parameters, clamping, intermediates, C0 inside
sequences, CAN, private markers, OSC by BEL and ST, too long, DCS, C1 as
code points, UTF-8 inside a sequence, DEL); the terminal on hand-written
streams (wrapping, the pending wrap, autowrap off, scrolling into the
scrollback, every cursor and editing function, regions, SGR in both forms,
erasing with the background, wide characters at the edge and half
overwritten, combining marks, invalid UTF-8, the alternate screen with the
cursor and pen restored, modes, insert mode, save and restore, replies,
title and bell, tabs, resizing, dirty rows, reset); the scrollback ring on
its own (trimming, wrap round, eviction, 1,000 pushes, a line longer than
the ring, none at all) and through the terminal (5,000 lines, every kept
line checked); a mixed stream fed whole and one byte at a time, compared
cell by cell; no allocation while feeding; 2,000 rounds of random,
sequence-shaped bytes with resizes.

Run here (2026-11-02, clang 21) as built, and with
`SQUARED_ABI='-fsanitize=undefined,bounds -fsanitize-trap=all
-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG'` (a build of its own,
`BUILD=hard`): UBSan without a runtime (the sandbox's clang 21 has none), and
libc++'s checks on every `span` and `array` index. A feed of 3.4 MB of
coloured lines (every one scrolling into the scrollback) took 68 ms here.

## Known limits

- No rewrap on resize; no combining marks; no palette changes (OSC 4); no
  origin mode, left and right margins, character sets (`ESC ( 0`), mouse
  reporting, or OSC 52 (the clipboard). Each waits for a use.
- `REP` repeats at most a screenful.
