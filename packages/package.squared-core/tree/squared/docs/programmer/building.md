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

**Tests run only when something they cover changed.** A test binary is
rebuilt when its sources, the archives it links or any header in the tree
change, and `make test` runs it only when it was rebuilt since it last
passed. Each module says how many ran: `console: 4 test(s) ran and passed, 0
unchanged since they last passed`. With nothing changed `make test` takes
about a second; `make retest` (or `make test RETEST=1`) runs all of them.
A failed test is run again next time, changed or not.

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

`SQUARED_ABI` is reserved for `-fno-exceptions -fno-rtti`. It does not build
today; see [../developer/build-and-toolchain.md](
../developer/build-and-toolchain.md).
