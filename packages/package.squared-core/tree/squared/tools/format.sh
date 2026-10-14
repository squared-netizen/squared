#!/usr/bin/env bash
#
# squared/tools/format.sh — format squared's C++ with clang-format.
#
#   squared/tools/format.sh --check    list files that would change; writes nothing
#   squared/tools/format.sh            format every file in place
#   squared/tools/format.sh --force    format even with uncommitted changes
#
# Uses squared/.clang-format (CDDA's style; docs/developer/code-style.md).
# Formats every .hpp and .cpp under squared/ except third_party/, build/ and
# docs/. Refuses to run on a git tree with uncommitted changes under squared/
# unless --force, so a formatting commit holds formatting and nothing else.
#
# Runs from anywhere: squared/ is found from this script's own path.
# Exit status: 0 clean (or formatted), 1 --check found files to change,
# 2 usage or environment error.

set -uo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root=$(cd "$script_dir/.." && pwd)

check=0
force=0
for argument in "$@"; do
    case "$argument" in
        --check) check=1 ;;
        --force) force=1 ;;
        -h|--help)
            sed -n '3,17p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)
            echo "format: unknown option $argument" >&2
            exit 2 ;;
    esac
done

command -v clang-format >/dev/null 2>&1 \
    || { echo "format: clang-format is not installed (pkg install clang)" >&2; exit 2; }
[ -f "$root/.clang-format" ] \
    || { echo "format: no $root/.clang-format" >&2; exit 2; }

version=$(clang-format --version | sed -n 's/.*version \([0-9][0-9]*\).*/\1/p')
if [ -z "$version" ] || [ "$version" -lt 18 ]; then
    echo "format: clang-format 18 or newer is needed (found: $(clang-format --version))" >&2
    exit 2
fi

if [ "$check" -eq 0 ] && [ "$force" -eq 0 ] \
    && git -C "$root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    if [ -n "$(git -C "$root" status --porcelain -- .)" ]; then
        echo "format: uncommitted changes under $root; commit them first so the" >&2
        echo "        formatting commit holds only formatting (or pass --force)" >&2
        exit 2
    fi
fi

files=()
while IFS= read -r -d '' f; do
    files+=("$f")
done < <(cd "$root" && find . \( -path ./third_party -o -path ./build -o -path ./docs \) -prune \
             -o -type f \( -name '*.hpp' -o -name '*.cpp' \) -print0 | sort -z)

total=${#files[@]}
[ "$total" -gt 0 ] || { echo "format: no C++ files under $root" >&2; exit 2; }
echo "format: clang-format $version, $total files under $root"

changed=0
done_count=0
cd "$root" || exit 2
for f in "${files[@]}"; do
    done_count=$((done_count + 1))
    if ! clang-format --style=file --dry-run --Werror "$f" >/dev/null 2>&1; then
        changed=$((changed + 1))
        if [ "$check" -eq 1 ]; then
            echo "  would change  ${f#./}"
        else
            clang-format --style=file -i "$f" \
                || { echo "format: clang-format failed on ${f#./}" >&2; exit 2; }
        fi
    fi
    if [ $((done_count % 25)) -eq 0 ] || [ "$done_count" -eq "$total" ]; then
        printf '  %d/%d checked, %d %s\n' "$done_count" "$total" "$changed" \
            "$([ "$check" -eq 1 ] && echo 'to change' || echo 'formatted')"
    fi
done

if [ "$check" -eq 1 ]; then
    [ "$changed" -eq 0 ] && echo "format: clean" && exit 0
    echo "format: $changed of $total files would change"
    exit 1
fi
echo "format: $changed of $total files formatted"
