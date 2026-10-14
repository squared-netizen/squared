#!/usr/bin/env bash
#
# squared/tools/docs-archive.sh — zip documentation into one archive.
#
#   squared/tools/docs-archive.sh [destination]       the framework's docs only
#   tools/docs-archive.sh [destination]               (from a workspace) every
#                                                     docs tree in the workspace
#   ... --list                                        what would go in; writes nothing
#   ... --workspace DIR                               archive DIR's docs trees
#
# Workspace mode collects, when present and not empty:
#   docs/             generated output: doxygen, LDoc, anything a tool writes
#   squared/docs/     the framework's programmer and developer docs
#   sq_app/docs/      your own, hand-written
#   sq_android/docs/  platform notes, if you keep any
#   <dir>/docs/       any other top-level directory's docs (a kit's, say)
# Paths inside the archive are kept as they are in the workspace.
#
# Named <project>-docs-<date>.zip in workspace mode and
# squared-docs-<version>-<date>.zip otherwise, and moved to `destination`
# ($HOME when none is given). The same name twice in a day gets -2, -3, ...
#
# The workspace wrapper (tools/docs-archive.sh) sets SQ_WORKSPACE, which turns
# on workspace mode. Runs from anywhere: every path is found from this
# script's own location or from SQ_WORKSPACE, never from the working directory.
# Bash 3.2 compatible.

set -uo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
framework=$(cd "$script_dir/.." && pwd)

destination=""
list_only=0
workspace=${SQ_WORKSPACE:-}

while [ $# -gt 0 ]; do
    case "$1" in
        --list) list_only=1 ;;
        --workspace)
            [ $# -ge 2 ] && [ -n "$2" ] || { echo "docs-archive: --workspace needs a directory" >&2; exit 2; }
            workspace=$2
            shift ;;
        -h|--help)
            sed -n '3,28p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        -*)
            echo "docs-archive: unknown option $1" >&2
            exit 2 ;;
        *)
            if [ -n "$destination" ]; then
                echo "docs-archive: only one destination may be given" >&2
                exit 2
            fi
            destination=$1 ;;
    esac
    shift
done

[ -n "$destination" ] || destination="$HOME"

command -v zip >/dev/null 2>&1 \
    || { echo "docs-archive: zip is not installed (pkg install zip)" >&2; exit 1; }

# --- version ---------------------------------------------------------------
#
# squared has no VERSION file of its own. In a workspace the generator records
# the framework version in mk/squared_generated.mk; inside squared-pg the
# package manifest carries it. Otherwise "unversioned" rather than a guess.

read_make_var() { # read_make_var <file> <name>
    awk -v k="$2" '$1 == k && $2 == ":=" { print $3; exit }' "$1" 2>/dev/null
}

framework_version() {
    local root_of_framework=$1 v=""
    if [ -f "$root_of_framework/VERSION" ]; then
        v=$(tr -d ' \t\n\r' < "$root_of_framework/VERSION")
    elif [ -f "$root_of_framework/../mk/squared_generated.mk" ]; then
        v=$(read_make_var "$root_of_framework/../mk/squared_generated.mk" SQ_FRAMEWORK_VERSION)
    elif [ -f "$root_of_framework/../../SQ-INF/manifest.json" ]; then
        v=$(sed -n 's/^  "version": "\([^"]*\)".*/\1/p' "$root_of_framework/../../SQ-INF/manifest.json" | head -n 1)
    fi
    echo "${v:-unversioned}"
}

# --- what goes in ----------------------------------------------------------
#
# A docs tree counts only if it holds at least one file. Obsidian's state,
# editor scratch files and timestamped backups are left out: they are
# per-machine, and would make two archives of the same docs differ.

has_files() { # has_files <dir>
    [ -n "$(find "$1" -type f ! -path '*/.obsidian/*' ! -path '*/.trash/*' \
        ! -name '.DS_Store' ! -name '*.swp' ! -name '*~' ! -name '*.tmp' ! -name '*.bak.*' 2>/dev/null | head -n 1)" ]
}

