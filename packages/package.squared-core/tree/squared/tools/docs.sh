#!/usr/bin/env bash
# docs.sh: the API reference, docs/reference/, made by clang-doc from the
# comments in the public headers. Committed, so it reads on GitHub without a
# build; made again by this script, never edited by hand.
#
#   squared/tools/docs.sh            write docs/reference/
#   squared/tools/docs.sh --check    say whether docs/reference/ is current;
#                                    writes nothing (exit 1 when it is not)
#   squared/tools/docs.sh --missing  also list the public functions and types
#                                    that have no comment
#
# Every public header (the workhorse headers at each module's root) becomes a
# one-line translation unit that includes it, so the reference covers what a
# user can include and nothing else; the headers are self-contained, which
# `make headers` checks. clang-doc's pages are then tidied: "Defined at"
# becomes a link to the header and line, `_Bool` reads `bool`, and the pages
# for anonymous types and the global namespace are dropped.
#
# The pages depend on clang-doc's version, so README.md records the one that
# made them, and --check compares only with the same major version.
#
# Exit status: 0 written (or current), 1 --check found it out of date,
# 2 usage or environment error.
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
        -h|--help) sed -n '2,24p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
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

if [ "$check" = 1 ] && [ -f "$out/README.md" ]; then
    made_by=$(sed -n 's/.*made by clang-doc \([0-9][0-9]*\).*/\1/p' "$out/README.md" | head -1)
    if [ -n "$made_by" ] && [ "$made_by" != "$version" ]; then
        echo "docs: docs/reference/ was made by clang-doc $made_by, this is $version; not compared (tools/docs.sh writes it again with $version)"
        exit 0
    fi
fi

# ---- one translation unit per public header ----------------------------------
rm -rf "$work"
mkdir -p "$work/tu"
headers=()
while IFS= read -r header; do headers+=("$header"); done < <(
    cd "$root" && find . -mindepth 2 -maxdepth 2 -name '*.hpp' \
        -not -path './build/*' -not -path './third_party/*' -not -path './docs/*' |
        sed 's|^\./||' | LC_ALL=C sort)
{
    printf '['
    first=1
    for header in "${headers[@]}"; do
        unit="$work/tu/$(printf '%s' "$header" | tr '/.' '__').cpp"
        printf '#include <squared/%s>\n' "$header" > "$unit"
        [ "$first" = 1 ] || printf ','
        first=0
        printf '\n{"directory":"%s","file":"%s","arguments":["clang++","-std=c++20","-I%s","-c","%s"]}' \
            "$work" "$unit" "$tree" "$unit"
    done
    printf '\n]\n'
} > "$work/compile_commands.json"
echo "docs: clang-doc $version over ${#headers[@]} public headers"

"$clang_doc" --format=md --public --output="$work/raw" --executor=all-TUs \
    "$work/compile_commands.json" > "$work/clang-doc.log" 2>&1 || {
    echo "docs: clang-doc failed; the end of its log:" >&2
    tail -20 "$work/clang-doc.log" >&2
    exit 2
}

# ---- tidy the pages --------------------------------------------------------------
# Kept: the namespace pages and the pages of named types under sq/.
mkdir -p "$work/reference"
pages=0
while IFS= read -r page; do
    rel=${page#"$work/raw/"}
    mkdir -p "$work/reference/$(dirname "$rel")"
    # From docs/reference/<rel> back to squared/: one ../ per directory.
    depth=$(printf '%s' "docs/reference/$rel" | tr -cd '/' | wc -c | tr -d ' ')
    up=""
    for (( i = 0; i < depth; i++ )); do up="../$up"; done
    awk -v root="$root/" -v up="$up" -v namespace_page="$([ "${rel##*/}" = index.md ] && echo 1 || echo 0)" '
        # A namespace has no comment of its own in squared; clang-doc gives it
        # whatever comment sits above a `namespace a::b::c` line in any header.
        namespace_page && NR > 1 && !sections && !/^## / { next }
        namespace_page && /^## / && !sections { print "" }
        /^## / { sections = 1 }
        {
            line = $0
            # "*Defined at <path>#<line>*", the path absolute or under squared/
            if( match( line, /\*Defined at [^#*]+#[0-9]+\*/ ) ) {
                inner = substr( line, RSTART + 12, RLENGTH - 13 )
                hash = index( inner, "#" )
                path = substr( inner, 1, hash - 1 )
                number = substr( inner, hash + 1 )
                if( index( path, root ) == 1 ) { path = substr( path, length( root ) + 1 ) }
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
    # A namespace page's enums and functions come in no fixed order either:
    # each block (from its "| enum" or "###" line to the next) is sorted.
    if [ "${rel##*/}" = index.md ]; then
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
    fi
    pages=$((pages + 1))
done < <(find "$work/raw/sq" -name '*.md' | LC_ALL=C sort)

{
    echo "# API reference"
    echo
    echo "Made by \`tools/docs.sh\` from the comments in the public headers; made by clang-doc $version."
    echo "Do not edit these pages: change the header's comment and run \`squared/tools/docs.sh\`."
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
