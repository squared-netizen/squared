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
<module>/Makefile
squared.cppm                 `import squared;`: what an app is written against
build/common.mk              shared build settings
build/modules.mk             how a module is built
third_party/                 vendored sources: yyjson, stb, and libcxx/ (libc++'s
                             std module, pinned: `import std;`)
tools/                       build, check, format, lint and docs scripts
docs/programmer/             using squared
docs/developer/              maintaining squared
```

Modules: `app`, `assets`, `console`, `data`, `dev`, `files`, `gles`,
`graphics`, `gui`, `math`, `messaging`, `stage`, `time`. Code writes `import
squared;` or `import sq.<module>;`
([docs/programmer/include-layout.md](docs/programmer/include-layout.md),
[docs/developer/modules.md](docs/developer/modules.md)).

## Build

squared needs clang and libc++ (Termux has both; Debian and Ubuntu: `apt
install clang libc++-dev libc++abi-dev`). Unpack the vendored third-party
sources once after checkout:

```sh
./tools/vendor_third_party.sh
```

Then:

```sh
./tools/build.sh                 # release
./tools/build.sh debug
make test                        # every module's tests
make headers                     # the pinned std sources, the modules' build order
make lint                        # format, source-rule and clang-tidy checks
```

That is the whole setup. yyjson is compiled into `libsquared_data.a` by the
build; there is nothing external to install. `local.mk` is optional,
per-checkout, and picked up automatically by `build/common.mk`.

## Documentation

- [docs/programmer/README.md](docs/programmer/README.md) — building something
  with squared
- [docs/developer/README.md](docs/developer/README.md) — internals, memory
  profile, and the standing priority-order findings

## One type, one file

`sq::gui::ScrollPane` is in `squared/gui/scroll_pane.cppm`, the partition
`sq.gui:scroll_pane`, and that file defines nothing else. `import sq.gui;`
gives it, and every other type of the module.
