#!/usr/bin/env fish

function usage
    echo 'Usage: fish tools/vendor-gdx-skins.fish ARCHIVE EXPECTED_SHA256 [FRAMEWORK]'
end

if test (count $argv) -lt 2; or test (count $argv) -gt 3
    usage >&2
    exit 2
end

set -l archive (path resolve "$argv[1]")
set -l expected_sha (string lower -- "$argv[2]")
set -l framework (path resolve (dirname (status --current-filename))/..)
if test (count $argv) -eq 3
    set framework (path resolve "$argv[3]")
end
set -l destination "$framework/packages/squared-gui/content/app/src/main/assets/gui/gdx-skins"

for command_name in sha256sum unzip find sort
    if not command -q "$command_name"
        echo "Error: $command_name is required to vendor gdx-skins." >&2
        exit 1
    end
end
if not test -f "$archive"
    echo "Error: gdx-skins archive not found: $archive" >&2
    exit 1
end
if not string match --quiet --regex '^[0-9a-f]{64}$' "$expected_sha"
    echo 'Error: expected SHA-256 must contain exactly 64 hexadecimal characters.' >&2
    exit 1
end

set -l checksum_fields (string split ' ' -- (sha256sum "$archive"))
set -l actual_sha "$checksum_fields[1]"
if test "$actual_sha" != "$expected_sha"
    echo 'Error: gdx-skins archive does not match the pinned SHA-256.' >&2
    echo "Expected: $expected_sha" >&2
    echo "Actual:   $actual_sha" >&2
    exit 1
end

set -l entries (unzip -Z1 "$archive")
if test $status -ne 0; or test (count $entries) -eq 0
    echo 'Error: gdx-skins archive directory could not be read.' >&2
    exit 1
end
for entry in $entries
    if string match --quiet --regex '(^/|(^|/)\.\.(/|$))' "$entry"
        echo "Error: unsafe archive entry: $entry" >&2
        exit 1
    end
end

set -l discovered_roots
for entry in $entries
    set -l path_fields (string split -m 1 '/' -- "$entry")
    set -a discovered_roots "$path_fields[1]"
end
set -l roots (printf '%s\n' $discovered_roots | sort -u)
if test (count $roots) -ne 1; or test -z "$roots[1]"
    echo 'Error: gdx-skins archive must contain one top-level directory.' >&2
    exit 1
end

set -l temporary (mktemp -d)
or begin
    echo 'Error: could not create a temporary extraction directory.' >&2
    exit 1
end
function cleanup --on-event fish_exit --inherit-variable temporary
    if set -q temporary; and test -d "$temporary"
        rm -rf -- "$temporary"
    end
end

unzip -q "$archive" -d "$temporary"
or begin
    echo 'Error: gdx-skins extraction failed.' >&2
    exit 1
end
set -l source_root "$temporary/$roots[1]"
for pattern in '*.json' '*.atlas' '*.png'
    set -l match (find "$source_root" -type f -iname "$pattern" -print -quit)
    if test -z "$match"
        echo "Error: gdx-skins archive contains no $pattern files." >&2
        exit 1
    end
end
set -l selected_source "$source_root/gdx-holo/skin"
for selected_file in default.fnt uiskin.atlas uiskin.json uiskin.png
    if not test -f "$selected_source/$selected_file"
        echo "Error: selected gdx-holo asset is missing: $selected_file" >&2
        exit 1
    end
end
if test -e "$destination"
    echo "Error: pinned gdx-skins destination already exists: $destination" >&2
    exit 1
end

mkdir -p "$destination"
or exit 1
cp -p "$archive" "$destination/gdx-skins.zip"
or begin
    echo 'Error: could not install the pinned gdx-skins archive.' >&2
    exit 1
end
mkdir -p "$destination/selected/gdx-holo"
or exit 1
for selected_file in default.fnt uiskin.atlas uiskin.json uiskin.png
    cp -p "$selected_source/$selected_file" \
        "$destination/selected/gdx-holo/$selected_file"
    or begin
        echo "Error: could not install selected gdx-holo asset: $selected_file" >&2
        exit 1
    end
end

printf '%s  %s\n' "$actual_sha" (path basename "$archive") > "$destination/PIN.sha256"
begin
    echo '# Pinned gdx-skins asset'
    echo
    echo "Archive SHA-256: `$actual_sha`"
    echo
    echo 'The complete validated archive is vendored unchanged as `gdx-skins.zip`.'
    echo 'It is copied into generated projects by the Squared GUI package. Runtime'
    echo 'loading uses the selected portable `selected/gdx-holo/` projection.'
    echo
    echo 'The upstream archive does not provide a repository-wide license file.'
    echo 'Squared records provenance but does not require license metadata during import.'
end > "$destination/SOURCE.md"

pushd "$source_root" >/dev/null
find . -type f | string replace --regex '^\\./' '' | sort > "$destination/ASSET_INDEX.txt"
popd >/dev/null

echo "Pinned gdx-skins asset installed: $destination"
echo "SHA-256: $actual_sha"
