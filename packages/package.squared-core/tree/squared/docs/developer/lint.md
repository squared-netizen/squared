# Lint

How `make lint` works, what it checks, and why each check is there.

Programmer side: none. Lint checks the framework's own source; a project's
code in `sq_app/` and `sq_android/` is the project's business. Related:
[code-style.md](code-style.md) (the format rules), [tools.md](tools.md) (the
script), [priority-audit.md](priority-audit.md) (the findings the rules track).

## Three layers

| layer | catches | tool | status |
|---|---|---|---|
| 1. clang-tidy | owning raw pointers, missing special members, copies, oversized enums, bugs | `squared/.clang-tidy` | in `make lint` |
| 2. source rules | what clang-tidy has no check for: `std::function`, exceptions, RTTI, any `new`/`delete` | `tools/lint-rules.txt` and `tools/lint-baseline.txt` | in `make lint` |
| 3. frame-allocation counter | a heap allocation inside the frame loop | the `dev` module, in `DEV_LOG` builds | reports on the device |

No static check can prove the frame loop does not allocate: a
`std::vector::push_back` is fine at load time and a bug at frame time, and the
code is the same. Layers 1 and 2 keep ownership honest; layer 3 measures the
loop.

A format check runs first, so `make lint` is the one command for all of it.

## Running it

```sh
make lint                              # everything, every file
make lint LINT_ARGS=--changed          # files changed since the last commit
make lint LINT_ARGS="--only tidy"      # one step: format, rules or tidy
tools/lint.sh gui/src/ui.cpp           # the files named
tools/lint.sh --jobs 2                 # fewer clang-tidy processes at once
tools/lint.sh --record-baseline        # accept today's rule matches
BUILD=debug tools/lint.sh --only tidy  # analyse with the debug flags
```

`LINT_ARGS` and not `LINT`: GNU make predefines `LINT` as a program name.

Progress prints as it goes: a line per 40 files for the format step, a line per
rule, a line per translation unit for clang-tidy. The report is
`build/release/lint/report.txt`; clang-tidy's full output, with its notes and
fix-it hints, is in `build/release/lint/tidy/<file>.log`.

| exit | meaning |
|---|---|
| 0 | no findings |
| 1 | findings; the report lists them |
| 2 | usage error, a named file that does not exist, `--changed` outside git, `--record-baseline` with a file list |

A step whose tool is missing says so and is skipped. It does not fail the run:
a machine without clang-tidy can still run the rules.

## Engine and data

The root `Makefile` holds one line for lint: `bash tools/lint.sh
$(LINT_ARGS)`. Everything else is in two kinds of file:

- **the engine**, `tools/lint.sh`: finds files, runs tools, compares with the
  baseline, prints progress, writes the report.
- **the data**: `.clang-tidy` (which checks) and `tools/lint-rules.txt` (which
  patterns), plus `tools/lint-baseline.txt` (which matches are accepted).

The split is there so the engine can be replaced (in Lua, with PEG matching
instead of regular expressions, say) without touching the Makefile or the
rules. A replacement must keep:

- the command line above, since `make lint` passes `LINT_ARGS` through;
- the rules format (below) and the baseline format, `rule file count`, one
  per line, `#` starting a comment or a reason;
- the report path and the exit statuses;
- the stripping contract for rules: a pattern is matched against code with
  comments removed and string and character literal contents blanked, line
  for line.

A PEG engine can read `match` as it likes, but the five rules today are
written as POSIX extended regular expressions plus `\b`, which GNU grep (what
Termux ships) and BusyBox grep both accept with `-E`.

## Layer 1: clang-tidy

`squared/.clang-tidy` enables checks by group. Each group serves a priority:

