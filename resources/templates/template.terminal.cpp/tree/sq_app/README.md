# sq_app — your code

This is the **user working directory**. Everything in it is yours.

The generator guarantees two things about this directory (Generator
Architecture §2.7.3):

- you never need to edit anything outside it to develop the application;
- it never writes a `generated`-class file inside it, so nothing here is at
  risk of being overwritten on a later update.

## Layout

```text
sq_app/include/    your headers
sq_app/src/        your sources — every .cpp here is compiled and linked
```

Add files freely. The build globs; there is no list to maintain.

## What you can use

`#include <squared/kit/terminal.hpp>` — from kit.terminal. Console I/O, a
small regex facade, and a libGDX-style `FileHandle`. Header-only.

`#include <squared/kit/lua_host.hpp>` — from kit.lua, if it was applied. Guard
it with `__has_include` as `app.cpp` does, and the same source compiles with or
without the kit.
