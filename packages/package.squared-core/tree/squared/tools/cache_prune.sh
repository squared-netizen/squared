#!/usr/bin/env bash
# tools/cache_prune.sh <cache> <megabytes>
#
# Keep the build cache (tools/cached_cc.sh) under a size: when it holds more
# than <megabytes>, delete what was used least lately, objects and lists
# alike, until it holds nine tenths of it (so that one more build does not
# prune again). The root Makefile runs it after every build, with
# SQUARED_CACHE and SQUARED_CACHE_SIZE (build/common.mk).
#
# Least lately used: tools/cached_cc.sh touches an object and its list each
# time it takes them, so a file's date is when it last saved a compile.
#
#   tools/cache_prune.sh ~/sqsysroot/.sqpg/cache 500

set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: cache_prune.sh <cache> <megabytes>" >&2
    exit 2
fi
cache=$1
megabytes=$2
case $megabytes in
    '' | *[!0-9]*)
        echo "cache_prune.sh: the size is in megabytes, a whole number: $megabytes" >&2
        exit 2
        ;;
esac
[ -d "$cache" ] || exit 0

cap=$((megabytes * 1024))
used=$(du -sk "$cache" | cut -f1)
[ "$used" -gt "$cap" ] || exit 0
goal=$((cap * 9 / 10))

removed=0
while read -r _ size file; do
    [ "$used" -gt "$goal" ] || break
    rm -f "$file"
    used=$((used - (size + 1023) / 1024))
    removed=$((removed + 1))
done < <(find "$cache" -type f -exec stat -c '%Y %s %n' {} + | sort -n)

# A source whose every list went: its directory too.
find "$cache/manifests" -mindepth 1 -maxdepth 1 -type d -empty -exec rmdir {} + 2>/dev/null || true

printf '  %-4s %s\n' cache "pruned $removed files, to $((used / 1024)) of $megabytes MB ($cache)"
