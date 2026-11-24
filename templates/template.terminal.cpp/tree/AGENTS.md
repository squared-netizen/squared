# AGENTS.md — {{project_name}}

Conventions for this workspace. Seeded by squared-pg; yours to change.

## Ground rules

- **Write code in `sq_app/`.** That is the user working directory. You should
  never need to edit anything outside it to develop the application.
- **Never edit `sq/build/` or anything under `sq_kit/`.** Those are
  generator-owned and are rewritten on update; changes there are lost.
  Project-wide build settings go in `build.lua`, which is yours
  (`sq/build/template.lua` lists every setting).
- **The build globs.** Every `.cpp` under `sq_app/src/` is compiled. There is
  no source list to keep in sync.

## Language

C++20. The build sets `-std=c++20 -Wall -Wextra -Wpedantic`; keep it warning-free.

Prefer the standard library. The kits deliberately add nothing that `<algorithm>`,
`<filesystem>`, `<regex>` or `<string>` already do well — they exist to make the
call sites shorter, not to replace the library.

## Kits

`#include <terminal/terminal.hpp>` for console I/O, regex helpers and
`FileHandle`.

If `kit.lua` was applied, `#include <lua/lua_host.hpp>` embeds Lua 5.4
and `sq_lua/` holds the scripts. Guard the include with `__has_include`, as
`app.cpp` does, so the same source compiles either way.

## Before committing

```sh
sqmake          # warning-free
sqmake test     # if you have added tests under sq_app/test/
square verify   # nothing generated changed by hand
```
