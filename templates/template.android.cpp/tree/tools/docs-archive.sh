#!/usr/bin/env bash
#
# Workspace wrapper: runs the framework tool of the same name,
# squared/tools/<this file's name>, with your arguments.
#
#   tools/docs-archive.sh --list
#   tools/docs-archive.sh ~/storage/shared/Download
#
# GENERATED FILE (ownership class: generated). The generator rewrites it on
# update; change the framework tool instead. Every wrapper in tools/ is this
# same file under another name, so a wrapper for another framework tool is a
# copy: cp tools/docs-archive.sh tools/<tool>.sh
#
# SQ_WORKSPACE tells the tool which workspace called it, so a tool that can act
# on the whole workspace (docs-archive.sh does) knows to.

set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root=$(cd "$here/.." && pwd)
name=$(basename "${BASH_SOURCE[0]}")
target="$root/squared/tools/$name"

if [ ! -f "$target" ]; then
    echo "$name: no $target" >&2
    echo "  The framework is vendored into squared/; if it is missing, restore it" >&2
    echo "  (sync_framework.sh, or regenerate the project)." >&2
    exit 1
fi

# bash, not exec of the file itself: the vendored copy may not be executable.
SQ_WORKSPACE=$root exec bash "$target" "$@"
