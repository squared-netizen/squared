# Tools

How the framework's scripts in `squared/tools/` and the workspace wrappers fit together.

Programmer side: [programmer/tools.md](../programmer/tools.md).

## Workspace wrappers

A generated project gets `tools/<name>.sh` for each framework tool meant for everyday use. Every wrapper is the same file, and it does three things:

1. **Finds the workspace** from its own path, never from the working directory.
2. **Resolves its target:** `squared/tools/<own name>`.
3. **Runs the target** with `bash`, setting `SQ_WORKSPACE` to the workspace root.

It runs the target through `bash` instead of executing the file directly because the template manifest's `executable` list covers template files only, and the package manifest has no such list. So a vendored framework script may arrive without its executable bit.

Ownership: wrappers are listed under `generated` in `template.android.cpp/SQ-INF/manifest.json`, so the generator rewrites them on update. They only mirror framework tools and hold nothing of the user's. They're also listed under `executable`. Adding a wrapper means adding the file to the template tree and its path to both lists.

`SQ_WORKSPACE` is the one contract between wrapper and tool. A tool that has a workspace-wide mode switches to it when that variable is set. A tool without one ignores it.

## sqgen

`tools/sqgen/sqgen.cpp`, the one C++ program here: it compiles layout files
and `names.json` at build time. Not a script, and not wrapped: the app build
compiles it for the build machine from `mk/pkg_squared_core.mk` and runs it,
and the gui Makefile builds and tests it in the framework. How it works:
[layouts.md](layouts.md#sqgen).

## docs-archive.sh

Two modes:

| mode | trees archived | archive name |
|---|---|---|
| framework (default) | `squared/docs` | `squared-docs-<version>-<date>.zip` |
| workspace (`SQ_WORKSPACE` or `--workspace`) | `docs`, `squared/docs`, `sq_app/docs`, `sq_android/docs`, then any other `<top-level dir>/docs` except `build/` | `<SQ_PROJECT>-docs-<date>.zip` |

The version is looked up in order, and the first one found wins:
1. `squared/VERSION`, which doesn't exist today.
2. `SQ_FRAMEWORK_VERSION` from the workspace's `mk/squared_generated.mk`.
3. `"version"` from the package's `SQ-INF/manifest.json`, when run inside squared-pg.
4. Otherwise `unversioned`, never a guess.

A tree counts only if it holds a file once the exclusions apply. The exclusions are `.obsidian/`, `.trash/`, `.DS_Store`, `*.tmp`, `*.swp`, `*~` and `*.bak.*`. The same patterns go to `zip -x` and `find`, so `--list` shows exactly what the archive holds.

The archive is staged in `$TMPDIR`, then moved into place, so an interrupted run never leaves a half-written file at the destination. `-X` keeps the archive identical across hosts. The script stays Bash 3.2 compatible: arrays, no associative arrays, no `mapfile`.

Memory: none of this runs in the application, so priority 1 doesn't apply. The script holds one list of tree paths and streams everything else through `find` and `zip`.

The squared-pg repo keeps its own `tools/docs-archive.sh` for squared-pg's docs. The two are separate files on purpose. squared-pg's version reads `VERSION` from the squared-pg root, which a vendored framework doesn't have.

## docs.sh

Writes the API reference, `docs/reference/`, with clang-doc from the comments
in the public headers. `make docs` runs it; `make docs-check` runs it with
`--check`, which writes nothing and exits 1 when the committed pages are not
what the headers say now. `--missing` lists the public functions and types
with no comment (constructors, destructors and assignments are left out).

- **Committed, on purpose** (decided 2026-10-19): the reference reads on
  GitHub without building anything. So it is made again in the same commit as
  any change to a public interface's declarations or comments, and never
  edited.
- **Input:** since 7b.4, each public interface (the partitions at a module's
  root, what were the workhorse headers) is a translation unit of its own, so
  the reference is what a user imports. Until 7b.4: a generated unit per
  workhorse header, including just that header.
- **The interfaces as text** (2026-11-02): clang-doc reads a copy of the
  interfaces, `build/release/docs/view/`, not the modules. In the copy,
  `export module` is `#pragma once`, each import is an `#include` of the
  partition's or module's file (`import std;` includes the standard headers
  the pinned `third_party/libcxx/std.cppm` includes, read with
  `-stdlib=libc++`), and `export namespace`/`export extern` lose `export`. The rewrite
  keeps every line on its line, so "Defined at" points at the real file.
  Why: clang never writes comments into a named module's BMI (ASTWriter.cpp:
  "Don't write comments to BMI to reduce the size of BMI"; no option turns it
  on), and clang-doc 19 and later keep one set of declarations already seen
  for the whole run (Mapper.cpp, `USRVisited`). A type first reached by a
  unit that imports it was documented without its comment, and its own unit
  then skipped it. With the units reading BMIs, the phone's clang-doc 21 lost
  most comments (581 functions and types without one, 3754 lines gone from
  the pages, a different set each run); clang-doc 18 merges instead and lost
  nothing. Read as text: 159 without a comment with 21, 158 with 18, the same
  pages on every run. No build is needed, and gles is always documented. A
  line beginning `export` the rewrite does not know stops clang-doc with an
  error (`export` outside a module), so a new form is added to the rewrite
  when the code first uses it.
