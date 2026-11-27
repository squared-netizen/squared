# sq_app — your code

This is the **user working directory**. Everything in it is yours; the
generator never writes here after making it.

```text
sq_app/src/     your modules (*.cppm) and sources (*.cpp): all compiled
sq_app/test/    tests, each a .cpp with its own main: `sqmake test`
```

## What you can use

`import sq.tui;`, squared-tui (package.squared-tui, in `sq_pkg/squared_tui/`):

| Type | For |
|---|---|
| `Terminal` | the terminal in raw mode: `open()`, `next(timeout)` (keys, mouse, paste, resize), `show(screen)` |
| `Screen`, `View` | a grid of cells: `put`, `fill`, `box`, `clear`; a View is a rectangle of it with its own (0, 0) |
| `Style`, `rgb` | colours (24-bit) and attributes (bold, underline, ...) |
| `Key`, `KeyName` | a key: `key == U'q'` for a plain character, `key.name == KeyName::up` for a named one |
| `Test` | a Screen rendered into squared's terminal emulator, read back as text |
| `Simple` | the first step for a learner: `clear`, `put`, `show`, `key()` |
