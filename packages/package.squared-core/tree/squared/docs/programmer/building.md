# Building

squared builds with g++ and GNU make. No CMake, no generator step, no network
access, and nothing that assumes a desktop.

## Desktop Linux and Termux

```sh
./tools/build.sh              # release
./tools/build.sh debug        # -O0 -g
./tools/build.sh release -j4  # extra arguments go to make
```

Or drive make directly:

```sh
make                 # every module
make gui             # one module
make BUILD=debug
make headers         # compile every public header on its own
make test            # every module's tests, each only if it changed (below)
make retest          # every test again
make clean
```

Archives land in `build/<release|debug>/lib/libsquared_<module>.a`.

**What a build prints.** One short line as each step starts, and the flags
once:

```
squared release: flags as before
  cc   messaging/src/telegram.cpp
  ar   libsquared_messaging.a
  bin  messaging/telegram_test
  test messaging/telegram_test
       ok
  gui: 4 unchanged since they passed
```

`cc` compiles one file (its path from `squared/`), `ar` makes a module's
archive, `bin` builds a test or tool, and `test` runs one: `ok` or `FAIL` is
the line after. The first line says the flags are what they were; when they
are not (another `CXX`, `SQUARED_ABI`, `BUILD`'s optimisation, or a first
build) it lists every flag, one to a line, marking each `(new)` or `(gone)`,
and everything they affect is rebuilt. Warnings and errors print in full, as
the compiler wrote them. A failed test prints its whole output after `FAIL`;
a passing test's output is in `build/<release|debug>/<test>.log`.

```sh
make V=1             # every command, and every test's output, in full
```

**Tests run only when something they cover changed.** A test binary is
rebuilt when its sources, the archives it links, any header in the tree or
the flags change, and `make test` runs it only when it was rebuilt since it
last passed. A module whose tests were all kept says how many: `gui: 4
unchanged since they passed`. With nothing changed `make test` takes about a
second; `make retest` (or `make test RETEST=1`) runs all of them. A failed
test is run again next time, changed or not.

## Modules outside this tree

Every squared module is in this tree, and so is the one third-party dependency.
Unpack it once after checkout:

```sh
./tools/vendor_third_party.sh
```

That reads `third_party/yyjson-*.tar.gz` and puts `yyjson.c` and `yyjson.h` in
`third_party/yyjson/`. It is idempotent, and it leaves an already-unpacked
dependency alone, so it is safe to re-run and safe to delete the tarball
afterwards.

`data/Makefile` compiles `yyjson.c` into `libsquared_data.a`, so an application
links one library rather than two, and `local.mk` is not needed for it.

**Why vendored rather than a system package.** This tree is built with the NDK
toolchain for an APK. `pkg install yyjson` under Termux gives you a header and
a library built for the host sysroot &mdash; the wrong target. It may even link
and then fail on a device. The source is compiled by your build, for your
target, or not at all.

`local.mk` still exists for anything else you need on the include path, and is
still picked up automatically.

`build/common.mk` picks it up automatically. `local.mk` is per-checkout and
should not be committed. Setting the variable on the command line still works
for a one-off:

```sh
SQUARED_EXTERNAL_INCLUDES="-I../application/include" ./tools/build.sh
```

## For Android

A project made from the Android template builds squared for you, for the
project's target: `make` in the project compiles the vendored `squared/` with
the same compiler, API level and NDK sysroot as your own code. You do not build
the framework separately.

The API level is `SQ_MIN_SDK` in the project's `mk/squared_generated.mk`,
default **22** (Android 5.1). It is what the code is compiled and linked for,
not only what the manifest claims, so one APK runs on Android 5.1 and on every
release after it.

```sh
make apk                  # for API 22
make apk SQ_MIN_SDK=26    # for Android 8.0 and later only
make android-status       # the NDK, the API level, the C++ runtime it found
```

| What happens | Why |
|---|---|
| A call to a function newer than `SQ_MIN_SDK` fails the **link** | It is not in that level's system libraries; reach it with `dlsym` and do without when it is absent |
| The C++ runtime is linked **into** your library | One `.so`, nothing beside it to get out of step; the APK carries no `libc++_shared.so` |
| The link fails if the library could not load on Android before 6.0 | It must carry the `DT_HASH` symbol table; the build adds it |
| Changing `SQ_MIN_SDK` or the NDK rebuilds the framework | Archives built for one level must not link into another |
| `CONSOLE=app` (or `monitor`) links the Console in; the default, `0`, carries none of it | See [console.md](console.md). Switching it rebuilds nothing in the framework: its Console archive is built every time and only linked when asked for |
| `TELEGRAPH_RING=512` (or another `TELEGRAPH_*`) sizes the Telegraph; unset ones keep their defaults | The app and the framework are built with the same ones, since the app makes the line; changing one rebuilds both. See [messaging.md](messaging.md#the-telegraph) |
| `DEV_LOG` and `CONSOLE` builds are **debuggable**; plain builds never are | `run-as` reaches a debuggable app's private files; `SQ_DEBUGGABLE=` turns it off, `SQ_DEBUGGABLE=1` on |
| `make install` uses `adb install -r` when adb reaches **this phone**, and Android's installer otherwise | No prompt with adb; a second device (a tablet) only when named, `DEVICE=<serial>` |

The NDK must have libraries for the level you ask for; `make` says which levels
it has if not. NDK r29 has 21 to 35.

### Installing, starting, the log

```sh
make install      # adb install -r on this phone when adb reaches it, else the installer
make run          # install, then start it (adb)
make logcat       # the app's lines, a library that failed to load, the crash summary (adb)
make devices      # what adb reaches, which is this phone, and how to connect
make dev          # the build you debug with: CONSOLE=app DEV_LOG=1 install
make console      # a CONSOLE build's Console in this terminal, no adb (see console.md)
make console-only # start it with only the Console running
```

On the phone itself adb comes from Termux's `android-tools` and Wireless
Debugging (Android 11 and later, on Wi-Fi): pair once, then connect each time
it is turned on. The project's README walks through it. Without adb,
`make install` hands the APK to Android's installer, as before, and Termux
cannot read the app's log at all; the dev log (`DEV_LOG`, [dev.md](dev.md))
and the Console ([console.md](console.md)) work without either.

## Flags

`build/common.mk` holds every shared setting and each one is overridable:

```sh
make CXX=clang++ BUILD=debug CXXFLAGS_EXTRA=-fsanitize=address
```

Defaults are `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion`, plus `-ffunction-sections -fdata-sections
-fvisibility=hidden` so the linker can drop what an application does not use.
The tree builds warning-free at that level.

**The standard headers are precompiled** with clang (7b.2): the build makes
`build/<BUILD>/pch/std.pch` (and `std-test.pch` for the tests) once per
flag set, about 22 MB each, and compiles every source with it, which takes a
third to three quarters off each compile on the phone. `make SQUARED_PCH=off`
builds without it. A standard header your change starts to include, that
`build/std_pch.hpp` does not list, still works but is not precompiled: `make
headers` says so, and `tools/std_pch.sh` writes the list again.

**A test is rebuilt only when something it includes changes**, and the
objects of the sources tests share are kept in
`build/<BUILD>/test-objects/` and reused.

`SQUARED_ABI` carries `-fno-rtti`, which builds and passes every test
(`make SQUARED_ABI=-fno-rtti test`); `-fno-exceptions` does not build yet.
See [../developer/build-and-toolchain.md](
../developer/build-and-toolchain.md).
