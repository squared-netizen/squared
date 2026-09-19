#!/usr/bin/env bash
# Compile every public header on its own.
#
#   ./tools/check_headers.sh          all modules
#   ./tools/check_headers.sh gui      one module
#   JOBS=4 ./tools/check_headers.sh   compile in parallel
#
# One type per file is only worth anything if each header actually stands
# alone. This proves it: a header that needs a neighbour included first fails
# here, which is the regression this layout exists to prevent.
#
# There are well over a hundred headers and each is a full compile, so this
# takes minutes on a phone. It prints progress as it goes for exactly that
# reason - a silent run is indistinguishable from a hung one.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

: "${CXX:=g++}"
: "${SQUARED_EXTERNAL_INCLUDES:=}"
: "${JOBS:=1}"

# Pick up local.mk the way build/common.mk does, so this script and the build
# see the same external modules.
if [ -z "$SQUARED_EXTERNAL_INCLUDES" ] && [ -f local.mk ]; then
  SQUARED_EXTERNAL_INCLUDES="$(
    sed -n 's/^[[:space:]]*SQUARED_EXTERNAL_INCLUDES[[:space:]]*[:?+]\{0,1\}=[[:space:]]*//p' \
      local.mk | tr '\n' ' ' | sed 's/\\//g'
  )"
  # local.mk is make syntax, so it may contain $(PREFIX). Rewrite make's
  # $(NAME) to shell's ${NAME} before expanding: passing $(...) through eval
  # makes the shell try to run the name as a command.
  SQUARED_EXTERNAL_INCLUDES="$(
    printf '%s' "$SQUARED_EXTERNAL_INCLUDES" \
      | sed 's/\$(\([A-Za-z_][A-Za-z_0-9]*\))/${\1}/g'
  )"
  eval "SQUARED_EXTERNAL_INCLUDES=\"$SQUARED_EXTERNAL_INCLUDES\""
fi

# Discover modules rather than listing them, so a new module is covered the
# moment it exists.
includes=""
for candidate in */include; do
  if [ -d "$candidate" ]; then
    includes="$includes -I$root/$candidate"
  fi
done

scope="${1:-}"
if [ -n "$scope" ]; then
  if [ ! -d "$scope/include" ]; then
    echo "no such module: $scope" >&2
    exit 2
  fi
  search="$scope/include"
else
  search="."
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

headers="$work/headers"
find "$search" -path '*/include/squared/*' -name '*.hpp' | sort > "$headers"
total="$(wc -l < "$headers" | tr -d ' ')"

if [ "$total" -eq 0 ]; then
  echo "no headers found under $search" >&2
  exit 2
fi

echo "checking $total headers with $CXX (JOBS=$JOBS)"

# One header, one compile. Writes a file into $work/fail on failure so the
# parent can count them without sharing a variable across subshells.
check_one() {
  header="$1"
  relative="${header#*/include/}"
  probe="$work/probe.$$.cpp"
  printf '#include <%s>\nint main() { return 0; }\n' "$relative" > "$probe"
  # shellcheck disable=SC2086
  if "$CXX" -std=c++20 -Wall -Wextra -Wpedantic -fsyntax-only \
      $includes $SQUARED_EXTERNAL_INCLUDES "$probe" 2> "$probe.err"; then
    printf '.'
  else
    printf 'F'
    {
      echo "=== not self-contained: $relative"
      sed -n '1,12p' "$probe.err"
    } >> "$work/failures"
    : > "$work/fail.$(echo "$relative" | tr '/' '_')"
  fi
  rm -f "$probe" "$probe.err"
}

export -f check_one
export work includes SQUARED_EXTERNAL_INCLUDES CXX

if [ "$JOBS" -gt 1 ] && command -v xargs >/dev/null 2>&1; then
  xargs -P "$JOBS" -I{} bash -c 'check_one "$@"' _ {} < "$headers"
else
  count=0
  while IFS= read -r header; do
    count=$((count + 1))
    check_one "$header"
    if [ $((count % 50)) -eq 0 ]; then
      printf ' %d/%d\n' "$count" "$total"
    fi
  done < "$headers"
fi

printf '\n'

failed="$(find "$work" -name 'fail.*' | wc -l | tr -d ' ')"
if [ "$failed" -gt 0 ]; then
  echo
  cat "$work/failures"
  echo
fi

echo "checked $total headers, $failed not self-contained"
[ "$failed" -eq 0 ]
