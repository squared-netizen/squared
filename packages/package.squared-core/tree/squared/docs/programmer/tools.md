# Tools

Scripts that ship with the framework in `squared/tools/`. In a generated project, the ones meant for everyday use also have a wrapper of the same name in the workspace's `tools/`, so you run them from the workspace root.

Developer side: [developer/tools.md](../developer/tools.md).

## docs.sh

`make docs` (or `squared/tools/docs.sh`) writes the API reference,
[docs/reference/](../reference/README.md): a page for every public type, with
each function's signature, its comment, and a link to the line in the header.
It is made from the headers' comments by clang-doc (`pkg install clang` in
Termux), and committed, so it reads on GitHub without a build.

```sh
make docs          # write docs/reference/
make docs-check    # is it current? exit 1 if a header changed since
squared/tools/docs.sh --missing   # what has no comment yet
```

Do not edit the pages: change the comment in the header and run it again.

A run prints a dot per header as clang-doc reads it, then a dot per page as
the pages are tidied, with the count every 50:

```
docs: clang-doc 21 over 96 public headers
.................................................. 50/96
.............................................. 96/96
docs: tidying 167 pages
...
```

If clang-doc cannot compile a header, nothing is written: it prints the
first few errors and where its log is (`build/release/docs/clang-doc.log`).
`docs: clang-doc cannot compile #include <cstdint>` means it cannot see the
C++ standard library at all; the lines under it are where it looked.

## sqgen

Compiles an app's `sq_app/names.json` and `sq_app/layouts/*.json` into C++:
typed keys (`names.hpp`) and constant layout tables (`layouts.hpp`,
`layouts.cpp`). The app's build runs it for you whenever those files exist,
so you rarely run it yourself:

    sqgen [--names FILE] [--layouts DIR] --out DIR

A C++ program in `squared/tools/sqgen/`, built for your machine. Its files,
its problems and its exit status: [layouts.md](layouts.md#sqgen-the-build-step).

## docs-archive.sh

Zips documentation into one archive, for reading on another device or sharing.

From the workspace root, it archives every docs tree in the workspace: `docs/` (generated output), `squared/docs/`, `sq_app/docs/`, `sq_android/docs/`, and any other top-level directory's `docs/`. Empty trees are skipped.

    tools/docs-archive.sh --list                      # what would go in; writes nothing
    tools/docs-archive.sh                             # -> ~/<project>-docs-<date>.zip
    tools/docs-archive.sh ~/storage/shared/Download   # somewhere you can share from

Run directly, `squared/tools/docs-archive.sh` archives only the framework's docs, as `squared-docs-<version>-<date>.zip`.

| argument | meaning |
|---|---|
| `destination` | directory to write to (default `$HOME`); created if missing |
| `--list` | print the archive name, destination, trees and files, then stop |
| `--workspace DIR` | archive DIR's docs trees (the wrapper does this for you) |

Failures, each with a message on stderr and a non-zero exit:

| exit | cause |
|---|---|
| 1 | `zip` not installed (`pkg install zip`), no docs found, destination not creatable or writable, zip failed |
| 2 | unknown option, two destinations, `--workspace` without a directory |

The archive leaves out Obsidian state (`.obsidian/`, `.trash/`), editor scratch files and `*.bak.*` backups. Running it twice on the same day writes `...-2.zip` rather than overwriting.

## dexgen.sh

Maintainer tool: checks, rewrites or prints the shim dex the console's monitor
process is built with. You do not need it to build an app.

    squared/tools/dexgen.sh             # the committed dex regenerates byte for byte
    squared/tools/dexgen.sh --dump      # print it

Exit status: 0 good, 1 a check failed, 2 usage or a failed build. Details:
[developer/shims.md](../developer/shims.md).

## shim_spike.sh

A device test, for Sprint 4 only: builds your project's `DEV_LOG` APK plus a
copy with a second launcher icon, "<project> console", that runs the same app in
its own process. Your project's files are not changed; the copy is
`build/spike/<project>-spike.apk`.

    squared/tools/shim_spike.sh         # in the project

