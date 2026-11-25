#!/usr/bin/env bash
# docs.sh: the API reference, docs/reference/, made by clang-doc from the
# comments in the public interfaces. Committed, so it reads on GitHub without a
# build; made again by this script, never edited by hand.
#
#   squared/tools/docs.sh            write docs/reference/
#   squared/tools/docs.sh --check    say whether docs/reference/ is current;
#                                    writes nothing (exit 1 when it is not)
#   squared/tools/docs.sh --missing  also list the public functions and types
#                                    that have no comment
#
# Every public interface (the partitions at each module's root, 7b.4: what were
# the workhorse headers) is a translation unit of its own, so the reference
# covers what a user imports and nothing else. clang-doc reads them as text,
# not as modules (see "the interfaces as text" below), so no build is needed.
# clang-doc's pages are then tidied: "Defined at" becomes a link to the file
# and line, `_Bool` reads `bool`, and the pages for anonymous types and the
# global namespace are dropped.
#
# The pages depend on clang-doc's version, so README.md records the one that
# made them, and --check compares only with the same major version.
#
# Any compile error fails the run and writes nothing; a one-unit probe first
# checks that clang-doc can see the C++ standard library.
#
# Exit status: 0 written (or current), 1 --check found it out of date,
# 2 usage or environment error (clang-doc missing, or it failed).
set -euo pipefail
# sqedit (github.com/squared-netizen/sqedit) does what sed, grep and awk did here.
SQEDIT=${SQEDIT:-sqedit}
command -v "$SQEDIT" >/dev/null ||
    { echo "docs: needs sqedit, on PATH or as $SQEDIT (github.com/squared-netizen/sqedit)" >&2; exit 2; }

root="$(cd "$(dirname "$0")/.." && pwd)" # squared/
tree="$(dirname "$root")"
out="$root/docs/reference"
work="$root/build/release/docs"
check=0
missing=0
for argument in "$@"; do
    case "$argument" in
        --check) check=1 ;;
        --missing) missing=1 ;;
        -h|--help) "$SQEDIT" lines 2..26 "$0" | "$SQEDIT" sub -x '"#" " "? {.*}' '{1}'; exit 0 ;;
        *) echo "docs: unknown option: $argument" >&2; exit 2 ;;
    esac
done

clang_doc="${CLANG_DOC:-}"
if [ -z "$clang_doc" ]; then
    for candidate in clang-doc clang-doc-22 clang-doc-21 clang-doc-20 clang-doc-19 clang-doc-18; do
        if command -v "$candidate" >/dev/null 2>&1; then
            clang_doc=$candidate
            break
        fi
    done
fi
if [ -z "$clang_doc" ]; then
    echo "docs: no clang-doc found (Termux: pkg install clang; Debian: clang-tools)" >&2
    exit 2
fi
version=$("$clang_doc" --version 2>/dev/null | "$SQEDIT" pick '"version " {%d+}' | head -1)
version=${version:-0}

# The compiler the translation units name. clang-doc finds the C++ standard
# library from where that compiler is. Termux's clang has a relative default
# sysroot (../..), taken from the compiler's directory: $PREFIX/bin/../.., and
# libc++ under it in usr/include/c++/v1. A bare "clang++" has no directory, so
# the sysroot was ../.. from wherever clang-doc stood, <cstdint> was found in
# no header, and clang-doc then crashed. So the compiler is named by an
# absolute path in clang-doc's own directory, the way clang-tidy names its
# compiler (<its directory>/clang-tool) when given flags after --. The file
# need not exist: only its directory is used.
tool_path=$(command -v "$clang_doc")
cxx="$(cd -P "$(dirname "$tool_path")" && pwd)/clang++"

# dot n total: one dot for item n of total, and the count after every 50th
# and the last. dots_end ends a row the loop left open.
dot() {
    printf '.'
    if [ $(($1 % 50)) = 0 ] || [ "$1" = "$2" ]; then printf ' %d/%d\n' "$1" "$2"; fi
}
dots_end() {
    if [ $(($1 % 50)) != 0 ] && [ "$1" != "$2" ]; then printf ' %d/%d\n' "$1" "$2"; fi
}

if [ "$check" = 1 ] && [ -f "$out/README.md" ]; then
    made_by=$("$SQEDIT" pick '"made by clang-doc " {%d+}' "$out/README.md" | head -1)
    if [ -n "$made_by" ] && [ "$made_by" != "$version" ]; then
        echo "docs: docs/reference/ was made by clang-doc $made_by, this is $version; not compared (tools/docs.sh writes it again with $version)"
        exit 0
    fi
