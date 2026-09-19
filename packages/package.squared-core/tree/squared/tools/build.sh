#!/usr/bin/env bash
# Build every squared module with g++ and GNU make.
#
#   ./tools/build.sh                     release
#   ./tools/build.sh debug               unoptimised with symbols
#   ./tools/build.sh release -j4         pass extra flags through to make
#
# Run ./tools/vendor_third_party.sh once first: it unpacks yyjson, which
# data/src/json.cpp needs and the build compiles into libsquared_data.a.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

build="${1:-release}"
if [ "$#" -gt 0 ]; then shift; fi

if [ "$build" != "release" ] && [ "$build" != "debug" ]; then
  echo "usage: $0 [release|debug] [extra make args...]" >&2
  exit 2
fi

command -v make >/dev/null 2>&1 || { echo "make not found" >&2; exit 1; }
: "${CXX:=g++}"
command -v "$CXX" >/dev/null 2>&1 || { echo "$CXX not found" >&2; exit 1; }

echo "squared: building $build with $CXX"
make BUILD="$build" "$@"
echo "squared: archives in build/$build/lib"
