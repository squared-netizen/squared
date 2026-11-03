#!/usr/bin/env bash
# tools/std_bmi.sh: `import std;` for one build, from the libc++ module
# sources pinned in third_party/libcxx/ (C++23; build/common.mk runs it).
#
#   std_bmi.sh <module dir> <archive> <compiler> <flags>...
#
# The std module is the whole standard library, a BMI of about 30 MB, the
# same for every project built with the same compiler, flags and libc++. So
# it is made once for each of those, in
#
#   $SQUARED_STD_HOME/<key>/       src/ (the pinned sources, copied: the BMI
#                                  names its sources' paths), std.pcm, std.o,
#                                  std.d (what it read), done
#
# where <key> is a hash of the compiler's version line, the flags (less the
# path maps, which name the workspace), the PINNED record and the libc++
# headers' sizes and dates. SQUARED_STD_HOME is ~/sqsysroot/.sqpg/std when
# squared-pg is set up, else this build's modules/std. The build then gets:
#
#   <module dir>/std.pcm      a link to <key>/std.pcm, where every compile
#                             finds it (-fprebuilt-module-path)
#   <module dir>/std.d        <key>/std.d, for make: a libc++ header that
#                             changes makes the BMI again
#   <module dir>/std.inputs   for the build cache's key: PINNED, and
#                             <module dir>/std.key (this BMI's key), so an
#                             object or BMI made against another std module
#                             (a libc++ updated in place) is never taken
#   <archive>                 <key>/std.o, which every program links
#
# The copy, not the pinned files, is made to fit the headers it is built
# against (seen on the phone, 2026-11-02: the sources pinned from the NDK r29,
# Termux's headers from r30, both "libc++ 210000"):
#
#   - libc++'s "headers not yet available" checks go: `#error "please update
#     the header information for <flat_set>"` when the headers have one the
#     sources do not export. Bookkeeping for libc++'s own developers; the
#     module simply does not export that header's names.
#   - A name the C library defines static (Termux's <stdio.h> makes tmpfile
#     a static inline function) cannot be exported: its `using` line is left
#     out, found by a first compile that only checks ("using declaration
#     referring to 'tmpfile' with internal linkage cannot be exported"), and
#     said: `std  left out: tmpfile`. std::tmpfile is then not in the module
#     on that libc; squared does not use it.
#
# Two builds at once, of the same key, both compile and the second rename
# wins, the same file; a build never reads a half-written one. The
# SQUARED_STD_KEEP (6) most lately used keys are kept, older ones removed (a
# build whose link then dangles makes it again: build/common.mk).
#
# Refuses, with what to do: a standard library that is not libc++; libc++
# headers of another major version than PINNED.
#
# SQUARED_SAY: empty for make V=1 (the commands print instead).

set -euo pipefail

if [ $# -lt 3 ]; then
    echo "usage: std_bmi.sh <module dir> <archive> <compiler> <flags>..." >&2
    exit 2
fi
modules=$1
archive=$2
cxx=$3
shift 3
flags=("$@")

root="$(cd "$(dirname "$0")/.." && pwd)" # squared/
pinned_dir="$root/third_party/libcxx"
pinned="$pinned_dir/PINNED"
home=${SQUARED_STD_HOME:-$modules/std}
keep=${SQUARED_STD_KEEP:-6}
say=${SQUARED_SAY-1}

fail() {
    echo "std_bmi: $*" >&2
    exit 1
}

[ -f "$pinned" ] && [ -f "$pinned_dir/std.cppm" ] ||
    fail "no pinned libc++ module sources in third_party/libcxx/: tools/pin_libcxx_modules.sh <share/libc++/v1> (third_party/libcxx/README.md)"

# --- which libc++ the flags compile against ---------------------------------
# The version, and the headers it read (their sizes and dates go in the key:
# a libc++ updated in place makes a new BMI).
probe=$(printf '#include <version>\n_LIBCPP_VERSION\n' |
    "$cxx" "${flags[@]}" -x c++ -E -P - 2>/dev/null | tail -n 1 | tr -d ' ') ||
    fail "$cxx cannot compile #include <version> with these flags: is libc++ installed for it? (Debian and Ubuntu: apt install libc++-dev libc++abi-dev; Termux and the NDK have it)"
case $probe in
    [0-9][0-9][0-9][0-9][0-9]*) ;;
    *) fail "the standard library is not libc++ (squared builds import std from libc++ only: -stdlib=libc++; Debian and Ubuntu: apt install libc++-dev libc++abi-dev)" ;;
esac
want=$(sed -n 's/^libc++: \([0-9]*\).*/\1/p' "$pinned")
if [ "$((probe / 10000))" != "$((want / 10000))" ]; then
    fail "libc++ headers $probe, module sources pinned from libc++ $want: pin again from this libc++ (tools/pin_libcxx_modules.sh, third_party/libcxx/README.md)"
fi
headers=$(printf '#include <version>\n' | "$cxx" "${flags[@]}" -x c++ -M - 2>/dev/null |
    tr -d '\\' | tr ' ' '\n' | grep '^/' | LC_ALL=C sort -u) || true
[ -n "$headers" ] || fail "$cxx could not list the headers <version> reads"

# --- the key --------------------------------------------------------------------
kept_flags=()
for f in "${flags[@]}"; do
    case $f in
        -ffile-prefix-map=* | -fdebug-prefix-map=* | -fmacro-prefix-map=*) ;;
        *) kept_flags+=("$f") ;;
    esac
