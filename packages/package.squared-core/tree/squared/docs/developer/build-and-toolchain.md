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
make lint            # format, source-rule and clang-tidy checks
make V=1             # every command in full
make clean
```

Module dependency order is declared in the top-level `Makefile`, so `make -j`
is safe.

## What a build prints

Since 2026-10-20 (sq-quiet-build) a build prints one short line as each step
starts, not make's echo of every command. On the phone that echo was most of
the screen: each compile repeated the full flag list and two absolute paths,
each test link every source and archive, and each module make two "Entering
directory" lines and a "Nothing to be done". The same build now prints:

```
squared release: flags as before
  cc   gui/src/stage.cpp
  ar   libsquared_gui.a
  bin  gui/ui_keys_test
  test gui/ui_keys_test
       ok
  dev: 3 unchanged since they passed
```

How, all in `build/common.mk`, with no filter and no pipe:

- `MAKEFLAGS += -s` unless `V=1`. Every module make inherits it: no command
  echo, no "Entering directory", no "Nothing to be done". Errors (`make:
  *** ... Error 1`) still print.
- `$(call say,<step>,<what>)` before each compile, archive and test link: one
  `printf` as the step starts, so no step is silent however long it takes.
  The path is from `squared/` (`$(MODULE)/$*.cpp`). Every line names its
  module because `make -j` interleaves modules; a heading per module would be
  wrong as soon as two build at once.
- The flags, said once (below).
- `run_tests` prints the test's name, runs it with its output in
  `<binary>.log`, then `ok`, or `FAIL (exit N)` / `FAIL (signal N)` and the
  whole log. Under `V=1` the output prints as the test runs.

Why not a filter script (`make 2>&1 | tools/quiet`): a pipe hides make's exit
status unless every caller sets `pipefail`, the compiler stops colouring
diagnostics when its output is not a terminal, and a filter that buffers
makes a long compile look hung. Printing from make costs one `printf` per step
(well under a millisecond against a compile of a second or more on the
phone). Measured on this host, once each: a clean `make -j8` took 35 s
before and 33.5 s after (the same, within noise), and a clean build plus
`make test` printed 7 KB where the build alone had printed 62 KB and the
tests 35 KB more.

Compiler diagnostics still carry absolute paths: the rules pass absolute
paths to the compiler, and the paths in `.d` files depend on that.

With the echo gone a warning stands out, so the three g++ printed and clang
does not were fixed in the same change: a `-Wsign-conversion` in
`console/src/file_commands.cpp` (a `? 1 : 0` added to a `std::size_t`, now an
`if` and `++`), and two `-Wunused-result` in tests, where g++ ignores a cast
to `void` and a `[[maybe_unused]]` named value now holds the result. g++ and
clang both build the tree and its tests without a warning.

**The flags stamp.** `build/<BUILD>/flags` holds the compiler and its flags
(`$(CXX) $(SQUARED_CXXFLAGS) $(SQUARED_EXTERNAL_INCLUDES)`); every object and
test binary depends on it, and it is rewritten only when they change. Before,
make compared timestamps only, so `make SQUARED_ABI=-fno-rtti` after a normal
build did nothing. The first make to read `common.mk` (the root `Makefile`
now includes it, or a module's when built alone) compares the stamp and says
`squared release: flags as before`, or lists every flag one to a line with
`(new)` and `(gone)` marks; it exports `SQUARED_FLAGS_SAID` so the module
makes say nothing more. Only a make whose goal builds or tests (`all`,
`test`, `retest`, a module) reads or writes the stamp: `tools/lint.sh` asks
the Makefiles for variables with other flags, and must not rewrite it. For
the screen only, an option keeps its path on its line (`-idirafter <dir>`)
and `$PREFIX` and `~` shorten paths.

`CXXFLAGS_EXTRA` is no longer appended to `CXXFLAGS`: compiles use
`SQUARED_CXXFLAGS = $(CXXFLAGS) $(CXXFLAGS_EXTRA)`. With the root Makefile
reading `common.mk`, a `CXXFLAGS` from the environment would otherwise reach
the module makes with the extra flags already in it and get them twice.

Tests and tools build with `TESTFLAGS = -std=c++20 -O1 -g $(SQUARED_ABI)`. The
`$(SQUARED_ABI)` is new: before, `make SQUARED_ABI=-fno-rtti test` failed to
link `gui`'s tests, built with RTTI against archives without it (`undefined
reference to typeinfo for sq::gui::Widget`).

Checked on this host, g++ and clang: clean build, `make test`, nothing
changed (two lines per make), `CXX=clang++ SQUARED_ABI=-fno-rtti` (listed as
`clang++ (new)`, `-fno-rtti (new)`, `g++ (gone)`; all 76 compiles and every
test rebuilt; all 22 tests pass), a planted compile error (the `cc` line, then
the compiler's message in full, then make's error), a planted failing test
(`FAIL (exit 3)`, its log, make's error), `V=1`, and the lint's variable
reads (no line printed, the stamp untouched).

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

## Android: target, sysroot and runtime

The Android template compiles the application and the vendored framework with
the host's clang (Termux's) against the **NDK's sysroot**, for the API level in
`SQ_MIN_SDK` (default 22), and links the **NDK's libc++ statically** (since
Sprint 3, task 3.3, 2026-10-16). The flags live in the template's
`mk/squared_generated.mk`; `mk/pkg_squared_core.mk` passes them to this tree's
build as `CXX` and `CXXFLAGS_EXTRA`.

```
--target=aarch64-linux-android$(SQ_MIN_SDK) --sysroot=<ndk>/sysroot     compile and link
-stdlib++-isystem <ndk>/sysroot/usr/include/c++/v1                      C++ compiles
-L<ndk>/sysroot/usr/lib/aarch64-linux-android/$(SQ_MIN_SDK)              link
-nostdlib++ --unwindlib=none -Wl,--hash-style=both -Wl,--gc-sections     link
libc++_static.a libc++abi.a <ndk clang>/libunwind.a                     link, last
```

**What it replaced, and what was wrong with it.** Termux's clang targets
`aarch64-unknown-linux-android24` and its own sysroot by default, and the build
never told it otherwise: `SQ_MIN_SDK` reached only the manifest. The library
was compiled against API 24's headers, linked against Termux's copies of the
system libraries, carried only `DT_GNU_HASH`, and needed `__register_atfork`
(API 23). Android 5.1 refuses such a library on all three counts. The probe
that found this is `probe_api22.sh` (3.3, not kept in the tree).

**Why this shape.**

- `--sysroot` replaces Termux's headers and libraries wholesale. Putting the
  NDK's headers on `-I` was tried long ago and broke `<cctype>` (two C
  libraries ahead of libc++'s wrappers); a sysroot is one coherent set. The
  `-idirafter` that `mk/pkg_squared_core.mk` still adds for the GL headers is
  now redundant and harmless.
- `-stdlib++-isystem` selects the NDK's libc++ headers, the ones
  `libc++_static.a` was built from, and affects only C++ compiles; Termux's
  clang would otherwise find Termux's own libc++ beside itself. In NDK r29 both
  are libc++ 21, but their `__config_site` need not agree.
- **Static runtime.** An application is one `.so`, the case the NDK documents
  the static runtime for. Only the libc++ code the program reaches is linked
  (with `--gc-sections`, which also drops this tree's unreferenced functions);
  there is one library fewer to map and relocate at start; and the class of
  failure where the bundled `libc++_shared.so` lacked a symbol the headers
  called (`std::__hash_memory`, 2026-10-13) cannot happen. Priority 1 governed:
  code only for what is used, no second library's relocated data per process.
  The trade: a second native library in one APK would each carry their own
  copy, and `operator new` replacement must stay in the one library (it does).
- **Named by path.** Termux's clang always asks for `-lc++_shared`
  (`-nostdlib++` stops it) and finds its unwinder beside itself, which the
  sysroot hides; `--unwindlib=none` plus the NDK clang's own `libunwind.a`
  replaces it. The same reasoning as the GLES libraries: a search order cannot
  go wrong when there is no search.
- `--hash-style=both`: `DT_HASH` for the loader before Android 6.0,
  `DT_GNU_HASH` for faster lookup after. A few hundred bytes.
- **The API level is enforced by the link.** That level's stubs define nothing
  newer, and the template links with `-Wl,--no-undefined`, so a newer call
  fails the build. Newer functions are reached with `dlsym` and recorded by the
  feature cache (3.4).

**Checked on the phone (2026-10-16, NDK r29, clang 21.1.8).** Every framework
and demo source (83 files) compiles for API 22 with the NDK's headers; a trial
library linked this way carries both hash tables, needs only `liblog`,
`libdl`, `libm` and `libc`, and references one symbol newer than API 22,
`memfd_create`, weakly (from the runtime; resolved as null on older releases).
The demo APK already carries a v1 signature, which Android 5.1 needs.

**Flags stamp.** The project's `squared-core` target writes the flags it
builds with to `build/release/.project-flags` and cleans the tree when they
change. It predates this tree's own stamp (above), which now rebuilds every
object when the compiler or a flag changes; the project's clean is kept
because its stamp also holds the backend and platform, which change which
files are compiled, not how. It exists so
archives for one level or one libc++ never link into a library for another.
Since 2026-10-19 the Console's sizes are in it whatever `CONSOLE` is, and
`SQUARED_CONSOLE` itself never is (no framework code reads it): switching
`CONSOLE` on or off left the stamp different and cleaned the whole tree,
though only the Console module reads a size and its archive is built in every
build anyway. Now only a change of size does.

## Building only what changed

Three things made a package round on the phone rebuild everything, and each
is fixed (2026-10-19):

1. **Vendoring replaced the example's `squared/` whole,** build output
   included, so the next `make` compiled the framework from nothing and every
   application file that includes it. `vendor_framework.sh` (examples repo)
   now writes only the files that differ from the commit, removes the ones the
   commit dropped, leaves the rest untouched (timestamps included), and keeps
   `build/release` and `build/debug`. A written file gets the current time,
   not the commit's: `git archive` dates files at their commit, which can be
   older than objects built from the previous version, and make would keep
   those. Checked on clones: a one-line change to `console/src/args.cpp`
   vendored as "1 changed, 404 unchanged", and the next `make` compiled one
   file where it had compiled 74.
2. **Switching `CONSOLE` cleaned the framework** (the stamp, above).
3. **Every test ran on every `make test`.** Each module's `test` target is now
   `$(call run_tests,$(TESTS))` (`build/common.mk`): a test runs when its
   binary is newer than its `<binary>.passed` mark, which only a pass writes.
   Every test binary depends on the headers its sources include (its `.d`,
   since 7b.2; until then every header in the tree, `SQUARED_HEADERS`), its
   sources and the archives it links, so an unchanged binary means unchanged
   inputs; the tests read nothing that
   varies between runs except the host's `/proc`, whose answers they check
   for shape. This reverses graphics' earlier rule ("every test runs on
   every `make test`"), which was right while tests did not depend on their
   headers: then an unchanged binary could hide a changed header. Recipes that
   passed `$^` to the compiler now pass `$(SQUARED_TEST_INPUTS)`, the sources,
   objects and archives among the prerequisites (7b.2; it was `$(filter-out
   %.hpp $(SQUARED_FLAGS_STAMP),$^)`, which the headers a `.d` adds, `.h`
   and `.inc` among them, would have passed to the compiler). Checked in
   2026-10-19:
   nothing changed, all 18 kept in under a second; a console `.cpp` touched,
   only console's 4 ran; a header touched, all ran; a failing test left no
   mark and ran again.

## Build speed (7b.2)

Measured on the phone first (2026-10-27, `measure_build.sh`, the 7b.1
package): 8 cores, clang 21.1.8, libc++ 21 on both sides, NDK 29.

| What | Time |
|---|---|
| the framework, clean, `-j8` | 124.6 s, 95 files |
| `make test`, every test built | 757 s, 39 binaries |
| `make test` after `stage/save.hpp`, which only stage includes | 766 s: 38 of 39 rebuilt |
| one framework file, whole / front end | 3.0 to 8.8 s / 2.7 to 5.5 s |
| the 47 standard headers squared includes, parsed alone | 4.3 s |
| the same files with those precompiled, whole / front end | 0.9 to 5.8 s / 0.6 to 2.7 s |

So two changes, decided with the user (plan, 7b):

**A test depends on what it includes.** Tests are built through
`tools/test_cc.sh` (`TEST_CXX`, `build/common.mk`), which takes the
compile-and-link a recipe would have run, compiles each source on its own
with `-MMD` (one command with several sources keeps only the last one's
dependencies), links the objects, and writes `<binary>.d`; each module
includes its tests' `.d` files. After `stage/save.hpp`: the two stage tests,
where it was 38. The objects are kept in `build/<BUILD>/test-objects/<key>/`,
the key a checksum of the compiler, the flags and the precompiled header, and
shared between tests: the tests had compiled the same sources 230 times for
39 binaries (`dev_log.cpp` into 13 of them, each Console source into 6). An
object is used again when nothing its `.d` names, nor the precompiled header,
is newer; each is written beside its name and renamed into place, so two tests
building at once under `-j` cannot leave half of one. Here (x86, 2 cores):
the first `make test` 214 s to 150 s.

**The standard headers are precompiled.** `build/std_pch.hpp` names every
standard header the sources include (written by `tools/std_pch.sh`; `make
headers` runs its `--check`, which fails when a source includes one the list
lacks). It is compiled once per flag set, `-MD` so a new libc++ makes it
again: `build/<BUILD>/pch/std.pch` with `SQUARED_CXXFLAGS` for the objects
(the root `Makefile` makes it before the modules, which build at once under
`-j`), `std-test.pch` with `TESTFLAGS` and `-pthread` for the tests (clang
refuses a precompiled header made with the other thread choice; every test
source is compiled with `-pthread` for it). Every object and test source is
compiled with `-include-pch`. 22 MB each on the phone. clang only; `make
SQUARED_PCH=off` turns it off, and g++ builds as before. The flags stamp now
names the tests' flags, the choice and the compiler's version, so a new
clang makes everything again (a precompiled header from another version is
refused).

What the precompiled header costs: every source sees all 47 headers, so a
`.cpp` that forgets `#include <vector>` still builds here. `make lint`
(clang-tidy, which compiles without it) and `make headers` (each header
alone, without it) still catch that. A pattern, in effect: a Flyweight of the
standard library's parse, shared by every file of one configuration.

