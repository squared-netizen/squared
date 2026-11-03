#!/usr/bin/env bash
# tools/check_modules.sh: what `make headers` checks since squared is modules
# (7b.4). Until then it compiled every header on its own; a module interface
# compiles on its own by construction, so that check went (the user's call,
# 2026-10-27). What can still go wrong without a compile error:
#
#   1. The pinned std module sources (third_party/libcxx/) edited or
#      missing: each file's sha256 against PINNED's. FreeType's too
#      (third_party/freetype/, since 12.3a), the same way. Since 2026-11-02, when
#      `import std;` (C++23, libc++'s) replaced squared's sq.std; until then
#      this checked sq.std named every std:: name the sources used.
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
pinned=third_party/libcxx/PINNED
if [ ! -f "$pinned" ]; then
    echo "check_modules: no third_party/libcxx/PINNED: pin libc++'s std module sources (tools/pin_libcxx_modules.sh)" >&2
    status=1
elif ! bad=$(cd third_party/libcxx && grep -E '^[0-9a-f]{64}  ' PINNED | sha256sum -c --quiet 2>&1); then
    printf '%s\n' "$bad" | head -5 | sed 's/^/  /' >&2
    echo "check_modules: third_party/libcxx/ is not what PINNED says: pin again (tools/pin_libcxx_modules.sh), never edit it" >&2
    status=1
else
    files=$(grep -cE '^[0-9a-f]{64}  ' "$pinned")
    echo "check_modules: std: $files pinned files as PINNED says ($(sed -n 's/^libc++: //p' "$pinned"))"
fi

# --- 1b: FreeType, pinned the same way (12.3a) ------------------------------
pinned=third_party/freetype/PINNED
if [ ! -f "$pinned" ]; then
    echo "check_modules: no third_party/freetype/PINNED: pin FreeType's sources (tools/pin_freetype.sh)" >&2
    status=1
elif ! bad=$(cd third_party/freetype && grep -E '^[0-9a-f]{64}  ' PINNED | sha256sum -c --quiet 2>&1); then
    printf '%s\n' "$bad" | head -5 | sed 's/^/  /' >&2
    echo "check_modules: third_party/freetype/ is not what PINNED says: pin again (tools/pin_freetype.sh), never edit it" >&2
    status=1
else
    files=$(grep -cE '^[0-9a-f]{64}  ' "$pinned")
    echo "check_modules: freetype: $files pinned files as PINNED says ($(sed -n 's/^freetype: //p' "$pinned"))"
fi

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
