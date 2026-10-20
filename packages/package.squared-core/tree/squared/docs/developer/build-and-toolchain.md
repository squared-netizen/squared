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

**Flags stamp.** This tree's build does not track flags. The project's
`squared-core` target writes the flags it builds with to
`build/release/.project-flags` and cleans the tree when they change, so
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
   Every test binary depends on all of the tree's headers
   (`SQUARED_HEADERS`), its sources and the archives it links, so an
   unchanged binary means unchanged inputs; the tests read nothing that
   varies between runs except the host's `/proc`, whose answers they check
   for shape. This reverses graphics' earlier rule ("every test runs on
   every `make test`"), which was right while tests did not depend on their
   headers: then an unchanged binary could hide a changed header. Recipes that
   passed `$^` to the compiler now pass `$(filter-out %.hpp,$^)`. Checked:
   nothing changed, all 18 kept in under a second; a console `.cpp` touched,
   only console's 4 ran; a header touched, all ran; a failing test left no
   mark and ran again.

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
JOBS=4 ./tools/check_headers.sh        # compile in parallel
./tools/check_headers.sh gui           # one module
```

It prints a dot per header as it goes. That is not decoration: a check this
slow that prints nothing is indistinguishable from a hung one, and it will get
killed halfway by someone who reasonably assumes it wedged.
