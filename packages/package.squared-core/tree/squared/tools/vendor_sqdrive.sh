#!/usr/bin/env bash
# tools/vendor_sqdrive.sh SQDRIVE: copy sqdrive's library (sq.drive,
# src/drive.cppm) into third_party/sqdrive/, and the sqcart it was built and
# tested with (its third_party/sqcart/: sq.cart, its build.lua, and what it
# builds with) into third_party/sqcart/, and record where from.
#
#   tools/vendor_sqdrive.sh ../sqdrive
#
# The preferences' export and import (sq.preferences) are a cartridge packed
# and exploded through the Drive, as square's are; build/squared.lua builds
# both, sq.cart with squared's own sq.data (sqcart_settings.data). The copy is
# committed, so a checkout builds offline. Run this again when sqdrive
# changes, then build, test and commit third_party/ with both PINNEDs.
set -euo pipefail
SRC=${1:?"a sqdrive checkout"}
[ -f "$SRC/src/drive.cppm" ] && [ -f "$SRC/third_party/sqcart/v3/build.lua" ] ||
  { echo "no src/drive.cppm and third_party/sqcart/v3/build.lua under $SRC" >&2; exit 1; }
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)
OUT=$ROOT/third_party/sqdrive
rm -rf "$OUT" "$ROOT/third_party/sqcart"
mkdir -p "$OUT/src"
cp "$SRC/src/drive.cppm" "$OUT/src/"
cp "$SRC/LICENSE" "$OUT/"
cp -R "$SRC/third_party/sqcart" "$ROOT/third_party/sqcart"
commit=$(git -C "$SRC" rev-parse HEAD 2>/dev/null || echo unknown)
{
  echo "# sqdrive's library, copied by tools/vendor_sqdrive.sh (with its sqcart: ../sqcart/PINNED)."
  echo "# Do not edit: change it in sqdrive, then copy it again."
  echo "from: sqdrive $commit"
  echo "copied: $(date -u +%Y-%m-%d)"
  (cd "$OUT" && find src -type f | LC_ALL=C sort | xargs sha256sum)
} > "$OUT/PINNED"
echo "sqdrive copied into third_party/sqdrive ($commit), its sqcart into third_party/sqcart"
