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
        -h|--help) sed -n '2,26p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
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
version=$("$clang_doc" --version 2>/dev/null | sed -n 's/.*version \([0-9][0-9]*\).*/\1/p' | head -1)
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
    made_by=$(sed -n 's/.*made by clang-doc \([0-9][0-9]*\).*/\1/p' "$out/README.md" | head -1)
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
    grep -q 'error:' "$work/probe/clang-doc.log"; then
    echo "docs: clang-doc cannot compile #include <cstdint> (compiler named as $cxx)" >&2
    echo "docs: where it looked, and what it said:" >&2
    sed -n '/search starts here/,/End of search list/p; /error:/p' "$work/probe/clang-doc.log" |
        head -20 | sed 's/^/  /' >&2
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
    awk -v module="$1" '
        /^module;$/ { print ""; next }
        /^export module [^;]*;$/ { print "#pragma once"; next }
        /^(export )?import std;$/ { print "#include \"build/std.hpp\""; next }
        /^(export )?import sq\.[a-z0-9_]+;$/ {
            name = $0; sub( /^(export )?import sq\./, "", name ); sub( /;$/, "", name )
            print "#include \"" name "/module.cppm\""; next
        }
        /^(export )?import :[a-z0-9_.]+;$/ {
            part = $0; sub( /^(export )?import :/, "", part ); sub( /;$/, "", part )
            gsub( /\./, "/", part )
            print "#include \"" module "/" part ".cppm\""; next
        }
        /^export (namespace|extern) / { sub( /^export /, "" ); print; next }
        { print }' "$2" > "$3"
}
rm -rf "$view"
mkdir -p "$view/build"
# std: the standard headers the pinned std.cppm includes (third_party/libcxx/),
# by their names: not libc++'s own (<__config>), nor its #if around some.
# libc++'s, as the build's (-stdlib=libc++ below, the probe's too).
if [ ! -f "$root/third_party/libcxx/std.cppm" ]; then
    echo "docs: no third_party/libcxx/std.cppm: pin libc++'s std module sources (tools/pin_libcxx_modules.sh)" >&2
    exit 2
fi
{
    echo '#pragma once'
    sed -n 's/^#[ ]*include <\([a-z][a-z0-9_.]*\)>.*/#include <\1>/p' "$root/third_party/libcxx/std.cppm"
} > "$view/build/std.hpp"
while IFS= read -r unit; do
    as_text "${unit%%/*}" "$root/$unit" "$view/$unit"
done < <(
    cd "$root" && find . -mindepth 2 -name '*.cppm' -not -path './build/*' \
        -not -path './third_party/*' -not -path './docs/*' -not -path '*/test/*' |
        sed 's|^\./||' | LC_ALL=C sort)

# ---- one translation unit per public interface ------------------------------
# The partitions at a module's root, not its primary (module.cppm re-exports,
# and declares nothing), nor what is under src/. -x c++: a .cppm would
# otherwise be read as a module interface.
headers=()
while IFS= read -r header; do headers+=("$header"); done < <(
    cd "$root" && find . -mindepth 2 -maxdepth 2 -name '*.cppm' -not -name module.cppm \
        -not -path './build/*' -not -path './third_party/*' -not -path './docs/*' |
        sed 's|^\./||' | LC_ALL=C sort)
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
# `make headers` shows its headers, so a run on the phone is never silent.
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
errors=$(grep -cE ' (fatal )?error: ' "$work/clang-doc.log" || true)
if [ "$clang_doc_status" != 0 ] || [ "$errors" != 0 ]; then
    how="exit $clang_doc_status"
    if [ "$clang_doc_status" -gt 128 ]; then
        how="killed by signal $((clang_doc_status - 128))"
        [ "$clang_doc_status" = 139 ] && how="$how, a segmentation fault"
    fi
    echo "docs: clang-doc failed ($how; $errors error line(s)); nothing written" >&2
    # Units run in parallel, so lines interleave; whole error lines only, the
    # fatal ones (a header not found: usually the cause of the rest) first.
    grep -oE '[^ ]+:[0-9]+:[0-9]+: (fatal )?error: .*' "$work/clang-doc.log" |
        sed "s|$root/||" | LC_ALL=C sort -u |
        awk '/fatal error/ { print; next } { rest[++n] = $0 } END { for( i = 1; i <= n; i++ ) print rest[i] }' |
        head -8 | sed 's/^/  /' >&2 || true # no error lines (a crash alone) is not a second failure
    echo "docs: the whole log: ${work#"$root/"}/clang-doc.log" >&2
    exit 2
