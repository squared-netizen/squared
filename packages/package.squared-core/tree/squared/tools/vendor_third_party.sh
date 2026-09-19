#!/usr/bin/env bash
# Unpack the vendored third-party sources squared builds from.
#
#   ./tools/vendor_third_party.sh
#
# Idempotent: a dependency already unpacked is left alone. Nothing is
# downloaded; the tarballs must already be in third_party/.
#
# yyjson is vendored rather than taken from a system package because this tree
# is built with the NDK toolchain for an APK. A package installed for the host
# sysroot is the wrong target and would fail at link time, or worse, at run
# time on a device.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
vendor="$root/third_party"
cd "$vendor"

unpack_yyjson() {
  # Already unpacked wins, so the tarball can be deleted afterwards without
  # breaking a later run.
  if [ -f yyjson/yyjson.h ] && [ -f yyjson/yyjson.c ]; then
    echo "third_party: yyjson already unpacked"
    return 0
  fi

  archive="$(ls yyjson-*.tar.gz 2>/dev/null | head -n 1 || true)"
  if [ -z "$archive" ]; then
    echo "third_party: no yyjson-*.tar.gz found in $vendor" >&2
    echo "third_party: put the release tarball there and re-run" >&2
    return 1
  fi

  echo "third_party: unpacking $archive"
  work="$(mktemp -d "$vendor/.unpack.XXXXXX")"
  trap 'rm -rf "$work"' RETURN
  tar -xzf "$archive" -C "$work"

  header="$(find "$work" -name yyjson.h -print | head -n 1)"
  source_file="$(find "$work" -name yyjson.c -print | head -n 1)"
  if [ -z "$header" ] || [ -z "$source_file" ]; then
    echo "third_party: yyjson.c or yyjson.h not found inside $archive" >&2
    return 1
  fi

  mkdir -p yyjson
  cp "$header" yyjson/yyjson.h
  cp "$source_file" yyjson/yyjson.c

  license="$(find "$work" -iname 'LICENSE*' -print | head -n 1 || true)"
  if [ -n "$license" ]; then
    cp "$license" yyjson/LICENSE
  fi

  echo "third_party: yyjson -> third_party/yyjson"
}

unpack_yyjson

check_stb() {
  # stb_image.h is a single public-domain header, committed rather than
  # tarballed. It is checked here so a missing one is reported by the same
  # command that reports every other dependency.
  if [ -f stb/stb_image.h ]; then
    echo "third_party: stb_image.h present"
    return 0
  fi
  echo "third_party: third_party/stb/stb_image.h is missing" >&2
  echo "third_party: graphics2d cannot decode images without it" >&2
  return 1
}

check_stb

# Lua is present for the scripting layer but nothing builds it yet. Left
# packed on purpose so it does not look wired up when it is not.
if ls lua-*.tar.gz >/dev/null 2>&1; then
  echo "third_party: lua tarball present, not unpacked (no build wired yet)"
fi

echo "third_party: done"