fi

# ---- first, can clang-doc see the standard library? ------------------------------
# One unit that includes <cstdint>, about a second. Without it a missing
# standard library shows as an error in every header, interleaved, and then a
# crash, minutes later.
rm -rf "$work"
mkdir -p "$work/probe" "$work/tu"
printf '#include <cstdint>\n' > "$work/probe/probe.cpp"
printf '[{"directory":"%s","file":"%s","arguments":["%s","-std=c++23","-stdlib=libc++","-c","%s"]}]\n' \
    "$work/probe" "$work/probe/probe.cpp" "$cxx" "$work/probe/probe.cpp" \
    > "$work/probe/compile_commands.json"
if ! "$clang_doc" --format=md --output="$work/probe/out" --executor=all-TUs --extra-arg=-v \
    "$work/probe/compile_commands.json" > "$work/probe/clang-doc.log" 2>&1 ||
    "$SQEDIT" match -qF 'error:' "$work/probe/clang-doc.log"; then
    echo "docs: clang-doc cannot compile #include <cstdint> (compiler named as $cxx)" >&2
    echo "docs: where it looked, and what it said:" >&2
    # The include search list, from its start to its end, and every error line.
    in_list=0
    while IFS= read -r l; do
        if [ "$in_list" = 1 ]; then
            printf '%s\n' "$l"
            case $l in *"End of search list"*) in_list=0 ;; esac
        else
            case $l in *"search starts here"*) printf '%s\n' "$l"; in_list=1 ;; esac
        fi
        case $l in *error:*) printf '%s\n' "$l" ;; esac
    done <"$work/probe/clang-doc.log" | head -20 | "$SQEDIT" sub -x '{.*}' '  {1}' >&2
    echo "docs: the whole log: ${work#"$root/"}/probe/clang-doc.log" >&2
    exit 2
fi

# ---- the interfaces as text --------------------------------------------------
# clang never writes comments into a named module's BMI (ASTWriter: "Don't
# write comments to BMI to reduce the size of BMI"), and clang-doc 19 and later
# keep one set of declarations already seen for the whole run (Mapper.cpp,
# USRVisited), whichever unit saw them. So a type that a unit importing it
# reached first was documented without its comment, and its own unit then
# skipped it: on the phone (clang-doc 21) most pages lost their comments, a
# different set each run (2026-11-02). clang-doc 18 merged instead, and lost
# nothing.
#
# So clang-doc reads a copy of the interfaces, build/release/docs/view/, made
# here line for line (a line stays on its line, so "Defined at" is right):
#
#   module;                     an empty line
#   export module <name>;       #pragma once
#   [export] import std;        #include "build/std.hpp": the standard headers
#                               the pinned std.cppm includes
#   [export] import sq.<m>;     #include "<m>/module.cppm"
#   [export] import :<a>.<b>;   #include "<this module>/<a>/<b>.cppm"
#   export namespace / extern   namespace / extern
#
# Every unit then sees each declaration as text, with its comment, as it did
# when they were headers. A line beginning `export` that is none of these is
# left as it is, and clang-doc stops on it: this list grows with the code.
view="$work/view"
as_text() { # as_text <module> <in> <out>
    mkdir -p "$(dirname "$3")"
    "$SQEDIT" sub -x "{~ $(printf '%s' "\
        ('module;' !.) -> ''
      / ('export module ' [^;]* ';' !.) -> '#pragma once'
      / (('export ')? 'import std;' !.) -> '#include \"build/std.hpp\"'
      / ('export ')? -> '' 'import sq.' -> '#include \"' [a-z0-9_]+ (';' !.) -> '/module.cppm\"'
      / ('export ')? -> '' 'import :' -> '#include \"$1/' ([a-z0-9_] / '.' -> '/')+ (';' !.) -> '.cppm\"'
      / 'export ' -> '' &('namespace ' / 'extern ') .*") ~}" '{1}' "$2" > "$3" || [ $? = 1 ]
}
rm -rf "$view"
mkdir -p "$view/build"
# std: the standard headers libc++'s std.cppm includes, by their names: not
# libc++'s own (<__config>), nor its #if around some. libc++'s, as the
# build's (-stdlib=libc++ below, the probe's too).
std_dir=$(bash "$root/tools/libcxx_modules.sh" "$cxx" -std=c++23 -stdlib=libc++) || exit 2
{
    echo '#pragma once'
    "$SQEDIT" pick -x '"#" " "* "include <" {[a-z] [a-z0-9_.]*} ">" .*' "$std_dir/std.cppm" |
        "$SQEDIT" sub -x '{.*}' '#include <{1}>'
} > "$view/build/std.hpp"
while IFS= read -r unit; do
    as_text "${unit%%/*}" "$root/$unit" "$view/$unit"
