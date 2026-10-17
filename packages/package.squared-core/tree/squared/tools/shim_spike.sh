#!/usr/bin/env bash
# Sprint 4, task 4.0: the device test of the console shim.
#
#   squared/tools/shim_spike.sh [project dir]     default: the current directory
#
# Builds the project's DEV_LOG APK as usual, then a second APK from it with one
# difference: a second launcher activity, dev.squared.ConsoleActivity, in its
# own process (":console") and its own task, whose class comes from the
# generated shim dex (tools/dexgen/shims.dex). For the spike it loads the
# project's own library, so tapping its icon runs the same app in the other
# process; the dev log's first line names the process.
#
# What it proves: Android's verifier accepts the dex; a manifest entry with
# android:process starts in its own process; NativeActivity, subclassed by a
# generated class with no code but a constructor, still finds its library
# through the entry's own android.app.lib_name. Nothing in the project is
# changed: the spike's files go to <project>/build/spike/.
#
# Exit codes: 0 built, 1 a step failed, 2 usage.
set -euo pipefail

proj=$(cd "${1:-.}" && pwd)
here=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
dex=$here/tools/dexgen/shims.dex
[ -f "$proj/mk/squared_generated.mk" ] || { echo "shim_spike: $proj is not an Android project" >&2; exit 2; }
[ -f "$dex" ] || { echo "shim_spike: no $dex" >&2; exit 1; }

tmp=$(mktemp -d "${TMPDIR:-/tmp}/spike.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
printf 'print-%%:\n\t@:$(info $($*))\n' >"$tmp/print.mk"
var() { make -s --no-print-directory -C "$proj" -f Makefile -f "$tmp/print.mk" DEV_LOG=1 "print-$1"; }

echo "== 1/4 the project's DEV_LOG APK"
make --no-print-directory -C "$proj" DEV_LOG=1 apk

project=$(var SQ_PROJECT)
build=$proj/$(var BUILD)
abi=$(var SQ_ABI)
out=$build/spike
mkdir -p "$out"

echo "== 2/4 the manifest: the console activity added, hasCode dropped"
src=$build/dev/AndroidManifest.xml
[ -f "$src" ] || src=$proj/sq_android/AndroidManifest.xml
grep -q '</application>' "$src" || { echo "shim_spike: no </application> in $src" >&2; exit 1; }
{
    sed -e 's/[[:space:]]*android:hasCode="false"//' -e '/<\/application>/,$d' "$src"
    cat <<EOF
        <!-- Added by squared/tools/shim_spike.sh: the generated shim class, in
             its own process and task, loading lib${project}.so. -->
        <activity
            android:name="dev.squared.ConsoleActivity"
            android:label="${project} console"
            android:process=":console"
            android:taskAffinity="${project}.console"
            android:exported="true"
            android:configChanges="orientation|keyboardHidden|screenSize|screenLayout|density|uiMode">
            <meta-data android:name="android.app.lib_name" android:value="${project}" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
EOF
    sed -n '/<\/application>/,$p' "$src"
} >"$out/AndroidManifest.xml"
grep -q 'hasCode="false"' "$out/AndroidManifest.xml" && { echo "shim_spike: hasCode=\"false\" is still there" >&2; exit 1; }
echo "  $out/AndroidManifest.xml"

echo "== 3/4 link, add classes.dex and the library"
assets=$(var SQ_ASSETS_DIR)
asset_args=()
[ -n "$(find "$proj/$assets" -type f 2>/dev/null | head -1)" ] && asset_args=(-A "$proj/$assets")
"$(var SQ_AAPT2)" link -o "$out/base.apk" -I "$(var SQ_ANDROID_JAR)" --manifest "$out/AndroidManifest.xml" \
    --min-sdk-version "$(var SQ_MIN_SDK)" --target-sdk-version "$(var SQ_TARGET_SDK)" \
    "${asset_args[@]}" "$build/apk/resources.zip"
rm -rf "$out/stage" && mkdir -p "$out/stage/lib/$abi"
cp "$dex" "$out/stage/classes.dex"
cp "$build/apk/lib/$abi/lib$project.so" "$out/stage/lib/$abi/"
cp "$out/base.apk" "$out/unsigned.apk"
(cd "$out/stage" && zip -q -X "$out/unsigned.apk" classes.dex "lib/$abi/lib$project.so")
echo "  classes.dex ($(wc -c <"$dex" | tr -d ' ') bytes), lib/$abi/lib$project.so"

echo "== 4/4 sign"
apk=$out/$project-spike.apk
"$(var SQ_APKSIGNER)" sign --ks "$(var SQ_KEYSTORE)" --ks-pass "pass:$(var SQ_KEY_PASS)" \
    --key-pass "pass:$(var SQ_KEY_PASS)" --ks-key-alias "$(var SQ_KEY_ALIAS)" --out "$apk" "$out/unsigned.apk"
"$(var SQ_APKSIGNER)" verify "$apk" >/dev/null
echo "  verified $apk ($(wc -c <"$apk" | tr -d ' ') bytes)"
cat <<EOF
== next
  termux-open "$apk"      # install; then two icons: "$project" and "$project console"
  nc -klu 5555            # in another session
  tap "$project console":   the dev log's first lines read "device: API ..., process <package>:console"
  tap "$project":           "process <package>", the main process, as before
  If the console icon closes at once, the dex or the entry was refused: Android logs why,
  under the app's own name (a bug report, or adb logcat, shows it).
EOF
