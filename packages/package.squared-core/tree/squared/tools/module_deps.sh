#!/usr/bin/env bash
# tools/module_deps.sh <module> <module dir> <object dir> <BMI dir> <interface>...
#
# The order of one module's interface units, as make rules (build/modules.mk
# includes what this prints): each interface's object waits for the BMIs of
# what it imports. Read from the import lines, which squared writes one to a
# line at the top of a unit, after its global module fragment:
#
#   export module sq.gui:label;
#   import sq.graphics;          another module's primary BMI
#   import :widget;              a partition of this one
#   export import :widget;       the same, re-exported
#
# Also checks that each file declares the module its path says: module.cppm
# is `export module sq.<module>;`, and <dir>/<name>.cppm is the partition
# `sq.<module>:<dir>.<name>` (slashes become dots). A file that does not is an
# error here, before clang meets it.

set -euo pipefail

if [ $# -lt 4 ]; then
    echo "usage: module_deps.sh <module> <module dir> <object dir> <BMI dir> <interface>..." >&2
    exit 2
fi
module=$1
dir=$2
objects=$3
bmis=$4
shift 4

status=0
printf '# The order of sq.%s'"'"'s interfaces (tools/module_deps.sh); written by the build.\n' "$module"
for unit in "$@"; do
    name=${unit%.cppm}
    if [ "$name" = module ]; then
        want="sq.$module"
    else
        want="sq.$module:$(printf '%s' "$name" | tr '/' '.')"
    fi
    declared=$(sed -n 's/^export module \([^;]*\);.*/\1/p' "$dir/$unit" | head -n 1)
    if [ "$declared" != "$want" ]; then
        echo "module_deps: $module/$unit declares '${declared:-no module}', its path says '$want'" >&2
        status=1
        continue
    fi
    needs=()
    while IFS= read -r imported; do
        case $imported in
            :*) needs+=("$bmis/sq.$module-${imported#:}.pcm") ;;
            sq.*) needs+=("$bmis/$imported.pcm") ;;
        esac
    done < <(sed -n 's/^\(export \)\{0,1\}import \([^;<"]*\);.*/\2/p' "$dir/$unit")
    if [ ${#needs[@]} -gt 0 ]; then
        printf '%s/%s.cppm.o: %s\n' "$objects" "$name" "${needs[*]}"
    fi
done
exit $status
