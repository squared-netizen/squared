#!/usr/bin/env bash
# tools/std_module.sh [--check]
#
# Write build/std.cppm: sq.std, squared's own module of the standard library
# (7b.4). Every squared file writes `import sq.std;` where it wrote standard
# includes: the standard headers are parsed once per flag set, into one BMI,
# and a file reads from it only what it uses. It is the `import std` that
# Termux's libc++ lacks and the NDK's std.cppm cannot yet build.
#
# What it exports: every name squared's sources write as std::<name> (and
# std::chrono::, std::filesystem::, std::this_thread::, std::ranges::), the
# operators argument-dependent lookup finds in an importer only when they are
# exported, chrono's literals, and libstdc++'s iterator operators (in
# __gnu_cxx). The headers it includes are the list below.
#
#   tools/std_module.sh            write it again
#   tools/std_module.sh --check    say whether it is current; exit 1 when not
#                                  (make headers runs it)
#
# Not exported, on purpose:
#   the <cctype> functions (isxdigit, ...): bionic declares them with internal
#     linkage, which a module cannot export; a file that uses one includes
#     <ctype.h> and calls it unqualified
#   the C macros (assert, errno, va_list's va_start, UINT32_MAX, SIGSEGV,
#     offsetof, stdout): macros do not travel through modules; a file that
#     uses one includes the C header that defines it (<assert.h>, ...)

set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
target="$root/build/std.cppm"
check=0
case "${1:-}" in
    --check) check=1 ;;
    "") ;;
    *) echo "usage: tools/std_module.sh [--check]" >&2; exit 2 ;;
esac

# The standard headers sq.std includes. A name from a header not here fails
# the module's build ("no member named"): add the header.
headers="algorithm array atomic cassert cctype cerrno charconv chrono cmath compare concepts csignal
cstdarg cstddef cstdint cstdio cstdlib cstring ctime deque exception filesystem fstream functional
initializer_list iterator limits map memory new numeric optional random span sstream stdexcept string
string_view system_error thread tuple type_traits unordered_map unordered_set utility variant vector"

# Names that are not exported (above), and names that are members, not names.
skip='^(isalnum|isalpha|isblank|iscntrl|isdigit|isgraph|islower|isprint|ispunct|isspace|isupper|isxdigit|tolower|toupper|type_index|npos|chrono_literals|literals)$'
nested='chrono filesystem this_thread ranges views'

new="$(mktemp "${TMPDIR:-/tmp}/std_module.XXXXXX")"
trap 'rm -f "$new"' EXIT

where=(--include=*.hpp --include=*.cpp --include=*.cppm --exclude-dir=third_party --exclude-dir=build
    --exclude-dir=docs)
names="$(cd "$root" && grep -rhoE 'std::([a-z_0-9]+::)*[A-Za-z_][A-Za-z_0-9]*' "${where[@]}" . | LC_ALL=C sort -u)"
# A namespace alias (`namespace fs = std::filesystem;`): its names, as std::'s.
aliases="$(cd "$root" && grep -rhoE 'namespace [a-z_]+ = std::[a-z_]+;' "${where[@]}" . | LC_ALL=C sort -u || true)"
while read -r _ alias _ aliased; do
    [ -n "${alias:-}" ] || continue
    aliased=${aliased%;}
    names="$names
$(cd "$root" && { grep -rhoE "\b$alias::[A-Za-z_][A-Za-z_0-9]*" "${where[@]}" . || true; } |
        sed "s/^$alias::/$aliased::/" | LC_ALL=C sort -u)"
done <<<"$aliases"
names="$(printf '%s\n' "$names" | grep -v '^$' | LC_ALL=C sort -u)"

# std::a -> top a; std::chrono::b -> chrono b; std::string::npos -> top string.
top() {
    printf '%s\n' "$names" | awk -F'::' -v nested=" $nested " '
        { name = $2; if (index(nested, " " name " ") == 0) print name }' |
        { grep -vE "$skip" || true; } | LC_ALL=C sort -u
}
inner() {
    printf '%s\n' "$names" | awk -F'::' -v ns="$1" '$2 == ns && NF >= 3 { print $3 }' |
        { grep -vE "$skip" || true; } | LC_ALL=C sort -u
}

{
    cat <<'EOF'
// sq.std: the standard library as one module, for squared and the apps on it.
// Every squared file writes `import sq.std;` where it wrote standard includes:
// the headers below are parsed once per flag set, into one BMI, and a file
// reads from it only what it uses (7b.4). The `import std` that Termux's libc++
// lacks and the NDK's std.cppm cannot yet build.
//
// Written by tools/std_module.sh, from the names squared writes as std::<name>;
// `make headers` says when it is behind them. Not here: the <cctype> functions
// (bionic gives them internal linkage) and every C macro (assert, errno,
// va_start, UINT32_MAX, SIGSEGV): a file that uses one includes its C header.

// clang-format off: this file is written, in the generator's order.
module;

EOF
    for h in $headers; do printf '#include <%s>\n' "$h"; done
    cat <<'EOF'

export module sq.std;

export namespace std
{

EOF
    top | while read -r name; do printf 'using std::%s;\n' "$name"; done
    cat <<'EOF'

// Found by argument-dependent lookup, so exported by name.
using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator>;
using std::operator<=;
using std::operator>=;
using std::operator<=>;
using std::operator+;
using std::operator-;
using std::operator<<;
using std::operator>>;
using std::operator|;
using std::operator&;
using std::operator^;
using std::operator~;
using std::operator|=;
using std::operator&=;
using std::operator^=;
EOF
    for ns in $nested; do
        list="$(inner "$ns")"
        [ -n "$list" ] || continue
        printf '\nnamespace %s\n{\n' "$ns"
        printf '%s\n' "$list" | while read -r name; do printf 'using std::%s::%s;\n' "$ns" "$name"; done
        if [ "$ns" = filesystem ]; then
            # a range-for over a directory_iterator finds these by argument-dependent lookup
            printf 'using std::filesystem::begin;\nusing std::filesystem::end;\n'
        fi
        if [ "$ns" = chrono ]; then
            for op in + - '*' / % == '<=>' '<' '>' '<=' '>='; do printf 'using std::chrono::operator%s;\n' "$op"; done
        fi
        printf '} // namespace %s\n' "$ns"
    done
    cat <<'EOF'

inline namespace literals
{
inline namespace chrono_literals
{
using std::literals::chrono_literals::operator""h;
using std::literals::chrono_literals::operator""min;
using std::literals::chrono_literals::operator""s;
using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ns;
} // namespace chrono_literals
} // namespace literals

} // namespace std

// libstdc++ keeps its iterators' operators in __gnu_cxx, where argument-
// dependent lookup in an importer finds them only when they are exported.
#if defined( __GLIBCXX__ )
export namespace __gnu_cxx
{
using __gnu_cxx::operator==;
using __gnu_cxx::operator<=>;
using __gnu_cxx::operator-;
using __gnu_cxx::operator+;
} // namespace __gnu_cxx
#endif
// clang-format on
EOF
} > "$new"

count=$(grep -c '^using std::[a-z_0-9]*;$' "$new" || true)
if [ "$check" = 1 ]; then
    if cmp -s "$new" "$target"; then
        echo "std_module: build/std.cppm is current ($count names exported)"
        exit 0
    fi
    echo "std_module: build/std.cppm is behind the sources; tools/std_module.sh writes it again:"
    diff "$target" "$new" | sed -n 's/^\([<>]\) using /  \1 /p' || true
    exit 1
fi
cp "$new" "$target"
echo "std_module: wrote build/std.cppm ($count names)"
