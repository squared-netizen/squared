# squared

A cross-platform Linux/Android SDK in C++20 with a Lua 5.4 scripting layer,
for games, simulation software and tooling.

## Layout

```
<module>/include/squared/<module>/<type>.hpp   one public type per header
<module>/src/<type>.cpp                        its definitions
<module>/src/detail/                           private, not installed
<module>/Makefile
build/common.mk                                shared build settings
tools/build.sh                               build wrapper
tools/check_headers.sh                       header self-containment check
tools/measure_sizes.sh                         sizeof report for the docs
docs/programmer/                               using squared
docs/developer/                                maintaining squared
```

Modules: `app`, `assets`, `data`, `files`, `gles`, `graphics`, `graphics2d`, `scene2d`, `gui`,
`math`, `messaging`, `time`.

## Build

```sh
./tools/build.sh                 # release
./tools/build.sh debug
make headers                       # every header, compiled alone
```

Unpack the vendored third-party sources once after checkout:

```sh
./tools/vendor_third_party.sh
```

That is the whole setup. yyjson is compiled into `libsquared_data.a` by the
build; there is nothing external to install and `local.mk` is optional.

`local.mk` is optional, per-checkout, and picked up automatically by
`build/common.mk`.

## Documentation

- [docs/programmer/README.md](docs/programmer/README.md) — building something
  with squared
- [docs/developer/README.md](docs/developer/README.md) — internals, memory
  profile, and the standing priority-order findings

The topic documents at the root of `docs/` are carried over unchanged from
before the split.

## One type, one header

`sq::gui::ScrollPane` is in `squared/gui/scroll_pane.hpp`, and that
header defines nothing else. Every header compiles on its own;
`tools/check_headers.sh` enforces it.

`squared/gui/gui.hpp`, `squared/gui/skin_loader.hpp` and
`squared/<module>/<module>.hpp` are aggregate include lists kept for
convenience and for source compatibility. Do not use them inside a header of
your own.
