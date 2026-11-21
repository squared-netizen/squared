#!/usr/bin/env bash
# tools/check_toolchain.sh [SQMAKE]: build/toolchain.lua (sqmake's) and
# build/toolchain.sh (make's) describe the same toolchain. Both are run on a
# fake layout (a stub clang that answers for its version, and the files each
# part must have), at the default API level and another, and must agree on
# every path and on the fingerprint (which covers the version lines and
# every flag). Run it after changing either file; SQMAKE is sqmake 0.4.0 or
# later (default: sqmake on PATH).
#
# Until make goes (sqmake phase 4) the toolchain is written twice; this is
# what keeps the two one.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SQMAKE=${1:-sqmake}
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQMAKE" >/dev/null || { echo "check_toolchain: no sqmake (give its path)" >&2; exit 2; }
command -v "$SQEDIT" >/dev/null || { echo "check_toolchain: no sqedit" >&2; exit 2; }
work=$(mktemp -d)
trap 'rm -r -f "$work"' EXIT

pfx=$work/pfx
mkdir -p "$pfx/bin" "$pfx/share/sysroot/usr/include" "$pfx/include/c++/v1" "$pfx/share/libc++/v1"
cat > "$pfx/bin/clang" <<'SH'
#!/bin/sh
case $1 in -dumpversion) echo "23.1.5" ;; --version) echo "clang version 23.1.5 (check_toolchain)"; echo "Target: x" ;; esac
SH
chmod +x "$pfx/bin/clang"
cp "$pfx/bin/clang" "$pfx/bin/clang++"
printf '#  define _LIBCPP_VERSION 230105\n' > "$pfx/include/c++/v1/__config"
: > "$pfx/share/libc++/v1/std.cppm"

failed=0
for api in 28 30; do
  mkdir -p "$pfx/lib/aarch64-unknown-linux-android$api"
  : > "$pfx/lib/aarch64-unknown-linux-android$api/libc++.a"
  sh_env=$(env -u SQ_LLVM_BIN -u SQ_SYSROOT -u SQ_RUNTIMES PREFIX="$pfx" SQ_API=$api bash "$ROOT/build/toolchain.sh" env)
  lua_json=$(echo "{\"sq\": 1, \"command\": \"toolchain\", \"args\": {\"file\": \"$ROOT/build/toolchain.lua\"}}" |
    env -u SQ_LLVM_BIN -u SQ_SYSROOT -u SQ_RUNTIMES PREFIX="$pfx" SQ_API=$api "$SQMAKE" call)
  sh_value() { printf '%s\n' "$sh_env" | "$SQEDIT" pick -x "\"$1=\" {.*}"; }
  lua_value() { printf '%s' "$lua_json" > "$work/response.json"; "$SQEDIT" json "result.$1" "$work/response.json"; }
  for pair in SQ_LLVM_BIN:llvm_bin SQ_SYSROOT:sysroot SQ_RUNTIMES:runtimes SQ_TRIPLE:target \
              SQ_STD_CPPM:std_cppm SQ_TOOLCHAIN_FINGERPRINT:fingerprint; do
    a=$(sh_value "${pair%%:*}") b=$(lua_value "${pair#*:}")
    if [ "$a" = "$b" ]; then
      printf '  same  %-26s %s (API %s)\n' "${pair%%:*}" "$a" "$api"
    else
      printf '  DIFF  %-26s toolchain.sh %s, toolchain.lua %s (API %s)\n' "${pair%%:*}" "$a" "$b" "$api"
      failed=1
    fi
  done
done
[ $failed = 0 ] && echo "check_toolchain: toolchain.sh and toolchain.lua agree" || { echo "check_toolchain: they differ" >&2; exit 1; }
