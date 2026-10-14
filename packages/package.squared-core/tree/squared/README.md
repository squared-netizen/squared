# squared

A cross-platform Linux/Android SDK in C++20 with a Lua 5.4 scripting layer,
for games, simulation software and tooling.

## Layout

```
<module>/<name>.hpp          workhorse headers: the ones users include
<module>/src/<name>.hpp      supporting headers: includable, rarely needed
<module>/src/<name>.cpp      implementation
<module>/src/detail/         private headers
<module>/test/
<module>/Makefile
build/common.mk              shared build settings
third_party/                 vendored sources (yyjson, stb)
tools/                       build, check, format, lint and docs scripts
docs/programmer/             using squared
docs/developer/              maintaining squared
```

Modules: `app`, `assets`, `data`, `dev`, `files`, `gles`, `graphics`, `gui`,
`math`, `messaging`, `time`. Headers are included as
`<squared/<module>/<name>.hpp>` through one include path, the directory that
holds `squared/` ([docs/developer/file-layout.md](docs/developer/file-layout.md)).

## Build

Unpack the vendored third-party sources once after checkout:

```sh
./tools/vendor_third_party.sh
```

Then:

```sh
./tools/build.sh                 # release
./tools/build.sh debug
make headers                     # every header, compiled alone
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

## One type, one header

`sq::gui::ScrollPane` is in `squared/gui/scroll_pane.hpp`, and that
header defines nothing else. Every header compiles on its own;
`tools/check_headers.sh` enforces it.

`squared/gui/gui.hpp`, `squared/gui/skin_loader.hpp` and
`squared/<module>/<module>.hpp` are aggregate include lists kept for
convenience and for source compatibility. Do not use them inside a header of
your own.
