#!/usr/bin/env bash
# tools/check_modules.sh: what `sqmake headers` checks since squared is modules
# (7b.4). Until then it compiled every header on its own; a module interface
# compiles on its own by construction, so that check went (the user's call,
# 2026-10-27). What can still go wrong without a compile error:
#
#   1. The pinned FreeType sources (third_party/freetype/, since 12.3a)
#      edited or missing: each file's sha256 against PINNED's. libc++'s std
#      module sources were pinned the same way from 2026-11-02 until
#      2026-11-20, when std came to be built from the libc++ installed
#      ("One C++23 toolchain").
#   2. A module imports one its library does not depend on, directly or
#      through another (build/squared.lua: `module("gui", {"app", ...})`).
#      sqmake orders the compiles by the imports, but the link takes the
#      libraries the dependencies give: the import would compile, and a
#      program using it fail to link. (Under make, the top Makefile's order
#      lines, which make -j needed for the same reason.)
#   3. A module's files naming another module than the one their path says
#      (tools/module_deps.sh checks it as it builds; here, for every module
#      at once, without building).
#
# Exit status: 0 when all pass, 1 when one fails, 2 on a usage error.

set -euo pipefail
# sqedit (github.com/squared-netizen/sqedit) does what sed, grep and awk did here.
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQEDIT" >/dev/null ||
    { echo "check_modules.sh: needs sqedit, on PATH or as $SQEDIT (github.com/squared-netizen/sqedit)" >&2; exit 2; }

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
status=0

# --- 1 -----------------------------------------------------------------------
pinned=third_party/freetype/PINNED
if [ ! -f "$pinned" ]; then
    echo "check_modules: no third_party/freetype/PINNED: pin FreeType's sources (tools/pin_freetype.sh)" >&2
    status=1
elif ! bad=$(cd third_party/freetype && "$SQEDIT" match -x '[0-9a-f]+ "  " .*' PINNED | sha256sum -c --quiet 2>&1); then
    printf '%s\n' "$bad" | head -5 | "$SQEDIT" sub -x '{.*}' '  {1}' >&2
    echo "check_modules: third_party/freetype/ is not what PINNED says: pin again (tools/pin_freetype.sh), never edit it" >&2
    status=1
else
    files=$("$SQEDIT" match -cx '[0-9a-f]+ "  " .*' "$pinned")
    echo "check_modules: freetype: $files pinned files as PINNED says ($("$SQEDIT" pick -x '"freetype: " {.*}' "$pinned"))"
fi

# --- 2 -----------------------------------------------------------------------
# build/squared.lua's module lines, `module("<module>", {"<module>", ...})`,
# as "module dep" pairs (squared's own; the C libraries, unquoted, left out),
# and files, a library of its own there that depends on nothing.
lines=$("$SQEDIT" pick -x '"module(" ["] {[a-z_]+} ["] ", {" {[^}]*} "}" .*' build/squared.lua)
modules="files $(printf '%s\n' "$lines" | while IFS=$'\t' read -r m _; do printf '%s ' "$m"; done)"
pairs=$(printf '%s\n' "$lines" | while IFS=$'\t' read -r from deps; do
    for d in ${deps//,/ }; do
        case $d in '"'*'"') printf '%s %s\n' "$from" "${d//\"/}" ;; esac
    done
done)
# reaches <from> <to>: whether <from>'s library depends on <to>'s.
reaches() {
    local seen=" $1 " todo="$1" next m d
    while [ -n "$todo" ]; do
        next=""
        for m in $todo; do
            for d in $(printf '%s\n' "$pairs" | while read -r a b; do if [ "$a" = "$m" ]; then echo "$b"; fi; done); do
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
    imported=$(find "$m" -type f \( -name '*.cppm' -o -name '*.cpp' -o -name '*.hpp' \) \
        -exec "$SQEDIT" pick -x '("export ")? "import sq." {[a-z_]+} ";" .*' {} + | LC_ALL=C sort -u || true)
    for i in $imported; do
        [ "$i" = std ] || [ "$i" = "$m" ] && continue
        if ! reaches "$m" "$i"; then
            echo "check_modules: $m imports sq.$i, and its library does not depend on $i's (add \"$i\" to module(\"$m\", ...) in build/squared.lua)" >&2
            order_bad=1
        fi
    done
done
if [ $order_bad = 0 ]; then
    echo "check_modules: the libraries' dependencies cover every import ($(printf '%s\n' $modules | wc -l | tr -d ' ') modules)"
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
    mapfile -t list < <(cd "$m" && find . -name '*.cppm' -not -path './test/*' | "$SQEDIT" pick -x '"./" {.*}' | LC_ALL=C sort)
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
