#!/usr/bin/env bash
#
# squared/tools/lint.sh — every lint check squared has, behind one command.
#
#   tools/lint.sh                    everything, every file (what `sqmake lint` runs)
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
#              compiles, module interfaces (.cppm) and implementations (.cpp),
#              with the build's own commands (sqmake's compile database);
#              also the Android sources where the SDK's toolchain is found
#              (sqterm). It reads the BMIs, so it builds first (sqmake, $SQMAKE
#              or on PATH: quick when built)
#
# --jobs defaults to the number of CPUs, at most 4: one clang-tidy can take a
# few hundred MB on a large file, and a phone has little to spare.
#
# Progress is printed as it goes; the full report is written to
# build/lint/report.txt, and clang-tidy's own output, with notes and fix-its,
# to build/lint/tidy/. Exit status: 0 clean, 1 findings,
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
OUT=$root/build/lint

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

# What clang-tidy analyses is what the build compiles, with the build's own
# commands: sqmake's compile database (build.lua, sqmake 0.13.0), the host's
# in build/sqmake and, where the SDK's toolchain is found (sqterm's
# build/toolchain.lua), the Android libraries' in build/sqmake-android, for the
# sources only they compile (the gles backend, the Android-only files). Each
# unit's file and the folder of the database that has it; the tests, the
# tools and third_party/ are not squared's units. Compiler warnings are left
# out (-w): the build already reports those.
SQMAKE=${SQMAKE:-sqmake}
declare -A planned=()
plan_db() { # plan_db <config> <build folder>; adds its units to $TMP/plan
    local cfg=$1 dir=$2 f
    [ -f "$root/$dir/compile_commands.json" ] || return 0
    while IFS= read -r f; do
        [ -z "${planned["$f"]+x}" ] || continue
        planned["$f"]=1
        printf '%s\037%s\037%s\n' "$f" "$cfg" "$dir" >>"$TMP/plan"
    done < <("$SQEDIT" pick -x '[ ]* ["] "file" ["] ": " ["] "../squared/" {[a-z] [a-z0-9_]* "/" [^"]*} ["] .*' \
                 "$root/$dir/compile_commands.json" |
             "$SQEDIT" match -vx '(!"/test/" .)* "/test/" .* / "third_party/" .* / "tools/" .* / [a-z0-9_]+ "/client/" .*' || true)
}

tidy_one() { # tidy_one <file> <config> <database folder> <i/n>; writes $TMP/tidy/<file>.log
    local f=$1 log=$TMP/tidy/$1.log k
    mkdir -p "$(dirname "$log")"
    (cd "$root" && clang-tidy --quiet -p "$3" --extra-arg=-w "$f") >"$log" 2>&1
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
        # Built first: the BMIs every unit imports, and the databases.
        if ! command -v "$SQMAKE" >/dev/null 2>&1; then
            echo "  tidy needs sqmake, on PATH or as SQMAKE (github.com/squared-netizen/sqmake)" >&2
            exit 2
        fi
        echo "  building first (sqmake: the BMIs the units import)"
        if ! (cd "$root" && "$SQMAKE" -j"$jobs") >"$TMP/build-host.log" 2>&1; then
            echo "  the build failed; tidy needs its BMIs:" >&2
            tail -n 20 "$TMP/build-host.log" >&2
            exit 2
        fi
        plan_db host build/sqmake
        configs="host only: no SDK toolchain for Android here"
        if (cd "$root" && SQUARED_TARGET=android "$SQMAKE" -j"$jobs") >"$TMP/build-android.log" 2>&1; then
            plan_db android build/sqmake-android
            configs="host and android"
        else
            echo "  the Android build for tidy did not run; its units are skipped: $(tail -n 1 "$TMP/build-android.log")"
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
        echo "  clang-tidy $(clang-tidy --version | "$SQEDIT" pick '"version " {[0-9.]+}'), $total translation unit(s) ($configs); a module's interfaces are units of their own"
        # Selected sources no configuration compiles are named, not analysed.
        cut -d$'\037' -f1 "$TMP/plan" | LC_ALL=C sort >"$TMP/planned"
        "$SQEDIT" match -x '[a-z] [a-z0-9_]* "/src/" (!(".cpp" !.) .)* ".cpp"' "$TMP/files" | LC_ALL=C sort | LC_ALL=C comm -23 - "$TMP/planned" \
            | while IFS= read -r f; do echo "  skipped $f: not compiled here ($configs)"; done
        mkdir -p "$TMP/tidy"
        i=0
        running=0
        while IFS=$'\037' read -r f cfg db; do
            i=$((i + 1))
            if [ "$jobs" -gt 1 ]; then
                tidy_one "$f" "$cfg" "$db" "$i/$total" &
                running=$((running + 1))
                if [ "$running" -ge "$jobs" ]; then wait -n; running=$((running - 1)); fi
            else
                tidy_one "$f" "$cfg" "$db" "$i/$total"
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
        # A pattern with rules is all rules, the first where it starts.
        "$SQEDIT" pick -x 'line <- (!(check !.) .)* {check}  check <- "[" [a-z] [a-z0-9.,-]* [A-Za-z0-9.]* "]"' "$TMP/tidy.found" \
            | sort | uniq -c | sort -rn | head -25 | "$SQEDIT" sub -x '{.*}' '    {1}'
        findings=$((findings + n))
    fi
fi

# --- result --------------------------------------------------------------------

mv -f "$REPORT" "$FINAL_REPORT"
echo "== $findings finding(s) in $((SECONDS / 60))m $((SECONDS % 60))s; report: ${FINAL_REPORT#"$root"/}"
[ "$findings" -eq 0 ]
