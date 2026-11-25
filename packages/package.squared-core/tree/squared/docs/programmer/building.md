# Building

squared builds with clang and [sqmake](https://github.com/squared-netizen/sqmake),
from Lua build files: no make, no CMake, no generator step, no network access,
and nothing that assumes a desktop. squared is modules, in C++23
([include-layout.md](include-layout.md)), with libc++ everywhere: Android's
and sqterm's already; on a desktop, clang 23 and libc++ 23 from apt.llvm.org
(Ubuntu 24.04's own clang is 18). sqmake comes with sqterm (`pkg install
sqmake`); elsewhere its `tools/bootstrap.sh` makes one with a compiler and a
shell.

## Desktop Linux and Termux

In `squared/`, which `build.lua` describes:

```sh
sqmake               # every library, sqconsole, dexgen and every test's program
sqmake --debug       # the same at -O0 -g
sqmake test          # every module's tests, each run from its module's folder
sqmake headers       # the pinned FreeType sources, the modules' imports and names
sqmake lint          # format, source rules, clang-tidy (../developer/lint.md)
sqmake sizes         # the core types' sizes
sqmake clean
```

`CXX` and `CC` name the compiler when `clang++` on PATH is not the one. The
build goes in `build/sqmake/` (`build/sqmake-debug/` for `--debug`):
`libsquared_<module>.a`, the tools, each test as `<module>/<test>`, and
`compile_commands.json`, every compile's command, for clang-tidy and clangd.

**What a build prints.** A line as each step starts, numbered against the
plan: `[12/301] module sq.gui:label ../squared/gui/label.cppm`, `cxx`, `cc`,
`ar`, `link`. Warnings and errors print in full, as the compiler wrote them.
`sqmake test` prints a line a test and its output, then `all tests passed` or
the failed ones by name. `sqmake --explain` runs nothing and says what would
run, and why.

**Only what changed is built.** A target is made again when its inputs (the
headers a compile read, the BMIs it imports) or its command change. With
nothing changed, `sqmake` says `up to date`.

## Third-party sources

Every squared module is in this tree, and so are yyjson, stb_image and
FreeType. Unpack them once after checkout:

```sh
./tools/vendor_third_party.sh
```

It is idempotent and leaves an unpacked dependency alone. The build compiles
them, as C, into `libsquared_data.a`, `libsquared_stb.a` and
`libsquared_freetype.a`, for your target.

**Why vendored rather than a system package.** `pkg install yyjson` gives a
library built for the host, which may even link and then fail on a device.
The source is compiled by your build, for your target, or not at all.

## For Android

A project made from the Android template (`sqpg new`) builds squared for
you, for the project's target: its `build.lua` (`app = {...}`, said by
`sq/build/template.lua`) includes `squared/build/squared.lua`, which builds the
framework with the same compiler, API level and flags as your own code. You do
not build the framework separately.

```sh
sqmake                 # build/<name>.apk
sqmake --debug install # the debug build: the Console, the dev log; debuggable
sqmake install         # install it (on sqterm, sqterm's installer)
sqmake run             # install, then start it
sqmake logcat          # the app's log lines (Android's own words: logcat, devices, uninstall)
sqmake console         # a debug build's Console in this terminal (console.md); CMD='...' runs one command
sqmake toolchain       # what the SDK's toolchain is, and where, or what is missing
```

The API level is `min_sdk` in `app = {...}`, default **28**, the sysroot
package's. It is what the code is compiled and linked for, not only what the
manifest claims. The C++ runtime is linked **into** your library: one `.so`,
nothing beside it to get out of step.

| `app = {...}` | What it does |
|---|---|
| `console = true` | the debug build's Console (`-DSQUARED_CONSOLE=1`); a release build carries none of it ([console.md](console.md)) |
| `dev_log = 5555` | the debug build's dev log, to that UDP port ([dev.md](dev.md)) |
| `shared = {...}` | what the app and squared must agree on, given to both: `-DSQUARED_TELEGRAPH_RING=512` and the other Telegraph sizes ([messaging.md](messaging.md#the-telegraph)), the Console's |
| `debuggable = true` | a release build that `run-as` reaches; a debug build always is |
| `optimise = false` | `-O0 -g`, squared's too |

### The SDK's own toolchain

clang 23, libc++ 23 and bionic, from sqterm's `llvm`, `runtimes` and `sysroot`
packages (on CI, apt.llvm.org's clang 23 with the same two packages). No NDK.
It is `build/toolchain.lua`, as data: sqmake finds each part itself, with no
shell (`toolchain.sdk`), under `$PREFIX` in sqterm; on another machine, name
them with `SQ_LLVM_BIN`, `SQ_SYSROOT` and `SQ_RUNTIMES`. `sqmake toolchain`
says what it found and why, or exactly what is missing and how to get it.
The build goes in a folder of its fingerprint, so its modules never meet
another toolchain's. (Until 2026-11-25 `build/toolchain.sh` said it again,
in bash, for sqterm's packages; they use sqmake's now, and it went.)

A build of your own includes squared's libraries the template's way:

```lua
toolchain.sdk("squared/build/toolchain.lua")
squared_settings = {root = "squared"}                  -- platform, graphics, debug, flags: build/squared.lua
local squared = include("squared/build/squared.lua")
shlib{name = "game", srcs = glob("sq_app/src/*.cpp"), deps = {squared.all}}
```

`sq.preferences` and `sq.settings` are libraries of their own, outside
`squared.all`: `deps = {squared.all, squared.settings}` where the app has the
Settings screen.

## Flags

`build/squared.lua` gives every library `-O2 -DNDEBUG` (`-O0 -g` when
`debug`), `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`,
and `-ffunction-sections -fdata-sections -fvisibility=hidden -fPIC`, so the
linker can drop what an application does not use. The tree builds
warning-free at that level. `squared_settings.flags` is what the app and
squared share.

**Every module is compiled before what imports it.** sqmake reads each
source's import lines and orders the compiles by them: `module sq.gui:label`
is an interface becoming its BMI. The standard library is a module too,
`std`, built from the installed libc++'s own `std.cppm`, once for each
compiler and flag set.

**Compiles are cached** by what each one reads: the source and every file it
included, the BMIs it imports, the compiler and the command. A compile that
would read the same again takes the object instead: a new workspace compiles
nothing another on the device already did. One cache for the device
(`$PREFIX/var/cache/sqmake` in sqterm, else `~/.cache/sqmake`), kept under
`SQMAKE_CACHE_SIZE` megabytes (512), least lately used out first:

```sh
SQMAKE_CACHE=off sqmake          # compile everything, keep nothing
SQMAKE_CACHE=/elsewhere sqmake   # another cache
```

Until 2026-11-25 squared also built with GNU make (its `Makefile`s,
`build/common.mk`, `tools/build.sh`); [../developer/build-and-toolchain.md](
../developer/build-and-toolchain.md) keeps how that build worked, and why.
