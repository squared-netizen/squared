#!/usr/bin/env bash
# tools/std_pch.sh [--check]
#
# Write build/std_pch.hpp: every standard header (<vector>, <cstdint>, ...)
# that squared's own sources and tests include, the list the build
# precompiles (build/common.mk, "the standard headers, precompiled").
#
#   tools/std_pch.sh            write it again
#   tools/std_pch.sh --check    say whether it is current; exit 1 when not
#
# A standard header is an include in angle brackets with no dot and no slash.
# third_party/ and build/ are not looked at.

set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
target="$root/build/std_pch.hpp"
check=0
case "${1:-}" in
    --check) check=1 ;;
    "") ;;
    *) echo "usage: tools/std_pch.sh [--check]" >&2; exit 2 ;;
esac

new="$(mktemp "${TMPDIR:-/tmp}/std_pch.XXXXXX")"
trap 'rm -f "$new"' EXIT

{
    cat <<'EOF'
// The standard headers squared includes, all of them, compiled once per flag
// set into a precompiled header that every squared source and test is
// compiled with (build/common.mk, "the standard headers, precompiled").
//
// Not a header to include: the build hands it to the compiler. A standard
// header missing here is still included where it is named, only not
// precompiled; tools/std_pch.sh --check says when this list is behind the
// sources (make headers runs it).
//
// Written by tools/std_pch.sh.

#pragma once

EOF
    (cd "$root" && grep -rhoE '^#include <[a-z_]+>' --include=*.hpp --include=*.cpp \
        --exclude-dir=third_party --exclude-dir=build . | LC_ALL=C sort -u)
} > "$new"

count=$(grep -c '^#include' "$new" || true)
if [ "$check" = 1 ]; then
    if cmp -s "$new" "$target"; then
        echo "std_pch: build/std_pch.hpp is current ($count standard headers precompiled)"
        exit 0
    fi
    echo "std_pch: build/std_pch.hpp is behind the sources; tools/std_pch.sh writes it again:"
    diff "$target" "$new" | sed -n 's/^[<>] #include/  &/p' || true
    exit 1
fi
cp "$new" "$target"
echo "std_pch: wrote build/std_pch.hpp ($count standard headers)"