`make headers` runs one compile per core by default (`nproc`; `JOBS=1` for
one at a time). The printed steps of a package no longer run `make
docs-check` after `make docs`, which has just written the pages it checks.

Not done: `import std`. Termux's libc++ ships no `std.cppm`; the NDK's does
(`toolchains/llvm/prebuilt/linux-arm64/share/libc++/v1/`), and it failed to
build against the NDK's headers as tried (`std/cctype.inc:11`). The
precompiled header takes the same parse off today; squared's own headers as
modules come after this (7b).

**The link check** (template `Makefile`, `verify-link`) refuses a library that
needs `libc++_shared.so` (the runtime was not linked in) and, when
`SQ_MIN_SDK` is below 23, one without `DT_HASH`.

## Android: debuggable builds and adb

Both live in the template's generated `mk/squared_android_package.mk` and its
`tools/device.sh`, not in this tree; they are recorded here because they
decide how every squared app is installed and read during development
(Sprint 4, tasks 4.8 and 4.8b, 2026-10-19).

**Debuggable.** `SQ_DEBUGGABLE` defaults to on when `DEV_LOG` is set or
`CONSOLE` is not `0`, and adds `--debug-mode` to `aapt2 link`, which writes
`android:debuggable="true"` into the linked manifest. The project's manifest
is never edited, so a release cannot inherit it. The link rule reads the
manifest back (`aapt2 dump xmltree`) and fails the build when the attribute
is missing; an `aapt2` that cannot print the manifest only says so. The
setting is stamped (`build/apk/.link-flags`), so switching it relinks the APK
and recompiles nothing. Why: `run-as`, and later a native debugger or
simpleperf, reach only debuggable apps. The cost is in the Java runtime,
which keeps its debugging support on for such a process; this app's code is
the same machine code. Memory is measured on builds that are not debuggable.

