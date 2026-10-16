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
