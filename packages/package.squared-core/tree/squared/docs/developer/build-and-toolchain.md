# Build and toolchain

## What the build is

GNU make plus g++. One `build/common.mk` with every shared setting, one
`Makefile` per module, one top-level `Makefile`, and two bash entry points in
`tools/`. No CMake, no generator step, no network access, no `sudo`, and no
path outside the project root &mdash; the same build runs under Termux on
Android and on desktop Linux.

## Targets

```sh
make                 # every module, release
make BUILD=debug     # -O0 -g
make gui             # one module
make headers         # compile every public header standalone
make clean
```

Module dependency order is declared in the top-level `Makefile`, so `make -j`
is safe.

## Flags

Defaults in `build/common.mk`:

```
-std=c++20 -O2 -DNDEBUG
-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
-fno-common -ffunction-sections -fdata-sections -fvisibility=hidden
```

The tree builds **warning-free** at that level. `-Wconversion` and
`-Wsign-conversion` are on deliberately: they are the warnings that catch the
narrowing bugs that hurt on a 32-bit Android target, and the code is already
clean under them, so keeping them on costs nothing and protects that.

`-ffunction-sections -fdata-sections -fvisibility=hidden` are there because
Android is the constraining target: they let the linker drop unreferenced code
from the final APK.

Every setting is overridable:

```sh
make CXX=clang++ CXXFLAGS_EXTRA=-fsanitize=address
```

## `SQUARED_ABI`

`SQUARED_ABI` carries the ABI-shrinking flags.

`-fno-rtti` builds clean for this tree and is verified:

```sh
make SQUARED_ABI=-fno-rtti
```

It is not the default because `sq::app` and `sq::data` live
outside this archive and have not been checked with it. Turn it on once they
are.

`-fno-exceptions` does not build yet: **21 remaining errors**, catalogued by
kind in [priority-audit.md](priority-audit.md) item 3. One of them is outside
this archive &mdash; `skin_loader.cpp` calls into `sq::data`, so that
module has to be exception-free first.

## External modules

`sq::gui` names `sq::app` (event and text-input) and
`sq::data` (JSON, used by the skin loader). Neither is in this archive.

```sh
SQUARED_EXTERNAL_INCLUDES="-I../application/include -I../data/include" make
```

## clang-format and clang-tidy

The project rules call for clang-format and clang-tidy on every C++ change.
Neither was available in the environment this refactor was performed in, so
**no C++ in this tree has been run through either tool.** The code is
unchanged from its input, so it carries whatever formatting it already had;
run both before merging.

## Checking self-containment

```sh
./tools/check_headers.sh
```

Compiles every public header alone with `-Wall -Wextra -Wpedantic`. Exit status
is the number that are not self-contained. Run it in CI; it is the one check
that protects the layout.

Each header is a full compile, so the whole tree takes minutes on a phone. Two
things make that bearable:

```sh
JOBS=4 ./tools/check_headers.sh        # compile in parallel
./tools/check_headers.sh gui           # one module
```

It prints a dot per header as it goes. That is not decoration: a check this
slow that prints nothing is indistinguishable from a hung one, and it will get
killed halfway by someone who reasonably assumes it wedged.
