#!/usr/bin/env bash
# tools/inputs_sum.sh <list> <sum> <public root>: what the files of a module's
# .inputs list hold, written once, for the build cache's keys.
#
# A .inputs list (build/modules.mk; std's from tools/std_bmi.sh) names the
# interface sources whose BMIs a compile may import: 233 files for a unit of
# gui or stage. tools/cached_cc.sh used to read every one of them for every
# object it looked up, about 90,000 files for a build that compiled nothing
# (44 s on one job here, minutes on the phone). Now the list's maker writes
# this file beside it, one line a file, its md5 and its path with the
# workspace's directory written @ROOT@:
#
#   d41d8cd98f00b204e9800998ecf8427e  @ROOT@/squared/gui/label.cppm
#
# and a cache key reads the one .sum instead of the files. The .sum is
# written whenever its list is, which is whenever one of the files changes
# (they are the list's prerequisites), and before the list, so a list is
# never newer than its .sum.

set -euo pipefail
# sqedit (github.com/squared-netizen/sqedit) does what sed and grep did here.
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQEDIT" >/dev/null ||
    { echo "inputs_sum.sh: needs sqedit, on PATH or as $SQEDIT (github.com/squared-netizen/sqedit)" >&2; exit 2; }

if [ $# -ne 3 ]; then
    echo "usage: inputs_sum.sh <list> <sum> <public root>" >&2
    exit 2
fi
list=$1
sum=$2
root=${3%/}

# shellcheck disable=SC2046 # one path a line, none with spaces
{ [ ! -s "$list" ] || md5sum $(cat "$list"); } |
    { "$SQEDIT" sub -F "$root/" '@ROOT@/' || [ $? = 1 ]; } >"$sum.$$"
mv -f "$sum.$$" "$sum"
