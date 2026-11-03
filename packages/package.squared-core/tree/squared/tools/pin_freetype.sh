#!/usr/bin/env bash
# tools/pin_freetype.sh: pin FreeType's sources into third_party/freetype/,
# from which the font module (font/) builds it (Sprint 12.3a).
#
#   tools/pin_freetype.sh <freetype source>
#
# <freetype source> is an unpacked FreeType release, or a checkout of its
# release tag (git clone --branch VER-2-15-0 https://github.com/freetype/
# freetype, or the .tar.xz from download.savannah.gnu.org). Its latest
# stable release, by the rule sqterm's tools follow.
#
# Only what a terminal's fonts need is taken: the public headers
# (include/), and the sources of the modules squared builds: base, truetype,
# sfnt, smooth, autofit, cff, psaux, pshinter, psnames (TrueType and
# OpenType/CFF outlines, the anti-aliased rasterizer, the auto-hinter),
# .c and .h files only. Which of them are compiled, and with which options,
# is font/src/freetype/ (squared's own), not here.
#
# What it writes, replacing what was pinned before:
#
#   third_party/freetype/include/, src/<module>/   as found
#   third_party/freetype/LICENSE.TXT, FTL.TXT      FreeType's licence
#   third_party/freetype/PINNED                    the version, where from,
#                                                  and a sha256 for each file
#                                                  (make headers checks them)
#
# README.md there is squared's, and left alone. Commit it all with PINNED.
#
# Exit status: 0 pinned, 1 not pinned (nothing changed), 2 usage.

set -euo pipefail

if [ $# -ne 1 ] || [ "$1" = -h ] || [ "$1" = --help ]; then
    sed -n '2,29p' "$0" | sed 's/^# \{0,1\}//'
    [ $# -eq 1 ] && exit 0
    exit 2
fi

modules=(base truetype sfnt smooth autofit cff psaux pshinter psnames)

root="$(cd "$(dirname "$0")/.." && pwd)" # squared/
from="$(cd "$1" 2>/dev/null && pwd)" || { echo "pin_freetype: no directory $1" >&2; exit 1; }
to="$root/third_party/freetype"

header="$from/include/freetype/freetype.h"
if [ ! -f "$header" ] || [ ! -f "$from/include/ft2build.h" ]; then
    echo "pin_freetype: $from has no include/freetype/freetype.h: not a FreeType source tree" >&2
    exit 1
fi
for module in "${modules[@]}"; do
    if [ ! -d "$from/src/$module" ]; then
        echo "pin_freetype: $from has no src/$module" >&2
        exit 1
    fi
done
part() { sed -n "s/^#define FREETYPE_$1 *\\([0-9][0-9]*\\).*/\\1/p" "$header" | head -n 1; }
version="$(part MAJOR).$(part MINOR).$(part PATCH)"
case $version in
    [0-9]*.[0-9]*.[0-9]*) ;;
    *) echo "pin_freetype: no version in $header" >&2; exit 1 ;;
esac

# Where from, as PINNED says it: the git tag and commit when it is a
# checkout, the directory (with ~ and $PREFIX) otherwise.
shown=$from
[ -n "${PREFIX:-}" ] && shown=${shown/#"$PREFIX"/\$PREFIX}
shown=${shown/#"$HOME"/\~}
if commit=$(git -C "$from" rev-parse HEAD 2>/dev/null); then
    tag=$(git -C "$from" describe --tags --exact-match 2>/dev/null || echo "untagged")
    shown="$shown (git $tag, $commit)"
fi

# Into a fresh directory beside it, then renamed: a pin that stops half way
# leaves the old one whole.
work="$to.new.$$"
rm -rf "$work"
mkdir -p "$work/src"
trap 'rm -rf "$work"' EXIT
cp -R "$from/include" "$work/include"
for module in "${modules[@]}"; do
    mkdir -p "$work/src/$module"
    find "$from/src/$module" -maxdepth 1 -type f \( -name '*.c' -o -name '*.h' \) \
        -exec cp {} "$work/src/$module/" \;
done
cp "$from/LICENSE.TXT" "$work/LICENSE.TXT"
cp "$from/docs/FTL.TXT" "$work/FTL.TXT"
find "$work" -type f -exec chmod 644 {} +
[ -f "$to/README.md" ] && cp "$to/README.md" "$work/README.md"

{
    echo "# FreeType's sources, pinned by tools/pin_freetype.sh."
    echo "# Do not edit the files: pin again. make headers checks the sums."
    echo "from: $shown"
    echo "freetype: $version"
    echo "modules: ${modules[*]}"
    echo "pinned: $(date +%Y-%m-%d)"
    (cd "$work" && find include src LICENSE.TXT FTL.TXT -type f | LC_ALL=C sort | xargs sha256sum)
} > "$work/PINNED"

rm -rf "$to.old.$$"
[ -d "$to" ] && mv "$to" "$to.old.$$"
mv "$work" "$to"
rm -rf "$to.old.$$"
trap - EXIT

count=$(grep -cE '^[0-9a-f]{64}  ' "$to/PINNED")
echo "pin_freetype: FreeType $version: $count files from $shown"
echo "pin_freetype: into third_party/freetype/ (commit it with PINNED)"
