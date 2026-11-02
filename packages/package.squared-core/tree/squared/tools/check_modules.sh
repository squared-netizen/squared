#!/usr/bin/env bash
# tools/check_modules.sh: what `make headers` checks since squared is modules
# (7b.4). Until then it compiled every header on its own; a module interface
# compiles on its own by construction, so that check went (the user's call,
# 2026-10-27). What can still go wrong without a compile error:
#
#   1. sq.std behind the sources: a file names std::<something> that
#      build/std.cppm does not export yet (tools/std_module.sh --check).
#   2. The build order: a module imports one the top Makefile does not build
#      before it (`gui: graphics ...`). Under make -j the import would race
#      the BMI it needs: sometimes built, sometimes "module not found".
#   3. A module's files naming another module than the one their path says
#      (tools/module_deps.sh checks it as it builds; here, for every module
#      at once, without building).
#
# Exit status: 0 when all pass, 1 when one fails, 2 on a usage error.

set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
status=0

# --- 1 -----------------------------------------------------------------------
bash tools/std_module.sh --check || status=1

# --- 2 -----------------------------------------------------------------------
modules=$(make -s --no-print-directory -f Makefile -f /dev/stdin print-modules <<'EOF' 2>/dev/null
print-modules:
	@echo $(MODULES) gles
EOF
)
# The top Makefile's order lines, `<module>: <modules>`, as "module dep" pairs.
pairs=$(sed -n 's/^\([a-z_]*\): \([a-z_ ]*\)$/\1 \2/p' Makefile |
    awk -v mods=" $modules " '{ if (index(mods, " " $1 " ") == 0) next; for (i = 2; i <= NF; i++) print $1, $i }')
# reaches <from> <to>: whether the order puts <to> before <from>.
reaches() {
    local seen=" $1 " todo="$1" next m d
    while [ -n "$todo" ]; do
        next=""
        for m in $todo; do
            for d in $(printf '%s\n' "$pairs" | awk -v m="$m" '$1 == m { print $2 }'); do
                [ "$d" = "$2" ] && return 0
                case $seen in *" $d "*) ;; *) seen="$seen$d " next="$next $d" ;; esac
            done
        done
        todo=$next
    done
    return 1
}
order_bad=0
for m in $modules; do
    [ -d "$m" ] || continue
    imported=$(grep -rhoE '^(export )?import sq\.[a-z_]+;' --include=*.cppm --include=*.cpp --include=*.hpp "$m" |
        sed 's/.*import sq\.\([a-z_]*\);/\1/' | LC_ALL=C sort -u || true)
    for i in $imported; do
        [ "$i" = std ] || [ "$i" = "$m" ] && continue
        if ! reaches "$m" "$i"; then
            echo "check_modules: $m imports sq.$i, and the top Makefile does not build $i before it (add $i to the '$m:' line)" >&2
            order_bad=1
        fi
    done
done
if [ $order_bad = 0 ]; then
    echo "check_modules: the build order covers every import ($(printf '%s\n' $modules | wc -l | tr -d ' ') modules)"
else
    status=1
fi

# --- 3 -----------------------------------------------------------------------
names_bad=0
units=0
for m in $modules; do
    [ -d "$m" ] || continue
    # One run a module, all its units at once: a process a unit took minutes
    # on the phone.
    mapfile -t list < <(cd "$m" && find . -name '*.cppm' -not -path './test/*' | sed 's|^\./||' | LC_ALL=C sort)
    [ ${#list[@]} -gt 0 ] || continue
    units=$((units + ${#list[@]}))
    printf '  %s: %s units\n' "$m" "${#list[@]}"
    bash tools/module_deps.sh "$m" "$m" /objects /bmis "${list[@]}" > /dev/null || names_bad=1
done
if [ $names_bad = 0 ]; then
    echo "check_modules: $units interface units, each named for its path"
else
    status=1
fi
exit $status
