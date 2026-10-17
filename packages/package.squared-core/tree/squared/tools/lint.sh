#!/usr/bin/env bash
#
# squared/tools/lint.sh — every lint check squared has, behind one command.
#
#   tools/lint.sh                    everything, every file (what `make lint` runs)
#   tools/lint.sh --changed          only files changed since the last commit
#   tools/lint.sh --since HEAD~1     only files changed since that commit: what a
#                                    commit touched, after it is made
#   tools/lint.sh gui/src/ui.cpp     only the files named
#   tools/lint.sh --only tidy        one step: format, rules or tidy
#   tools/lint.sh --jobs 4           run clang-tidy on 4 files at once
#   tools/lint.sh --record-baseline  accept today's rule matches (see lint-rules.txt)
#
# Steps, each skipped cleanly if its tool is missing:
#   1. format  files clang-format would change (tools/format.sh fixes them)
#   2. rules   tools/lint-rules.txt, checked against tools/lint-baseline.txt
#   3. tidy    clang-tidy with squared/.clang-tidy, on each .cpp the build
#              compiles (also the Android sources when the NDK is installed)
#
# --jobs defaults to the number of CPUs, at most 4: one clang-tidy can take a
# few hundred MB on a large file, and a phone has little to spare.
#
# Progress is printed as it goes; the full report is written to
# build/release/lint/report.txt, and clang-tidy's own output, with notes and
# fix-its, to build/release/lint/tidy/. Exit status: 0 clean, 1 findings,
# 2 usage or environment error. Runs from anywhere: squared/ is found from
# this script.
#
# This file is the engine; what is checked lives in .clang-tidy and
# lint-rules.txt, so the engine can be replaced without touching either.

set -uo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
root=$(cd "$script_dir/.." && pwd)
RULES=$script_dir/lint-rules.txt
BASELINE=$script_dir/lint-baseline.txt
OUT=$root/build/release/lint

only=""
changed=0
base=HEAD
# nproc first: Termux has it (coreutils) but not getconf.
jobs=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
[ "$jobs" -ge 1 ] 2>/dev/null || jobs=1
[ "$jobs" -le 4 ] || jobs=4
record=0
named=()
while [ $# -gt 0 ]; do
    case "$1" in
        --changed) changed=1 ;;
        --since)
            [ $# -ge 2 ] && [ -n "$2" ] || { echo "lint: --since needs a commit, for example HEAD~1" >&2; exit 2; }
            changed=1
            base=$2
            shift ;;
        --record-baseline) record=1 ;;
        --only)
            [ $# -ge 2 ] || { echo "lint: --only needs format, rules or tidy" >&2; exit 2; }
            case "$2" in format|rules|tidy) only=$2 ;; *) echo "lint: --only takes format, rules or tidy" >&2; exit 2 ;; esac
            shift ;;
        --jobs)
            [ $# -ge 2 ] && [ "$2" -ge 1 ] 2>/dev/null || { echo "lint: --jobs needs a number" >&2; exit 2; }
            jobs=$2
            shift ;;
        -h|--help)
            sed -n '3,29p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        -*) echo "lint: unknown option $1" >&2; exit 2 ;;
        *) named+=("${1#./}") ;;
    esac
    shift
