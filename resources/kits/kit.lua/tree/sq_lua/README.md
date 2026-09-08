# sq_lua — script workspace

Your Lua code. Seeded once; the generator will not touch these files again.

`{{project_name}}` embeds Lua 5.4 through `sq::lua::Host` (see
`sq_kit/include/squared/kit/lua_host.hpp`). At startup it adds this directory
to `package.path` and runs `main.lua`.

## Hooks

`main.lua` defines two globals the application calls:

| Global | When |
|---|---|
| `sq_greet(name)` | once at startup; the returned string is printed |
| `sq_transform(line)` | for each input line, before the application handles it |

Add your own and call them from C++ with `host.call_global("name", {args})`.

## Is Lua actually available?

```sh
make lua-status
```

Lua is acquired from the build host, not vendored into this project. If it is
missing, the application still builds and runs — it just reports that scripts
are inactive.

```sh
pkg install lua54          # Termux
apt install liblua5.4-dev  # Debian, Ubuntu
```

If your Lua lives somewhere unusual, set the flags yourself:

```sh
make SQ_LUA_CFLAGS=-I/opt/lua/include SQ_LUA_LIBS='-L/opt/lua/lib -llua'
```
