# Console: design

2026-11-20. What the Console is for, and the rules a change to it is weighed
against. Internals: [console.md](console.md). Using it:
[../programmer/console.md](../programmer/console.md).

## What the Console is

The Console is a very light debugging tool embedded in a squared app: it
starts before the app does, and lets you look into both the app's model and
its GUI from inside the running program. It is for apps where embedding
sqterm, a whole terminal, would be overkill.

- **Text in, text out:** an input line, a scrollback and commands. It is
  meant for when the rest of the program is what's broken.
- **Embedded, not installed:** a module of squared-core (`sq.console`),
  linked only into a `CONSOLE=app` or `CONSOLE=monitor` build. A `CONSOLE=0`
  build, the default, carries none of it.
- **Launches first:** it draws from the first frame, before the app is
  created. When the app fails to start, it is still there (`app start`,
  `crash`, `log previous`).

## Purposes

It has four jobs; anything else is out of scope.

| Purpose | What it gives you | Examples today |
| --- | --- | --- |
| Debug the model | Read and poke the app's state on the device, while it runs | the Telegraph's tools (`telegraph`), the app's own commands, `dump state` |
| Debug the GUI | See what is on screen and why, from inside the window | `screens`, `display`, `gl`, the safe area |
| Survive a broken start | Reach the app when its `create()` fails or it crashed last time | `app stop`/`app start`, `crash`, `log previous`, `make console-only` |
| Watch the process | Memory, threads, files, the log | `mem`, `heap`, `threads`, `fds`, `logcat`, `alloc` (with `DEV_LOG`) |

The same Console is reachable from a Termux terminal through the bridge
(`make console`): its commands from a real keyboard, and its files pulled
and pushed. The bridge is a way in, not a second tool.

## What it is not

The Console is not a terminal and not a shell: it runs no programs and has
no tools installed into it. sqterm is the terminal; the two are different
code with different purposes.

| | Console | sqterm |
| --- | --- | --- |
| What it is | a module embedded in any squared app | an app of its own (a terminal emulator) |
| Runs | the commands the app programmed into it | programs: a shell, packages, anything in its prefix |
| Its commands | compiled in, added by the app with `console.add` | installed with `pkg` |
| Input | its own input line: keys, history, completion | a terminal: keys, a keyboard's paste, Ctrl+V |
| Size | about 73 KB of fixed state; none of it in a release build | a prefix of packages, tens of MB |
| When | debugging the app it is in | daily work on the phone |

A feature an app needs from a terminal (a shell, scripts, its own programs)
means embedding sqterm, not growing the Console.

## Commands are programmed

Every command is code compiled into the app: a name, one line of help, and a
function with a context pointer, added with `console.add`. Nothing is loaded
or installed at run time.

- **Built in, always:** `help`, `clear`, `close`, `dump`.
- **The system's, when added:** `mem`, `heap`, `threads`, `fds`, `device`;
  the files (`ls`, `pull`, `push`), `logcat`, `libs`, `crash`, `log`, `copy`.
- **The platform layer's:** `app` (`start`, `stop`), `screens`, `display`,
  `gl`, `paste`, `quit`; `alloc` with `DEV_LOG`.
- **The app's own:** whatever its model and GUI need to show or change.
  This is the Console's main use: a game adds `spawn`, a tool adds `reload`.
- **Groups:** `dump <group>` runs a named list of commands in turn
  (`dump sys`, `dump state`, `dump crash`), one line per frame so no frame
  waits on it.

The table holds 64 commands by default (`CONSOLE_COMMANDS`). A full table
refuses the next one and says so once in the log.

## Light by design

The Console costs a fixed, known amount and nothing in a release build.

- **Fixed size:** `sizeof(Console)` is 74,592 bytes at the defaults, most of
  it the 64 KB scrollback; every size is a compile-time constant checked by
  `static_assert`.
- **No allocation after start:** printing, running, completing and editing
  allocate nothing (checked by the frame allocation counter). Output is
  `printf` into a 512-byte buffer, not `std::format`.
- **Its own drawing:** an 8x8 bitmap font and a few quads over the app's
  surface; no GUI toolkit, so it still draws when the app's GUI is broken.
- **Off by default:** `CONSOLE=0` links none of it and passes no flag, so a
  release APK carries none of it.
- **One thread:** the app's (glue) thread; nothing in it is shared.
- **The monitor (`CONSOLE=monitor`):** a second process with its own copy,
  so the Console outlives an app crash. Planned, not built.

## Input

The Console takes keys and a few touches, and nothing a keyboard sends for
the app (decided 2026-11-20).

- **Keys:** its input line, with history (Up, Down) and completion (Tab).
  While it has the keyboard, the keys are its own, except the volume keys.
- **Paste:** the `paste` command puts the clipboard into the input line, to
  read before Enter. Ctrl+V does the same. A keyboard's own paste
  (CleverKeys' clipboard keys) is the app's text field's, not the Console's.
- **Touch:** only what it cannot do without. A tap gives it the keyboard, a
  drag scrolls, the close button closes it, a three-finger tap opens or
  closes it. No other gestures (the hold-to-paste was removed for `paste`).
- **Backtick** toggles it from a hardware keyboard.

## Rules for changes

A change to the Console is weighed against these, in order:

1. It serves one of the four purposes: the model, the GUI, a broken start,
   the process.
2. It is a command before it is anything else. A new gesture, panel or mode
   needs a reason a command cannot meet.
3. It keeps the size fixed and adds no allocation after start.
4. It adds nothing to a `CONSOLE=0` build.
5. Terminal features go to sqterm, not here.

## Open questions

- [ ] The monitor (`CONSOLE=monitor`): still wanted, or is the bridge plus
  `make console-only` enough?
- [ ] Should the commands an app adds be listed apart from the built-ins in
  `help`?
- [ ] Does any app need a smaller scrollback than 64 KB by default?
