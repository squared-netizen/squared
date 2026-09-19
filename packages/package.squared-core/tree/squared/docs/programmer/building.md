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
make clean
```

Archives land in `build/<release|debug>/lib/libsquared_<module>.a`.

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
