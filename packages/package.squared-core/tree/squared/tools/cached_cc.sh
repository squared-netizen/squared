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
#                        it holds its workspace's paths). Each by the .sum
#                        beside it (tools/inputs_sum.sh), one file holding
#                        their md5s: reading the 233 files themselves for
#                        every lookup was most of a build that compiled
#                        nothing (2026-11-02); a list without one is read
#                        file by file, as before
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
#
# A lookup that finds its object runs once for every unit of a build that
# compiles nothing: 314 of them for squared, more in an app. So it starts few
# programs (2026-11-02, from about 20 to 8; each costs a few milliseconds here
# and more on the phone): the text work is bash's own (the @ROOT@ rewriting,
# the dependency file, the lists' order), and what is left is md5sum, stat,
# cp, mv and touch.

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

# rooted <text>: REPLY, the text with the workspace's directory written
# @ROOT@ (before a slash anywhere, or ending a line).
rooted() {
    local line lines=()
    while IFS= read -r line; do
        line=${line//"$root/"/@ROOT@/}
        [[ $line != *"$root" ]] || line=${line%"$root"}@ROOT@
        lines+=("$line")
    done <<<"$1"
    REPLY=
    [ ${#lines[@]} -eq 0 ] || printf -v REPLY '%s\n' "${lines[@]}"
}

# md5_text <text>: REPLY, its md5.
md5_text() {
    local sum rest
    read -r sum rest < <(md5sum <<<"$1")
    REPLY=$sum
}

# The files a dependency file names, one a line: not the target, the line
# breaks or -MP's empty rules, each once; and the .inputs lists (by their
# .sums). Only after a compile, so a few programs are fine here.
files_of() {
    {
        tr -d '\\' <"$1" | tr ' \t' '\n\n'
        local inputs
        for inputs in ${SQUARED_CACHE_INPUTS:-}; do
            if [ -f "${inputs%.inputs}.sum" ]; then
                printf '%s\n' "${inputs%.inputs}.sum"
            elif [ -f "$inputs" ]; then
                cat "$inputs"
            fi
        done
    } | grep -v -e ':$' -e '^$' -e '\.pcm$' | awk '!seen[$0]++'
}

# STATE: what the files of a list hold now, squared's own by content, the
# rest by size and date. A file gone is left out, which changes the text.
state_of() {
    local own=() other=() f text=
    while IFS= read -r f; do
        case $f in
            @ROOT@/*) own+=("$root/${f#@ROOT@/}") ;;
            *) other+=("$f") ;;
        esac
    done <"$1"
    STATE=
    if [ ${#own[@]} -gt 0 ]; then
        text=$(md5sum "${own[@]}" 2>/dev/null) || true
        rooted "$text"
        STATE=$REPLY
    fi
    if [ ${#other[@]} -gt 0 ]; then
        text=$(stat -c '%n %s %Y' "${other[@]}" 2>/dev/null) || true
        STATE+=$text
    fi
}

# key_of <list>: REPLY, k2, for what the list's files hold now.
key_of() {
    state_of "$1"
    [ -n "$STATE" ] || return 1
    md5_text "$k1
$STATE"
}

# LISTS: this source's lists, the newest first (there are at most 4).
newest_first() {
    local f t i j
    LISTS=()
    for f in "$lists"/*; do
        [ -f "$f" ] && LISTS+=("$f")
    done
    for ((i = 1; i < ${#LISTS[@]}; i++)); do
        t=${LISTS[i]}
        j=$i
        while [ "$j" -gt 0 ] && [[ $t -nt ${LISTS[j - 1]} ]]; do
            LISTS[j]=${LISTS[j - 1]}
            j=$((j - 1))
        done
        LISTS[j]=$t
    done
}

version="${SQUARED_CXX_VERSION:-$("$cxx" --version 2>/dev/null | head -n 1)}"
printf -v flag_text '%s\n' "${flags[@]}"
rooted "$flag_text"
rooted_flags=$REPLY
rooted "$source"
# k1: the version, the flags, the source's path and (the second md5) its
# contents. "inputs by .sum": lists from before the .sums are not taken.
k1_text="$version
$rooted_flags${bmi:+bmi
}inputs by .sum
$REPLY"
mapfile -t k1_sums < <(md5sum - "$source" <<<"$k1_text")
k1=${k1_sums[0]%% *}${k1_sums[1]%% *}
lists="$cache/manifests/$k1"
objects="$cache/objects"

# From the cache, when a list's files hold what they held. Fails when none.
take() {
    local list k2 f deps rules
    [ -d "$lists" ] || return 1
    newest_first
    for list in "${LISTS[@]}"; do
        key_of "$list" || continue
        k2=$REPLY
        cp -f "$objects/$k2.o" "$out.$$" 2>/dev/null || continue
        if [ -n "$bmi" ] && ! cp -f "$objects/$k2.pcm" "$bmi.$$" 2>/dev/null; then
            rm -f "$out.$$" "$bmi.$$"
            continue
        fi
        # The dependency file -MD -MP would have written, for this
        # workspace's paths.
        deps=
        rules=
        while IFS= read -r f; do
            [[ $f != @ROOT@/* ]] || f=$root/${f#@ROOT@/}
            deps+=" $f"
            rules+="$f:
"
        done <"$list"
        printf '%s:%s\n%s' "$out" "$deps" "$rules" >"$depfile.$$" || return 1
        [ -z "$bmi" ] || mv -f "$bmi.$$" "$bmi"
        mv -f "$out.$$" "$out"
        mv -f "$depfile.$$" "$depfile"
        touch "$objects/$k2.o" "$list" ${bmi:+"$objects/$k2.pcm"} 2>/dev/null || true
        [ -z "${SQUARED_HITS:-}" ] || echo hit >>"$SQUARED_HITS"
        return 0
    done
    return 1
}

# Into the cache, unless one of squared's files changed while it compiled
# (the object is then of neither version).
keep() {
    local started=$1 text name k2 f i own=()
    text=$(files_of "$depfile") || return 1
    rooted "$text"
    text=$REPLY
    while IFS= read -r f; do
        case $f in @ROOT@/*) own+=("$root/${f#@ROOT@/}") ;; esac
    done <<<"$text"
    for f in "${own[@]}"; do
        [ "$f" -nt "$started" ] && return 0
    done
    md5_text "$text"
    name=$REPLY
    mkdir -p "$lists" "$objects" || return 1
    if [ ! -f "$lists/$name" ]; then
        printf '%s' "$text" >"$lists/.$name.$$" || return 1
        mv -f "$lists/.$name.$$" "$lists/$name" || return 1
    fi
    touch "$lists/$name" || return 1
    key_of "$lists/$name" || return 1
    k2=$REPLY
    if [ -n "$bmi" ]; then
        cp -f "$bmi" "$objects/.$k2.pcm.$$" || return 1
        mv -f "$objects/.$k2.pcm.$$" "$objects/$k2.pcm" || return 1
    fi
    cp -f "$out" "$objects/.$k2.$$" || return 1
    mv -f "$objects/.$k2.$$" "$objects/$k2.o" || return 1
    # Four sets of headers is plenty: an old workspace and a new one.
    newest_first
    for ((i = 4; i < ${#LISTS[@]}; i++)); do rm -f "${LISTS[i]}"; done
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
