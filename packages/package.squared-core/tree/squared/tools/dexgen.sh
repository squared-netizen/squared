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
# sqedit (github.com/squared-netizen/sqedit) does what sed, grep and awk did here.
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQEDIT" >/dev/null ||
    { echo "dexgen.sh: needs sqedit, on PATH or as $SQEDIT (github.com/squared-netizen/sqedit)" >&2; exit 2; }

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
src=$root/tools/dexgen/dexgen.cpp
dex=$root/tools/dexgen/shims.dex
out=$root/build/sqmake

mode=check
target=$dex
case ${1:-} in
    "") ;;
    --write) mode=write ;;
    --dump) mode=dump; target=${2:-$dex} ;;
    --self-test) mode=self-test ;;
    -h | --help) "$SQEDIT" lines '2.."set -euo"' "$0" | "$SQEDIT" match -vx '"set -euo" .*'; exit 0 ;;
    *) echo "dexgen.sh: unknown option: $1 (see --help)" >&2; exit 2 ;;
esac

# dexgen imports std (C++23): built by the framework's build.lua, as the
# tests are, so it gets the std module (sqmake: $SQMAKE, or sqmake on PATH).
# Quiet unless it fails.
build() {
    local sqmake=${SQMAKE:-sqmake}
    command -v "$sqmake" >/dev/null ||
        { echo "dexgen.sh: needs sqmake, on PATH or as SQMAKE (github.com/squared-netizen/sqmake)" >&2; exit 2; }
    mkdir -p "$out"
    "$sqmake" -C "$root" build/sqmake/dexgen build/sqmake/dexgen-self-test >"$out/dexgen.log" 2>&1 ||
        { cat "$out/dexgen.log" >&2; echo "dexgen.sh: dexgen did not build (sqmake build/sqmake/dexgen)" >&2; exit 2; }
}

tmp=$(mktemp -d "${TMPDIR:-/tmp}/dexgen.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

case $mode in
    check)
        build
        "$out/dexgen" "$tmp/shims.dex" >/dev/null
        if [ ! -f "$dex" ]; then
            echo "dexgen.sh: no committed dex at ${dex#"$root"/}; run tools/dexgen.sh --write" >&2
            exit 1
        fi
        if cmp -s "$tmp/shims.dex" "$dex"; then
            echo "  shims.dex      regenerates byte for byte ($(wc -c <"$dex" | tr -d ' ') bytes, sha256 $(sha256sum "$dex" | cut -c1-16))"
            "$out/dexgen" --dump "$dex" | "$SQEDIT" match -x '"class " .*'
        else
            echo "dexgen.sh: the committed shims.dex differs from what dexgen writes now" >&2
            echo "  if the shim table changed on purpose: tools/dexgen.sh --write, then commit" >&2
            exit 1
        fi
        ;;
    write)
        build
        "$out/dexgen" "$dex"
        "$out/dexgen" --dump "$dex" | "$SQEDIT" lines 2..6
        ;;
    dump)
        build
        "$out/dexgen" --dump "$target"
        ;;
    self-test)
        build
        "$out/dexgen-self-test" "$tmp/test.dex"
        "$out/dexgen-self-test" --dump "$tmp/test.dex" >"$tmp/dump" ||
            { cat "$tmp/dump"; echo "dexgen.sh: the self-test dex does not read back" >&2; exit 1; }
        echo "  native lines   $("$SQEDIT" match -cF native "$tmp/dump")"
        "$SQEDIT" match -x '"class " .*' "$tmp/dump"
        echo "  self-test      ok"
        ;;
esac