trees=()
if [ -n "$workspace" ]; then
    [ -d "$workspace" ] || { echo "docs-archive: no workspace at $workspace" >&2; exit 1; }
    root=$(cd "$workspace" && pwd)
    project=$(read_make_var "$root/mk/squared_generated.mk" SQ_PROJECT)
    [ -n "$project" ] || project=$(basename "$root")
    name_stem="$project-docs"

    # The root docs/ first, then the framework, the app and the platform in
    # that order, then any other top-level directory that keeps docs.
    for candidate in docs squared/docs sq_app/docs sq_android/docs; do
        if [ -d "$root/$candidate" ] && has_files "$root/$candidate"; then
            trees+=("$candidate")
        fi
    done
    for dir in "$root"/*/; do
        base=$(basename "$dir")
        case "$base" in
            docs|squared|sq_app|sq_android|build) continue ;;
        esac
        if [ -d "$root/$base/docs" ] && has_files "$root/$base/docs"; then
            trees+=("$base/docs")
        fi
    done
else
    root=$(cd "$framework/.." && pwd)
    fw=$(basename "$framework")
    name_stem="squared-docs-$(framework_version "$framework")"
    if [ -d "$framework/docs" ] && has_files "$framework/docs"; then
        trees+=("$fw/docs")
    fi
fi

[ ${#trees[@]} -gt 0 ] || { echo "docs-archive: no documentation found under $root" >&2; exit 1; }

date_stamp=$(date +%Y-%m-%d)
name="$name_stem-$date_stamp.zip"

EXCLUDES=(
    '*/.obsidian/*'
    '*/.trash/*'
    '*.DS_Store'
    '*.tmp'
    '*.swp'
    '*~'
    '*.bak.*'
)

if [ "$list_only" -eq 1 ]; then
    echo "would archive as: $name"
    echo "             to: $destination"
    echo "          trees: ${trees[*]}"
    echo
    ( cd "$root" && find "${trees[@]}" -type f \
        ! -path '*/.obsidian/*' ! -path '*/.trash/*' \
        ! -name '.DS_Store' ! -name '*.swp' ! -name '*~' ! -name '*.tmp' ! -name '*.bak.*' | sort )
    exit 0
fi

# --- build it --------------------------------------------------------------
#
# Staged in $TMPDIR and moved into place, so an interrupted run never leaves a
# half-written archive at the destination. $TMPDIR, never /tmp: Termux has none.

scratch="${TMPDIR:-/tmp}/squared-docs.$$"
mkdir -p "$scratch" || { echo "docs-archive: could not create $scratch" >&2; exit 1; }
trap 'rm -rf "$scratch"' EXIT INT TERM

# -r recurse, -q quiet, -X drop platform-specific extra attributes so the same
# docs produce the same archive on any host.
( cd "$root" && zip -r -q -X "$scratch/$name" "${trees[@]}" -x "${EXCLUDES[@]}" ) \
    || { echo "docs-archive: zip failed" >&2; exit 1; }

if [ ! -d "$destination" ]; then
    mkdir -p "$destination" \
        || { echo "docs-archive: cannot create $destination" >&2; exit 1; }
fi

[ -w "$destination" ] || { echo "docs-archive: $destination is not writable" >&2; exit 1; }

target="$destination/$name"
if [ -e "$target" ]; then
    # Same name on the same day, twice. Overwriting silently would lose the
    # earlier one; a counter keeps both and says which is which.
    counter=2
    while [ -e "$destination/${name%.zip}-$counter.zip" ]; do
        counter=$((counter + 1))
    done
    target="$destination/${name%.zip}-$counter.zip"
fi

mv "$scratch/$name" "$target" || { echo "docs-archive: could not move into place" >&2; exit 1; }

files=$(unzip -l "$target" 2>/dev/null | tail -n 1 | awk '{print $2}')
size=$(du -h "$target" | cut -f1)

echo "$target"
echo "  ${files:-?} files, $size  (${trees[*]})"
