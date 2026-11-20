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
#   3. tidy    clang-tidy with squared/.clang-tidy, on each unit the build
#              compiles, module interfaces (.cppm) and implementations (.cpp);
#              also the Android sources when the NDK is installed. It reads
#              the BMIs, so it builds first (make: quick when built; for the
#              Android sources, a build of its own in build/lint-android)
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
# sqedit (github.com/squared-netizen/sqedit) does what sed, grep and awk did here.
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQEDIT" >/dev/null ||
    { echo "lint: needs sqedit, on PATH or as $SQEDIT (github.com/squared-netizen/sqedit)" >&2; exit 2; }

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
            "$SQEDIT" lines 3..29 "${BASH_SOURCE[0]}" | "$SQEDIT" sub -x '"#" " "? {.*}' '{1}'
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
        -o -type f \( -name '*.hpp' -o -name '*.cpp' -o -name '*.cppm' \) -print | "$SQEDIT" pick -x '"./" {.*}' | LC_ALL=C sort)
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
        | "$SQEDIT" match -x '(!("." ("hpp" / "cppm" / "cpp") !.) .)* "." ("hpp" / "cppm" / "cpp")' \
        | "$SQEDIT" match -vx '("third_party" / "build" / "docs") "/" .*' | LC_ALL=C sort -u >"$TMP/files"
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
        echo "  clang-format $(clang-format --version | "$SQEDIT" pick '"version " {[0-9.]+}')"
        # 40 files per clang-format run: few processes, and a progress line per run.
        split -l 40 "$TMP/files" "$TMP/chunk."
        : >"$TMP/unformatted"
        done_n=0
        for chunk in "$TMP"/chunk.*; do
            (cd "$root" && xargs clang-format --style=file --dry-run --Werror <"$chunk") 2>&1 \
                | "$SQEDIT" pick -x '{[^ :]+} ":" %d+ ":" %d+ ": " ("error" / "warning") .*' \
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
# the source's: a block comment's characters become spaces (its line breaks
# stay), a line comment goes, a literal keeps its quotes and blanks the rest
# (an escape is two spaces). One sqedit run for every file, each changed in
# place in a copy. Raw string literals are not understood; squared has none.
cat >"$TMP/strip.peg" <<'PEG'
strip   <- {~ (block / line / dq / sq / .)* ~}
block   <- '/*' -> '  ' (!'*/' (%nl / . -> ' '))* ('*/' -> '  ')?
line    <- '//' [^%nl]* -> ''
dq      <- '"' (escape / [^"%nl] -> ' ')* '"'?
sq      <- "'" (escape / [^'%nl] -> ' ')* "'"?
escape  <- '\' [^%nl]? -> '  '
PEG
strip_all() {
    mkdir -p "$TMP/stripped"
    (cd "$root" && tar -cf - -T "$TMP/files") | (cd "$TMP/stripped" && tar -xf -)
    (cd "$TMP/stripped" && xargs "$SQEDIT" sub --whole --in-place -g "$TMP/strip.peg" 'strip' '{1}' <"$TMP/files") || true
}

# Rules, one per block: rule / why / match / files.
parse_rules() {
    local key val r= m= f= w=
    flush() { [ -z "$r" ] || printf '%s\037%s\037%s\037%s\n' "$r" "$m" "$f" "$w"; r= m= f= w=; }
    while IFS=$' \t' read -r key val; do
        case $key in
            '#'*) ;;
            '') flush ;;
            rule) r=$val ;; match) m=$val ;; files) f=$val ;; why) w=$val ;;
        esac
    done <"$RULES"
    flush
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
        [ -f "$BASELINE" ] && "$SQEDIT" match -vx '"#" .*' "$BASELINE" >"$TMP/baseline" || : >"$TMP/baseline"
        declare -A allowed=()
        while read -r br bf bc _; do [ -z "$bf" ] || allowed["$br $bf"]=$bc; done <"$TMP/baseline"
        n=0
        # \037 (unit separator), not a tab: read collapses repeated whitespace
        # separators, which would drop an empty files field.
        while IFS=$'\037' read -r rule match globs why; do
            # The files this rule covers: all, or those matching its globs.
            if [ -z "$globs" ]; then
                # Framework code only: tests may use what the framework may not
                # (an NDK stub returns raw pointers, a test may catch).
                "$SQEDIT" match -vF '/test/' "$TMP/files" >"$TMP/rulefiles" || true
            else
                while IFS= read -r f; do
                    for g in $globs; do
                        # shellcheck disable=SC2254
                        case "$f" in $g) echo "$f"; break ;; esac
                    done
                done <"$TMP/files" >"$TMP/rulefiles"
            fi
            : >"$TMP/hits"
            # /dev/null too: sqedit names the file with each line when it reads more than one.
            [ -s "$TMP/rulefiles" ] && (cd "$TMP/stripped" && xargs "$SQEDIT" match -nx -- "$match" /dev/null <"$TMP/rulefiles") \
                >"$TMP/hits" 2>/dev/null
            # rule file count, then the files over the baseline: file count allowed
            cut -d: -f1 "$TMP/hits" | LC_ALL=C sort | uniq -c \
                | while read -r count file; do echo "$rule $file $count"; done >"$TMP/rulecounts"
            cat "$TMP/rulecounts" >>"$TMP/counts"
            while read -r cr cf cc; do
                ca=${allowed["$cr $cf"]:-0}
                if [ "$cc" -gt "$ca" ]; then echo "$cf $cc $ca"; fi
            done <"$TMP/rulecounts" >"$TMP/over"
            total=$(wc -l <"$TMP/hits" | tr -d ' ')
            if [ "$record" -eq 1 ]; then
                printf '  %-14s %s match(es)\n' "$rule" "$total"
                continue
            fi
            over=0
            while read -r _ oc oa; do over=$((over + oc - oa)); done <"$TMP/over"
            printf '  %-14s %s match(es), %s over the baseline\n' "$rule" "$total" "$over"
            [ "$over" -eq 0 ] || echo "[$rule] $why" >>"$REPORT"
            shown=0
            while read -r f count allowed; do
                shown=$((shown + 1))
                [ "$shown" -gt 10 ] || echo "    $f: $count (baseline $allowed)"
                # Each matching line, quoted from the real source.
                mapfile -t source <"$root/$f"
                while IFS=: read -r hf hn _; do
                    [ "$hf" = "$f" ] || continue
                    s=${source[hn - 1]}
                    s=${s#"${s%%[![:space:]]*}"}
                    echo "  $f:$hn: [$rule] ${s:0:90}"
                done <"$TMP/hits" >>"$REPORT"
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
                declare -A note=()
                while IFS= read -r line; do
                    case $line in '#'*) continue ;; *'#'*) read -r na nb _ <<<"$line"; note["$na $nb"]="#${line#*#}" ;; esac
                done <"$TMP/oldbaseline"
                LC_ALL=C sort "$TMP/counts" | while IFS= read -r line; do
                    read -r na nb _ <<<"$line"
                    if [ -n "${note["$na $nb"]+x}" ]; then echo "$line  ${note["$na $nb"]}"; else echo "$line"; fi
                done
            } >"$TMP/newbaseline"
            # Say what rose: raising a count is accepting new matches, which a
            # reviewer should see in the commit and here. (Not on a first record:
            # everything rose from nothing.)
            if [ -s "$TMP/oldbaseline" ]; then
                declare -A old=()
                while read -r oa ob oc _; do case $oa in '#'*) ;; *) old["$oa $ob"]=$oc ;; esac; done <"$TMP/oldbaseline"
                while read -r na nb nc _; do
                    case $na in '#'*) continue ;; esac
                    o=${old["$na $nb"]:-0}
                    if [ "$nc" -gt "$o" ]; then printf '  accepted %s %s: %d -> %d\n' "$na" "$nb" "$o" "$nc"; fi
                done <"$TMP/newbaseline"
            fi
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
# root Makefile's MODULES, then each module's SOURCES and INTERFACES (its
# .cppm units, 7b.4), STANDARD, OPTIMISE (NDEBUG decides which asserts
# exist), SQUARED_ABI, INCLUDES and SQUARED_MODULE_USE (where the BMIs are: a
# unit's imports are read from them, so they are built first). Warning flags
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
    gl3=$("$SQEDIT" match -F "/$host/" "$TMP/ndk" | tail -n 1)
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
        out=$(mk_print "$root/$m" SOURCES INTERFACES STANDARD OPTIMISE SQUARED_ABI INCLUDES SQUARED_MODULE_USE)
        srcs=$(printf '%s\n' "$out" | "$SQEDIT" lines 1..2 | tr '\n' ' ')
        flags=$(printf '%s\n' "$out" | "$SQEDIT" lines 3.. | tr '\n' ' ')
        for src in $srcs; do
            "$SQEDIT" match -qF "$m/$src"$'\037' "$TMP/plan" || printf '%s\037%s\037%s\n' "$m/$src" "$cfg" "$flags" >>"$TMP/plan"
        done
    done
}

