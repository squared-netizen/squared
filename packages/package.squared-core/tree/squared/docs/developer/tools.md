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
  any change to a public header's declarations or comments, and never edited.
- **Input:** one generated translation unit per workhorse header (the `.hpp`
  files at a module's root), each including just that header, so the reference
  is what a user can include. The headers are self-contained (`make headers`),
  so these compile anywhere with `-std=c++20 -I` the tree.
- **Tidying:** "Defined at" becomes a link to the header's line; `_Bool` reads
  `bool`, `basic_string` and `basic_string_view` read `std::string` and
  `std::string_view`; lists, and a namespace page's enums and functions, are
  sorted (clang-doc merges in parallel, and their order changed from run to
  run: five runs in a row now match); the pages of anonymous
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

## format.sh

Runs clang-format over every `.hpp` and `.cpp` under `squared/`, skipping
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

