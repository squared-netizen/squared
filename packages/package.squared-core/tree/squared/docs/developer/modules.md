# Modules

How squared is built as named modules (7b.4; C++23 and `import std;` since
2026-11-02): what the build makes, in what order, what every importer must
share with it, and what it cost.

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
(`-fprebuilt-module-path`, `SQUARED_MODULE_USE`), std's BMI included (a link
there, below). Implementation units (`src/*.cpp`, `module sq.<module>;`)
compile as other objects did, through the build cache
([build-and-toolchain.md](build-and-toolchain.md#the-build-cache-7b3)).

## The order

clang needs a BMI before anything imports it, and refuses an importer built
against an older one. Three levels of order:

1. **std first**, from the root Makefile: every unit imports it.
2. **Modules in the top Makefile's order** (`gui: graphics math data dev files
   messaging ...`; `term: app` since 12.3d, for the key names). `make headers` (`tools/check_modules.sh`) checks that
   every module a module's files import is built before it: under `make -j`
   a missing line would race the BMI, and fail only sometimes.
3. **Interfaces inside a module from their import lines**:
   `tools/module_deps.sh` reads `import :widget;` and `import sq.graphics;`
   from each unit and writes `build/<BUILD>/modules/<module>.mk`, which
   `modules.mk` includes. It also refuses a unit whose declared module is not
   the one its path names.

A module's objects and tests depend on the BMIs they may import: std's (its
`std.ready`), their own primary's, and the primaries of the modules they use
(`USES_MODULES`, found from the import lines of every file in the module,
tests included). A primary depends on its partitions, so a change to any
interface reaches every importer, and nothing else does.

The module list and what each module uses are found, not written:
`INTERFACES` is every `.cppm` in the module (not `test/`), `USES_MODULES`
every `import sq.<other>;` in its files.

## std

`import std;`, C++23's module of the whole standard library, libc++'s own:
its sources (`std.cppm`, `std/*.inc`) are pinned in `third_party/libcxx/`
(decided 2026-11-02: "pinned in third_party/", "libc++ only").

- **Pinned.** `tools/pin_libcxx_modules.sh <share/libc++/v1>` copies them
  from a libc++ installation, the NDK's on the phone
  (`toolchains/llvm/prebuilt/<host>/share/libc++/v1`, libc++ 21.0.0), and
  writes `PINNED`: where from, the libc++ version of the headers beside
  them, a sha256 for each file. They must match the headers squared compiles
  against: the std module names every standard type, and one the headers
  lack stops its build. The NDK's headers and Termux's are the same version
  (21.0.0, 2026-11-02), so one pin serves the Android build and the phone's
  own tools. `make headers` checks the sums; the build refuses headers of
  another major version than `PINNED` (pin again).
- **libc++ only.** A standard library that is not libc++ is refused when the
  std module is made: libstdc++ ships no std module before GCC 15. A desktop
  installs libc++ (Debian and Ubuntu: `libc++-dev libc++abi-dev`);
  `STANDARD` passes `-stdlib=libc++` (Android's and Termux's default
  already), except where the flags name libc++'s headers already
  (`-stdlib++-isystem`, an Android build): there clang warned it was
  unused, on every interface of the demo's build (2026-11-02).
- **Made once per compiler, flags and libc++, and shared.**
  `tools/std_bmi.sh` (run by `build/common.mk`'s std rule) makes it in
  `$(SQUARED_STD_HOME)/<key>/`: `~/sqsysroot/.sqpg/std/<key>/` when
  squared-pg is set up, else the build's `modules/std/<key>/`. The key is a
  hash of the compiler's version line, the flags (less `-ffile-prefix-map`
  and the other path maps, which name the workspace), `PINNED`, and the
  sizes and dates of the libc++ headers `<version>` reads (a libc++ updated
  in place makes another). The directory holds `src/` (the pinned sources,
  copied: a BMI names its sources' paths, and a project's own copy may go),
  `std.pcm`, `std.o`, `std.d` and `done`. A build then gets
  `modules/std.pcm`, a link to it; `modules/std.d` for make;
  `modules/std.inputs` (PINNED, for the build cache's key);
  `lib/libsquared_std.a` (its object: the module's initializer, which every
  program links); and `modules/std.ready`, which everything that imports std
  waits for: the link's own date is the shared BMI's, older than the build.
- **What it costs.** Here (x86, clang 21, libc++ 21.1.8): 3.3 s to compile (6.3 s with the checking compile below), a
  33 MB BMI, a 4 KB object. Once per key on a device instead of once per
  project build directory (the user's measure: 30 MB in every toy project's
  build with CMake's `import std`). A second workspace with the same
  compiler and flags makes nothing: `mod  std (libc++ ..., made before:
  ~/sqsysroot/.sqpg/std/<key>)`, its clean build 18.5 s against 50 s here,
  the objects from the cache too.
- **The copy is fitted to the headers, the pins never.** On the phone
  (2026-11-02) the sources pinned from the NDK r29 met Termux's headers,
  which come from the NDK r30 (Termux's packages), both "libc++ 210000", and
  the std module failed twice: libc++'s "headers not yet available" check
  (`#error "please update the header information for <flat_set>"`: r30's
  headers have `<flat_set>`, r29's sources do not export it), and `tmpfile`,
  which Termux's `<stdio.h>` patch defines `static inline` (a module cannot
  export a name with internal linkage; no macro turns the patch off). So
  `std_bmi.sh` fits its copy (`src/`), not `third_party/libcxx/`: the
  header-availability checks go (bookkeeping for libc++'s developers; the
  module just does not export that header's names), and a checking compile
  (`-fsyntax-only`) finds every `using` the compiler refuses for internal
  linkage, comments it out, and tries again, at most 3 rounds. What is left
  out is listed in `src/LEFT_OUT` and said once: `std  left out, static in
  this C library's headers: tmpfile`. squared does not use `std::tmpfile`.
  The checking compile doubles the std build (6.3 s here, once per key); the
  script's own hash is part of the key, so a change in how it fits makes a
  new BMI. Tried here with a planted copy (a check for `<vector>`, a static
  function exported): both handled, the BMI made.
- **Kept small.** `SQUARED_STD_KEEP` (6) keys, the most lately used (each use
  touches its directory); older ones are removed after a std build. A
  project whose key went finds its link dangling: `build/common.mk` runs the
  std rule again (`squared_std_gone`, and an empty rule for the shared
  source `std.d` names), which makes it.
- **Two builds at once** of the same key both compile and rename into place
  (`mv`): the same bytes, and nobody reads a half-written file; the sources
  are copied to `src.<pid>` and renamed.
- **Android's ctype.** bionic's `<ctype.h>` declares `isalpha` and the rest
  `static inline` unless `__BIONIC_CTYPE_INLINE` is defined, and a module
  cannot export a name with internal linkage (`std/cctype.inc`). So
  `STANDARD` has `-D__BIONIC_CTYPE_INLINE=inline` (from the user's CMake
  experiment), the std module's build and every importer alike; an app's
  code gets it from `mk/pkg_squared_core.mk`. On glibc it names nothing.

### What came before

- **sq.std** (7b.4 to 2026-11-02): squared's own module, `build/std.cppm`,
  written by `tools/std_module.sh`: the 47 headers squared used, and a
  `using std::<name>;` for each of the 161 names its sources wrote, the
  operators argument-dependent lookup needs, and libstdc++'s `__gnu_cxx`
  iterator operators. It existed because neither toolchain had `import std`
  working then: Termux's libc++ ships no `std.cppm`, and the NDK's failed on
  the ctype functions (the define above is the fix). `make headers` checked
  it named every `std::` name the sources used. Gone with the generator.
- **The precompiled header** (7b.2): a precompiled header and modules do not
  mix (a BMI made with one names it by path and loads it at every import;
  std from both sides is "different definitions in different modules";
  clang 18 crashed importing `sq.time` made with it). Measured on the phone
  (2026-10-27, clang 21, every framework file 8 at a time): 35.1 s with the
  precompiled header, 33.3 s with sq.std.

### Includes and imports

A module carries no macros: a unit that uses `assert`, `errno`, `va_start`,
`UINT32_MAX`, `SIGSEGV`, `offsetof` or `stdout` includes the C header
(`<assert.h>`, ...): an interface or implementation unit in its global
module fragment, any other file (a test, sqgen, sqconsole) above its
imports. **Every include before every import**: libc++'s C headers
(`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<math.h>`, `<ctype.h>`, ...) are
C++ wrappers that bring in libc++'s internals, and read after `import std;`
those clash with the module's ("type alias template redefinition",
`__promote_t`, from `<math.h>` after an import; 2026-11-02, 40 files
reordered: the tests, sqgen, sqconsole). A header of the project's own that
imports goes after the system headers too, for the same reason, and the
file's own imports come last. libc++ 21 has no `<stdint.h>` of its own (the
C library's is read), so `_LIBCPP_VERSION` is defined only through a C++
header; nothing in squared tests it any more.

## What an importer must share

clang refuses a BMI made with other language options: "configuration
mismatch". Measured: `-O`, `-g`, `-fPIC` and `-D` may differ; the C++
standard, threads, RTTI and exceptions may not. So:

- `STANDARD = -std=c++23 -stdlib=libc++ -pthread -D__BIONIC_CTYPE_INLINE=inline`
  (`build/common.mk`; `-stdlib=libc++` only without `-stdlib++-isystem`):
  the framework, tests and tools; `-pthread` changes
  nothing on Android but the check.
- A project's code gets `-pthread -D__BIONIC_CTYPE_INLINE=inline
  -fprebuilt-module-path=...` from `mk/pkg_squared_core.mk`, and must keep
  `CXXSTD` at c++23.
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
and std's import each time; 233 units), and the partitions of a module
build in their import order. An implementation unit is quicker than it was
(`gui/src/label.cpp` 0.48 s to 0.26 s), so the cost is the interfaces'. The
user chose the partitions (2026-10-27): each type stays a unit of its own,
compiled alone, which clangd and per-file tidy want. At alpha, when the
types settle, each module becomes one interface unit (the user's plan).

## The build cache

Every unit is cached, interfaces too since 2026-11-02 (their BMI with their
object: `objects/<k2>.pcm` beside `<k2>.o`, both or neither). A key adds the
sources of every interface whose BMI the unit may import
(`SQUARED_CACHE_INPUTS`, the module's `.inputs` list), because clang's
dependency file names only the BMIs; and std's `std.inputs`: `PINNED` and
`std.key`, the std BMI's own key, so a libc++ updated in place (a new std
BMI) takes nothing made against the old one. An interface waits for its
module's `.inputs` list: an interface of its own module that it imports is
part of its key.

**A BMI from another workspace.** A BMI names its sources by path, and an
importer opens them: a BMI copied into workspace B, with A moved away,
failed with "cannot open file '<A>/gui/src/detail/ui.cppm'" (clang 21). So
interfaces are compiled with `-Xclang -fmodules-embed-all-files`
(`SQUARED_EMBED`): the sources go into the BMI, 40 MB of BMIs became 41 MB,
and importers need no flag. The BMIs a BMI imports are found by name in the
importer's own module directory. The umbrella (`squared.cppm`, the top
`Makefile`) is cached the same way since the next change of the same day
(before, the one `mod squared` a fresh workspace still printed); std's BMI
is shared its own way (above).

**The `.sum` beside each `.inputs`.** A key read every file a unit's
`.inputs` named, 233 interface sources for a unit of gui or stage: about
90,000 files read for a build that compiled nothing. Now the rule that
writes `sq.<module>.inputs` first writes `sq.<module>.sum`, the md5 of each
file with its path under `@ROOT@` (`tools/inputs_sum.sh`; std's rule writes
`std.sum`), and a key reads that one file. It is as fresh as the list: both
are made when one of the files changes, and the list's objects are rebuilt
after. A list without a `.sum` (a build directory from before) is read file
by file. `k1` holds "inputs by .sum", so lists made the old way are never
taken.

**Few programs a lookup.** The rest of a lookup's cost was starting
programs, about 20 a unit (sed for every `@ROOT@` rewrite, cut, ls, tr).
`cached_cc.sh` now does that text work in bash itself; a hit starts md5sum
twice, stat, cp (twice for an interface), mv three times and touch: about 8.

Measured here (x86, 2 jobs, clang 21), the cache empty: workspace A 47 s
(219 interfaces); then a fresh workspace B 13.8 s, every unit from the cache
(18.5 s before, compiling the interfaces), and every test passed in B with A
moved away. `gui/src/size.cppm` edited in B: gui's 74 interfaces, stage's 6
and 44 objects compiled, nothing else; edited back: everything from the
cache.

A build of a fresh workspace that compiles nothing, on one job (2026-11-02,
x86): 44 s before the `.sum`s, 19.5 s with them, 12.9 s with few programs a
lookup (12.9 s: about 8 of it lookups, 3 make's own work). The phone pays
more for each program started, so it gains more: hello's `make dev` after
pkg75, with every unit from the cache, took 3 min 39 s. Checked again: 37
tests ok in the fresh workspace; `size.cppm` edited, 125 compiled (gui's
113, stage's 11, the umbrella), edited back, 0.

**Vendored C in a module's archive** (yyjson in data, stb_image in
graphics, FreeType in font) imports no module, so `build/modules.mk` leaves
it out of what waits for, and is rebuilt after, the module's BMIs, and its
cache key leaves the interfaces out (`THIRD_PARTY_OBJECTS`, 2026-11-03).
Before, every change to font's interfaces compiled FreeType's 16 objects
again, on the phone and in every workspace.

## The tools

- **lint** (`tools/lint.sh`): `.cppm` files are checked by clang-format and
  the rules; clang-tidy runs on every unit, interfaces included, and reads the
  BMIs, so it builds first (`make`), and for the Android sources a build of
  its own in `build/lint-android/`. clang-tidy must be the compiler's
  version: it reads clang's BMIs, std's among them.
- **docs** (`tools/docs.sh`): clang-doc runs on each public interface (the
  partitions at a module's root), as text: a copy where imports are
  `#include`s (`import std;` the standard headers the pinned `std.cppm`
  includes, read with libc++), because a BMI holds no comments and
  clang-doc 19 and later keep the first unit's view of a type
  ([tools.md](tools.md#docssh)). No build needed.
- **Stale objects from before 7b.4:** an object compiled with the
  precompiled header names `build/std_pch.hpp` in its dependency file, and
  clang 21 can list it first, with no `-MP` rule. `build/common.mk` has empty
  rules for it and the old `.pch` files, so make rebuilds those objects
  instead of stopping at "No rule to make target" (hello's first build after
  7b.4, 2026-11-02).
- **format** (`tools/format.sh`): `.cppm` too; `third_party/` is left as it is.
- **make headers** (`tools/check_modules.sh`): the pinned std sources as
  `PINNED` says, the build order, every unit named for its path. The
  self-containment check of every header went with the headers (the user's
  call, 2026-10-27): an interface compiles alone by construction.
- **sqgen** writes `import sq.gui;`, `import sq.messaging;`, `import std;`.
  A project builds sqgen and sqconsole for its own machine from the framework
  built for it (`make BUILD=host CXX=<host compiler> sqgen`, into
  `squared/build/host/`), with its own std key (Termux's libc++ headers).
- **The modules.mk "missing BMI" rule** covers squared's modules only
  (`sq.%.pcm`): matching std too, it made `std.pcm` a target of its own and
  stopped a build whenever `std.inputs` was older than its rule's inputs
  (found 2026-11-02, before shipping).

## GCC

Cut (decided 2026-10-26: "no dual builds"). clang only: `CXX` defaults to
clang++, and another compiler stops the build at once. On Termux, `g++` and
`c++` are clang already.

## Known

- A clang that changes version rebuilds everything (the flags stamp names
  it), and makes another std key: a BMI from another version is refused.
- When std's rule runs again (new flags, a new pin, a new `std_bmi.sh`), every
  interface compiles again, even when it finds the BMI made before: it waits
  for `std.ready`, which the rule touches. Objects come from the cache.
