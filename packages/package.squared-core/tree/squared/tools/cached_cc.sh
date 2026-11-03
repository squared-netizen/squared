#!/usr/bin/env bash
# tools/cached_cc.sh <compiler> <flags...> -c <source> -o <object>
#
# Compile one framework source, or take its object from the build cache when
# the same source, reading the same headers, was compiled by the same
# compiler with the same flags before: in this workspace or in any other
# (7b.3). Either way it writes <object> and <object less .o>.d, the
# dependency file a compile with -MD -MP writes.
#
# The cache (SQUARED_CACHE, a directory; build/common.mk chooses it):
#   manifests/<k1>/<list>   what a compile of this source with these flags
#                           read, one file a line, squared's own written
#                           @ROOT@/<path>; up to 4, one per set of headers
#   objects/<k2>.o          an object
#   objects/<k2>.pcm        and, for a module interface unit, its BMI
# k1 is an md5 of the compiler's version, the flags (the workspace's own
# directory in them written @ROOT@), the source's path under @ROOT@ and its
# contents. k2 is an md5 of k1 and what each file of a list holds now:
# squared's own by their contents (a vendored copy is dated when it was
# copied, so its dates say nothing), the compiler's and the system's by size
# and date (they change only with a package update, and reading every one
# for every object would cost more than the compile saves). A list whose k2
# names an object: the object is copied out, the list written as the
# dependency file, for this workspace's paths. Else it is compiled, and kept.
#
# Objects do not name the workspace: build/common.mk compiles with
# -ffile-prefix-map, so two workspaces share one copy. Every file is written
# beside its name and renamed into place, so builds at once under make -j, or
# in two workspaces, never leave half of one, and a lookup that loses a race
# (an object pruned between finding and copying it) compiles. The cache is
# pruned by tools/cache_prune.sh, least lately used first: each use touches
# what it used.
#
#   SQUARED_CACHE        the cache; empty or off: compile, keep nothing
#   SQUARED_PUBLIC_ROOT  the directory that holds squared/ (written @ROOT@)
#   SQUARED_CXX_VERSION  the compiler's version, one line
#   SQUARED_SAY          what to call the source in `  cc   <it>`; empty:
#                        nothing printed (make V=1 prints the command)
#   SQUARED_HITS         a file that gets a line for each object from the
#                        cache (the archive's line counts them); may be empty
#   SQUARED_CACHE_INPUTS files (build/modules.mk's .inputs) naming the module
#                        interfaces whose BMIs this compile may import: they
#                        join what it read, as clang's dependency file does
#                        not name them (a BMI itself is never part of a key:
#                        it holds its workspace's paths)
#   SQUARED_SAY_LINE     the whole line to print instead of `  cc   <it>`
#                        (an interface says `  mod  sq.gui-label`)
#
# A module interface unit (a .cppm, with -fmodule-output=<bmi>) is cached the
# same way, its BMI with its object, and a hit gives both, or neither (the
# prune may have taken one). Since 2026-11-02; before, the 233 interfaces were
# compiled in every workspace. A BMI names its sources by path, so a BMI from
# another workspace would read that workspace's files: build/modules.mk
# compiles interfaces with -Xclang -fmodules-embed-all-files, which puts the
# sources in the BMI (here 40 MB of BMIs became 41 MB), and an importer reads
# them from there, the other workspace gone or not. The imported BMIs are
# found by name (-fprebuilt-module-path), in this workspace.

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "usage: cached_cc.sh <compiler> <flags...> -c <source> -o <object>" >&2
    exit 2
fi
cxx=$1
shift

out=
source=
bmi=
flags=()
while [ $# -gt 0 ]; do
    case $1 in
        -o)
            out=$2
            shift 2
            continue
            ;;
        -c) ;;
        -MMD | -MD | -MP) ;; # this script writes the dependency file
        -MF | -MT | -MQ)
            shift 2
            continue
            ;;
        *.cpp | *.cc | *.c | *.cppm)
            if [ -z "$source" ]; then source=$1; else flags+=("$1"); fi
            ;;
        -fmodule-output=*) bmi=${1#-fmodule-output=} ;;
        *) flags+=("$1") ;;
    esac
    shift
done
if [ -z "$out" ] || [ -z "$source" ]; then
    echo "cached_cc.sh: needs -c <source> and -o <object>" >&2
    exit 2
fi
depfile="${out%.o}.d"
root="${SQUARED_PUBLIC_ROOT%/}"
cache="${SQUARED_CACHE:-}"
[ "$cache" = off ] && cache=

# The BMI's flag, when there is one: written beside its name, then renamed.
bmi_flag=()
[ -z "$bmi" ] || bmi_flag=(-fmodule-output="$bmi.$$")

say() {
    if [ -n "${SQUARED_SAY_LINE:-}" ]; then
        printf '%s\n' "$SQUARED_SAY_LINE"
    elif [ -n "${SQUARED_SAY:-}" ]; then
        printf '  %-4s %s\n' cc "$SQUARED_SAY"
    fi
}

compile() {
    say
    "$cxx" "${flags[@]}" "${bmi_flag[@]}" -MD -MP -MT "$out" -MF "$depfile.$$" -c "$source" -o "$out.$$"
    [ -z "$bmi" ] || mv -f "$bmi.$$" "$bmi"
    mv -f "$out.$$" "$out"
    mv -f "$depfile.$$" "$depfile"
}

if [ -z "$cache" ] || [ -z "$root" ]; then
    compile
    exit 0
fi