done
run() { [ -z "$only" ] || [ "$only" = "$1" ]; }
if [ "$record" -eq 1 ]; then
    if [ ${#named[@]} -gt 0 ] || [ "$changed" -eq 1 ]; then
        echo "lint: --record-baseline needs the whole tree; drop the file list, --changed and --since" >&2
        exit 2
    fi
    only=rules
fi

mkdir -p "$OUT" || { echo "lint: cannot create $OUT" >&2; exit 2; }
TMP=$(mktemp -d "${TMPDIR:-/tmp}/sqlint.XXXXXX") || exit 2
trap 'rm -rf "$TMP"' EXIT
# The report is built in scratch space and moved into place at the end, and only
# by a run that checked something: a run with nothing to check, or one that
# stops early, leaves the last report as it was.
FINAL_REPORT=$OUT/report.txt
REPORT=$TMP/report.txt
: >"$REPORT"
SECONDS=0

# --- which files ---------------------------------------------------------------

all_sources() {
    (cd "$root" && find . \( -path ./third_party -o -path ./build -o -path ./docs \) -prune \
        -o -type f \( -name '*.hpp' -o -name '*.cpp' \) -print | sed 's|^\./||' | LC_ALL=C sort)
}
if [ ${#named[@]} -gt 0 ]; then
    printf '%s\n' "${named[@]}" >"$TMP/files"
elif [ "$changed" -eq 1 ]; then
    git -C "$root" rev-parse --is-inside-work-tree >/dev/null 2>&1 \
        || { echo "lint: --changed and --since need a git checkout" >&2; exit 2; }
    git -C "$root" rev-parse --verify --quiet "$base^{commit}" >/dev/null \
        || { echo "lint: no such commit: $base" >&2; exit 2; }
    # Deleted files are left out (--diff-filter=d): a change that removes a
    # header has nothing of it left to check.
    { git -C "$root" diff --name-only --relative --diff-filter=d "$base" -- .
      git -C "$root" ls-files --others --exclude-standard -- . ; } \
        | grep -E '\.(hpp|cpp)$' | grep -vE '^(third_party|build|docs)/' | LC_ALL=C sort -u >"$TMP/files"
else
    all_sources >"$TMP/files"
fi
for f in $(cat "$TMP/files"); do
    [ -f "$root/$f" ] || { echo "lint: no such file under squared/: $f" >&2; exit 2; }
done
nfiles=$(wc -l <"$TMP/files" | tr -d ' ')
echo "lint: $nfiles file(s) under $root"
[ "$nfiles" -gt 0 ] || { echo "lint: nothing to check; ${FINAL_REPORT#"$root"/} left as it was"; exit 0; }

findings=0

# --- 1. format -----------------------------------------------------------------

if run format; then
    echo "== 1/3 format"
    if ! command -v clang-format >/dev/null 2>&1 || [ ! -f "$root/.clang-format" ]; then
        echo "  skipped: clang-format or squared/.clang-format missing"
    else
        # The tree is formatted with clang-format 21 (docs/developer/code-style.md);
        # an older one can disagree about a few files.
        echo "  $(clang-format --version | sed -n 's/.*version \([0-9.]*\).*/clang-format \1/p')"
        # 40 files per clang-format run: few processes, and a progress line per run.
        split -l 40 "$TMP/files" "$TMP/chunk."
        : >"$TMP/unformatted"
        done_n=0
        for chunk in "$TMP"/chunk.*; do
            (cd "$root" && xargs clang-format --style=file --dry-run --Werror <"$chunk") 2>&1 \
                | grep -oE '^[^ :]+:[0-9]+:[0-9]+: (error|warning)' | cut -d: -f1 \
                | LC_ALL=C sort -u >>"$TMP/unformatted"
            done_n=$((done_n + $(wc -l <"$chunk")))
            [ "$nfiles" -le 40 ] || echo "  checked $done_n/$nfiles"
        done
        while IFS= read -r f; do
            echo "$f: [format] clang-format would change this file (tools/format.sh)" >>"$REPORT"
            echo "  would change  $f"
        done <"$TMP/unformatted"
        n=$(wc -l <"$TMP/unformatted" | tr -d ' ')
        echo "  $n file(s) not formatted"
        findings=$((findings + n))
    fi
fi

# --- 2. rules ------------------------------------------------------------------

# Copies of the source files with comments removed and the contents of string
# and character literals blanked, line for line, so a match's line number is
# the source's. One awk run for every file. Raw string literals are not
# understood; squared has none.
cat >"$TMP/strip.awk" <<'AWK'
FNR == 1 { if (dest != "") close(dest); dest = out "/" FILENAME; block = 0 }
{
    line = $0; o = ""; i = 1; n = length(line)
    while (i <= n) {
        c = substr(line, i, 1); d = substr(line, i, 2)
        if (block) {
            if (d == "*/") { block = 0; o = o "  "; i += 2 } else { o = o " "; i++ }
        } else if (d == "//") {
            break
        } else if (d == "/*") {
            block = 1; o = o "  "; i += 2
        } else if (c == "\"" || c == "'") {
            q = c; o = o c; i++
            while (i <= n) {
                c = substr(line, i, 1)
                if (c == "\\") { o = o "  "; i += 2; continue }
                if (c == q) { o = o c; i++; break }
                o = o " "; i++
            }
        } else {
            o = o c; i++
        }
    }
    print o > dest
}
AWK
strip_all() {
    mkdir -p "$TMP/stripped"
    while IFS= read -r f; do
        case "$f" in */*) echo "${f%/*}" ;; esac
    done <"$TMP/files" | LC_ALL=C sort -u | (cd "$TMP/stripped" && xargs mkdir -p)
    (cd "$root" && xargs awk -v out="$TMP/stripped" -f "$TMP/strip.awk" <"$TMP/files")
}

# Rules, one per block: rule / why / match / files.
parse_rules() {
    awk '
    function flush() { if (r != "") print r "\037" m "\037" f "\037" w; r = m = f = w = "" }
    /^[[:space:]]*#/ { next }
    /^[[:space:]]*$/ { flush(); next }
    {
        key = $1; val = $0; sub(/^[^[:space:]]+[[:space:]]+/, "", val)
        if (key == "rule") r = val; else if (key == "match") m = val
        else if (key == "files") f = val; else if (key == "why") w = val
    }
    END { flush() }' "$RULES"
}

if run rules; then
    echo "== 2/3 rules"
    if [ ! -f "$RULES" ]; then
        echo "  skipped: no $RULES"
    else
        parse_rules >"$TMP/rules"
        echo "  $(wc -l <"$TMP/rules" | tr -d ' ') rule(s) from tools/lint-rules.txt"
        strip_all
        : >"$TMP/counts"
        [ -f "$BASELINE" ] && grep -v '^#' "$BASELINE" >"$TMP/baseline" || : >"$TMP/baseline"
        n=0
        # \037 (unit separator), not a tab: read collapses repeated whitespace
        # separators, which would drop an empty files field.
        while IFS=$'\037' read -r rule match globs why; do
            # The files this rule covers: all, or those matching its globs.
            if [ -z "$globs" ]; then
                # Framework code only: tests may use what the framework may not
                # (an NDK stub returns raw pointers, a test may catch).
                grep -v '/test/' "$TMP/files" >"$TMP/rulefiles" || true
            else
                while IFS= read -r f; do
                    for g in $globs; do
                        # shellcheck disable=SC2254
                        case "$f" in $g) echo "$f"; break ;; esac
                    done
                done <"$TMP/files" >"$TMP/rulefiles"
            fi
            : >"$TMP/hits"
            [ -s "$TMP/rulefiles" ] && (cd "$TMP/stripped" && xargs grep -nHE -- "$match" <"$TMP/rulefiles") \
                >"$TMP/hits" 2>/dev/null
            # rule file count, then the files over the baseline: file count allowed
            cut -d: -f1 "$TMP/hits" | LC_ALL=C sort | uniq -c \
                | awk -v r="$rule" '{ print r, $2, $1 }' >"$TMP/rulecounts"
            cat "$TMP/rulecounts" >>"$TMP/counts"
            awk 'FILENAME == ARGV[1] { allowed[$1 " " $2] = $3; next }
                 { a = allowed[$1 " " $2] + 0; if ($3 > a) print $2, $3, a }' \
                "$TMP/baseline" "$TMP/rulecounts" >"$TMP/over"
            total=$(wc -l <"$TMP/hits" | tr -d ' ')
            if [ "$record" -eq 1 ]; then
                printf '  %-14s %s match(es)\n' "$rule" "$total"
                continue
            fi
            over=$(awk '{ s += $2 - $3 } END { print s + 0 }' "$TMP/over")
            printf '  %-14s %s match(es), %s over the baseline\n' "$rule" "$total" "$over"
            [ "$over" -eq 0 ] || echo "[$rule] $why" >>"$REPORT"
            shown=0
            while read -r f count allowed; do
                shown=$((shown + 1))
                [ "$shown" -gt 10 ] || echo "    $f: $count (baseline $allowed)"
                # Each matching line, quoted from the real source.
                awk -F: -v f="$f" -v r="$rule" '
                    FILENAME == ARGV[1] { if ($1 == f) want[$2] = 1; next }
                    FNR in want { s = $0; sub(/^[[:space:]]+/, "", s); print "  " f ":" FNR ": [" r "] " substr(s, 1, 90) }' \
                    "$TMP/hits" "$root/$f" >>"$REPORT"
            done <"$TMP/over"
            [ "$shown" -le 10 ] || echo "    ... and $((shown - 10)) more file(s); see the report"
            n=$((n + over))
        done <"$TMP/rules"
        if [ "$record" -eq 1 ]; then
            # A reason written after '#' on an entry's line is kept.
            [ -f "$BASELINE" ] && cp "$BASELINE" "$TMP/oldbaseline" || : >"$TMP/oldbaseline"
            {
                echo "# tools/lint-baseline.txt — accepted rule matches: rule, file, count."
                echo "# Written by tools/lint.sh --record-baseline; counts may only go down."
                echo "# Add a reason after '#' on an entry's line; re-recording keeps it."
                LC_ALL=C sort "$TMP/counts" | awk '
                    FILENAME == ARGV[1] {
                        if ($0 !~ /^#/ && (h = index($0, "#")) > 0) note[$1 " " $2] = substr($0, h)
                        next
                    }
                    { k = $1 " " $2; print $0 ((k in note) ? "  " note[k] : "") }' "$TMP/oldbaseline" -
            } >"$TMP/newbaseline"
            # Say what rose: raising a count is accepting new matches, which a
            # reviewer should see in the commit and here. (Not on a first record:
            # everything rose from nothing.)
            [ ! -s "$TMP/oldbaseline" ] || awk 'FILENAME == ARGV[1] { if ($0 !~ /^#/) old[$1 " " $2] = $3; next }
                 /^#/ { next }
                 { o = old[$1 " " $2] + 0; if ($3 > o) printf "  accepted %s %s: %d -> %d\n", $1, $2, o, $3 }' \
                "$TMP/oldbaseline" "$TMP/newbaseline"
            cat "$TMP/newbaseline" >"$BASELINE"
            echo "  baseline recorded: $(wc -l <"$TMP/counts" | tr -d ' ') entr(ies) in tools/lint-baseline.txt"
        else
            echo "  $n match(es) over the baseline"
            findings=$((findings + n))
        fi
    fi
fi

# --- 3. tidy -------------------------------------------------------------------

# What clang-tidy analyses is what the build compiles, with the build's flags,
# both read from the Makefiles (and so from build/common.mk and local.mk): the
# root Makefile's MODULES, then each module's SOURCES, STANDARD, OPTIMISE
# (NDEBUG decides which asserts exist), SQUARED_ABI and INCLUDES. Warning flags
# are left out; the build already reports those. Nothing is built: a helper
# makefile adds one target that prints a variable.
#
# Two configurations: the one local.mk selects ("host"), then, when the NDK's
# headers are installed, the Android one (gles backend, android platform), for
# the sources only it compiles. The Android one is compiled the way the
# template builds it: for API $android_api, against the NDK's sysroot and its
# libc++ headers (mk/squared_generated.mk, "Compiler and link flags").
android_api=22
printf 'print-%%:\n\t@:$(info $($*))\n' >"$TMP/print.mk"
mk_print() { # mk_print <dir> <variables...>; one line per variable
    local dir=$1
    shift
    local targets=()
    for v in "$@"; do targets+=("print-$v"); done
    make -s --no-print-directory -C "$dir" -f Makefile -f "$TMP/print.mk" "${overrides[@]}" \
        "${targets[@]}" 2>/dev/null
}

# The NDK's sysroot headers (EGL, GLES, android/), newest NDK first, from the
# prebuilt directory for this machine when an NDK ships several.
ndk_include() {
    local host gl3
    case "$(uname -m)" in aarch64 | arm64) host=linux-arm64 ;; *) host=linux-x86_64 ;; esac
    find "${PREFIX:-/usr}/opt/android-sdk/ndk" ${ANDROID_NDK_HOME:+"$ANDROID_NDK_HOME"} \
        -path '*sysroot/usr/include/GLES3/gl3.h' 2>/dev/null | sort -V >"$TMP/ndk"
    gl3=$(grep "/$host/" "$TMP/ndk" | tail -n 1)
    [ -n "$gl3" ] || gl3=$(tail -n 1 "$TMP/ndk")
    [ -n "$gl3" ] && printf '%s\n' "${gl3%/GLES3/gl3.h}"
}

# $TMP/plan: one line per translation unit, "file<US>config<US>flags".
plan_config() { # plan_config <name>; uses ${overrides[@]}
    local cfg=$1 mods m out srcs flags src
    [ -f "$root/Makefile" ] || return 0
    mods=$(mk_print "$root" MODULES)
    for m in $mods; do
        [ -f "$root/$m/Makefile" ] || continue
        out=$(mk_print "$root/$m" SOURCES STANDARD OPTIMISE SQUARED_ABI INCLUDES)
        srcs=$(printf '%s\n' "$out" | sed -n 1p)
        flags=$(printf '%s\n' "$out" | sed 1d | tr '\n' ' ')
        for src in $srcs; do
            grep -qF "$m/$src"$'\037' "$TMP/plan" || printf '%s\037%s\037%s\n' "$m/$src" "$cfg" "$flags" >>"$TMP/plan"
        done
    done
}

tidy_one() { # tidy_one <file> <config> <flags> <i/n>; writes $TMP/tidy/<file>.log
    local f=$1 log=$TMP/tidy/$1.log k
    mkdir -p "$(dirname "$log")"
    # shellcheck disable=SC2086
    (cd "$root" && clang-tidy --quiet "$f" -- $3) >"$log" 2>&1
    k=$(grep -cE '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' "$log" || true)
    printf '  [%s] %-50s %-8s %s\n' "$4" "$f" "$2" "$([ "$k" -gt 0 ] && echo "$k finding(s)" || echo ok)"
}

if run tidy; then
    echo "== 3/3 tidy"
    if ! command -v clang-tidy >/dev/null 2>&1; then
        echo "  skipped: clang-tidy is not installed (pkg install clang)"
    elif [ ! -f "$root/.clang-tidy" ]; then
        echo "  skipped: no $root/.clang-tidy"
    else
        : >"$TMP/plan"
        overrides=()
        plan_config host
        ndk=$(ndk_include || true)
        if [ -n "$ndk" ]; then
            sysroot=${ndk%/usr/include}
            overrides=(SQUARED_GRAPHICS_BACKEND=gles SQUARED_PLATFORM=android
                "SQUARED_EXTERNAL_INCLUDES=--target=aarch64-linux-android$android_api --sysroot=$sysroot -stdlib++-isystem $sysroot/usr/include/c++/v1")
            plan_config android
        fi
        if [ ! -s "$TMP/plan" ]; then
            # No Makefiles to ask: every .cpp under <module>/src/, layout defaults.
            grep -E '^[a-z][a-z0-9_]*/src/.*\.cpp$' "$TMP/files" | while IFS= read -r f; do
                printf '%s\037layout\037-std=c++20 -I%s -I%s/%s/src\n' "$f" "$(dirname "$root")" "$root" "${f%%/*}"
            done >"$TMP/plan"
        fi
        # The selected files the build compiles. A selected header brings in
        # the sources that include it directly, so --changed on a header-only
        # change still analyses it.
        cp "$TMP/files" "$TMP/want"
        grep -E '\.hpp$' "$TMP/files" | sed 's|^|squared/|' >"$TMP/headers" || true
        if [ -s "$TMP/headers" ]; then
            cut -d$'\037' -f1 "$TMP/plan" | (cd "$root" && xargs grep -lF -f "$TMP/headers") >>"$TMP/want" 2>/dev/null || true
        fi
        awk -F'\037' 'FILENAME == ARGV[1] { want[$0] = 1; next } $1 in want' "$TMP/want" "$TMP/plan" \
            | LC_ALL=C sort >"$TMP/tidyplan"
        total=$(wc -l <"$TMP/tidyplan" | tr -d ' ')
        echo "  $(clang-tidy --version | sed -n 's/.*version \([0-9.]*\).*/clang-tidy \1/p'), $total translation unit(s) ($([ -n "$ndk" ] && echo "host and android" || echo "host only: no NDK headers found")); headers are checked through the files that include them"
        # Selected sources no configuration compiles are named, not analysed.
        cut -d$'\037' -f1 "$TMP/plan" | LC_ALL=C sort >"$TMP/planned"
        grep -E '^[a-z][a-z0-9_]*/src/.*\.cpp$' "$TMP/files" | LC_ALL=C sort | LC_ALL=C comm -23 - "$TMP/planned" \
            | while IFS= read -r f; do echo "  skipped $f: not compiled in $([ -n "$ndk" ] && echo "either configuration" || echo "this configuration (no NDK headers)")"; done
        mkdir -p "$TMP/tidy"
        i=0
        running=0
        while IFS=$'\037' read -r f cfg flags; do
            i=$((i + 1))
            if [ "$jobs" -gt 1 ]; then
                tidy_one "$f" "$cfg" "$flags" "$i/$total" &
                running=$((running + 1))
                if [ "$running" -ge "$jobs" ]; then wait -n; running=$((running - 1)); fi
            else
                tidy_one "$f" "$cfg" "$flags" "$i/$total"
            fi
        done <"$TMP/tidyplan"
        wait
        # One finding per place and check: a header included by ten files is reported once.
        find "$TMP/tidy" -name '*.log' -exec cat {} + 2>/dev/null \
            | grep -E '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' \
            | sed "s|^$root/||" | LC_ALL=C sort -u >"$TMP/tidy.found" || true
        cat "$TMP/tidy.found" >>"$REPORT"
        # Full clang-tidy output, notes and fix-it hints included, per file.
        rm -rf "$OUT/tidy" && cp -R "$TMP/tidy" "$OUT/tidy"
        n=$(wc -l <"$TMP/tidy.found" | tr -d ' ')
        [ "$n" -eq 0 ] && echo "  no findings" || echo "  $n finding(s); by check:"
        grep -oE '\[[a-z][a-z0-9.,-]*[A-Za-z0-9.]*\]$' "$TMP/tidy.found" | sort | uniq -c | sort -rn | head -25 \
            | sed 's/^/    /'
        findings=$((findings + n))
    fi
fi

# --- result --------------------------------------------------------------------

mv -f "$REPORT" "$FINAL_REPORT"
echo "== $findings finding(s) in $((SECONDS / 60))m $((SECONDS % 60))s; report: ${FINAL_REPORT#"$root"/}"
[ "$findings" -eq 0 ]