done < <(
    cd "$root" && find . -mindepth 2 -name '*.cppm' -not -path './build/*' \
        -not -path './third_party/*' -not -path './docs/*' -not -path '*/test/*' |
        "$SQEDIT" pick -x '"./" {.*}' | LC_ALL=C sort)

# ---- one translation unit per public interface ------------------------------
# The partitions at a module's root, not its primary (module.cppm re-exports,
# and declares nothing), nor what is under src/. -x c++: a .cppm would
# otherwise be read as a module interface.
headers=()
while IFS= read -r header; do headers+=("$header"); done < <(
    cd "$root" && find . -mindepth 2 -maxdepth 2 -name '*.cppm' -not -name module.cppm \
        -not -path './build/*' -not -path './third_party/*' -not -path './docs/*' |
        "$SQEDIT" pick -x '"./" {.*}' | LC_ALL=C sort)
{
    printf '['
    first=1
    for header in "${headers[@]}"; do
        unit="$view/$header"
        [ "$first" = 1 ] || printf ','
        first=0
        printf '\n{"directory":"%s","file":"%s","arguments":["%s","-std=c++23","-stdlib=libc++","-pthread","-D__BIONIC_CTYPE_INLINE=inline","-Wno-pragma-once-outside-header","-I%s","-x","c++","-c","%s"]}' \
            "$view" "$unit" "$cxx" "$view" "$unit"
    done
    printf '\n]\n'
} > "$work/compile_commands.json"
echo "docs: clang-doc $version over ${#headers[@]} public interfaces"

# clang-doc exits 0 after errors and writes what it could: pages with types
# missing. So an error in its log fails the run too, and nothing is written.
#
# Progress: a dot as clang-doc starts each unit, the count every 50, the way
# `sqmake headers` shows its headers, so a run on the phone is never silent.
# clang-doc says "[n/N] Processing file" on stderr, unbuffered, and the dots
# are counted by a shell `read` loop, which takes each line as it comes (awk
# would not: mawk holds its input back until the end). The full output still
# goes to the log. With pipefail the pipeline's status is clang-doc's.
#
# clang-doc runs in a subshell whose own stderr is dropped: that is where bash
# reports a command killed by a signal, and it would print the whole pipeline.
# The signal is said below instead.
clang_doc_status=0
(
    "$clang_doc" --format=md --public --output="$work/raw" --executor=all-TUs \
        "$work/compile_commands.json" 2>&1
    exit $? # a second command, so bash does not exec clang-doc in its place
) 2>/dev/null | tee "$work/clang-doc.log" | {
    n=0
    while IFS= read -r line; do
        case $line in
            *"] Processing file "*) n=$((n + 1)); dot "$n" "${#headers[@]}" ;;
        esac
    done
    dots_end "$n" "${#headers[@]}"
} || clang_doc_status=$?
errors=$("$SQEDIT" match -c '" " ("fatal ")? "error: "' "$work/clang-doc.log" || true)
if [ "$clang_doc_status" != 0 ] || [ "$errors" != 0 ]; then
    how="exit $clang_doc_status"
    if [ "$clang_doc_status" -gt 128 ]; then
        how="killed by signal $((clang_doc_status - 128))"
        [ "$clang_doc_status" = 139 ] && how="$how, a segmentation fault"
    fi
    echo "docs: clang-doc failed ($how; $errors error line(s)); nothing written" >&2
    # Units run in parallel, so lines interleave; whole error lines only, the
    # fatal ones (a header not found: usually the cause of the rest) first.
    found=$("$SQEDIT" pick '{[^ :]+ ":" %d+ ":" %d+ ": " ("fatal ")? "error: " .*}' "$work/clang-doc.log" |
        "$SQEDIT" sub -F "$root/" '' | LC_ALL=C sort -u)
    # no error lines (a crash alone) is not a second failure
    [ -z "$found" ] || {
        printf '%s\n' "$found" | "$SQEDIT" match -F 'fatal error'
        printf '%s\n' "$found" | "$SQEDIT" match -vF 'fatal error'
    } | head -8 | "$SQEDIT" sub -x '{.*}' '  {1}' >&2
    echo "docs: the whole log: ${work#"$root/"}/clang-doc.log" >&2
    exit 2
fi