- **Doc comments sit right above their declaration.** clang-doc takes the
  nearest comment, `//` too, and merges comments only across a single line
  break. A `NOLINT` comment goes above the doc comment with a blank line
  between (`messaging/model_runner.cppm`'s constructor, which lost its
  description to a `NOLINTNEXTLINE` between them).
- **Tidying:** "Defined at" becomes a link to the header's line; `_Bool` reads
  `bool`, `basic_string` and `basic_string_view` read `std::string` and
  `std::string_view`; lists, and every page's enums and functions, are
  sorted (clang-doc merges in parallel, and their order changed from run to
  run: five runs in a row now match). Until 7b.4 a type's page kept its
  functions in declaration order, read from one unit; read as text, a type is
  in many units, so its page is sorted too (three runs, one at 8 threads,
  match). A link into the copy (`build/release/docs/view/`) fails the run;
  the pages of anonymous
  types and the global namespace are dropped, and a namespace page loses its
  description (clang-doc reads every comment, `//` too, and gave `sq` the one
  above `namespace sq::console::bridge_protocol`).
- **The compiler it names** (fixed 2026-10-20): each unit's compile command
  names `<clang-doc's directory>/clang++`, an absolute path. clang-doc finds
  the C++ standard library from where that compiler is. Termux builds clang
  with a relative default sysroot (`DEFAULT_SYSROOT=../..`), which the driver
  takes from the compiler's directory: `$PREFIX/bin/../..`, with libc++ under
  it in `usr/include/c++/v1`. A bare `clang++` has no directory, so the
  sysroot was `../..` from wherever clang-doc stood; on the phone every header
  failed with `'cstdint' file not found` and clang-doc 21 then crashed
  (segmentation fault). clang-tidy never had the problem: given flags after
  `--` it names its compiler `<its directory>/clang-tool`, the same trick. The
  file need not exist; only its directory is used. On Debian the host's
  libstdc++ is found either way. (Why the bare name worked for 4.9a's and
  6.1's runs is not known; the absolute name does not depend on it.)
- **Failing loudly:** clang-doc exits 0 after compile errors and writes the
  pages it could, with types missing (seen with clang-doc 18: 275 errors,
  exit 0). So any `error:` in its log fails the run and nothing is written;
  the first few distinct errors print, fatal ones (a header not found) first,
  with the log's path. Before the 96 units, a one-unit probe
  (`#include <cstdint>`, about a second) checks the standard library is
  visible and, if not, prints where clang-doc looked.
- **Progress** (2026-10-20, after a silent run of minutes on the phone): a dot
  as clang-doc starts each unit, then a dot per page tidied, with the count
  every 50, as `make headers` prints. clang-doc says `[n/N] Processing file`
  on stderr, unbuffered; a bash `read` loop counts those lines as they come.
  Not awk: mawk holds its input until the end, so every dot came at once. With
  `pipefail` the pipeline's status is clang-doc's. clang-doc runs in a
  subshell whose own stderr is dropped, because that is where bash reports a
  signal and it would print the whole pipeline; the script says the signal
  itself ("killed by signal 11, a segmentation fault"). The subshell has a
  second command (`exit $?`) so bash cannot exec clang-doc in its place.
- **Versions:** the pages depend on clang-doc's version, so `README.md` records
  it and `--check` compares only with the same major version.
- **Limits of clang-doc** (18 checked): a member variable's comment is not
  shown; an override shows no text (its comment is on the base); enums appear
  on the namespace page.
- Memory: a tool, so priority 1 does not apply. About 25 s for 92 headers here.

## pin_libcxx_modules.sh

`tools/pin_libcxx_modules.sh <share/libc++/v1>`: copies libc++'s std module
sources (`std.cppm`, `std/*.inc`) from a libc++ installation into
`third_party/libcxx/`, replacing what was pinned, and writes `PINNED`: where
from (with `~` and `$PREFIX`), the `_LIBCPP_VERSION` of the headers beside
them (the NDK's `sysroot/usr/include/c++/v1`, or an installation's
`include/c++/v1`), the date, and a sha256 for each file. Refuses a directory
without them, or without headers beside it. `make headers` checks the sums.
Run when the NDK's libc++ moves; then build, test, commit. Why pinned, and
from where: [modules.md](modules.md#std).

## pin_freetype.sh

`tools/pin_freetype.sh <freetype source>`: copies FreeType's `include/` and
the sources of the modules the font module builds (base, truetype, sfnt,
smooth, autofit, cff, psaux, pshinter, psnames; `.c` and `.h` only) from an
unpacked release or a checkout of its tag into `third_party/freetype/`,
replacing what was pinned, through a new directory renamed into place; and
writes `PINNED`: where from (the git tag and commit when it is a
checkout), the version, the modules, the date, a sha256 for each file.
Keeps squared's `README.md` there. Refuses a directory that is not
FreeType's. `make headers` checks the sums. Why, and what is compiled:
[font.md](font.md#freetype-as-squared-builds-it).

## std_bmi.sh

`tools/std_bmi.sh <module dir> <archive> <compiler> <flags>...`, run by
`build/common.mk`'s std rule: makes `import std;`'s BMI and object for one
compiler, flag set and libc++, once on the device, in
`$(SQUARED_STD_HOME)/<key>/`, or finds them made; links the build's
`modules/std.pcm` to it, writes `modules/std.d` and `std.inputs`, archives
`libsquared_std.a`, and removes keys beyond `SQUARED_STD_KEEP` (6). Refuses a
standard library that is not libc++, and libc++ headers of another major
version than `PINNED`. Fits its copy of the sources to the headers (libc++'s
header-availability checks out; a `using` of a name the C library makes
static left out, and said: Termux's `tmpfile`), never the pinned files. One
`mod  std (...)` line, `into` or `made before`.
The key, the sharing and the races: [modules.md](modules.md#std).

## inputs_sum.sh

`tools/inputs_sum.sh <list> <sum> <public root>`: writes `<sum>`, the md5 of
every file a module's `.inputs` list names, one line each, the path with the
workspace's directory written `@ROOT@`. Run by `build/modules.mk`'s
`.inputs` rule (before the list is renamed into place) and by
`build/common.mk`'s std rule (`std.sum`). The build cache's keys read it
instead of the files: [modules.md](modules.md#the-build-cache).

## format.sh

Runs clang-format over every `.hpp`, `.cpp` and `.cppm` under `squared/`, skipping
`third_party/`, `build/` and `docs/`, with `squared/.clang-format`. The rules
and their source are in [code-style.md](code-style.md).

- `--check` lists the files that would change and exits 1 if there are any.
  `make lint` checks the same thing in its first step, without calling this
  script.
- Without it, files are formatted in place, but only on a tree with no
  uncommitted changes under `squared/` (`--force` overrides), so the result can
  be committed as formatting alone.
- Needs clang-format 18 or newer, and says which it found.

It has no workspace wrapper: it formats the framework, and a project's own code
follows the project's style.

## lint.sh

The engine behind `make lint`; the root `Makefile` calls it in one line and
holds nothing else about linting. Three steps, each skipped with a message if
its tool is missing: format (clang-format), rules (`tools/lint-rules.txt`
against `tools/lint-baseline.txt`), tidy (clang-tidy on what the build
compiles, with the build's flags). What each checks and why is in
[lint.md](lint.md).

| option | effect |
|---|---|
| `--changed` | only files changed since the last commit, untracked ones included; deleted ones left out |
| `--since <commit>` | the same, compared with that commit: `--since HEAD~1` checks what the last commit touched |
| `--only format\|rules\|tidy` | one step |
| `--jobs N` | clang-tidy processes at once (default: CPUs, at most 4) |
| `--record-baseline` | write today's rule matches to the baseline; whole tree only |
| file paths | only those files, relative to `squared/` |

Progress prints throughout; the report goes to `build/release/lint/report.txt`
and clang-tidy's full output to `build/release/lint/tidy/`. Exit status: 0
clean, 1 findings, 2 usage or environment error.

The checks live in data files, not in the script, so the engine can be
rewritten (in Lua, say) without touching the Makefile or the rules;
[lint.md](lint.md#engine-and-data) lists what a replacement has to keep. Unlike
`docs-archive.sh`, it needs bash 4.3 (`wait -n` for `--jobs`); Termux and the
Linux targets have bash 5.

No workspace wrapper, for the same reason as `format.sh`.

## dexgen.sh

Builds `tools/dexgen/dexgen.cpp` (with `$CXX`, default `c++`, into
`build/release/tools/`) and checks the committed shim dex against it. What the
dex is, and why: [shims.md](shims.md).

| option | effect |
|---|---|
| (none) | regenerate into a temporary file and compare byte for byte with `tools/dexgen/shims.dex`; exit 1 if they differ |
| `--write` | regenerate the committed dex, after changing the shim table |
| `--dump [dex]` | print a dex: checksums, tables, classes, methods, instructions (default: the committed one) |
| `--self-test` | build with a test shim that has native methods and parameter lists, write it, read it back |

Exit status: 0 good, 1 a check failed, 2 usage or a failed build. The binary is
rebuilt when the source is newer than it. No workspace wrapper: it is a
maintainer tool.

## shim_spike.sh

Sprint 4's device test of the console shim, run in a project:
`squared/tools/shim_spike.sh [project dir]`. It builds the project's
`DEV_LOG=1` APK, then a second one in `build/spike/` with
`dev.squared.ConsoleActivity` added as a launcher activity in its own process
(`:console`) and task, `android:hasCode="false"` dropped, and `classes.dex`
(the committed shim dex) added before signing. The project's files are not
touched. It reads the build's own settings (`SQ_AAPT2`, `SQ_ANDROID_JAR`,
`SQ_MIN_SDK`, the keystore) from the project's Makefile, the way `lint.sh`
reads module settings. It is temporary: the `CONSOLE=monitor` packaging in the
template's `mk/` (4.7) replaces it.

