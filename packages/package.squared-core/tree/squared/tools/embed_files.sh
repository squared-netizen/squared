#!/usr/bin/env bash
# Compile a directory's files into a C++ source: constant bytes, and a table
# of sq::files::EmbeddedFile that an sq::files::EmbeddedFileSystem reads.
#
#   tools/embed_files.sh SOURCE_DIR PREFIX FUNCTION HEADER OUT.cpp
#
#   SOURCE_DIR  the files to embed (not its subdirectories), from the tree root
#   PREFIX      the directory they appear under, inside the file system
#   FUNCTION    the function OUT.cpp defines in sq::detail, which returns the table
#   HEADER      the header that declares it, as OUT.cpp includes it
#
# The Stage's default skin is made with
#
#   tools/embed_files.sh stage/skin/default/skin skins/default/skin \
#       default_skin detail/default_skin.hpp stage/src/default_skin.cpp
#
# The output is committed, so a build needs no host tool and no build step;
# a test compares it with the files, so it cannot drift. Files go in by name
# (byte order), 16 bytes to a line. Writes OUT.cpp only when it changes.
# POSIX tools only (od, sed, sort): runs in Termux.
set -euo pipefail

if [ "$#" -ne 5 ]; then
  sed -n '2,10p' "$0" >&2
  exit 2
fi
source_dir=$1
prefix=$2
function_name=$3
header=$4
out=$5

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
if [ ! -d "$source_dir" ]; then
  echo "embed_files: $source_dir is not a directory" >&2
  exit 1
fi

part="$out.part"
files=$(cd "$source_dir" && LC_ALL=C ls -1 | while IFS= read -r name; do
  [ -f "$name" ] && printf '%s\n' "$name"
done)
if [ -z "$files" ]; then
  echo "embed_files: no files in $source_dir" >&2
  exit 1
fi

{
  printf '// %s\n' "$out"
  printf '// Written by tools/embed_files.sh from %s/: do not edit.\n' "$source_dir"
  printf '// Change the files and run it again (its comment has the command line).\n\n'
  printf '#include "%s"\n\n' "$header"
  printf '#include <squared/files/embedded_file_system.hpp>\n\n'
  printf '#include <array>\n#include <span>\n\n'
  printf 'namespace sq::detail\n{\n\nnamespace\n{\n\n// clang-format off\n'
  index=0
  printf '%s\n' "$files" | while IFS= read -r name; do
    printf '\n// %s/%s\n' "$source_dir" "$name"
    if [ -s "$source_dir/$name" ]; then
      printf 'constexpr unsigned char bytes_%d[] = {\n' "$index"
      od -An -v -t x1 "$source_dir/$name" | sed -e 's/^ *//' -e 's/ *$//' \
          -e 's/\([0-9a-f][0-9a-f]\)/0x\1,/g' -e 's/, */, /g' -e 's/^/    /' -e 's/ *$//'
      printf '};\n'
    else
      printf 'constexpr std::span<const unsigned char> bytes_%d{};\n' "$index"
    fi
    index=$((index + 1))
  done
  count=$(printf '%s\n' "$files" | wc -l | tr -d ' ')
  printf '\nconstexpr std::array<files::EmbeddedFile, %s> table = { {\n' "$count"
  index=0
  printf '%s\n' "$files" | while IFS= read -r name; do
    printf '    { "%s/%s", bytes_%d },\n' "$prefix" "$name" "$index"
    index=$((index + 1))
  done
  printf '} };\n// clang-format on\n\n} // namespace\n\n'
  printf 'std::span<const files::EmbeddedFile> %s() noexcept\n{\n    return table;\n}\n\n' \
      "$function_name"
  printf '} // namespace sq::detail\n'
} > "$part"

if [ -f "$out" ] && cmp -s "$part" "$out"; then
  rm -f "$part"
  echo "embed_files: $out unchanged"
else
  mv "$part" "$out"
  echo "embed_files: wrote $out ($count files)"
fi
