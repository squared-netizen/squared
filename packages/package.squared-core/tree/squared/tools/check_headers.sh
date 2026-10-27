#!/usr/bin/env bash
# Check every header a user can include: the workhorse headers at a module
# root and the supporting headers in its src/. src/detail/ is private and is
# not checked.
#
#   ./tools/check_headers.sh          all modules
#   ./tools/check_headers.sh gui      one module
#   JOBS=1 ./tools/check_headers.sh   one compile at a time (the default is
#                                     one per core)
#
# Two checks, both protecting the layout (docs/developer/file-layout.md):
#
#   1. Reachability. Every supporting header is reached through the includes
#      of some workhorse header, so a user never has to know src/ exists to
#      get a type. Fast: one grep over the tree and one awk.
#   2. Self-containment. Each header compiles on its own. One type per file is
#      only worth anything if each header actually stands alone; a header that
#      needs a neighbour included first fails here.
#
# There are well over a hundred headers and each compile is a full one, so the
# second check takes minutes on a phone. It prints progress as it goes for
# exactly that reason - a silent run is indistinguishable from a hung one.
#
# Exit status: 0 when both checks pass, 1 when either fails, 2 on a usage or
# environment error.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

# Headers are included as <squared/...>, found through the directory that holds
# this one, so this one has to be called squared.
if [ "$(basename "$root")" != "squared" ]; then
  echo "this directory must be named squared, not $(basename "$root")" >&2
  exit 2
fi

: "${CXX:=g++}"
: "${SQUARED_EXTERNAL_INCLUDES:=}"
: "${JOBS:=$(nproc 2>/dev/null || echo 1)}"

# Supporting headers that no workhorse header includes, on purpose: only
# someone extending squared needs them, and no everyday include should pull
# them in. Each entry is a decision recorded in docs/developer/file-layout.md.
# None today: the three were the old dispatcher's provider interface, removed
# with it (2026-10-20).
extension_surface=""

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

# One public include root: the directory that holds squared/.
includes="-I$(dirname "$root")"

scope="${1:-}"
scope_module=""
if [ -n "$scope" ]; then
  if [ ! -d "$scope" ]; then
    echo "no such module: $scope" >&2
    exit 2
  fi
  scope_module="${scope%/}"
  scope_module="${scope_module#./}"
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# A module's headers are its root and src/. Modules are discovered rather than
# listed, so a new one is covered the moment it exists. Depth 3 reaches
# ./module/src/name.hpp and no further. Reachability always reads the whole
# tree, because a module's headers can be reached from another module's.
all_headers="$work/all"
find . -maxdepth 3 -name '*.hpp' \
  -not -path '*/detail/*' -not -path '*/test/*' \
  -not -path './third_party/*' -not -path './build/*' \
  -not -path './docs/*' -not -path './tools/*' | sort > "$all_headers"

headers="$work/headers"
if [ -n "$scope_module" ]; then
  grep "^\./$scope_module/" "$all_headers" > "$headers" || :
else
  cp "$all_headers" "$headers"
fi
total="$(wc -l < "$headers" | tr -d ' ')"

if [ "$total" -eq 0 ]; then
  echo "no headers found under ${scope:-.}" >&2
  exit 2
fi

# ---- 1. Reachability -------------------------------------------------------
#
# The include graph comes from one grep: "./gui/ui.hpp:#include <squared/...>".
# Every root header except the module's aggregate, squared/<module>/<module>.hpp,
# is a starting point. The aggregate includes the whole module, so counting it
# would let anything pass.
edges="$work/edges"
xargs grep -H '^[[:space:]]*#[[:space:]]*include[[:space:]]*<squared/' \
  < "$all_headers" > "$edges" || :

unreached="$work/unreached"
summary="$work/summary"
# shellcheck disable=SC2086
awk -v scope="$scope_module" -v surface="$(echo $extension_surface)" \
    -v summary="$summary" '
  FNR == NR { n++; name[n] = "squared/" substr($0, 3); present[name[n]] = 1; next }
  {
    colon = index($0, ":")
    from = "squared/" substr($0, 3, colon - 3)
    to = substr($0, colon + 1)
    sub(/^[^<]*</, "", to)
    sub(/>.*$/, "", to)
    out[from] = out[from] " " to
  }
  END {
    count = split(surface, listed, " ")
    for (i = 1; i <= count; i++) skip[listed[i]] = 1
    top = 0
    for (i = 1; i <= n; i++) {
      k = split(name[i], part, "/")
      if (k == 3 && part[3] != part[2] ".hpp") { seen[name[i]] = 1; queue[++top] = name[i] }
    }
    while (top > 0) {
      x = queue[top--]
      m = split(out[x], targets, " ")
      for (j = 1; j <= m; j++) {
        if (!(targets[j] in seen)) { seen[targets[j]] = 1; queue[++top] = targets[j] }
      }
    }
    supporting = 0; reached = 0; surface_count = 0; missing = 0
    for (i = 1; i <= n; i++) {
      k = split(name[i], part, "/")
      if (k != 4 || part[3] != "src") continue
      if (scope != "" && part[2] != scope) continue
      supporting++
      if (name[i] in seen) reached++
      else if (name[i] in skip) surface_count++
      else { print "  " name[i]; missing++ }
    }
    for (i = 1; i <= count; i++) {
      split(listed[i], part, "/")
      if (scope != "" && part[2] != scope) continue
      if (!(listed[i] in present)) { print "  " listed[i] " (listed as extension surface; no such header)"; missing++ }
    }
    print supporting, reached, surface_count, missing > summary
  }
' "$all_headers" "$edges" > "$unreached"

read -r supporting reached surface_count unreachable < "$summary"
echo "reachability: $reached of $supporting supporting headers reached through a workhorse header; $surface_count extension surface"
if [ "$unreachable" -gt 0 ]; then
  echo "reachability: $unreachable not reached through any workhorse header:"
  cat "$unreached"
  echo "  include each from the workhorse header that uses it, move it to the module"
  echo "  root, or list it as extension surface in tools/check_headers.sh"
  echo "  (docs/developer/file-layout.md)"
fi

# The standard headers the build precompiles: the list must name every one
# the sources include (build/std_pch.hpp; tools/std_pch.sh writes it).
std_pch_ok=1
bash "$root/tools/std_pch.sh" --check || std_pch_ok=0

# ---- 2. Self-containment ---------------------------------------------------

echo "checking $total headers with $CXX (JOBS=$JOBS)"

# One header, one compile. Writes a file into $work/fail on failure so the
# parent can count them without sharing a variable across subshells.
check_one() {
  header="$1"
  relative="squared/${header#./}"
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

echo "checked $total headers, $failed not self-contained, $unreachable not reachable"
[ "$failed" -eq 0 ] && [ "$unreachable" -eq 0 ] && [ "$std_pch_ok" = 1 ]
