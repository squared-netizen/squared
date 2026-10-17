#!/usr/bin/env bash
# The shim dex: build dexgen, and check, rewrite or print tools/dexgen/shims.dex.
#
#   tools/dexgen.sh               build dexgen, regenerate the dex into a temporary
#                                 file and compare it byte for byte with the committed
#                                 one (exit 1 if they differ)
#   tools/dexgen.sh --write       regenerate the committed dex (after changing the
#                                 shim table in tools/dexgen/dexgen.cpp)
#   tools/dexgen.sh --dump [dex]  print a dex: header, checksums, tables, classes,
#                                 methods, instructions (default: the committed one)
#   tools/dexgen.sh --self-test   build with a test shim that uses native methods and
#                                 parameter lists, write it, and check it reads back
#
# The shims are classes Android calls into, with no logic of their own: see
# docs/developer/shims.md. No javac, no d8, no JDK, no network.
#
# Exit codes: 0 good, 1 a check failed, 2 usage or build failure.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
src=$root/tools/dexgen/dexgen.cpp
dex=$root/tools/dexgen/shims.dex
out=$root/build/${BUILD:-release}/tools
cxx=${CXX:-c++}

mode=check
target=$dex
case ${1:-} in
    "") ;;
    --write) mode=write ;;
    --dump) mode=dump; target=${2:-$dex} ;;
    --self-test) mode=self-test ;;
    -h | --help) sed -n '2,/^set -euo/p' "$0" | sed '$d'; exit 0 ;;
    *) echo "dexgen.sh: unknown option: $1 (see --help)" >&2; exit 2 ;;
esac

build() { # build <binary> [defines...]
    local bin=$1
    shift
    mkdir -p "$out"
    if [ ! -x "$bin" ] || [ "$src" -nt "$bin" ] || [ $# -gt 0 ]; then
        echo "  building $(basename "$bin") with $($cxx --version 2>/dev/null | head -1)"
        "$cxx" -std=c++20 -O2 -Wall -Wextra -Wpedantic "$@" "$src" -o "$bin" ||
            { echo "dexgen.sh: dexgen did not build" >&2; exit 2; }
    fi
}

tmp=$(mktemp -d "${TMPDIR:-/tmp}/dexgen.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

case $mode in
    check)
        build "$out/dexgen"
        "$out/dexgen" "$tmp/shims.dex" >/dev/null
        if [ ! -f "$dex" ]; then
            echo "dexgen.sh: no committed dex at ${dex#"$root"/}; run tools/dexgen.sh --write" >&2
            exit 1
        fi
        if cmp -s "$tmp/shims.dex" "$dex"; then
            echo "  shims.dex      regenerates byte for byte ($(wc -c <"$dex" | tr -d ' ') bytes, sha256 $(sha256sum "$dex" | cut -c1-16))"
            "$out/dexgen" --dump "$dex" | sed -n '/^class /p'
        else
            echo "dexgen.sh: the committed shims.dex differs from what dexgen writes now" >&2
            echo "  if the shim table changed on purpose: tools/dexgen.sh --write, then commit" >&2
            exit 1
        fi
        ;;
    write)
        build "$out/dexgen"
        "$out/dexgen" "$dex"
        "$out/dexgen" --dump "$dex" | sed -n '2,6p'
        ;;
    dump)
        build "$out/dexgen"
        "$out/dexgen" --dump "$target"
        ;;
    self-test)
        build "$out/dexgen-self-test" -DDEXGEN_SELF_TEST
        "$out/dexgen-self-test" "$tmp/test.dex"
        "$out/dexgen-self-test" --dump "$tmp/test.dex" >"$tmp/dump" ||
            { cat "$tmp/dump"; echo "dexgen.sh: the self-test dex does not read back" >&2; exit 1; }
        grep -c 'native' "$tmp/dump" | sed 's/^/  native lines   /'
        sed -n '/^class /p' "$tmp/dump"
        echo "  self-test      ok"
        ;;
esac