| group | checks | priority |
|---|---|---|
| ownership | `cppcoreguidelines-owning-memory`, `-no-malloc`, `modernize-make-unique`, `-make-shared` | 1: memory is owned by a type that frees it |
| class hygiene | `cppcoreguidelines-special-member-functions`, `-slicing`, `-virtual-class-destructor`, `-rvalue-reference-param-not-moved`, `-missing-std-forward` | 1: no accidental copies, no leaks through a base pointer |
| size and copies | `performance-*`, including `performance-enum-size` | 1, then 3 |
| bugs | `bugprone-*` except `easily-swappable-parameters` (fires on every two-float function) and `narrowing-conversions` (the build's `-Wconversion` already reports it); `exception-escape` ignores three standard-library exception types ([triage](#triage-round-1-sprint-2-task-23)) | correctness |
| ODR and includes | `misc-definitions-in-headers`, `-header-include-cycle`, unused using and alias declarations | the self-containment invariant |
| modern spelling | `nullptr`, `override`, `using`, `= default`, `= delete`, no `(void)` | 2 |
| readability | only checks that prevent a bug or a slow path | 2, 3 |
| static analysis | `clang-analyzer-core`, `-cplusplus`, `-deadcode`, `-unix` | correctness |

Not enabled, on purpose: `cppcoreguidelines-avoid-magic-numbers`,
`readability-identifier-length`, `modernize-use-trailing-return-type` and the
other style checks. They produce volume, not safety, and the style is
clang-format's job.

`HeaderFilterRegex: '/squared/[a-z]+/'` reports findings in squared's own
headers and never in `third_party/` (the underscore stops `[a-z]+`) or system
headers. The squared-pg repository has its own, broader `.clang-tidy` at its
root; `squared/.clang-tidy` does not inherit it, so the framework's checks are
the same in the package, in an example and in a generated workspace.

### What is analysed

Exactly what the build compiles, with the build's flags. The engine asks the
Makefiles, through a helper makefile that adds a `print-<variable>` target and
builds nothing:

1. the root `Makefile`'s `MODULES`;
2. each module's `SOURCES`, `STANDARD`, `OPTIMISE`, `SQUARED_ABI` and
   `INCLUDES`.

`OPTIMISE` matters: `-DNDEBUG` decides which asserts exist. Warning flags are
not passed; the build already reports those, and clang-tidy would report them
again as `clang-diagnostic-*`.

There are two configurations:

- **host**: whatever `local.mk` selects (by default the null backend and the
  POSIX platform);
- **android**: added when the NDK's sysroot headers are found
  (`$PREFIX/opt/android-sdk/ndk/*/toolchains/llvm/prebuilt/<host>/sysroot/usr/include`,
  or under `$ANDROID_NDK_HOME`). It sets `SQUARED_GRAPHICS_BACKEND=gles`,
  `SQUARED_PLATFORM=android` and `SQUARED_EXTERNAL_INCLUDES=-idirafter <sysroot>`,
  the same way the `gles` module test is built, and contributes only the
  sources the host configuration does not compile (`gles/`, the GLES backend,
  the Android asset file system).

A selected `.cpp` that neither configuration compiles is named as skipped,
never analysed with guessed flags. Each progress line says which configuration
a file was analysed in.

Headers are analysed through the `.cpp` files that include them, so a header
included by ten files would be reported ten times; findings are deduplicated
by place and check. Naming a header, or changing one under `--changed`,
analyses the sources that include it directly. A header that no `.cpp`
includes is not analysed by clang-tidy at all; `make headers` still compiles
every one of them.

A tree without Makefiles (a test fixture) falls back to every
`<module>/src/**.cpp` with `-std=c++20` and the layout's include paths.

### Versions

Written against clang-tidy 21 (Termux) and checked with 18. The file uses no
option either rejects; in particular not `ExcludeHeaderFilterRegex`, which 18
does not have. Findings can differ slightly between versions: a check gains
cases.

## Layer 2: source rules

`tools/lint-rules.txt` holds the rules, one block each:

```
rule  raw-new
why   no hand-written new: use std::make_unique, a container, or the owning pool (priority 1). ...
match (^|[^_[:alnum:]])new\b[[:space:]]*[(_[:alpha:]:]
files gui/*.hpp gui/src/*      (optional; default: every .hpp and .cpp)
```

The default file set is every `.hpp` and `.cpp` outside `third_party/`,
`build/`, `docs/` and any `test/` directory. Tests may do what the framework
may not: `files/test/asset_stub.cpp` stands in for the NDK's C API, which hands
out raw pointers.

| rule | matches | why |
|---|---|---|
| `std-function` | `std::function` | heap capture and an indirect call; [priority-audit.md](priority-audit.md) item 5 |
| `exceptions` | `throw`, `try`, `catch` | the move to `-fno-exceptions`; items 1 and 3 |
| `rtti` | `dynamic_cast`, `typeid` | the tree builds with `-fno-rtti` and has to keep doing so; item 2 |
| `raw-new` | a `new` expression, placement new included | ownership goes through a type |
| `raw-delete` | a `delete` expression, not `= delete` | the same |

Matching runs on a stripped copy of each file: comments removed and the
contents of string and character literals blanked, line for line, so a match's
line number is the source's. A comment that says "do not throw" and the string
`"try again"` do not match. Raw string literals are not understood; squared has
none.

### The baseline

Code that already breaks a rule is not fixed in one go. `tools/lint-baseline.txt`
records, per rule and file, how many matches are accepted:

```
exceptions gui/src/ui.cpp 11
std-function gui/widget.hpp 1  # TooltipFactory: side table planned (priority-audit.md item 5)
```

A file may match a rule as many times as its entry says; a file without an
entry may not match at all. The report quotes every matching line of a file
over its count, since the engine cannot tell which match is the new one.

After fixing, `tools/lint.sh --record-baseline` writes the smaller counts;
commit that file with the fix. Recording also accepts any increase, and it says
so (`accepted exceptions gui/src/ui.cpp: 9 -> 11`): raising a count is a
decision, made in a commit a reviewer sees. A reason written after `#` on an
entry's line survives re-recording. Recording needs the whole tree, so it
refuses a file list and `--changed`.

## Heap allocation is not the problem; unowned allocation is

The rules and the ownership checks do not forbid the heap. A texture atlas's
pixels are megabytes and belong on the heap. What they forbid is memory nobody
owns:

```cpp
// Rejected: raw-new, and owning-memory (a raw pointer that must be deleted by hand)
auto *pixels = new std::byte[ width * height * 4 ];

// Accepted: the same single allocation, owned; freed by the destructor
auto pixels = std::make_unique_for_overwrite<std::byte[]>( width * height * 4 );

// Accepted: owned and sized, when the size is needed later anyway
std::vector<std::byte> pixels( width * height * 4 );
```

`make_unique_for_overwrite` (C++20) skips the zero fill a pixel buffer is about
to overwrite: priority 3, at no memory cost. `std::vector` adds 16 bytes for its
size and capacity: acceptable once per resource, not per sprite.

Where allocation happens is a separate rule (load time, not frame time), and
layer 3 is what checks it.

A pool that constructs objects in its own storage needs placement `new`. That
is the one legitimate `raw-new` match: accept it in the baseline with a reason
on the entry's line.

## Layer 3: the frame-allocation counter

A development build (`make DEV_LOG=1 apk` in the Android template) replaces
the global `operator new` with one that counts, and the template brackets each
frame's `update()` and `render()` with `sq::dev::frame_begin()` and
`frame_end()`. Frames that allocate are reported over the log sink:

```
alloc: frame 3 allocated 2 times, 96 bytes (the first frame that allocated)
alloc: 41 of the last 120 frames allocated: 82 times, 3936 bytes; most in one frame: 2
```

Only the frame thread is counted, and a release build contains none of it.
Unlike layers 1 and 2 it does not fail `make lint`: it runs on the device, so
its result is read from `nc -klu 5555`. How it works:
[dev.md](dev.md#the-frame-allocation-counter); how to use it:
[../programmer/dev.md](../programmer/dev.md#frame-allocations).

## Memory and cost

None of this is in the application: priority 1 does not apply to it. What it
costs is time and the phone's RAM while it runs.

| step | processes | notes |
|---|---|---|
| format | one `clang-format` per 40 files | about a second for the tree |
| rules | one `awk` to strip every file, one `grep` per rule, one `awk` per file over its count | under a second |
| tidy | one `make` per module per configuration, one `clang-tidy` per translation unit | minutes on a phone |

`--jobs` defaults to the number of CPUs, at most 4. One clang-tidy on a large
file can take a few hundred megabytes; four is the most a phone should run
next to an editor. Scratch files go to `$TMPDIR` and are removed on exit.

The engine needs bash 4.3 or newer (`wait -n`); Termux and current Linux
distributions have bash 5. It uses no GNU-only option of `find`, `grep`, `sed`,
`awk`, `split` or `sort`.

## Known limitations

- Rules are textual. A `new` inside a macro defined elsewhere is not seen, and
  a pattern cannot tell a type from a variable. clang-tidy's ownership checks
  cover the cases that matter.
- clang-tidy cannot see a header that no `.cpp` includes, and a changed
  header pulls in its direct includers only, not the sources that reach it
  through another header.
- The android configuration needs the NDK. Without it, the GLES and Android
  sources are skipped, and the run says so.
- `--changed` compares with `HEAD`: committed work is not re-checked.
- clang-tidy versions disagree at the edges: 21 finds more `exception-escape`
  cases (it sees libc++'s throws), 18 finds guarded optionals that 21 does
  not. The phone's clang-tidy 21 is the reference.

## Triage, round 1 (Sprint 2, task 2.3)

The reference run is the phone's: clang-tidy 21.1.8, host and Android
configurations, package `6c2d2a0`. Format clean with clang-format 21; rules 65
matches, all in the baseline (54 `exceptions` in 17 files, 11 `std-function`);
clang-tidy 104 findings. Each was fixed, suppressed with a reason, or
configured away:

| findings | check | outcome |
|---:|---|---|
| 41 | `performance-enum-size` | **fixed**: every one is now `: std::uint8_t`. Measured: `gui::Cell` 80 → 72 bytes, `app::Event` 96 → 88; the other types holding these enums keep their size because of padding, so the gain there waits for a member-ordering pass ([memory-profile.md](memory-profile.md)) |
| 33 | `bugprone-exception-escape` | **configured**: all 33 are standard-library throws inside `noexcept` functions: `bad_array_new_length` (15) and `length_error` (9) from container growth, `out_of_range` (9) from `substr`. Those three are ignored (`IgnoredExceptions` in `.clang-tidy`), because in a `noexcept` function they terminate, which is what they do under `-fno-exceptions` anyway. All 9 `substr` calls were checked: each position is at most the size. squared's own exception types stay checked |
| 5 | `bugprone-implicit-widening-of-multiplication-result` | **fixed**: the limit constants multiply in the wide type (`std::size_t{ 8 } * 1024U * 1024U`) |
| 4 | `performance-no-int-to-ptr` | **suppressed**: GL takes buffer offsets in pointer parameters (3, `vertex_array.cpp`, `gles/sprite_batch.cpp`), and `dladdr` takes a program counter (1, `dev_log.cpp`) |
| 4 | `cppcoreguidelines-owning-memory` | **fixed**: `FILE *` in `posix_file_system.cpp` is a `std::unique_ptr` with a stateless closer (8 bytes, no change), so a `bad_alloc` while sizing the read buffer no longer leaks the stream; the write path still reports `fclose` failure |
| 1 | `cppcoreguidelines-no-malloc` | **fixed**: `json.cpp` owns yyjson's output in a `std::unique_ptr` before copying it, so a failed copy no longer leaks it |
| 4 | `cppcoreguidelines-missing-std-forward` | **fixed** 1 (`DeadlineQueue::cancel` calls its visitor once, so it forwards); **suppressed** 3 (`poll_due`, `visit_ordered`, `inspect_pending` call it once per entry, where forwarding would let the first call move it away) |
| 3 | `cppcoreguidelines-special-member-functions` | **fixed**: `Group` and `ToggleButton` delete copy and move explicitly (they already were, through `Actor`); `TextureRecoveryTarget` declares its destructor |
| 2 | `performance-unnecessary-value-param` | **fixed**: `Dialog::choose` (private) takes `std::string_view`, which also fixes a second click of the same dialog button reporting an empty result (the old lambda moved its string away on the first); the dispatcher's delivery lambda takes `const PendingDelivery &` |
| 2 | `clang-analyzer-deadcode.DeadStores` | **fixed**: `skin_loader.cpp`, `ui.cpp` |
| 1 | `performance-inefficient-string-concatenation` | **fixed** (an error path in `skin_loader.cpp`) |
| 1 | `bugprone-optional-value-conversion` | **fixed**: `queue_receipt` passes the sender's optional through |
| 1 | `bugprone-incorrect-roundings` | **suppressed**: the value is in (0, 1) there, so `+ 0.5` then truncation rounds half up exactly |
| 1 | `misc-header-include-cycle` | **suppressed, deferred**: `gui.hpp` and the `skin_loader.hpp` aggregate include each other; harmless under `#pragma once`, and Sprint 3 decides the aggregates |
| 1 | `bugprone-empty-catch` | **suppressed, deferred**: `~ToggleButton` swallows what a change callback throws during rebalance; it goes with `-fno-exceptions` ([priority-audit.md](priority-audit.md) item 3) |

clang-tidy 18 also reports 7 `bugprone-unchecked-optional-access` that 21 does
not: guarded accessors called twice. The six that were still present now bind
the optional once, which reads better and satisfies both.

Found on the way, not by a check: `tools/measure_sizes.sh` named a namespace
that no longer exists and did not compile; fixed, and it now also measures the
error, event, file and messaging types. gcc's `-Wsign-conversion` flagged
`messaging_detail.hpp`; fixed.

### Suppressions

Every suppression names its check and gives the reason on the line above:

```cpp
// value is in (0, 1) here, so adding 0.5 and truncating rounds half up exactly.
// NOLINTNEXTLINE(bugprone-incorrect-roundings)
```

`NOLINTBEGIN(check)` and `NOLINTEND(check)` bracket a block when one reason
covers several lines. A bare `NOLINT` with no check named is not used.
