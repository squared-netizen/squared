# squared-tui: character-cell programs (design)

Status: agreed direction (2026-11-22). Built: layers 1 and 2, the test bench
and `Simple` (the second teaching step), as `sq.tui`, now a package of its
own, `package.squared-tui` (`packages/package.squared-tui`; internals in its
`tree/sq_pkg/squared_tui/docs/tui.md`), on squared-core's `sq.term` and
`sq.keys`. Its templates: `template.sq_tui.cpp` (a program) and
`template.sq_tui.cpp.edu` (a learner's, from `Simple`); the terminal
templates are left as they are (decided 2026-11-26). Next: a sample.

squared-tui is one of squared's add-on packages (`squared-`*name*: the
framework's extensions, as squared-core is the framework itself). It is for
programs that draw in a terminal's grid of character cells: tools, games,
editors, dashboards, and the exercises a course in programming starts with.

## Why

Until sqgui exists, the terminal is where most squared programs a person can
write today will show their work, and it is where most teaching and a good
part of tooling already is. What is available for it is either curses (C, a
global screen, terminfo, an API shaped by the terminals of 1980) or libraries
written for games. squared already has half of what a better library needs:
`sq.term`, the terminal emulator sqterm runs on.

## What it is not

- **Not curses.** No curses-compatible API, not even a subset: most of the
  teaching material written for curses is poor, and copying its API would
  carry its global state and its terminfo along with it.
- **No terminfo.** Every terminal that matters speaks VT and xterm's
  extensions: sqterm, Termux, the Linux console and its emulators, macOS's
  Terminal and iTerm2, Windows Terminal. squared-tui writes those sequences
  and nothing else.
- **Not a game engine.** Sprites, tiles and frame timing are squared-core's
  and squared-game's; a game that draws in cells uses squared-tui's screen as
  its surface.

## Emulator and library: mirror images

```
           program                                  terminal
   ┌──────────────────────┐   bytes (VT/xterm)  ┌──────────────────────┐
   │ squared-tui          │ ──────────────────► │ sq.term (sqterm)     │
   │  cells → diff → bytes│                     │  bytes → parse → cells│
   │  bytes → parse → keys│ ◄────────────────── │  keys → bytes         │
   └──────────────────────┘                     └──────────────────────┘
```

A terminal emulator turns bytes into cells and keys into bytes; a terminal
library turns cells into bytes and bytes into keys. So squared-tui takes from
`sq.term` the parts both ends share, and they then cannot disagree:

| From sq.term | In squared-tui |
|---|---|
| `Cell`, `CellAttributes`, `CellFlags`, `Rgb`, the palette | the screen's cells: the same type |
| `char_width` | how many columns a character takes. A library and an emulator using one table cannot disagree about a wide character, the commonest cause of a curses screen drawn out of place |
| the UTF-8 decoder and the escape-sequence parser | reading input: keys, mouse reports (SGR), bracketed paste, focus, resize |
| `key_bytes` | the reverse table: the bytes a key arrives as |
| `Terminal` itself | the test bench and the in-app surface (below) |

These stay in squared-core's `sq.term`; squared-tui imports them.

## Three layers

Each is usable without the ones above it.

### 1. The terminal: `sq.tui.terminal`

The connection to a real terminal.

- Raw mode, the alternate screen, the cursor's visibility, mouse and paste
  reporting: switched on by one object and restored by its destructor, and
  restored as well on a fatal signal and at `std::terminate`. A program that
  crashes leaves a usable shell.
- The size, and a resize event (SIGWINCH).
- One input stream of typed events:
  `Key{code, mods, text}`, `Mouse{x, y, button, kind, mods}`, `Paste{text}`,
  `Resize{columns, rows}`, `Focus{in}`. An escape on its own, and Alt plus a
  key, told apart by a short timeout, as every terminal program must.
- Errors as `std::expected`, no exceptions for the ordinary (not a terminal,
  no size).

```cpp
import sq.tui;

auto term = sq::tui::Terminal::open();          // std::expected<Terminal, Error>
if (!term) return report(term.error());
while (auto event = term->next(100ms)) {        // std::optional<Event>
    if (auto* key = std::get_if<sq::tui::Key>(&*event); key && key->code == 'q') break;
}
```

### 2. The screen: `sq.tui.screen`

A grid of `sq::term::Cell` the program draws into, and a renderer that
writes only what changed since the last frame.

- Two buffers; `show()` compares them and writes the fewest sequences: runs
  of cells, cursor moves only where they are cheaper than writing, attributes
  changed only where they differ.
- Each frame inside synchronized output (mode 2026) so a terminal that
  supports it never shows half a frame; one write per frame.
- 24-bit colour by default, the 256 and 16 colour palettes when asked for.
- Text in UTF-8, placed by `char_width`; a wide character takes two cells and
  the second is marked as its continuation, as `sq.term` marks it.
- Drawing: `put(x, y, text, style)`, `fill(rect, cell)`, `box(rect, style)`,
  clipped views (`screen.view(rect)`) that are just a rectangle and a
  reference: a window in curses' sense, with no state of its own.

**Where it draws** (the one place two outputs are real, so a choice, not a
virtual: an enum or a `std::variant`, as squared's extension policy says):

| Output | For |
|---|---|
| a real terminal (layer 1) | the program as people run it |
| an in-memory `sq::term::Terminal` | tests, and inside an app (below) |

### 3. The interface: `sq.tui.ui` (later)

Layout and widgets: lists, text fields, tables, menus, dialogs, a scrolling
log. Built on the layout model sqgui settles for squared's GUI, so one
layout can be shown in cells or in pixels: a terminal is a surface with
very coarse pixels. Not started until sqgui's layout model is decided.

## What squared can do that others cannot

**A test bench with no terminal.** The screen's output goes through
`sq.term`'s emulator in the test, and the test reads the resulting grid:

```cpp
sq::tui::Test t{80, 24};                     // a screen drawing into an emulator
draw_menu(t.screen(), items);
t.show();
CHECK(t.text_at(2, 1) == "> Open");          // what a person would see
CHECK(t.cell(2, 1).attributes.bold);
t.type("j");                                 // input, as bytes, through the same parser
```

Golden-file tests of a whole screen, of a resize, of a wide character at the
right edge: what a terminal program is hardest to test for.

**A terminal program as an app.** The in-memory output is a
`sq::term::Terminal`, and squared already draws one with GLES (sqterm, the
Console). So a squared-tui program can be built as an APK and run as an app
of its own, its screen drawn by the same renderer and its input from the same
keyboard code, with no terminal app in between. The same source runs in
sqterm, in Termux, and on a desktop terminal.

## For teaching

Three steps, each a complete thing to learn with:

1. **Lines**: `kit.terminal` as it is (`print`, `input`, `prompt`).
2. **Cells**: draw and read keys with no event plumbing, in the spirit of
   conio and of BASIC's `LOCATE` and `INKEY$`:

   ```cpp
   import sq.tui;

   int main() {
       sq::tui::Simple screen;                  // raw mode, alternate screen, restored on exit
       int x = 10, y = 5;
       for (;;) {
           screen.clear();
           screen.put(x, y, "@");
           screen.show();
           const sq::tui::Key key = screen.key();   // waits for one key
           if (key == 'q') return 0;
           if (key == 'h') --x;
           if (key == 'l') ++x;
           if (key == 'k') --y;
           if (key == 'j') ++y;
       }
   }
   ```

   A thin layer over layers 1 and 2, no state of its own, so a learner who
   outgrows it moves to the real types without unlearning anything.
3. **Interfaces**: layer 3.

The second step has a template of its own, `template.sq_tui.cpp.edu`, which
starts from it; `template.term.edu.cpp` stays the first step's (decided
2026-11-26). `kit.terminal` says a TUI belongs here.

## Platforms

| | Output | Input |
|---|---|---|
| Linux, Android (sqterm, Termux), macOS | VT sequences to the tty | termios raw mode, the tty's bytes, SIGWINCH |
| Windows 10 and later | VT sequences (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`) | `ReadConsoleInputW` (keys, mouse, resize) turned into the same events; or VT input mode where it is reliable |
| in an app | an `sq::term::Terminal`, drawn by GLES | the app's keyboard and touch, as sqterm's |

Windows is not a first target: Linux and Android first, as squared is.

## Order of work

1. Layer 1 on Linux and Android: raw mode with restore, size and resize,
   input events (keys, paste, resize first; mouse, focus next).
2. Layer 2 with the terminal output, and the test bench (`sq::tui::Test`)
   at the same time: every layer 2 test runs through `sq.term`.
3. `sq.tui.simple`, and `template.sq_tui.cpp.edu` made from it (with
   `template.sq_tui.cpp` for a program of any kind).
4. A sample in squared-examples: a small editor or a roguelike map, in
   sqterm and as an APK.
5. Layer 3, after sqgui's layout model.

Where the code lives: built in squared-core's tree first (as `sq.term` was),
then moved into a package of its own, `package.squared-tui`, beside
squared-core in this repository (2026-11-26), with sq.app's `Key` and
`KeyModifiers` split out as `sq.keys` so a terminal program builds none of
sq.app's graphics. A repository of its own, `squared-tui`, can follow.

## Open questions

- Decided (2026-11-22): `Simple::key()` returns a `Key`, which compares
  equal to a plain character (`key == 'q'`), so a learner gets modifiers and
  named keys without losing the simple comparison.
- Whether to read terminal capabilities (DA1, XTVERSION, the kitty keyboard
  protocol) at start, to use what the terminal has beyond the baseline. Not
  needed for the first layers; noted so the input reader leaves room for it.
