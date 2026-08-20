#!/usr/bin/env fish

set -l fix 0
set -l full 0

for argument in $argv
    switch $argument
        case --fix
            set fix 1
        case --full
            set full 1
        case --help -h
            echo "usage: tools/clang-check.fish [--fix] [--full]"
            exit 0
        case '*'
            echo "error: unknown argument: $argument" >&2
            exit 2
    end
end

set -l root (pwd)
while not test -f "$root/.clang-tidy"
    set -l parent (dirname "$root")
    if test "$parent" = "$root"
        echo "error: could not find project root containing .clang-tidy" >&2
        exit 2
    end
    set root "$parent"
end
cd "$root"

set -l required clang-format clang-tidy fd python3
if test $full -eq 1
    set -a required scan-build cmake
end
for command_name in $required
    if not command -q $command_name
        echo "error: required command is unavailable: $command_name" >&2
        exit 2
    end
end

set -l compile_database ""
if set -q CLANG_COMPILE_DB
    set compile_database "$CLANG_COMPILE_DB"
else
    for candidate in \
        compile_commands.json \
        build/compile_commands.json \
        native-build/compile_commands.json
        if test -f "$candidate"
            set compile_database "$candidate"
            break
        end
    end
end

if test -z "$compile_database"; or not test -f "$compile_database"
    echo "error: compile_commands.json is required" >&2
    echo "configure CMake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >&2
    echo "or set CLANG_COMPILE_DB to its path" >&2
    exit 2
end

set compile_database (realpath "$compile_database")
set -l compile_directory (dirname "$compile_database")
set -l overall 0

set -l format_files (fd --type f \
    --extension c --extension cc --extension cpp --extension cxx \
    --extension h --extension hh --extension hpp --extension hxx \
    --exclude build --exclude native-build --exclude external \
    --exclude third_party --exclude vendor --exclude generated .)

if test (count $format_files) -eq 0
    echo "error: no project-owned C/C++ files were found" >&2
    exit 2
end

echo "project root: $root"
echo "compile database: $compile_database"
echo "project-owned format files: "(count $format_files)
echo
echo "== clang-format =="

if test $fix -eq 1
    clang-format -i $format_files
    or set overall 1
else
    clang-format --dry-run --Werror $format_files
    or set overall 1
end

echo
echo "== clang-tidy =="
set -l translation_list (mktemp)
python3 -c '
import json
import os
import sys

database, root = sys.argv[1], os.path.realpath(sys.argv[2])
excluded = ("build", "native-build", "external", "third_party", "vendor", "generated")
seen = set()
with open(database, encoding="utf-8") as stream:
    commands = json.load(stream)
for command in commands:
    filename = command.get("file")
    directory = command.get("directory", root)
    if not filename:
        continue
    absolute = os.path.realpath(filename if os.path.isabs(filename) else os.path.join(directory, filename))
    try:
        relative = os.path.relpath(absolute, root)
    except ValueError:
        continue
    if relative.startswith(".." + os.sep) or relative == "..":
        continue
    if any(part in excluded for part in relative.split(os.sep)):
        continue
    if os.path.splitext(relative)[1].lower() not in {".c", ".cc", ".cpp", ".cxx"}:
        continue
    if absolute not in seen:
        seen.add(absolute)
        print(absolute)
' "$compile_database" "$root" >$translation_list

set -l translation_units (string split \n (string collect <$translation_list))
rm -f "$translation_list"

if test (count $translation_units) -eq 0
    echo "error: compilation database contains no project-owned translation units" >&2
    exit 2
end

echo "project-owned translation units: "(count $translation_units)
set -l translation_index 0
for translation_unit in $translation_units
    set translation_index (math $translation_index + 1)
    echo "[clang-tidy $translation_index/"(count $translation_units)"] $translation_unit"
    if test $fix -eq 1
        clang-tidy -p "$compile_directory" --fix --format-style=file "$translation_unit"
    else
        clang-tidy -p "$compile_directory" "$translation_unit"
    end
    or set overall 1
end

if test $full -eq 1
    echo
    echo "== Clang Static Analyzer =="
    if set -q CLANG_FULL_BUILD_COMMAND
        fish -c "$CLANG_FULL_BUILD_COMMAND"
        or set overall 1
    else if test -f CMakeLists.txt
        set -l analyzer_build build/clang-analyzer
        scan-build --status-bugs cmake -S . -B "$analyzer_build" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        and scan-build --status-bugs cmake --build "$analyzer_build" --clean-first
        or set overall 1
    else
        echo "error: --full requires CLANG_FULL_BUILD_COMMAND for this project layout" >&2
        set overall 1
    end
end

echo
if test $overall -eq 0
    echo "ALL CLANG CHECKS PASSED"
else
    echo "CLANG CHECKS FAILED" >&2
end
exit $overall
