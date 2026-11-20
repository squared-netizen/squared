#!/usr/bin/env bash
# build/toolchain.sh: the SDK's toolchain, defined once ("One C++23
# toolchain: transition plan", 2026-11-19). Every C++ module the SDK builds
# is built by it: clang 23, libc++ 23 and bionic, from sqterm's own
# packages (llvm, runtimes, sysroot) on the phone, and from apt.llvm.org's
# clang 23 with the same runtimes and sysroot on CI. A compiled module (a
# BMI) is tied to the compiler, its library and the flags that change the
# language or the target, so nothing here is a project's or a target's to
# choose.
#
#   toolchain.sh env              KEY=value lines: what follows, resolved
#   toolchain.sh wrappers DIR     DIR/clang and DIR/clang++ for the target,
#                                 every flag below on every call, and
#                                 DIR/resource-dir (clang's headers, the
#                                 runtimes' builtins)
#   toolchain.sh fingerprint      16 hex digits: the compiler's version,
#                                 libc++'s, the target and the flags (not
#                                 the paths); a folder of BMIs is named by it
#
# Where things are (the environment, else the defaults):
#
#   SQ_LLVM_BIN    clang 23's bin/: in sqterm, $PREFIX/bin; elsewhere
#                  /usr/lib/llvm-23/bin
#   SQ_SYSROOT     bionic (the sysroot package): $PREFIX/share/sysroot
#   SQ_RUNTIMES    the runtimes package's prefix (lib/<triple>/libc++.a,
#                  include/c++/v1, share/libc++/v1/std.cppm, lib/clang/*):
#                  $PREFIX
#   SQ_API         the Android API level: 28
#
# The flags, all of them on every compile and link:
#
#   --target=aarch64-unknown-linux-android$SQ_API --sysroot=$SQ_SYSROOT
#   -resource-dir   clang's own headers with the runtimes' builtins
#   -rtlib=compiler-rt -unwindlib=libunwind
#                   not link flags only: they decide target features
#                   (outline atomics, function multiversioning), and a BMI
#                   made without them is refused by a compile with them
#                   (sqterm #21)
#   -pthread        clang refuses a BMI made with threads by a compile
#                   without, and the other way
#   -D__BIONIC_CTYPE_INLINE=inline
#                   the std module exports bionic's ctype functions, which
#                   must be plain inline
#   C++ alone: -std=c++23 -stdlib=libc++ -nostdinc++, the runtimes' libc++
#   headers (its __config_site first)
set -euo pipefail
# sqedit (github.com/squared-netizen/sqedit) does what sed and grep did here.
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQEDIT" >/dev/null ||
    { echo "toolchain.sh: needs sqedit, on PATH or as $SQEDIT (github.com/squared-netizen/sqedit)" >&2; exit 2; }

SQ_API=${SQ_API:-28}
TRIPLE=aarch64-unknown-linux-android$SQ_API
if [ -z "${SQ_LLVM_BIN:-}" ]; then
  if [ -n "${PREFIX:-}" ] && [ -x "$PREFIX/bin/clang++" ]; then SQ_LLVM_BIN=$PREFIX/bin; else SQ_LLVM_BIN=/usr/lib/llvm-23/bin; fi
fi
SQ_SYSROOT=${SQ_SYSROOT:-${PREFIX:-/nonexistent}/share/sysroot}
SQ_RUNTIMES=${SQ_RUNTIMES:-${PREFIX:-/nonexistent}}
MAJOR=23

fail() { printf 'toolchain: %s\n' "$*" >&2; exit 1; }

check() {
  [ -x "$SQ_LLVM_BIN/clang++" ] || fail "no clang at $SQ_LLVM_BIN (set SQ_LLVM_BIN; in sqterm: pkg install llvm)"
  local version
  version=$("$SQ_LLVM_BIN/clang" -dumpversion)
  [ "${version%%.*}" = "$MAJOR" ] || fail "clang $version at $SQ_LLVM_BIN: the toolchain is clang $MAJOR"
  [ -d "$SQ_SYSROOT/usr/include" ] || fail "no sysroot at $SQ_SYSROOT (set SQ_SYSROOT; in sqterm: pkg install sysroot)"
  [ -f "$SQ_RUNTIMES/lib/$TRIPLE/libc++.a" ] || fail "no libc++ for $TRIPLE in $SQ_RUNTIMES (set SQ_RUNTIMES; in sqterm: pkg install runtimes)"
}