fi

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
    awk -v root="$view/" -v view="${view#"$root/"}/" -v up="$up" -v namespace_page="$([ "${rel##*/}" = index.md ] && echo 1 || echo 0)" '
        # A namespace has no comment of its own in squared; clang-doc gives it
        # whatever comment sits above a `namespace a::b::c` line in any header.
        namespace_page && NR > 1 && !sections && !/^## / { next }
        namespace_page && /^## / && !sections { print "" }
        /^## / { sections = 1 }
        {
            line = $0
            # "*Defined at <path>#<line>*", the path in the view (which mirrors
            # squared/): absolute, or from squared/ when clang-doc runs there
            if( match( line, /\*Defined at [^#*]+#[0-9]+\*/ ) ) {
                inner = substr( line, RSTART + 12, RLENGTH - 13 )
                hash = index( inner, "#" )
                path = substr( inner, 1, hash - 1 )
                number = substr( inner, hash + 1 )
                if( index( path, root ) == 1 ) { path = substr( path, length( root ) + 1 ) }
                if( index( path, view ) == 1 ) { path = substr( path, length( view ) + 1 ) }
                line = substr( line, 1, RSTART - 1 ) "*Defined at [" path ":" number "](" up path "#L" number ")*" substr( line, RSTART + RLENGTH )
            }
            gsub( /_Bool/, "bool", line )
            gsub( /basic_string_view/, "\001", line )
            gsub( /basic_string/, "std::string", line )
            gsub( /\001/, "std::string_view", line )
            # a namespace link names a directory; GitHub wants its page
            if( line ~ /^\* Namespace: \[[^]]*\]\([^).]*\)$/ ) { sub( /\)$/, "/index.md)", line ) }
            # lists come in no fixed order (clang-doc merges in parallel): sorted
            if( line ~ /^\* \[/ ) {
                if( !listing ) { fflush() } # what came before goes first
                print line | "LC_ALL=C sort"; listing = 1; next
            }
            if( listing ) { close( "LC_ALL=C sort" ); listing = 0 }
            print line
        }
        END { if( listing ) { close( "LC_ALL=C sort" ) } }' "$page" > "$work/reference/$rel"
    # Enums and functions come in no fixed order either, on a namespace page
    # and, since every unit reads a type as text (7b.4), on a type's page
    # too: each block (from its "| enum" or "###" line to the next) is sorted.
    awk '
        # A block keeps no blank lines at its end, and gets one: where a
        # block falls, so do the blank lines after it.
        function keep() {
            sub( /(\001)+$/, "", current )
            blocks[++count] = current "\001"
            current = ""
        }
        function emit(   i, command ) {
            if( current != "" ) { keep() }
            if( count == 0 ) { return }
            fflush()
            command = "LC_ALL=C sort | tr \"\\001\" \"\\n\""
            for( i = 1; i <= count; i++ ) { print blocks[i] | command }
            close( command )
            count = 0
        }
        /^## / { emit(); print; next }
        /^### / || /^\| enum / {
            if( current != "" ) { keep() }
            current = $0
            next
        }
        current != "" { current = current "\001" $0; next }
        { print }
        END { emit() }' "$work/reference/$rel" > "$work/reference/$rel.sorted"
    mv "$work/reference/$rel.sorted" "$work/reference/$rel"
    pages=$((pages + 1))
    dot "$pages" "$page_total"
done < <(find "$work/raw/sq" -name '*.md' | LC_ALL=C sort)
dots_end "$pages" "$page_total"
# A link into the copy would be committed and lead nowhere: a path form the
# tidying above does not know.
if grep -rlF "${view#"$root/"}" "$work/reference" > "$work/stray.txt"; then
    echo "docs: $(wc -l < "$work/stray.txt" | tr -d ' ') page(s) link into ${view#"$root/"}/, not to the interfaces; nothing written" >&2
    sed "s|$work/reference/|  |" "$work/stray.txt" | head -5 >&2
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
awk '
    function flush() {
        if( zone == "type" && !type_described ) { print file ": " type }
        # constructors, destructors and assignments say what they do by their kind
        special = name == type || name == "~" type || name == "operator="
        if( zone == "function" && !function_described && !special ) { print file ": " type "::" name }
    }
    FNR == 1 {
        flush()
        file = FILENAME; sub( /.*\/reference\//, "", file )
        type = $0; sub( /^# [a-z]* */, "", type )
        zone = "type"; type_described = 0
        next
    }
    /^### / { flush(); zone = "function"; name = substr( $0, 5 ); function_described = 0; next }
    /^## / { flush(); zone = "section"; next }
    /^[ \t]*$/ || /^\*.*\*$/ || /^Inherits from/ { next }
    {
        if( zone == "type" ) { type_described = 1 }
        if( zone == "function" ) { function_described = 1 }
    }
    END { flush() }
' "${type_pages[@]}" | LC_ALL=C sort -u > "$work/missing.txt"
undocumented=$(wc -l < "$work/missing.txt" | tr -d ' ')

# ---- write, or compare --------------------------------------------------------------
if [ "$check" = 1 ]; then
    if [ -d "$out" ] && diff -r -q "$out" "$work/reference" > "$work/diff.txt" 2>&1; then
        echo "docs: docs/reference/ is current ($pages pages; $undocumented without a comment)"
        status=0
    else
        echo "docs: docs/reference/ is out of date; squared/tools/docs.sh writes it:"
        sed 's|'"$work/reference"'|(new)|; s|'"$out"'|docs/reference|' "$work/diff.txt" | head -20 | sed 's/^/  /'
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
    sed 's/^/  /' "$work/missing.txt"
fi
exit "$status"
