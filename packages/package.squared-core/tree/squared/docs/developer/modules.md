# Modules

How squared is built as C++20 named modules (7b.4): what the build makes,
in what order, what every importer must share with it, and what it cost.

Programmer counterpart: [../programmer/include-layout.md](../programmer/include-layout.md).
The layout of a module's files: [file-layout.md](file-layout.md).

## What the build makes

Every interface unit (`module.cppm`, and a partition per type) is compiled
once, by `build/modules.mk`, into two files:

- its object, in the module's archive (`libsquared_<module>.a`): what an
  importer calls (member functions defined in a class are not implicitly
  inline in a module, so they are emitted here, once), and the module's
  initializer, which every importer calls;
- its BMI, `build/<BUILD>/modules/sq.<module>[-<partition>].pcm`, the
  compiled interface an importer reads.

One compile does both: `-c <unit>.cppm -fmodule-output=<bmi> -o <object>`.
Every compile looks for what it imports in that one directory
(`-fprebuilt-module-path`, `SQUARED_MODULE_USE`). Implementation units
(`src/*.cpp`, `module sq.<module>;`) compile as other objects did, through
the build cache ([build-and-toolchain.md](build-and-toolchain.md#the-build-cache-7b3)).

## The order

clang needs a BMI before anything imports it, and refuses an importer built
against an older one. Three levels of order:

1. **sq.std first**, from the root Makefile: every unit imports it.
2. **Modules in the top Makefile's order** (`gui: graphics math data dev files
   messaging ...`). `make headers` (`tools/check_modules.sh`) checks that
   every module a module's files import is built before it: under `make -j`
   a missing line would race the BMI, and fail only sometimes.
3. **Interfaces inside a module from their import lines**:
   `tools/module_deps.sh` reads `import :widget;` and `import sq.graphics;`
   from each unit and writes `build/<BUILD>/modules/<module>.mk`, which
   `modules.mk` includes. It also refuses a unit whose declared module is not
   the one its path names.

A module's objects and tests depend on the BMIs they may import: sq.std's,
their own primary's, and the primaries of the modules they use
(`USES_MODULES`, found from the import lines of every file in the module,
tests included). A primary depends on its partitions, so a change to any
interface reaches every importer, and nothing else does.

The module list and what each module uses are found, not written:
`INTERFACES` is every `.cppm` in the module (not `test/`), `USES_MODULES`
every `import sq.<other>;` in its files.

## sq.std

squared's own module of the standard library, `build/std.cppm`, written by
`tools/std_module.sh`:

```cpp
module;
#include <algorithm>
// ... the 47 standard headers squared uses
export module sq.std;
export namespace std
{
using std::vector;
// ... every name squared writes as std::<name>, 161 of them
using std::operator==;   // and the operators argument-dependent lookup needs
}
```

- **Why its own.** `import std` is the standard way, and neither toolchain
  has it: Termux's libc++ ships no `std.cppm`, and the NDK's fails to build
  (`std/cctype.inc:11`: bionic declares the `<cctype>` functions with internal
  linkage, and a module cannot export those).
- **Why not the precompiled header.** Until 7b.4 the standard headers were
  precompiled (7b.2). A precompiled header and modules do not mix: a BMI made
  with one names it by path and loads it at every import; std from a
  precompiled header on one side and parsed on the other is "different
  definitions in different modules"; and clang 18 crashed (ASTReader,
  `loadDeclUpdateRecords`) importing `sq.time` when its partitions were made
  with the precompiled header. `sq.std` does the same work, measured on the
  phone (2026-10-27, clang 21, every framework file 8 at a time): 35.1 s with
  the precompiled header, 33.3 s with sq.std; the NDK's 35.9 s and 31.8 s.
- **What it exports.** The names squared's sources write as `std::<name>`
  (and through `namespace fs = std::filesystem;`), in `std`, `std::chrono`,
  `std::filesystem`, `std::this_thread` and `std::ranges`; the operators found
  by argument-dependent lookup, which an importer finds only when exported
  (comparisons, `+`, `-`, `<<`, `>>`, the bitwise ones for `std::ios`'
  flags, chrono's arithmetic, `std::filesystem::begin` and `end` for a
  range-for over a `directory_iterator`); chrono's literals; and libstdc++'s
  iterator operators, which live in `__gnu_cxx`.
- **What it does not.** Macros (a module carries none): a unit that uses
  `assert`, `errno`, `va_start`, `UINT32_MAX`, `SIGSEGV`, `offsetof` or `stdout`
  includes the C header (`<assert.h>`, ...) in its global module fragment.
  The `<cctype>` functions (internal on bionic): `<ctype.h>`, unqualified.
- **Kept current.** `make headers` runs `tools/std_module.sh --check`; a new
  `std::` name fails it until the file is written again. A name from a header
  not in the list fails the module's own build: add the header.
- **Its object** is in `libsquared_std.a`, which every program that imports
  a squared module links (the module's initializer).

A textual C++ standard header beside `import sq.std;` mostly works (the
duplicate declarations merge), and an app may do it for a name sq.std lacks.
squared's own files do not: libstdc++'s `<cmath>` beside the module gave
"type alias template redefinition" (`__promote_t`) here.

## What an importer must share

clang refuses a BMI made with other language options: "configuration
mismatch". Measured: `-O`, `-g`, `-fPIC` and `-D` may differ; the C++
standard, threads, RTTI and exceptions may not. So:

- `STANDARD := -std=c++20 -pthread` (`build/common.mk`): the framework,
  tests and tools; `-pthread` changes nothing on Android but the check.
- A project's code gets `-pthread -fprebuilt-module-path=...` from
  `mk/pkg_squared_core.mk`, and must keep `CXXSTD` at c++20.
- `SQUARED_ABI=-fno-rtti` applies to everything built here, tests included.

Tests are compiled with `TESTFLAGS` (`-O1 -g`, no `NDEBUG`) against BMIs made
with the framework's flags: an `assert` in a function defined in an
interface follows the framework's `NDEBUG`, as the archives always did. A
test links its module's archive, the archives of the modules it uses,
`libsquared_app.a` and `libsquared_std.a`, in one link group
(`SQUARED_TEST_LINK`).

## Mangling, and the global module

A name attached to a named module is mangled with it (`sq::App@sq.stage`).
What code outside the modules declares or defines must be attached to the
global module instead (`extern "C++"`), or the link names two functions:
[file-layout.md](file-layout.md#what-a-partition-may-declare) lists the six
places.

## A partition per type

Measured here (2026-10-27, x86, 8 jobs, clang 18), the framework clean:

| Layout | Clean build | CPU | First `make test` |
|---|---|---|---|
| headers, precompiled (7b.2, 7b.4's first step) | 34.8 s | 53 s | 84 s |
| a partition per type (chosen) | 43.5 s | 64 s | 80 s |
| one interface unit per module | 32.5 s | 49 s | 76 s |

A partition costs a compile of its own (0.1 to 0.3 s here, a process start
and sq.std's import each time; 233 units), and the partitions of a module
build in their import order. An implementation unit is quicker than it was
(`gui/src/label.cpp` 0.48 s to 0.26 s), so the cost is the interfaces'. The
user chose the partitions (2026-10-27): each type stays a unit of its own,
compiled alone, which clangd and per-file tidy want.

## The build cache

Implementation units are cached as other objects were: their key adds the
sources of every interface whose BMI they may import (`SQUARED_CACHE_INPUTS`,
the module's `.inputs` list), because clang's dependency file names no BMI.
Interface units are compiled in each workspace: a fresh workspace takes every
implementation object from the cache and compiles the 233 interfaces (20 s
here, where the headers' build took 6 s). A BMI holds its workspace's paths;
copied into another workspace it still compiled here (clang 18), with others
made elsewhere beside it, so caching the interfaces too is the next step,
once tried with clang 21 on the phone.

## The tools

- **lint** (`tools/lint.sh`): `.cppm` files are checked by clang-format and
  the rules; clang-tidy runs on every unit, interfaces included, and reads the
  BMIs, so it builds first (`make`), and for the Android sources a build of
  its own in `build/lint-android/`.
- **docs** (`tools/docs.sh`): clang-doc runs on each public interface (the
  partitions at a module's root), reading the BMIs: build first. gles is built
  for it when the GLES headers are there.
- **format** (`tools/format.sh`): `.cppm` too. `build/std.cppm` is written in
  the generator's order (`// clang-format off`).
- **make headers** (`tools/check_modules.sh`): sq.std current, the build
  order, every unit named for its path. The self-containment check of every
  header went with the headers (the user's call, 2026-10-27): an interface
  compiles alone by construction.
- **sqgen** writes `import sq.gui;`, `import sq.messaging;`, `import sq.std;`.
  A project builds sqgen and sqconsole for its own machine from the framework
  built for it (`make BUILD=host CXX=<host compiler> sqgen`, into
  `squared/build/host/`).

## GCC

Cut (decided 2026-10-26: "no dual builds"). clang only: `CXX` defaults to
clang++, and another compiler stops the build at once. On Termux, `g++` and
`c++` are clang already.

## Known

- libstdc++'s deprecation warnings (`std::unary_function`, `binary_function`)
  show through the module in importers on a desktop with libstdc++ (30 in a
  clean build here); libc++ on the phone gives none.
- A clang that changes version rebuilds everything (the flags stamp names
  it): a BMI from another version is refused.