tidy_one() { # tidy_one <file> <config> <flags> <i/n>; writes $TMP/tidy/<file>.log
    local f=$1 log=$TMP/tidy/$1.log k
    mkdir -p "$(dirname "$log")"
    # shellcheck disable=SC2086
    (cd "$root" && clang-tidy --quiet "$f" -- $3) >"$log" 2>&1
    k=$("$SQEDIT" match -cx '[^ :]+ ":" %d+ ":" %d+ ": " ("warning" / "error") ":" .*' "$log" || true)
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
            # The target in CXX too, as the package mk passes it: the std
            # BMI is made with CXX and its flags, not the includes, and one
            # made for Termux's own API (24) refuses a unit for 22 (seen
            # 2026-11-03, the first Android units since C++23).
            overrides=(BUILD=lint-android SQUARED_GRAPHICS_BACKEND=gles SQUARED_PLATFORM=android
                "CXX=${CXX:-clang++} --target=aarch64-linux-android$android_api --sysroot=$sysroot"
                "SQUARED_EXTERNAL_INCLUDES=--target=aarch64-linux-android$android_api --sysroot=$sysroot -stdlib++-isystem $sysroot/usr/include/c++/v1")
            plan_config android
        fi
        if [ ! -s "$TMP/plan" ]; then
            # No Makefiles to ask: every .cpp under <module>/src/, layout defaults.
            "$SQEDIT" match -x '[a-z] [a-z0-9_]* "/src/" (!(".cpp" !.) .)* ".cpp"' "$TMP/files" | while IFS= read -r f; do
                printf '%s\037layout\037-std=c++23 -I%s -I%s/%s/src\n' "$f" "$(dirname "$root")" "$root" "${f%%/*}"
            done >"$TMP/plan"
        fi
        # The selected files the build compiles. A selected header brings in
        # the sources that include it directly, so --changed on a header-only
        # change still analyses it.
        cp "$TMP/files" "$TMP/want"
        "$SQEDIT" match -x '(!(".hpp" !.) .)* ".hpp"' "$TMP/files" | "$SQEDIT" sub -x '{.*}' 'squared/{1}' >"$TMP/headers" || true
        if [ -s "$TMP/headers" ]; then
            # A source that names any of them: one pattern, "a" / "b" / ...
            headers=$(while IFS= read -r h; do printf '"%s" / ' "$h"; done <"$TMP/headers")
            cut -d$'\037' -f1 "$TMP/plan" | while IFS= read -r src; do
                if "$SQEDIT" match -q "${headers% / }" "$root/$src" 2>/dev/null; then echo "$src"; fi
            done >>"$TMP/want"
        fi
        declare -A want=()
        while IFS= read -r w; do want["$w"]=1; done <"$TMP/want"
        while IFS= read -r line; do
            if [ -n "${want["${line%%$'\037'*}"]+x}" ]; then printf '%s\n' "$line"; fi
        done <"$TMP/plan" | LC_ALL=C sort >"$TMP/tidyplan"
        total=$(wc -l <"$TMP/tidyplan" | tr -d ' ')
        # The BMIs every unit imports: the host build, and the Android one in
        # build/lint-android (its objects come from the build cache when the
        # same were built before).
        if [ "$total" -gt 0 ]; then
            echo "  building the BMIs first (make)"
            if ! make -s --no-print-directory -C "$root" -j"$jobs" all >"$TMP/build-host.log" 2>&1; then
                echo "  the build failed; tidy needs its BMIs:" >&2
                tail -n 20 "$TMP/build-host.log" >&2
                exit 2
            fi
            if "$SQEDIT" match -qF $'\037android\037' "$TMP/tidyplan" &&
                ! make -s --no-print-directory -C "$root" -j"$jobs" "${overrides[@]}" all >"$TMP/build-android.log" 2>&1; then
                echo "  the Android build for tidy failed; its units are skipped:" >&2
                tail -n 10 "$TMP/build-android.log" >&2
                "$SQEDIT" match -vF $'\037android\037' "$TMP/tidyplan" >"$TMP/tidyplan.host" || true
                mv -f "$TMP/tidyplan.host" "$TMP/tidyplan"
                total=$(wc -l <"$TMP/tidyplan" | tr -d ' ')
            fi
        fi
        echo "  clang-tidy $(clang-tidy --version | "$SQEDIT" pick '"version " {[0-9.]+}'), $total translation unit(s) ($([ -n "$ndk" ] && echo "host and android" || echo "host only: no NDK headers found")); a module's interfaces are units of their own"
        # Selected sources no configuration compiles are named, not analysed.
        cut -d$'\037' -f1 "$TMP/plan" | LC_ALL=C sort >"$TMP/planned"
        "$SQEDIT" match -x '[a-z] [a-z0-9_]* "/src/" (!(".cpp" !.) .)* ".cpp"' "$TMP/files" | LC_ALL=C sort | LC_ALL=C comm -23 - "$TMP/planned" \
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
            | "$SQEDIT" match -x '[^ :]+ ":" %d+ ":" %d+ ": " ("warning" / "error") ":" .*' \
            | "$SQEDIT" sub -x "'$root/' {.*}" '{1}' | LC_ALL=C sort -u >"$TMP/tidy.found" || true
        cat "$TMP/tidy.found" >>"$REPORT"
        # Full clang-tidy output, notes and fix-it hints included, per file.
        rm -rf "$OUT/tidy" && cp -R "$TMP/tidy" "$OUT/tidy"
        n=$(wc -l <"$TMP/tidy.found" | tr -d ' ')
        [ "$n" -eq 0 ] && echo "  no findings" || echo "  $n finding(s); by check:"
        "$SQEDIT" pick -x '(!(check !.) .)* {check}  check <- "[" [a-z] [a-z0-9.,-]* [A-Za-z0-9.]* "]"' "$TMP/tidy.found" \
            | sort | uniq -c | sort -rn | head -25 | "$SQEDIT" sub -x '{.*}' '    {1}'
        findings=$((findings + n))
    fi
fi

# --- result --------------------------------------------------------------------

mv -f "$REPORT" "$FINAL_REPORT"
echo "== $findings finding(s) in $((SECONDS / 60))m $((SECONDS % 60))s; report: ${FINAL_REPORT#"$root"/}"
[ "$findings" -eq 0 ]
