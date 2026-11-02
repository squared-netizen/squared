#!/usr/bin/env bash
# tools/pin_libcxx_modules.sh: pin libc++'s std module sources into
# third_party/libcxx/, from which every build makes `import std;` (C++23).
#
#   tools/pin_libcxx_modules.sh <share/libc++/v1>
#
# <share/libc++/v1> is where a libc++ installation keeps its module sources:
# std.cppm and std/*.inc. The NDK has them at
# toolchains/llvm/prebuilt/<host>/share/libc++/v1; an LLVM release at
# share/libc++/v1. They must come from the same libc++ as the headers squared
# is compiled against (the NDK's for Android, Termux's for the phone's own
# tools: both libc++ 21.0.0 on 2026-11-02), or the std module names
# something the headers do not have.
#
# What it writes, replacing what was pinned before:
#
#   third_party/libcxx/std.cppm      as found
#   third_party/libcxx/std/*.inc     as found
#   third_party/libcxx/PINNED        where they came from, the libc++ version
#                                    of the headers beside them, and a sha256
#                                    for each file (make headers checks them)
#
# README.md and LICENSE.TXT there are squared's, and left alone. The pinned
# files are committed, so a checkout builds without the NDK's copy; pin again
# when the NDK's libc++ moves (the build refuses headers of another major
# version than PINNED names).
#
# Exit status: 0 pinned, 1 not pinned (nothing changed), 2 usage.

set -euo pipefail

if [ $# -ne 1 ] || [ "$1" = -h ] || [ "$1" = --help ]; then
    sed -n '2,29p' "$0" | sed 's/^# \{0,1\}//'
    [ $# -eq 1 ] && exit 0
    exit 2
fi

root="$(cd "$(dirname "$0")/.." && pwd)" # squared/
from="$(cd "$1" 2>/dev/null && pwd)" || { echo "pin_libcxx_modules: no directory $1" >&2; exit 1; }
to="$root/third_party/libcxx"

if [ ! -f "$from/std.cppm" ] || ! ls "$from"/std/*.inc > /dev/null 2>&1; then
    echo "pin_libcxx_modules: $from has no std.cppm and std/*.inc: not a libc++ module directory" >&2
    exit 1
fi

# The libc++ version of the headers installed beside the sources: the NDK's
# sysroot, or an LLVM installation's include/.
version=""
for config in "$from/../../../sysroot/usr/include/c++/v1/__config" \
              "$from/../../../include/c++/v1/__config"; do
    if [ -f "$config" ]; then
        version=$(sed -n 's/^#[ ]*define _LIBCPP_VERSION \([0-9][0-9]*\).*/\1/p' "$config" | head -n 1)
        [ -n "$version" ] && break
    fi
done
if [ -z "$version" ]; then
    echo "pin_libcxx_modules: no libc++ headers found beside $from (looked for sysroot/usr/include/c++/v1 and include/c++/v1)" >&2
    exit 1
fi

# Shown with ~ and $PREFIX, so PINNED reads the same on every phone.
shown=$from
[ -n "${PREFIX:-}" ] && shown=${shown/#"$PREFIX"/\$PREFIX}
shown=${shown/#"$HOME"/\~}

mkdir -p "$to"
rm -rf "$to/std" "$to/std.cppm" "$to/PINNED.new"
mkdir -p "$to/std"
cp "$from/std.cppm" "$to/std.cppm"
cp "$from"/std/*.inc "$to/std/"
chmod 644 "$to/std.cppm" "$to"/std/*.inc

{
    echo "# libc++'s std module sources, pinned by tools/pin_libcxx_modules.sh."
    echo "# Do not edit the files: pin again. make headers checks the sums."
    echo "from: $shown"
    echo "libc++: $version"
    echo "pinned: $(date +%Y-%m-%d)"
    (cd "$to" && find std.cppm std -type f | LC_ALL=C sort | xargs sha256sum)
} > "$to/PINNED.new"
mv -f "$to/PINNED.new" "$to/PINNED"

count=$(find "$to/std" -name '*.inc' | wc -l | tr -d ' ')
echo "pin_libcxx_modules: libc++ $version: std.cppm and $count std/*.inc from $shown"
echo "pin_libcxx_modules: into third_party/libcxx/ (commit it with PINNED)"
