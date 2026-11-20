#!/usr/bin/env bash
# tools/libcxx_modules.sh: where libc++'s own module sources are
# (share/libc++/v1: std.cppm and std/*.inc) for the libc++ a compiler and its
# flags compile against. One folder printed; tools/std_bmi.sh builds std from
# it, tools/docs.sh reads its list of headers.
#
#   libcxx_modules.sh <compiler> <flags>...
#
# The first that has them ("One C++23 toolchain", 2026-11-20: no pinned
# copy, so one std module per toolchain):
#
#   $SQ_STD_CPPM's folder          named outright (build/toolchain.sh env)
#   <prefix>/share/libc++/v1       where the headers are <prefix>/include/c++/v1
#                                  (sqterm's runtimes package, apt.llvm.org,
#                                  Termux), or <prefix>/sysroot/usr/include/c++/v1
#                                  (an NDK's)
#   <compiler>/../share/libc++/v1  beside the compiler (the NDK's)
#   the NDK's, for Termux's clang  $SQ_NDK, $ANDROID_NDK_HOME, else the newest
#                                  in $PREFIX/opt/android-sdk/ndk/: Termux's
#                                  headers are the NDK's, and may come without
#                                  the module sources (until sqterm's runtimes
#                                  package replaces both)
#
# Refuses, saying what to do: none found; sources beside headers of another
# libc++ major version than the compiler's.

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "usage: libcxx_modules.sh <compiler> <flags>..." >&2
    exit 2
fi
cxx=$1
shift

fail() {
    echo "libcxx_modules: $*" >&2
    exit 1
}

version=$(printf '#include <version>\n_LIBCPP_VERSION\n' |
    "$cxx" "$@" -x c++ -E -P - 2>/dev/null | tail -n 1 | tr -d ' ') || true
case $version in
    [0-9][0-9][0-9][0-9][0-9]*) ;;
    *) fail "$cxx does not compile against libc++ with these flags (-stdlib=libc++)" ;;
esac
config=$(printf '#include <version>\n' | "$cxx" "$@" -x c++ -M - 2>/dev/null |
    tr -d '\\' | tr ' ' '\n' | grep '/c++/v1/__config$' | head -n 1) || true

candidates=()
[ -n "${SQ_STD_CPPM:-}" ] && candidates+=("$(dirname "$SQ_STD_CPPM")")
if [ -n "$config" ]; then
    candidates+=("${config%/include/c++/v1/__config}/share/libc++/v1")
    # The NDK's headers are in <ndk prebuilt>/sysroot/usr/include/c++/v1, its
    # module sources in <ndk prebuilt>/share/libc++/v1.
    case $config in
        */sysroot/usr/include/c++/v1/__config) candidates+=("${config%/sysroot/usr/include/c++/v1/__config}/share/libc++/v1") ;;
    esac
fi
compiler=$(command -v "$cxx" 2>/dev/null || true)
[ -n "$compiler" ] && candidates+=("$(dirname "$(readlink -f "$compiler")")/../share/libc++/v1")
for ndk in "${SQ_NDK:-}" "${ANDROID_NDK_HOME:-}" \
    $(ls -d "${PREFIX:-/nonexistent}"/opt/android-sdk/ndk/*/ 2>/dev/null | sort -V -r); do
    [ -n "$ndk" ] || continue
    for c in "${ndk%/}"/toolchains/llvm/prebuilt/*/share/libc++/v1; do candidates+=("$c"); done
done
found=
for c in "${candidates[@]}"; do
    if [ -f "$c/std.cppm" ] && [ -d "$c/std" ]; then
        found=$(cd "$c" && pwd -P)
        break
    fi
done
[ -n "$found" ] ||
    fail "no libc++ module sources (share/libc++/v1/std.cppm) beside libc++ $version's headers or $cxx: set SQ_STD_CPPM (in sqterm: pkg install runtimes; Debian and Ubuntu: apt install libc++-dev)"

# Sources beside headers of their own: the same major version, or refused.
beside=$(sed -n 's/^#[[:space:]]*define[[:space:]]*_LIBCPP_VERSION[[:space:]]*\([0-9]*\).*/\1/p' \
    "${found%/share/libc++/v1}/include/c++/v1/__config" 2>/dev/null | head -n 1) || true
if [ -n "$beside" ] && [ "$((beside / 10000))" != "$((version / 10000))" ]; then
    fail "libc++ $version's headers, but module sources from libc++ $beside ($found): build against that libc++, or set SQ_STD_CPPM"
fi
printf '%s\n' "$found"
