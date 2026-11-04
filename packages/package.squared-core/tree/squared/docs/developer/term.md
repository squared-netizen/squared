# term &mdash; internals

Sprint 12.1 (2026-11-02): the core of sqterm's terminal, a module of its own
so the Neovim front end (9d) and any app with a terminal pane use it too.
Bytes in, cells out (`Terminal`). Sprint 12.2 (2026-11-03): a program in a
pseudo-terminal (`Pty`), apart from the `Terminal`: neither knows the other.

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
  pty.cppm pty_config.cppm pty_error.cppm process_exit.cppm
  screen_publisher.cppm screen_view.cppm key_bytes.cppm
  text_point.cppm paste_text.cppm
  src/   utf8_decoder.cppm parser.cppm csi_sequence.cppm char_width.cppm palette.cppm
         grid.cppm grid.cpp scrollback.cppm scrollback.cpp terminal.cpp pty.cpp
         screen_layout.cppm screen_snapshot.cpp key_bytes.cpp text.cpp
  test/  term_test.cpp pty_test.cpp key_test.cpp text_test.cpp
  Makefile
```

It imports `std`, and `sq.app` for its key names (`key_bytes`, 12.3d): the
top `Makefile` builds it after `app` (`term: app`). Only the enum and the
modifiers are used, so nothing of app's is linked; what it costs is that a
change to app's interfaces (or graphics', which app imports) rebuilds term's
objects. Chosen over a key enum of term's own (a second vocabulary for the
same keys, against `sq::app::Key`'s "one key vocabulary in squared": priority
2 over build time). Not in the umbrella (`import squared;`): an app that
wants a terminal imports `sq.term`.

## What keys send (12.3d)

`key_bytes` and `key_text_bytes` (`src/key_bytes.cpp`) are xterm's encoding,
the one programs expect, though compatibility comes last elsewhere: a key
has to mean what readline, zsh and neovim think it means. Cursor keys and
Home and End are `CSI` letters, `SS3` in DECCKM; the modifier parameter is
xterm's (1 + Shift + 2 Alt + 4 Ctrl + 8 Meta), and with one the key is
`CSI 1 ; m` whatever the mode, as xterm does. F1 to F4 are `SS3` letters
(xterm's default, not the VT220's `CSI 11 ~`); F5 to F12 the VT220 tilde
numbers with xterm's gaps (16 and 22 unused). Backspace sends DEL (as
`stty erase ^?` expects, Termux's choice too), Ctrl-Backspace BS for
editors that tell the two apart. Ctrl with a character follows xterm's
table, including the digits (`2` NUL to `8` DEL). Alt is ESC before the
key (xterm's `metaSendsEscape`), never the eighth bit, which UTF-8 makes
ambiguous. `KeyBytes` is 16 bytes: an array and a count, returned by value,
no allocation. Tested by `test/key_test.cpp`: every named key plain, in
both cursor modes, and with each modifier and a pair; the editing keys;
F1 to F12; the control keys with Ctrl and Alt; Ctrl over the whole table,
non-ASCII text, Alt with and without Ctrl, the 14-byte limit.

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

`visit(back, newest_back, f)` (12.4, for `copy_text`) finds line `back` the
same way, then walks forward by the headers (`start += length + 2`, back to
0 at `wrap_end` when the ring has wrapped): a run of lines is one walk, not
one `line()` each (which would be quadratic over a whole scrollback). A
template in the interface, so the visitor is inlined; no `std::function`.

`pushed()` counts every line ever pushed, kept or not (a 0-byte scrollback
counts too): the terminal's line numbers. 8 bytes; the ring object is 64.

Cells as the ring's unit (over bytes with lengths written in): no casts and
no alignment to think about; costs 24 bytes a line instead of 8.

## Terminal

Members: the two grids, the scrollback, tab stops (a bit per column), the
parser, the pen (a `Cell` whose colours and attributes the next character
takes), the saved cursors (`ESC 7` and the one `?1049` keeps), the title
(128 bytes) and replies (256 bytes) inline, the cursor, the region, the
modes. 968 bytes (`static_assert`ed at most 1024).

- **Line numbers** (12.4). `screen_line()` is the scrollback's `pushed()`:
  every line that went up was pushed, so the screen's row 0 is that many
  lines from the first. `first_line()` is `pushed() - size()`. Nothing to
  keep in step: `clear` (ED 3, RIS) forgets lines and the numbers go on;
  `resize` pushes the rows it drops. Chosen over a counter of the
  terminal's own (the same number, kept twice) and over (history index,
  row) pairs (they move with every line scrolled: a selection would drift).
- **copy_text** (`src/text.cpp`): the points ordered and held to
  `[first_line(), screen_line() + rows() - 1]`; scrollback lines through
  `visit`, then the screen's rows; each line's part from its column range,
  trailing blanks (`code == ' '`) trimmed unless the line wrapped and the
  part runs to its end, LF unless wrapped, none after the last line; wide
  tails and code 0 skipped; UTF-8 written by a small `Out` that takes whole
  pieces until one does not fit and counts them all (a surrogate or a code
  point past U+10FFFF becomes U+FFFD). Two calls, one to size: a copy is
  rare, so the walk twice beats a growing buffer (priority 1, nothing
  allocated in term).

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

## Pty

POSIX only, the same calls on Linux and Android, so it sits in this module
with no porting layer (the porting layer is for what differs).

**spawn.** `posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC)`, `grantpt`,
`unlockpt`, `ptsname_r`; the other end opened in the parent (so a failure
is a returned error, not a child that dies); its termios given IUTF8 (the
line editor erases a whole UTF-8 character) and XON/XOFF off, as Termux's;
the size set with `TIOCSWINSZ` on the master. Then everything the child
needs is made: the program's path, the arguments, the environment and the
failure message as one `std::string` with a NUL after each, and two
`std::vector<char *>` into it (`prepare`). A program named without a `/` is
looked up on PATH here, in the parent (`resolve`: the first `dir/name` that
`access(X_OK)` accepts, an empty entry being `.`), on the given
environment's PATH or else this process's, `/system/bin:/usr/bin:/bin` with
neither; `argv[0]` stays the name as given. Not `execvp` in the child: it is
not async-signal-safe on every C library. Not found: the child skips
`execve` and says "not found". Then `fork`.

**The child** (`run_child`) calls only what is async-signal-safe, since the
parent has threads (an app always does): `setsid`; `ioctl(TIOCSCTTY)`;
`dup2` onto 0, 1 and 2; `close` of every descriptor from 3 to the soft
`RLIMIT_NOFILE` (at most 65,536): an app's binder, sockets and files never
reach the program (`close_range` would be one call, but it is API 31 and
Linux 5.9, and squared builds for API 22); every signal's disposition back
to default and the mask cleared (an app's runtime blocks and catches
several); `chdir`; `execve`. If `execve` fails, the child writes
`pty: cannot run <program>: <why>` (the why chosen from `errno` by a switch
over fixed strings; `strerror` is not async-signal-safe) to the terminal and
`_exit(127)`.

**The parent** closes its copy of the other end and makes the master
non-blocking. `read` returns 0 on `EAGAIN`; `EIO` (Linux, once every
process has closed the other end) or 0 sets `closed()`. `write` loops over
partial writes and, on `EAGAIN`, polls for `POLLOUT` until its deadline. In
line mode (ICANON) Linux's line discipline drops input past its 4,095-byte
buffer instead of pushing back, so only a program that turned line mode off
can fill it (the test does, with `stty -icanon`).

**Ending.** `poll_exit` is `waitpid(WNOHANG)`; the status is kept, so asking
again gives the same answer. `hang_up` closes the master, sends SIGHUP to the
process group (the child is its session's and group's leader, so `-pid`),
polls 20 times 10 ms, then SIGKILLs the group and the process and waits.
The destructor calls it: a `Pty` leaves no process and no zombie behind.
`ECHILD` from `waitpid` (an app's own SIGCHLD handler reaped it) counts as
ended, exit code unknown (0).

**Size.** 16 bytes (`static_assert`ed): the master, the pid, the status,
two flags. `spawn` allocates the strings and pointer tables and frees them
before it returns (in the parent; the child's copies go with `execve`).

Decided without asking (2026-11-03, to say if wrong): the `Pty` lives in
`sq.term` (POSIX, no platform difference); hanging up waits 200 ms before
SIGKILL; a write waits 1 s by default. PATH is searched (the user,
2026-11-03: "Typing the full path is unreasonable"; first shipped without
it). The environment sqterm gives its shell (PATH, HOME, the prefix) is the
app's to set, and the user's `.zshrc` to extend (12.5).

## The screen as a snapshot (12.3c)

`ScreenPublisher` (56 bytes, and a `std::vector<std::uint32_t>` of row
versions, grown with the screen on the model's thread) and `ScreenView`
(192 bytes: a copy of the header and two spans) share one layout,
`src/screen_layout.cppm`: a 160-byte `ScreenHeader` (layout number, size,
cursor, modes, title, history size, how far back, and since layout 2
(12.4) `top_line`, the number of the line row 0 shows), a version a row,
then the rows' cells. Both are this module's, so writer and reader never drift;
`ScreenView::read` refuses another layout number or too few bytes.

- **Versions over dirty bits**: the UI may miss publishes, so "changed
  since the last publish" would lose changes; a row's version only grows,
  and the UI compares it with the one it drew. Every row's moves when the
  size, the view's offset, or (scrolled back) the history moved.
- **Scrolled back**: row r of the view is screen row `r - back` when that
  is 0 or more, else history line `back - r - 1`; a history line shorter
  than the screen (trimmed) is padded with blanks, a longer one cut.
- **Cells as bytes**: written with `memcpy`; read back as `const Cell *`
  over the channel's buffer (new'd `std::byte[]`, aligned for any type; the
  cells start at a multiple of 4). The one `reinterpret_cast`.
- **Cost per publish**: a header, 4 bytes a row, 12 a cell copied; 24 KB
  for 50 x 40, three buffers in the channel, 72 KB.

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
| `Terminal` itself | where its owner puts it | 968 bytes |
| Each grid | constructor, `resize` | rows x columns x 12, + 2 a row (table), + 1 a row (wrapped), + 8 per 64 rows (dirty) |
| Scrollback ring | constructor | `scrollback_bytes`, rounded down to 12 |
| Tab stops | constructor, `resize` | 8 bytes per 64 columns |
| `feed` | never | 0 |
| `Pty` itself | where its owner puts it | 16 bytes |
| `Pty::spawn` | each start, freed before it returns | the arguments and environment, once, plus two pointer tables |
| `Pty::read`, `write`, `resize`, `poll_exit` | never | 0 |
| `ScreenPublisher::write` | when the screen gets taller: its row versions | 4 bytes a row |
| `key_bytes`, `key_text_bytes` | never | 0 (a 16-byte `KeyBytes` returned) |
| `copy_text`, `paste_text` | never | 0 (into the caller's span; sqterm's model writes `copy_text` into a snapshot's room) |
| The snapshot channel's three buffers (messaging's) | when a snapshot is larger than the buffer's largest before | the screen's bytes, 3 times |

80 x 24 with the default scrollback: 2 x 23,112 + 1,048,572 + 16 + 968, about
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

Run here (2026-11-02, clang 21) as built; under AddressSanitizer and UBSan
(2026-11-03, `make BUILD=asan SQUARED_CACHE=off SQUARED_ABI='-fsanitize=
address,undefined -fno-sanitize-recover=undefined'`); and with
`SQUARED_ABI='-fsanitize=undefined,bounds -fsanitize-trap=all
-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG'` (a build of its own,
`BUILD=hard`): UBSan without a runtime (the sandbox's clang 21 has none), and
libc++'s checks on every `span` and `array` index. A feed of 3.4 MB of
coloured lines (every one scrolling into the scrollback) took 68 ms here.

`term/test/text_test.cpp` (12.4): line numbers (lines scrolled off and out
of a small scrollback, a scrollback of none); `copy_text` over the screen
(whole lines, part of one, from mid-line to mid-line, either order, past
the last line, only blanks), wrapped lines (joined, a wrapped line's spaces
kept, in the scrollback too), wide characters (once; started on the second
cell), across the scrollback and the screen, the alternate screen, a ring
that has wrapped (the forward walk across its end, every line against
`history_line`), a short buffer (whole characters); `paste_text` (newlines,
ESC and C1 out, bracketed, tabs, wide and four-byte characters, invalid
UTF-8, a short buffer); the snapshot's `top_line`, scrolled back too; no
allocation in either. Run as built and under AddressSanitizer and UBSan
(2026-11-04, `make BUILD=asan SQUARED_ABI='-fsanitize=address,undefined
-fno-omit-frame-pointer'`).

`term/test/pty_test.cpp` (12.2): a shell (`/bin/sh`, or `/system/bin/sh`
on Android) in a `Pty`, read through a `Terminal`: output with UTF-8 and the
exit code; a name found on this process's PATH (`argv[0]` kept), on a
given environment's PATH, and not found (127); the environment and
directory given; the size at the start and
again after `resize` (SIGWINCH, read with `stty size` in a trap); a line
typed and echoed; Ctrl-C ending `sleep` with SIGINT; a program that cannot
run (the message, 127); refusals (no program, already running);
a full pty (a 1 MB paste stops at its 200 ms timeout); hanging up a program
that ignores SIGHUP (killed in time, reaped, `kill(pid, 0)` then fails); a
moved `Pty`; nothing left after destruction. Run 6 times here, hardened,
and under AddressSanitizer and UBSan with leak checking (2026-11-03: the
sandbox's clang 21 has its sanitizer runtimes now).

## Known limits

- No rewrap on resize; no combining marks; no palette changes (OSC 4); no
  origin mode, left and right margins, character sets (`ESC ( 0`), mouse
  reporting, or OSC 52 (the clipboard). Each waits for a use.
- `copy_text` finds its first scrollback line by walking back from the
  newest: a copy from the oldest of 40,000 lines walks 40,000 headers
  (twice, sizing then writing), well under a millisecond.
- `REP` repeats at most a screenful.
- `Pty`: one program per `Pty`; no reading of the program's current
  directory or name (`/proc/<pid>` can, later); a write is synchronous up to
  its timeout, so a large paste into a busy program waits on the caller's
  thread.