**Which device.** `tools/device.sh` decides, so every target (`install`,
`run`, `logcat`, `uninstall`, `devices`) agrees:

1. `DEVICE=<serial>`, when set, and it must be attached; otherwise an error
   listing what is.
2. Otherwise **this phone**: the attached device whose
   `ro.build.fingerprint` equals the one Termux reads with `getprop`. A
   Wireless Debugging connection shows up under several serials
   (`localhost:<port>`, the Wi-Fi address, an mDNS name), so the serial is no
   test; the fingerprint is the same for all of them and differs on the
   tablet. Two phones of one model on one build would look alike; accepted.
3. Only when adb's server is already running. Asking `adb devices` would
   start one, which wakes whatever is plugged in; a bash `/dev/tcp`
   connection to the server's port (`ANDROID_ADB_SERVER_PORT`, 5037) asks
   without starting anything. `connect` and a named `DEVICE` start it,
   because they were asked to.
4. Off Android (no `getprop`), the one attached device, as plain
   `adb install` would.

With no device, `install` falls back to the package installer (`termux-open`),
exactly as before 4.8b; `run`, `logcat` and `run-as` refuse with the steps to
connect. `run` starts the app with the launcher's own intent (`monkey -p
<package> -c LAUNCHER 1`), so it needs no activity name and starts whichever
activity a monitor build makes the launcher. `logcat` filters to the app's tag
plus `AndroidRuntime` (a library that failed to load is a Java exception
there), `DEBUG` (the crash summary) and `libc` (an abort's message): Termux's
own `logcat` sees only Termux's lines, so the old `make logcat` showed
nothing.

**Checked** (here, 2026-10-19): shellcheck clean; 29 scenarios against a fake
`adb`, `getprop` and `termux-open` (no adb, server down, this phone among two,
only the tablet, a named device, a named device that is absent, desktop with
one and with two devices, run, logcat, run-as, connect by mDNS and by port,
uninstall); two planted bugs (any device taken as this phone; the server not
checked) each failed several. The link rule against a fake `aapt2`: the
attribute found, missing (build fails, APK removed), unprintable (noted,
build goes on); toggling relinks only. Not checkable here: the real `aapt2`
and `adb`.

## `SQUARED_ABI`

`SQUARED_ABI` carries the ABI-shrinking flags.

`-fno-rtti` builds clean for this tree and is verified, tests included
(since 2026-10-20 they build with `SQUARED_ABI` too; clang, all 22 pass):

```sh
make SQUARED_ABI=-fno-rtti test
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

