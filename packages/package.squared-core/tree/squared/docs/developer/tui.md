# tui &mdash; internals

2026-11-22: squared-tui's first two layers, its test bench and the teaching layer, built in
squared-core's tree as `term` was (`sq.tui`, `tui/`), to move to a repository
of its own, `squared-tui`, once they have settled. The design, and what comes
after (widgets on sqgui's layout model):
[squared-tui's design](../../../../../../docs/design/squared-tui.md) in the
squared repository.

## What is here

| Type | Does |
|---|---|
| `Screen` | a grid of `sq::term::Cell`: `put` (UTF-8), `fill`, `box`, the cursor; everything clipped |
| `View` | a rectangle of a Screen with its own (0, 0): a Screen pointer and a Rect, nothing else |
| `Style`, `rgb`, `Rect`, `Point` | how text is drawn; where |
| `Renderer` | a frame into the bytes that show what changed since the last |
| `InputDecoder` | bytes into `Event`s: `Key`, `Mouse`, `Paste`, `Resize`, `Focus` |
| `Terminal` | the program's terminal in raw mode, put back however the program ends; `next(timeout)` and `show(screen)` |
| `Test` | a Screen rendered into `sq::term::Terminal`, read back as a person sees it |
| `Simple` | the terminal for a first program: `clear`, `put`, `show`, `key()`; a resize handled, other events passed over; a Screen and a Terminal underneath |

## Decided (2026-11-22, the user)

- No curses compatibility, not even a subset, and no terminfo: VT and xterm's
  extensions are what every terminal that matters speaks.
- Linux and Android first; no Apple platforms for now.

## What it takes from the rest of squared

squared-tui writes as little of its own as it can. From the rest of squared:

| From | Used for |
|---|---|
| sq.term `Cell`, `CellAttributes`, `Rgb` | the screen's cells and styles |
| sq.term `char_width` | how many columns a character takes |
| sq.term `Utf8Decoder`, `encode_utf8` | UTF-8 in (text drawn, keys read) and out (the renderer); the encoder moved out of `term/src/text.cpp` into `term/src/utf8_encoder.cppm` so both use one |
| sq.term `Terminal` | the test bench's emulator |
| sq.term `key_bytes`, `key_text_bytes`, `paste_text` | the tests: the decoder is checked as their inverse |
| sq.app `Key`, `KeyModifiers` | the named keys and modifiers (`sq::tui::KeyName`, `sq::tui::Modifiers`): the same an app's key events carry |

What is its own: the cell rectangle (`Rect`, `Point`: integer cells, where
squared's other rectangles are float pixels), the renderer's output, the
input decoder, and the POSIX terminal handling.

## The cell is sq.term's

`Screen` holds `sq::term::Cell`, and places characters by
`sq::term::detail::char_width`, the table the emulator uses. A wide
character's two cells are marked as the emulator marks them (`wide`,
`wide_tail`), and writing over either half blanks the other (`unsplit`). A
character of width 0 (a combining mark) is not drawn: one code per cell, as
the emulator's cell holds one. UTF-8 is read by sq.term's `Utf8Decoder`, so
bad bytes become U+FFFD here as they do in sqterm.

## The renderer

It keeps the last frame's cells and, for the next, visits every cell and
writes those that differ, skipping a wide character's tail (its head writes
both). Per cell written: a cursor move (CUP) unless the cursor is already
there after the previous write, and an SGR only when the style differs from
the one the terminal has. An SGR always starts from `0`, so no state is
carried that can go wrong. Colours are 24-bit (`38;2` and `48;2`); a default
colour is left to the terminal's theme.

A frame is wrapped in synchronized output (`CSI ? 2026 h` ... `l`). A frame
with nothing changed and the cursor where it was writes nothing at all. A
size change, or `invalidate()` (the Terminal calls it on a resize), writes
the next frame whole: reset, clear, then every cell that is not a plain
blank.

After the last column the cursor's position is left unknown, since
terminals differ there (pending wrap), and the next cell is reached with a
CUP.

## The input decoder

Bytes are held until they make a whole event; a sequence may be split
between reads anywhere. What it reads:

| Bytes | Event |
|---|---|
| UTF-8 text | `Key{character}` |
| `0x01`..`0x1A` | Ctrl and a letter (lower case); `\r`, `\n` Enter; `\t` Tab; `0x7F` Backspace, `0x08` Ctrl+Backspace; `0x00` Ctrl+Space |
| `ESC` then a key | that key with Alt; `ESC ESC` Alt+Escape |
| `CSI` `A` `B` `C` `D` `H` `F`, `SS3` the same and `P`..`S` | arrows, Home, End, F1..F4; `CSI 1;m X` with modifiers |
| `CSI n ~` | Home, Insert, Delete, End, Page Up and Down, F1..F12; `CSI n;m ~` with modifiers |
| `CSI Z` | Shift+Tab |
| `CSI < b;x;y M` / `m` | SGR mouse: press, release, motion, wheel, with modifiers |
| `CSI 200~` .. `CSI 201~` | one `Paste`, its text as it came (escapes in it are text) |
| `CSI I`, `CSI O` | focus in, out |

A sequence it does not know is dropped whole, up to 64 bytes. An escape on
its own is held until `flush`: the Terminal calls it when nothing has
followed for 25 ms.

## The terminal

`Terminal::open` reads the terminal's mode, sets raw input (no echo, no
canonical lines, no signals from keys, 8-bit, `VMIN` 1), turns on the
alternate screen, bracketed paste, and mouse and focus reports when asked,
and hides the cursor. It writes down, where a signal handler can reach it,
the old mode and the bytes that turn all of that off again.