# The workspace's directory, for sed: as a pattern, and as a replacement.
root_pattern=$(printf '%s' "$root" | sed 's/[][\.*^$|]/\\&/g')
root_text=$(printf '%s' "$root" | sed 's/[\&|]/\\&/g')

# @ROOT@ for the workspace's directory, in a text; and back.
rooted() { sed -e "s|$root_pattern/|@ROOT@/|g" -e "s|$root_pattern\$|@ROOT@|"; }
unrooted() { sed "s|^@ROOT@/|$root_text/|"; }

md5() { md5sum | cut -d' ' -f1; }

# The files a dependency file names, one a line: not the target, the line
# breaks or -MP's empty rules, each once.
files_of() {
    {
        tr -d '\\' <"$1" | tr ' \t' '\n\n'
        local inputs
        for inputs in ${SQUARED_CACHE_INPUTS:-}; do
            [ ! -f "$inputs" ] || cat "$inputs"
        done
    } | grep -v -e ':$' -e '^$' -e '\.pcm$' | awk '!seen[$0]++'
}

# What the files of a list hold now: squared's own by content, the rest by
# size and date. A file gone is left out, which changes the text.
state_of() {
    local own=() other=() f
    while IFS= read -r f; do
        case $f in
            @ROOT@/*) own+=("$root/${f#@ROOT@/}") ;;
            *) other+=("$f") ;;
        esac
    done <"$1"
    if [ ${#own[@]} -gt 0 ]; then md5sum "${own[@]}" 2>/dev/null | rooted || true; fi
    if [ ${#other[@]} -gt 0 ]; then stat -c '%n %s %Y' "${other[@]}" 2>/dev/null || true; fi
}

# key_of <list>: k2, for what the list's files hold now.
key_of() {
    local state
    state=$(state_of "$1")
    [ -n "$state" ] || return 1
    printf '%s\n%s\n' "$k1" "$state" | md5
}

version="${SQUARED_CXX_VERSION:-$("$cxx" --version 2>/dev/null | head -n 1)}"
k1=$( {
    printf '%s\n' "$version"
    printf '%s\n' "${flags[@]}" | rooted
    [ -z "$bmi" ] || printf 'bmi\n'
    printf '%s\n' "$source" | rooted
    md5 <"$source"
} | md5)
lists="$cache/manifests/$k1"
objects="$cache/objects"

# From the cache, when a list's files hold what they held. Fails when none.
take() {
    local list k2
    [ -d "$lists" ] || return 1
    for list in $(ls -t "$lists" 2>/dev/null); do
        [ -f "$lists/$list" ] || continue
        k2=$(key_of "$lists/$list") || continue
        cp -f "$objects/$k2.o" "$out.$$" 2>/dev/null || continue
        if [ -n "$bmi" ] && ! cp -f "$objects/$k2.pcm" "$bmi.$$" 2>/dev/null; then
            rm -f "$out.$$" "$bmi.$$"
            continue
        fi
        {
            printf '%s:' "$out"
            unrooted <"$lists/$list" | sed 's|^| |' | tr -d '\n'
            printf '\n'
            unrooted <"$lists/$list" | sed 's|$|:|'
        } >"$depfile.$$" || return 1
        [ -z "$bmi" ] || mv -f "$bmi.$$" "$bmi"
        mv -f "$out.$$" "$out"
        mv -f "$depfile.$$" "$depfile"
        touch "$objects/$k2.o" "$lists/$list" 2>/dev/null || true
        [ -z "$bmi" ] || touch "$objects/$k2.pcm" 2>/dev/null || true
        [ -z "${SQUARED_HITS:-}" ] || echo hit >>"$SQUARED_HITS"
        return 0
    done
    return 1
}

# Into the cache, unless one of squared's files changed while it compiled
# (the object is then of neither version).
keep() {
    local started=$1 text name k2 f own=()
    text=$(files_of "$depfile" | rooted) || return 1
    while IFS= read -r f; do
        case $f in @ROOT@/*) own+=("$root/${f#@ROOT@/}") ;; esac
    done <<<"$text"
    for f in "${own[@]}"; do
        [ "$f" -nt "$started" ] && return 0
    done
    name=$(printf '%s\n' "$text" | md5)
    mkdir -p "$lists" "$objects" || return 1
    if [ ! -f "$lists/$name" ]; then
        printf '%s\n' "$text" >"$lists/.$name.$$" || return 1
        mv -f "$lists/.$name.$$" "$lists/$name" || return 1
    fi
    touch "$lists/$name" || return 1
    k2=$(key_of "$lists/$name") || return 1
    if [ -n "$bmi" ]; then
        cp -f "$bmi" "$objects/.$k2.pcm.$$" || return 1
        mv -f "$objects/.$k2.pcm.$$" "$objects/$k2.pcm" || return 1
    fi
    cp -f "$out" "$objects/.$k2.$$" || return 1
    mv -f "$objects/.$k2.$$" "$objects/$k2.o" || return 1
    # Four sets of headers is plenty: an old workspace and a new one.
    for f in $(ls -t "$lists" | tail -n +5); do rm -f "$lists/$f"; done
    return 0
}

if take; then
    exit 0
fi
rm -f "$out.$$" "$depfile.$$"
[ -z "$bmi" ] || rm -f "$bmi.$$"

started="$out.started.$$"
mkdir -p "$(dirname "$out")"
: >"$started"
trap 'rm -f "$started" "$out.$$" "$depfile.$$" ${bmi:+"$bmi.$$"}' EXIT
compile
if ! keep "$started"; then
    rm -f "$objects"/.*."$$" "$lists"/.*."$$" 2>/dev/null || true
fi