done
key=$({
    "$cxx" --version 2>/dev/null | head -n 1
    printf '%s\n' "$cxx" "${kept_flags[@]}"
    cat "$pinned"
    sha256sum < "$0" # how the copy is fitted (below) is part of what it is
    # shellcheck disable=SC2086 # one path a line, none with spaces
    stat -c '%n %s %Y' $headers
} | sha256sum | cut -c1-16)
dir="$home/$key"
shown=$dir
shown=${shown/#"$HOME"/\~}

# --- the copy, fitted to these headers --------------------------------------------
# fit <src dir>: the header-availability checks out, then a checking compile
# (no output) until no `using` names a static function, at most 3 rounds: each
# such line is commented out and its name added to <src dir>/LEFT_OUT. Any
# other error is left for the real compile, which shows it.
fit() {
    local src=$1 round out found file line name
    awk '
        /^#[ \t]*if __has_include\(<[^>]*>\)[ \t]*$/ { held = $0; next }
        held != "" {
            if( $0 ~ /please update the header information/ ) { held = ""; dropping = 1; next }
            print held
            held = ""
        }
        dropping && /^#[ \t]*endif/ { dropping = 0; next }
        { print }
        END { if( held != "" ) print held }' "$src/std.cppm" > "$src/std.cppm.new"
    mv -f "$src/std.cppm.new" "$src/std.cppm"
    : > "$src/LEFT_OUT"
    for round in 1 2 3; do
        out=$("$cxx" "${flags[@]}" -w -Wno-reserved-module-identifier -fsyntax-only "$src/std.cppm" 2>&1) && return 0
        found=$(printf '%s\n' "$out" | sed -n "s/^\\(.*\\):\\([0-9][0-9]*\\):[0-9][0-9]*: error: using declaration referring to '\\([^']*\\)' with internal linkage cannot be exported.*/\\1 \\2 \\3/p")
        [ -n "$found" ] || return 0
        while read -r file line name; do
            sed -i "${line}s|^|// squared: left out, static in this C library: |" "$file"
            printf '%s\n' "$name" >> "$src/LEFT_OUT"
        done <<< "$found"
    done
}

# --- made already, or make it ---------------------------------------------------
if [ -f "$dir/done" ]; then
    [ -z "$say" ] || printf '  %-4s %s\n' mod "std (libc++ $probe, made before: $shown)"
else
    [ -z "$say" ] || printf '  %-4s %s\n' mod "std (libc++ $probe, into $shown)"
    mkdir -p "$dir"
    # The sources, beside the BMI that names them: copied whole and fitted,
    # then renamed into place, so a build of the same key never reads a
    # half-made one.
    if [ ! -d "$dir/src" ]; then
        rm -rf "$dir/src.$$"
        mkdir -p "$dir/src.$$"
        cp -R "$pinned_dir/std.cppm" "$pinned_dir/std" "$dir/src.$$/"
        fit "$dir/src.$$"
        mv -T "$dir/src.$$" "$dir/src" 2>/dev/null || rm -rf "$dir/src.$$"
    fi
    if [ -s "$dir/src/LEFT_OUT" ] && [ -n "$say" ]; then
        printf '  %-4s %s\n' std "left out, static in this C library's headers: $(paste -sd ' ' "$dir/src/LEFT_OUT")"
    fi
    # -w: libc++'s own code, not squared's to warn about. Its module name is
    # one the standard reserves, for the library itself: no warning for that.
    [ -z "$say" ] && set -x
    "$cxx" "${flags[@]}" -w -Wno-reserved-module-identifier \
        -MD -MP -MT std.pcm -MF "$dir/std.d.$$" \
        -c "$dir/src/std.cppm" -fmodule-output="$dir/std.pcm.$$" -o "$dir/std.o.$$"
    { set +x; } 2>/dev/null
    mv -f "$dir/std.pcm.$$" "$dir/std.pcm"
    mv -f "$dir/std.o.$$" "$dir/std.o"
    mv -f "$dir/std.d.$$" "$dir/std.d"
    : > "$dir/done"
fi
touch "$dir"

# --- this build's view of it ----------------------------------------------------
mkdir -p "$modules" "$(dirname "$archive")"
ln -sfn "$dir/std.pcm" "$modules/std.pcm"
# With an empty rule for the source too (-MP gives every header one, not the
# file compiled): a shared copy pruned or wiped makes make run this again,
# not stop at "No rule to make target".
{
    sed "1s|^std\\.pcm:|$modules/std.ready:|" "$dir/std.d"
    printf '%s:\n' "$dir/src/std.cppm"
} > "$modules/std.d"
printf '%s\n' "$key" > "$modules/std.key"
printf '%s\n%s\n' "$pinned" "$modules/std.key" > "$modules/std.inputs"
rm -f "$archive"
"${AR:-ar}" rcs "$archive" "$dir/std.o"

# --- the keys not used lately go ------------------------------------------------
if [ -d "$home" ]; then
    # shellcheck disable=SC2012 # names are hex keys
    ls -t "$home" 2>/dev/null | tail -n +"$((keep + 1))" | while IFS= read -r old; do
        case $old in
            [0-9a-f]*) [ "$home/$old" = "$dir" ] || rm -rf "${home:?}/$old" ;;
        esac
    done
fi