# ---- the page tidying, as functions ---------------------------------------------
# tidy_page <page> <root/> <view/> <up> <1 on a namespace page>
tidy_page() {
    local root_prefix=$2 view_prefix=$3 up=$4 namespace_page=$5
    local n=0 sections=0 line path number before after list=()
    flush_list() { [ ${#list[@]} -eq 0 ] || printf '%s\n' "${list[@]}" | LC_ALL=C sort; list=(); }
    while IFS= read -r line || [ -n "$line" ]; do
        n=$((n + 1))
        # A namespace has no comment of its own in squared; clang-doc gives it
        # whatever comment sits above a `namespace a::b::c` line in any header.
        if [ "$namespace_page" = 1 ] && [ "$n" -gt 1 ] && [ "$sections" = 0 ]; then
            case $line in '## '*) ;; *) continue ;; esac
        fi
        case $line in '## '*) [ "$namespace_page" = 1 ] && [ "$sections" = 0 ] && { flush_list; echo ""; }; sections=1 ;; esac
        # "*Defined at <path>#<line>*", the path in the view (which mirrors
        # squared/): absolute, or from squared/ when clang-doc runs there
        if [[ $line =~ \*Defined\ at\ ([^#*]+)#([0-9]+)\* ]]; then
            path=${BASH_REMATCH[1]} number=${BASH_REMATCH[2]}
            before=${line%%"${BASH_REMATCH[0]}"*}
            after=${line#*"${BASH_REMATCH[0]}"}
            [ "${path#"$root_prefix"}" = "$path" ] || path=${path#"$root_prefix"}
            [ "${path#"$view_prefix"}" = "$path" ] || path=${path#"$view_prefix"}
            line="$before*Defined at [$path:$number]($up$path#L$number)*$after"
        fi
        line=${line//_Bool/bool}
        line=${line//basic_string_view/$'\001'}
        line=${line//basic_string/std::string}
        line=${line//$'\001'/std::string_view}
        # a namespace link names a directory; GitHub wants its page
        if [[ $line =~ ^\*\ Namespace:\ \[[^]]*\]\([^\).]*\)$ ]]; then line="${line%)}/index.md)"; fi
        # lists come in no fixed order (clang-doc merges in parallel): sorted
        case $line in '* ['*) list+=("$line"); continue ;; esac
        flush_list
        printf '%s\n' "$line"
    done < "$1"
    flush_list
}
# sort_blocks: each block, from a "### " or "| enum " line to the next block or
# "## " line, sorted among its neighbours; a block keeps no blank lines at its
# end, and gets one.
sort_blocks() {
    local line current= blocks=() have=0
    keep() {
        while [ "${current%$'\001'}" != "$current" ]; do current=${current%$'\001'}; done
        blocks+=("$current"$'\001'); current=; have=0
    }
    emit() {
        [ "$have" = 0 ] || keep
        [ ${#blocks[@]} -eq 0 ] || printf '%s\n' "${blocks[@]}" | LC_ALL=C sort | tr '\001' '\n'
        blocks=()
    }
    while IFS= read -r line || [ -n "$line" ]; do
        case $line in
            '## '*) emit; printf '%s\n' "$line" ;;
            '### '* | '| enum '*) [ "$have" = 0 ] || keep; current=$line; have=1 ;;
            *) if [ "$have" = 1 ]; then current=$current$'\001'$line; else printf '%s\n' "$line"; fi ;;
        esac
    done
    emit
}

# ---- tidy the pages --------------------------------------------------------------
# Kept: the namespace pages and the pages of named types under sq/.
# A dot a page, the count every 50, as above.
mkdir -p "$work/reference"
page_total=$(find "$work/raw/sq" -name '*.md' | wc -l | tr -d ' ')
echo "docs: tidying $page_total pages"
pages=0
while IFS= read -r page; do
    rel=${page#"$work/raw/"}
    mkdir -p "$work/reference/$(dirname "$rel")"
    # From docs/reference/<rel> back to squared/: one ../ per directory.
    depth=$(printf '%s' "docs/reference/$rel" | tr -cd '/' | wc -c | tr -d ' ')
    up=""
    for (( i = 0; i < depth; i++ )); do up="../$up"; done
    tidy_page "$page" "$view/" "${view#"$root/"}/" "$up" "$([ "${rel##*/}" = index.md ] && echo 1 || echo 0)" > "$work/reference/$rel"
    # Enums and functions come in no fixed order either, on a namespace page
    # and, since every unit reads a type as text (7b.4), on a type's page
    # too: each block (from its "| enum" or "###" line to the next) is sorted.
    sort_blocks < "$work/reference/$rel" > "$work/reference/$rel.sorted"
    mv "$work/reference/$rel.sorted" "$work/reference/$rel"
    pages=$((pages + 1))
    dot "$pages" "$page_total"
done < <(find "$work/raw/sq" -name '*.md' | LC_ALL=C sort)
dots_end "$pages" "$page_total"
# A link into the copy would be committed and lead nowhere: a path form the
# tidying above does not know.
find "$work/reference" -type f | LC_ALL=C sort | while IFS= read -r f; do
    if "$SQEDIT" match -qF "${view#"$root/"}" "$f"; then echo "$f"; fi
done > "$work/stray.txt"
if [ -s "$work/stray.txt" ]; then
    echo "docs: $(wc -l < "$work/stray.txt" | tr -d ' ') page(s) link into ${view#"$root/"}/, not to the interfaces; nothing written" >&2
    "$SQEDIT" sub -F "$work/reference/" '  ' "$work/stray.txt" | head -5 >&2
    exit 2
fi

{
    echo "# API reference"
    echo
    echo "Made by \`tools/docs.sh\` from the comments in the public interfaces; made by clang-doc $version."
    echo "Do not edit these pages: change the interface's comment and run \`squared/tools/docs.sh\`."
    echo
    echo "The guides are [../programmer/README.md](../programmer/README.md) (using squared) and"
    echo "[../developer/README.md](../developer/README.md) (how it works). This is the list of"
    echo "every public type and function, with its comment."
    echo
    echo "* Namespace: [sq](sq/index.md)"
} > "$work/reference/README.md"

type_pages=()
while IFS= read -r page; do type_pages+=("$page"); done < <(
    find "$work/reference/sq" -name '*.md' -not -name index.md | LC_ALL=C sort)

# ---- what has no comment ---------------------------------------------------------
# A type with nothing between its heading and its first section, a function
# with nothing under its heading but its signature and where it is defined,
# leaving out constructors, destructors and assignments.
# (clang-doc does not show members' comments, so members are not counted.)
for page in "${type_pages[@]}"; do
    file=${page#*/reference/} n=0 zone= type= name= type_described=0 function_described=0
    missing_flush() {
        if [ "$zone" = type ] && [ "$type_described" = 0 ]; then echo "$file: $type"; fi
        # constructors, destructors and assignments say what they do by their kind
        if [ "$zone" = function ] && [ "$function_described" = 0 ] &&
            [ "$name" != "$type" ] && [ "$name" != "~$type" ] && [ "$name" != "operator=" ]; then
            echo "$file: $type::$name"
        fi
    }
    while IFS= read -r line || [ -n "$line" ]; do
        n=$((n + 1))
        if [ "$n" = 1 ]; then
            type=${line#\# }
            [[ $type =~ ^[a-z]*\ * ]] && type=${type#"${BASH_REMATCH[0]}"}
            zone=type type_described=0
            continue
        fi
        case $line in
            '### '*) missing_flush; zone=function name=${line:4} function_described=0; continue ;;
            '## '*) missing_flush; zone=section; continue ;;
        esac
        [[ $line =~ ^[[:space:]]*$ || $line =~ ^\*.*\*$ || $line == 'Inherits from'* ]] && continue
        [ "$zone" = type ] && type_described=1
        [ "$zone" = function ] && function_described=1
    done < "$page"
    missing_flush
done | LC_ALL=C sort -u > "$work/missing.txt"
undocumented=$(wc -l < "$work/missing.txt" | tr -d ' ')

# ---- write, or compare --------------------------------------------------------------
if [ "$check" = 1 ]; then
    if [ -d "$out" ] && diff -r -q "$out" "$work/reference" > "$work/diff.txt" 2>&1; then
        echo "docs: docs/reference/ is current ($pages pages; $undocumented without a comment)"
        status=0
    else
        echo "docs: docs/reference/ is out of date; squared/tools/docs.sh writes it:"
        "$SQEDIT" sub -F "$work/reference" '(new)' "$work/diff.txt" | "$SQEDIT" sub -F "$out" 'docs/reference' |
            head -20 | "$SQEDIT" sub -x '{.*}' '  {1}'
        status=1
    fi
else
    rm -rf "$out"
    mkdir -p "$(dirname "$out")"
    cp -R "$work/reference" "$out"
    echo "docs: wrote docs/reference/: $pages pages; $undocumented public functions or types without a comment"
    status=0
fi
if [ "$missing" = 1 ]; then
    "$SQEDIT" sub -x '{.*}' '  {1}' "$work/missing.txt"
fi
exit "$status"
