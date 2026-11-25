# squared

A cross-platform Linux/Android SDK in C++23 with a Lua 5.4 scripting layer,
for games, simulation software and tooling.

## Layout

squared is named modules, one per directory, in C++23 with libc++, built with
clang.

```
<module>/module.cppm         the module, sq.<module>: re-exports its partitions
<module>/<name>.cppm         a partition per everyday type
<module>/src/<name>.cppm     a partition per supporting type
<module>/src/<name>.cpp      implementation units
<module>/src/detail/         the framework's own types
<module>/test/
squared.cppm                 `import squared;`: what an app is written against
build.lua                    squared's own build, for sqmake: libraries, tools, tests
build/squared.lua            the libraries, as an app's build includes them
build/toolchain.lua          the SDK's toolchain (sqmake's toolchain.sdk)
third_party/                 vendored sources: yyjson, stb, freetype
tools/                       build, check, format, lint and docs scripts
docs/programmer/             using squared
docs/developer/              maintaining squared
```

Modules: `app`, `assets`, `console`, `data`, `dev`, `files`, `font`, `gles`,
`graphics`, `gui`, `math`, `messaging`, `preferences`, `settings`, `stage`,
`term`, `time`, `tui`. Code writes `import
squared;` or `import sq.<module>;`
([docs/programmer/include-layout.md](docs/programmer/include-layout.md),
[docs/developer/modules.md](docs/developer/modules.md)).

## Build

squared needs clang and libc++ 23 (Termux and sqterm have both; Debian and
Ubuntu: clang-23, libc++-23-dev and libc++abi-23-dev from apt.llvm.org),
[sqmake](https://github.com/squared-netizen/sqmake), which builds it from
`build.lua` (sqterm: `pkg install sqmake`; elsewhere its
`tools/bootstrap.sh` makes one with a compiler and a shell), and
[sqedit](https://github.com/squared-netizen/sqedit) on PATH (or as
`SQEDIT`), which does the tools' text work in place of sed, grep and awk
(sqterm: `pkg install sqedit`; elsewhere: `sqmake -C sqedit`). Unpack the
vendored third-party sources once after checkout:

```sh
./tools/vendor_third_party.sh
```

Then:

```sh
sqmake                           # the libraries, sqconsole, dexgen, every test (build/sqmake/)
sqmake --debug                   # the same at -O0 (build/sqmake-debug/)
sqmake test                      # the tests, each from its module's folder
sqmake headers                   # the pinned sources, the modules' imports and names
sqmake lint                      # format, source-rule and clang-tidy checks
sqmake sizes                     # the core types' sizes
tools/dexgen.sh                  # the shim dex written again, byte for byte
```

`CXX` and `CC` name the compiler when `clang++` on PATH is not the one. That
is the whole setup: yyjson, stb_image and FreeType are compiled by the build;
there is nothing else external to install.

## Documentation

- [docs/programmer/README.md](docs/programmer/README.md) — building something
  with squared
- [docs/developer/README.md](docs/developer/README.md) — internals, memory
  profile, and the standing priority-order findings

## One type, one file

`sq::gui::ScrollPane` is in `squared/gui/scroll_pane.cppm`, the partition
`sq.gui:scroll_pane`, and that file defines nothing else. `import sq.gui;`
gives it, and every other type of the module.