Both run through `make lint` ([lint.md](lint.md)). The tree was formatted in
one commit with clang-format 21 (`244cc13`, listed in the repo's
`.git-blame-ignore-revs`); the rules are in [code-style.md](code-style.md).
`tools/format.sh` reformats. clang-tidy analyses what this build compiles, with
the flags above minus the warnings, read from the Makefiles, so a flag changed
here changes what it analyses too.

## Checking the headers

```sh
./tools/check_headers.sh
```

Two checks, which together protect the layout ([file-layout.md](file-layout.md)):

1. **Reachability.** Every header under `src/` must be reached through the
   includes of a header at a module root (not counting the module's
   aggregate). Instant.
2. **Self-containment.** Every public header is compiled alone with
   `-Wall -Wextra -Wpedantic`.

Exit status is 0 when both pass, 1 when either fails, and 2 for a usage or
environment error. `make headers` runs it; run it in CI.

Each header is a full compile, so the second check takes minutes on a phone. Two
things make that bearable:

```sh
./tools/check_headers.sh               # one compile per core (nproc)
JOBS=1 ./tools/check_headers.sh        # one at a time
./tools/check_headers.sh gui           # one module
```

It prints a dot per header as it goes. That is not decoration: a check this
slow that prints nothing is indistinguishable from a hung one, and it will get
killed halfway by someone who reasonably assumes it wedged.