# What changes a BMI, less the paths (which differ between the phone and CI;
# a BMI never travels between them anyway).
COMMON="--target=$TRIPLE -rtlib=compiler-rt -unwindlib=libunwind -pthread -D__BIONIC_CTYPE_INLINE=inline"
CXX_ONLY="-std=c++23 -stdlib=libc++ -nostdinc++"
LINK="-fuse-ld=lld -static-libstdc++ -Wl,-z,max-page-size=16384"

std_cppm() { ls "$SQ_RUNTIMES"/share/libc++/v*/std.cppm 2>/dev/null | head -n 1; }

libcxx_version() {
  { "$SQEDIT" pick -x '"#" %s* "define" %s* "_LIBCPP_VERSION" %s* {%d*} .*' "$SQ_RUNTIMES/include/c++/v1/__config" || [ $? = 1 ]; } | head -n 1
}

fingerprint() {
  {
    "$SQ_LLVM_BIN/clang" --version | head -n 1
    echo "libc++ $(libcxx_version)"
    echo "$COMMON"
    echo "$CXX_ONLY"
    echo "$LINK"
  } | sha256sum | cut -c 1-16
}

case ${1:-} in
  env)
    check
    printf 'SQ_LLVM_BIN=%s\nSQ_SYSROOT=%s\nSQ_RUNTIMES=%s\nSQ_TRIPLE=%s\nSQ_STD_CPPM=%s\n' \
      "$SQ_LLVM_BIN" "$SQ_SYSROOT" "$SQ_RUNTIMES" "$TRIPLE" "$(std_cppm)"
    printf 'SQ_TOOLCHAIN_FLAGS=%s\nSQ_TOOLCHAIN_CXXFLAGS=%s\nSQ_TOOLCHAIN_LDFLAGS=%s\n' "$COMMON" "$CXX_ONLY" "$LINK"
    printf 'SQ_TOOLCHAIN_FINGERPRINT=%s\n' "$(fingerprint)"
    ;;
  wrappers)
    check
    dir=${2:?"toolchain.sh wrappers DIR"}
    mkdir -p "$dir"
    dir=$(cd "$dir" && pwd -P)
    rd=$dir/resource-dir
    rm -rf "$rd" && mkdir -p "$rd"
    cp -R "$("$SQ_LLVM_BIN/clang" -print-resource-dir)/include" "$rd/"
    cp -R "$SQ_RUNTIMES"/lib/clang/*/lib "$rd/"
    # Not a BMI's: a link flag on a compile is not worth a warning on every
    # call (each wrapper gives -rtlib and the rest to compiles and links alike).
    # The runtimes' libraries on every link: -unwindlib=libunwind wants
    # libunwind.a for C as for C++.
    paths="--sysroot=$SQ_SYSROOT -resource-dir=$rd -L$SQ_RUNTIMES/lib/$TRIPLE -Wno-unused-command-line-argument"
    printf '#!/bin/sh\n# The SDK toolchain (squared build/toolchain.sh), fingerprint %s.\nexec "%s" %s %s "$@"\n' \
      "$(fingerprint)" "$SQ_LLVM_BIN/clang" "$COMMON" "$paths" > "$dir/clang"
    printf '#!/bin/sh\n# The SDK toolchain (squared build/toolchain.sh), fingerprint %s.\nexec "%s" %s %s %s -isystem "%s" -isystem "%s" "$@"\n' \
      "$(fingerprint)" "$SQ_LLVM_BIN/clang++" "$COMMON" "$paths" "$CXX_ONLY" \
      "$SQ_RUNTIMES/include/$TRIPLE/c++/v1" "$SQ_RUNTIMES/include/c++/v1" > "$dir/clang++"
    chmod +x "$dir/clang" "$dir/clang++"
    ;;
  fingerprint)
    check
    fingerprint
    ;;
  *)
    "$SQEDIT" lines 2..24 "$0" >&2
    exit 2
    ;;
esac
