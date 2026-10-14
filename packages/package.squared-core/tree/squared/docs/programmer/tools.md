# Tools

Scripts that ship with the framework in `squared/tools/`. In a generated project, the ones meant for everyday use also have a wrapper of the same name in the workspace's `tools/`, so you run them from the workspace root.

Developer side: [developer/tools.md](../developer/tools.md).

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