Those are put back by the destructor, and also:

- on a fatal signal (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGTERM,
  SIGHUP, SIGQUIT, SIGINT): the handler writes the bytes and sets the mode
  (both async-signal-safe), restores the signal's previous handler and
  raises it again, so the process ends as it would have;
- at `exit()`, which skips destructors (`atexit`);
- at `std::terminate`.

Only one Terminal may be open in a process (`already_open`), since the state
those handlers reach is one.

A resize arrives as SIGWINCH: the handler sets a flag and writes a byte to a
pipe that `next` polls beside the input, so a wait wakes for it. `next`
reads the new size (TIOCGWINSZ), invalidates the renderer and returns
`Resize`.

## Tests

`tui/test/tui_test.cpp` (`make -C tui test`):

- the screen: clipping, wide characters at the edge and cut in half, views
  and views of views, a box, bad UTF-8;
- the renderer, read back through `sq::term::Terminal`: the text and colours
  a person would see, nothing written for an unchanged frame, one cell
  written for one cell changed, attributes on and off, the cursor, a wide
  character replaced and restored, a resize;
- the decoder: every row of the table above, sequences split across reads,
  a paste whose end marker is split, an escape inside a paste; and as the
  inverse of sq.term's `key_bytes` (every named key, every mix of Shift,
  Ctrl and Alt, both cursor-key modes: 400 and more), `key_text_bytes`
  (letters with Ctrl and Alt) and `paste_text`. Where two keys send the same
  bytes (Shift or Ctrl with Enter or Escape, Shift with Backspace, Ctrl with
  Tab, Alt with Shift+Tab) the test says so, and expects what the bytes say;
- the Terminal on a pty (`posix_openpt`): raw while open, a second refused,
  keys and a lone escape read through `next`, a frame shown, and after it
  closes the mode as it was and the alternate screen turned off after the
  frame.
- `Simple` on a pty: a coordinate off the screen ignored, a frame shown, no
  key within a wait, a key read, and the terminal put back.

## Simple

What a first program needs and nothing else: it opens the Terminal, sizes a
Screen to it, and its `key()` waits for a key, handling a resize itself
(the screen takes the new size and is shown again) and passing over what is
not a key. Coordinates are `int`, so a learner's -1 is off the screen rather
than 65535. When the terminal cannot be opened it says why and exits with
status 1 instead of returning an error: a first program has nowhere to put
one. `screen()` and `terminal()` are the real types underneath, so moving on
from it is using them directly.
