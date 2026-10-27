#!/usr/bin/env bash
# tools/test_cc.sh <compiler> <arguments...>
#
# Build a test (or a small host tool) as one compile-and-link command would,
# and write <output>.d beside it: the headers each of its sources included,
# so make rebuilds the binary when one of those changes, and only then.
#
# Why not one command: a compile-and-link of several sources with -MMD keeps
# only the last source's dependencies. So each source is compiled on its own,
# with -MMD, and the objects are linked.
#
# The objects are kept and shared (SQUARED_TEST_OBJECTS, a directory): the
# tests compile the same sources again and again (dev_log.cpp into 13 of
# them, the Console's sources into 6 each; 230 compiles for 39 binaries). An
# object is used again when it was compiled from the same source with the
# same flags, and nothing it includes is newer than it; else it is compiled
# again. Each is written beside its final name and renamed into place, so two
# tests building at once under make -j cannot leave half of one.
#
# The arguments are the ones the compile-and-link took: sources (.cpp, .c),
# -o <output>, link inputs (.a, .o, .so, -l, -L, -Wl,), and everything else,
# which is a compile flag (and given to the link too: -fsanitize, -pthread).
#
# SQUARED_TEST_PCH: a precompiled header for the C++ sources (build/common.mk);
# empty or unset, none. SQUARED_TEST_OBJECTS: where the objects are kept;
# empty or unset, beside the output, not shared.

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "usage: test_cc.sh <compiler> <arguments...>" >&2
    exit 2
fi
cxx=$1
shift

out=
sources=()
flags=()
link=()
while [ $# -gt 0 ]; do
    case $1 in
        -o)
            out=$2
            shift 2
            continue
            ;;
        *.cpp | *.cc | *.c) sources+=("$1") ;;
        *.a | *.o | *.so) link+=("$1") ;;
        -l* | -L* | -Wl,*) link+=("$1") ;;
        *) flags+=("$1") ;;
    esac
    shift
done

if [ -z "$out" ] || [ ${#sources[@]} -eq 0 ]; then
    echo "test_cc.sh: needs -o <output> and at least one source" >&2
    exit 2
fi

pch=()
if [ -n "${SQUARED_TEST_PCH:-}" ]; then
    # Made with -pthread (build/common.mk): every source must say the same.
    pch=(-include-pch "$SQUARED_TEST_PCH" -pthread)
fi

# The objects' directory: one per compiler, flags and precompiled header, so
# an object is only ever reused for exactly what made it.
key=$(printf '%s\n' "$cxx" "${flags[@]}" "${pch[@]}" | cksum | cut -d' ' -f1)
store="${SQUARED_TEST_OBJECTS:-$out.parts}/$key"
mkdir -p "$store"

# fresh <object> <deps>: the object exists, and nothing its .d names is newer.
fresh() {
    [ -f "$1" ] && [ -f "$2" ] || return 1
    # A precompiled header made again (a new libc++): every object with it.
    if [ -n "${SQUARED_TEST_PCH:-}" ] && [ "$SQUARED_TEST_PCH" -nt "$1" ]; then
        return 1
    fi
    local dep
    # The .d's words: the target, a colon, then the files; -MP's empty rules
    # repeat them. Backslashes join lines.
    for dep in $(tr -d '\\' <"$2" | tr ' ' '\n' | grep -v ':$' || true); do
        [ -e "$dep" ] || return 1
        [ "$dep" -nt "$1" ] && return 1
    done
    return 0
}

objects=()
deps="$out.d.new.$$"
: >"$deps"
for source in "${sources[@]}"; do
    case $source in
        /*) absolute=$source ;;
        *) absolute="$PWD/$source" ;;
    esac
    name=$(printf '%s' "$absolute" | tr '/' '_')
    object="$store/$name.o"
    depfile="$store/$name.d"
    if ! fresh "$object" "$depfile"; then
        # A C source takes no C++ precompiled header.
        case $source in
            *.c) use=() ;;
            *) use=("${pch[@]}") ;;
        esac
        "$cxx" "${flags[@]}" "${use[@]}" -MMD -MP -MT SQUARED_OBJECT -MF "$depfile.$$" \
            -c "$source" -o "$object.$$"
        mv -f "$object.$$" "$object"
        mv -f "$depfile.$$" "$depfile"
    fi
    # The binary depends on what each object depends on: its .d, retargeted.
    sed "s|^SQUARED_OBJECT:|$out:|" "$depfile" >>"$deps"
    objects+=("$object")
done
threads=()
[ ${#pch[@]} -eq 0 ] || threads=(-pthread)
"$cxx" "${flags[@]}" "${threads[@]}" "${objects[@]}" "${link[@]}" -o "$out"
# Only once it linked: a failed build leaves the last good list, or none.
mv -f "$deps" "$out.d"
